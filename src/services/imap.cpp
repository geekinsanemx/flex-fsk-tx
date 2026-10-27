/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * IMAP Module - multi-account IMAP polling, scheduled checks
 */

#include "../core/config.h"
#include "../services/imap.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../protocol/transmission.h"
#include "../protocol/flex_protocol.h"
#include "../core/hardware.h"
#include "../core/utils.h"

#ifdef ENABLE_IMAP

#include <ReadyMail.h>
#include <WiFiClientSecure.h>
#include <SPIFFS.h>
#include "../core/tx_lock.h"
#include <ArduinoJson.h>
#include <algorithm>

// =============================================================================
// GLOBALS (extern, declared in imap.h)
// =============================================================================
IMAPConfig imap_config;
unsigned long last_imap_check = 0;
int imap_failed_cycles = 0;

// =============================================================================
// FILE-LOCAL STATE
// =============================================================================
static const uint32_t IMAP_CONNECTION_TIMEOUT_MS = 30000UL;
static const uint32_t IMAP_BOOT_DELAY_MS = 60000UL;

static uint64_t imap_last_uid = 0;

struct IMAPScheduleEntry {
    uint8_t account_id;
    unsigned long next_check_time;
    uint8_t failed_attempts;
    bool suspended;
};

static std::vector<IMAPScheduleEntry> imap_schedule;
static bool imap_system_enabled = false;

static std::vector<uint32_t>* message_nums_ptr = nullptr;
static String from_str, subject_str, body_str;

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================
static IMAPAccount load_account_from_config(uint8_t account_id);
static bool imap_connect();
static bool process_message_clean(ReadyMailIMAP::IMAPClient& imap_client, uint32_t msg_num, const IMAPAccount& account);
static bool imap_check_account_clean(uint8_t account_id);
static void load_default_imap_config();

// =============================================================================
// ACCOUNT CONFIG LOOKUP (direct SPIFFS read, used only by the poller)
// =============================================================================
static IMAPAccount load_account_from_config(uint8_t account_id) {
    IMAPAccount account = {};

    FlashGuard fg;
    if (!fg.ok()) return account;

    if (!SPIFFS.exists("/imap_settings.json")) {
        logMessage("IMAP: Config file not found");
        return account;
    }

    File file = SPIFFS.open("/imap_settings.json", "r");
    if (!file) {
        logMessage("IMAP: Failed to open config file");
        return account;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        logMessage("IMAP: Failed to parse config JSON");
        return account;
    }

    JsonArray accounts = doc["accounts"];
    for (JsonVariant account_data : accounts) {
        if (account_data["id"] == account_id) {
            account.id = account_data["id"];
            strlcpy(account.name, account_data["name"] | "", sizeof(account.name));
            strlcpy(account.server, account_data["server"] | "", sizeof(account.server));
            account.port = account_data["port"] | 993;
            account.use_ssl = account_data["use_ssl"] | true;
            strlcpy(account.username, account_data["username"] | "", sizeof(account.username));

            String encoded_password = account_data["password"] | "";
            String decoded_password = base64_decode_string(encoded_password);
            strlcpy(account.password, decoded_password.c_str(), sizeof(account.password));

            account.check_interval_min = account_data["check_interval_min"] | IMAP_MIN_CHECK_INTERVAL;
            account.capcode = account_data["capcode"] | settings.default_capcode;
            account.frequency = account_data["frequency"] | settings.default_frequency;
            account.mail_drop = account_data["mail_drop"] | false;
            break;
        }
    }

    return account;
}

// =============================================================================
// LEGACY (DEAD) ENTRY POINT - kept for parity, never called
// =============================================================================
static bool imap_connect() {
    logMessage("IMAP: Old system disabled - use IMAP configuration");
    return false;
}

// =============================================================================
// POLLING
// =============================================================================
static bool imap_check_account_clean(uint8_t account_id) {
    IMAPAccount account = load_account_from_config(account_id);

    if (strlen(account.server) == 0 || strlen(account.username) == 0 || strlen(account.password) == 0) {
        logMessagef("IMAP: Account %d - missing server or credentials", account_id);
        return false;
    }

    uint32_t heap_before = ESP.getFreeHeap();
    logMessagef("HEAP: Before IMAP check - Free: %u bytes", heap_before);

    WiFiClientSecure ssl_client;
    ssl_client.setInsecure();
    ssl_client.setTimeout(30000);

    ReadyMailIMAP::IMAPClient imap_client(ssl_client);

    logMessagef("IMAP: Account %d ('%s') - Connecting to %s:%d", account_id, account.name, account.server, account.port);

    unsigned long connect_start = millis();
    bool connect_result = imap_client.connect(account.server, account.port);
    unsigned long connect_duration = millis() - connect_start;

    if (!connect_result) {
        if (connect_duration >= IMAP_CONNECTION_TIMEOUT_MS) {
            logMessagef("IMAP: Account %d - Connection timeout after %lu ms", account_id, connect_duration);
        } else {
            logMessagef("IMAP: Account %d - Connection failed after %lu ms", account_id, connect_duration);
        }
        ssl_client.stop();
        return false;
    }

    if (!imap_client.authenticate(account.username, account.password, readymail_auth_password)) {
        logMessagef("IMAP: Account %d - Authentication failed", account_id);
        imap_client.close();
        ssl_client.stop();
        return false;
    }

    if (!imap_client.select("INBOX", false)) {
        logMessagef("IMAP: Account %d - Failed to select INBOX", account_id);
        imap_client.close();
        ssl_client.stop();
        return false;
    }

    std::vector<uint32_t> message_nums;
    message_nums_ptr = &message_nums;

    auto collection_callback = [](ReadyMailIMAP::IMAPCallbackData &data) -> void {
        if (data.event() == imap_data_event_search && message_nums_ptr) {
            if (message_nums_ptr->size() < 10) {
                message_nums_ptr->push_back(data.messageNum());
            }
        }
    };

    if (!imap_client.search("UID SEARCH UNSEEN", 10, true, collection_callback, true)) {
        logMessagef("IMAP: Account %d - Search failed", account_id);
        imap_client.close();
        ssl_client.stop();
        return false;
    }

    logMessagef("IMAP: Account %d - Found %d unread messages", account_id, message_nums.size());

    std::sort(message_nums.begin(), message_nums.end());

    for (uint32_t msg_num : message_nums) {
        if (queue_is_full()) {
            logMessage("IMAP: Queue full, stopping message processing");
            break;
        }

        if (process_message_clean(imap_client, msg_num, account)) {
            logMessagef("IMAP: Message %d queued successfully", msg_num);
        }

        yield();
        feed_watchdog();
    }

    imap_client.close();
    ssl_client.stop();

    uint32_t heap_after = ESP.getFreeHeap();
    logMessagef("HEAP: After IMAP cleanup - Free: %u bytes (diff: %d)", heap_after, (int32_t)heap_after - (int32_t)heap_before);

    return true;
}

static bool process_message_clean(ReadyMailIMAP::IMAPClient& imap_client, uint32_t msg_num, const IMAPAccount& account) {
    from_str = "";
    subject_str = "";
    body_str = "";

    auto fetch_callback = [](ReadyMailIMAP::IMAPCallbackData &data) {
        extern String from_str, subject_str, body_str;

        if (data.event() == imap_data_event_fetch_envelope) {
            for (size_t i = 0; i < data.headerCount(); i++) {
                String header_name = data.getHeader(i).first;
                String header_value = data.getHeader(i).second;

                if (header_name.equalsIgnoreCase("From")) {
                    from_str = header_value;
                    int bracket_pos = from_str.indexOf('<');
                    if (bracket_pos > 0) {
                        from_str = from_str.substring(0, bracket_pos);
                        from_str.trim();
                    }
                } else if (header_name.equalsIgnoreCase("Subject")) {
                    subject_str = header_value;
                }
            }

            for (size_t i = 0; i < data.fileCount(); i++) {
                if (data.fileInfo(i).mime == "text/plain") {
                    data.fetchOption(i) = true;
                }
            }
        } else if (data.event() == imap_data_event_fetch_body) {
            if (data.fileInfo().mime == "text/plain" && data.fileChunk().size > 0) {
                char temp_body[513];
                size_t copy_size = min((size_t)512, (size_t)data.fileChunk().size);
                strncpy(temp_body, (char*)data.fileChunk().data, copy_size);
                temp_body[copy_size] = '\0';
                body_str = String(temp_body);
            }
        }
    };

    if (!imap_client.fetchUID(msg_num, fetch_callback, nullptr, true, 8192)) {
        logMessagef("IMAP: Failed to fetch message %d", msg_num);
        return false;
    }

    String email_message = from_str + ": " + subject_str + "\n" + body_str;
    String truncated_message = truncate_message_with_ellipsis(email_message);

    uint64_t capcode = settings.default_capcode;
    float frequency = account.frequency > 0 ? account.frequency : settings.default_frequency;
    int power = settings.default_txpower;

    if (queue_add_message(capcode, frequency, power, account.mail_drop, truncated_message.c_str())) {
        logMessagef("IMAP: Message %d from '%s' subject '%s' queued", msg_num, from_str.c_str(), subject_str.c_str());
        imap_client.sendCommand("UID STORE " + String(msg_num) + " +FLAGS (\\Seen)", nullptr, true);
        return true;
    }

    logMessagef("IMAP: Failed to queue message %d - queue may be full", msg_num);
    imap_client.sendCommand("UID STORE " + String(msg_num) + " -FLAGS (\\Seen)", nullptr, true);
    return false;
}

// =============================================================================
// SCHEDULER
// =============================================================================
void init_imap_scheduler() {
    imap_schedule.clear();
    imap_system_enabled = imap_config.enabled;

    if (!imap_system_enabled) {
        return;
    }

    unsigned long current_time = millis();

    for (uint8_t i = 0; i < imap_config.account_count; i++) {
        IMAPScheduleEntry entry;
        entry.account_id = i + 1;
        entry.next_check_time = current_time + IMAP_BOOT_DELAY_MS + (i * 10000UL);
        entry.failed_attempts = 0;
        entry.suspended = false;
        imap_schedule.push_back(entry);
    }

    logMessagef("IMAP: Scheduler initialized with %d accounts", imap_schedule.size());
}

void imap_scheduler_loop() {
    if (!imap_system_enabled || imap_schedule.empty()) {
        return;
    }

    unsigned long current_time = millis();

    for (auto& entry : imap_schedule) {
        if (entry.suspended || current_time < entry.next_check_time) {
            continue;
        }

        logMessagef("IMAP: Processing account %d", entry.account_id);

        if (imap_check_account_clean(entry.account_id)) {
            entry.failed_attempts = 0;
            entry.next_check_time = current_time + (10 * 60000UL);
        } else {
            entry.failed_attempts++;
            unsigned long backoff = min(entry.failed_attempts * 30000UL, 300000UL);
            entry.next_check_time = current_time + backoff;

            if (entry.failed_attempts >= 5) {
                entry.suspended = true;
                logMessagef("IMAP: Account %d suspended after %d failures", entry.account_id, entry.failed_attempts);
            }
        }

        return;
    }
}

// =============================================================================
// PERSISTENT CONFIG (/imap_settings.json) - used by web UI CRUD
// =============================================================================
bool save_imap_config() {
    FlashGuard fg;
    if (!fg.ok()) return false;

    logMessage("IMAP: Saving configuration");

    File file = SPIFFS.open("/imap_settings.json", "w");
    if (!file) {
        logMessage("IMAP: Failed to create IMAP config file");
        return false;
    }

    DynamicJsonDocument doc(4096);
    doc["enabled"] = imap_config.enabled;
    doc["account_count"] = imap_config.account_count;

    JsonArray accounts = doc.createNestedArray("accounts");
    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        JsonObject account = accounts.createNestedObject();
        account["id"] = imap_config.accounts[i].id;
        account["name"] = imap_config.accounts[i].name;
        account["server"] = imap_config.accounts[i].server;
        account["port"] = imap_config.accounts[i].port;
        account["use_ssl"] = imap_config.accounts[i].use_ssl;
        account["username"] = imap_config.accounts[i].username;
        account["password"] = base64_encode_string(String(imap_config.accounts[i].password));
        account["check_interval_min"] = imap_config.accounts[i].check_interval_min;
        account["capcode"] = imap_config.accounts[i].capcode;
        account["frequency"] = imap_config.accounts[i].frequency;
        account["mail_drop"] = imap_config.accounts[i].mail_drop;
    }

    if (serializeJson(doc, file) == 0) {
        logMessage("IMAP: Failed to write IMAP config JSON");
        file.flush();
        file.close();
        return false;
    }

    file.flush();
    file.close();
    logMessage("IMAP: Config saved successfully");
    return true;
}

bool load_imap_config() {
    FlashGuard fg;
    if (!fg.ok()) return false;

    logMessage("IMAP: Loading configuration");

    if (!SPIFFS.exists("/imap_settings.json")) {
        logMessage("IMAP: Config file not found, using defaults");
        load_default_imap_config();
        save_imap_config();
        return false;
    }

    File file = SPIFFS.open("/imap_settings.json", "r");
    if (!file) {
        logMessage("IMAP: Failed to open config file");
        load_default_imap_config();
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        logMessage("IMAP: Failed to parse config JSON, using defaults");
        load_default_imap_config();
        save_imap_config();
        return false;
    }

    imap_config.accounts.clear();

    imap_config.enabled = doc["enabled"] | false;
    imap_config.account_count = doc["account_count"] | 0;

    JsonArray accounts = doc["accounts"];
    for (JsonObject account : accounts) {
        if (imap_config.accounts.size() >= IMAP_MAX_ACCOUNTS) {
            logMessage("IMAP: Maximum accounts reached, skipping additional accounts");
            break;
        }

        IMAPAccount temp_account;
        temp_account.id = account["id"] | (imap_config.accounts.size() + 1);
        strlcpy(temp_account.name, account["name"] | "", sizeof(temp_account.name));
        strlcpy(temp_account.server, account["server"] | "", sizeof(temp_account.server));
        temp_account.port = account["port"] | 993;
        temp_account.use_ssl = account["use_ssl"] | true;
        strlcpy(temp_account.username, account["username"] | "", sizeof(temp_account.username));
        String encoded_password = account["password"] | "";
        String decoded_password = base64_decode_string(encoded_password);
        strlcpy(temp_account.password, decoded_password.c_str(), sizeof(temp_account.password));
        temp_account.check_interval_min = max((uint16_t)(account["check_interval_min"] | IMAP_MIN_CHECK_INTERVAL), (uint16_t)IMAP_MIN_CHECK_INTERVAL);
        temp_account.capcode = account["capcode"] | settings.default_capcode;
        temp_account.frequency = account["frequency"] | settings.default_frequency;
        temp_account.mail_drop = account["mail_drop"] | false;

        temp_account.last_check = 0;
        temp_account.failed_check_cycles = 0;
        temp_account.suspended = false;

        imap_config.accounts.push_back(temp_account);
    }

    imap_config.account_count = imap_config.accounts.size();
    logMessagef("IMAP: Config loaded successfully, %d accounts configured", imap_config.account_count);
    return true;
}

static void load_default_imap_config() {
    logMessage("IMAP: Loading default IMAP configuration");
    imap_config.enabled = false;
    imap_config.accounts.clear();
    imap_config.account_count = 0;

    DynamicJsonDocument doc(4096);
    doc["enabled"] = false;
    doc["account_count"] = 0;

    JsonArray accounts = doc.createNestedArray("accounts");

    save_imap_config();
}

// =============================================================================
// ACCOUNT CRUD - called from web handlers
// =============================================================================
bool add_imap_account(const String& name, const String& server, uint16_t port, bool use_ssl,
                      const String& username, const String& password, uint16_t check_interval_min,
                      uint64_t capcode, float frequency, bool mail_drop) {
    if (imap_config.accounts.size() >= IMAP_MAX_ACCOUNTS) {
        logMessage("IMAP: Cannot add IMAP account - maximum accounts reached");
        return false;
    }

    if (name.length() == 0 || server.length() == 0 || username.length() == 0) {
        logMessage("IMAP: Cannot add IMAP account - required fields empty");
        return false;
    }

    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        if (String(imap_config.accounts[i].name) == name) {
            logMessage("IMAP: Cannot add IMAP account - name already exists");
            return false;
        }
    }

    IMAPAccount temp_account;
    temp_account.id = imap_config.accounts.size() + 1;
    strlcpy(temp_account.name, name.c_str(), sizeof(temp_account.name));
    strlcpy(temp_account.server, server.c_str(), sizeof(temp_account.server));
    temp_account.port = port;
    temp_account.use_ssl = use_ssl;
    strlcpy(temp_account.username, username.c_str(), sizeof(temp_account.username));
    strlcpy(temp_account.password, password.c_str(), sizeof(temp_account.password));
    temp_account.check_interval_min = max(check_interval_min, (uint16_t)IMAP_MIN_CHECK_INTERVAL);
    temp_account.capcode = capcode;
    temp_account.frequency = frequency;
    temp_account.mail_drop = mail_drop;
    temp_account.last_check = 0;
    temp_account.failed_check_cycles = 0;
    temp_account.suspended = false;

    imap_config.accounts.push_back(temp_account);
    imap_config.account_count = imap_config.accounts.size();

    logMessagef("IMAP: Account '%s' added successfully (ID: %d)", name.c_str(), temp_account.id);
    return true;
}

bool edit_imap_account(uint8_t id, const String& name, const String& server, uint16_t port, bool use_ssl,
                       const String& username, const String& password, uint16_t check_interval_min,
                       uint64_t capcode, float frequency, bool mail_drop) {
    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        if (imap_config.accounts[i].id == id) {
            for (size_t j = 0; j < imap_config.accounts.size(); j++) {
                if (j != i && String(imap_config.accounts[j].name) == name) {
                    logMessage("IMAP: Cannot edit IMAP account - name already exists");
                    return false;
                }
            }

            strlcpy(imap_config.accounts[i].name, name.c_str(), sizeof(imap_config.accounts[i].name));
            strlcpy(imap_config.accounts[i].server, server.c_str(), sizeof(imap_config.accounts[i].server));
            imap_config.accounts[i].port = port;
            imap_config.accounts[i].use_ssl = use_ssl;
            strlcpy(imap_config.accounts[i].username, username.c_str(), sizeof(imap_config.accounts[i].username));
            strlcpy(imap_config.accounts[i].password, password.c_str(), sizeof(imap_config.accounts[i].password));
            imap_config.accounts[i].check_interval_min = max(check_interval_min, (uint16_t)IMAP_MIN_CHECK_INTERVAL);
            imap_config.accounts[i].capcode = capcode;
            imap_config.accounts[i].frequency = frequency;
            imap_config.accounts[i].mail_drop = mail_drop;

            logMessagef("IMAP: account ID %d updated successfully", id);
            return true;
        }
    }

    logMessagef("IMAP: account ID %d not found for editing", id);
    return false;
}

bool delete_imap_account(uint8_t id) {
    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        if (imap_config.accounts[i].id == id) {
            String account_name = String(imap_config.accounts[i].name);
            imap_config.accounts.erase(imap_config.accounts.begin() + i);
            imap_config.account_count = imap_config.accounts.size();

            for (size_t j = 0; j < imap_config.accounts.size(); j++) {
                imap_config.accounts[j].id = j + 1;
            }

            logMessagef("IMAP: account '%s' (ID: %d) deleted successfully", account_name.c_str(), id);
            return true;
        }
    }

    logMessagef("CONFIG: account ID %d not found for deletion", id);
    return false;
}

bool any_imap_accounts_suspended() {
    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        if (imap_config.accounts[i].suspended) {
            return true;
        }
    }
    return false;
}

#endif // ENABLE_IMAP
