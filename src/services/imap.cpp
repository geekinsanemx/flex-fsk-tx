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
#include "../network/tls_trust.h"
#include "../services/service_worker.h"
#include "../network/network.h"

#ifdef ENABLE_IMAP

#include <ReadyMail.h>
#include <WiFiClientSecure.h>
#include <SPIFFS.h>
#include "../core/tx_lock.h"
#include <ArduinoJson.h>
#include <algorithm>
#include <memory>

// =============================================================================
// GLOBALS (extern, declared in imap.h)
// =============================================================================
IMAPConfig imap_config;
unsigned long last_imap_check = 0;
int imap_failed_cycles = 0;

// =============================================================================
// FILE-LOCAL STATE
// =============================================================================
static const uint32_t IMAP_BOOT_DELAY_MS = 60000UL;
static const uint32_t IMAP_RELOAD_DELAY_MS = 10000UL;
static const uint32_t IMAP_RETRY_STEP_MS = 30000UL;
static const uint32_t IMAP_RETRY_MAX_MS = 300000UL;
static const uint32_t IMAP_TEST_UNREAD_LIMIT = 100;

static uint64_t imap_last_uid = 0;

struct IMAPScheduleEntry {
    uint8_t account_id;
    unsigned long next_check_time;
    bool in_flight;
};

struct IMAPCheckResult {
    bool success = false;
    String error;
    uint32_t unread = 0;
    uint32_t queued = 0;
};

static std::vector<IMAPScheduleEntry> imap_schedule;
static bool imap_system_enabled = false;
static uint32_t imap_schedule_generation = 0;
static IMAPTestStatus imap_test_status[IMAP_MAX_ACCOUNTS];

static std::vector<uint32_t>* message_nums_ptr = nullptr;
static uint32_t imap_search_count = 0;
static String from_str, subject_str, body_str;

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================
static bool imap_connect();
static bool process_message_clean(ReadyMailIMAP::IMAPClient& imap_client, uint32_t msg_num, const IMAPAccount& account);
static IMAPCheckResult imap_check_account(const IMAPAccount& account);
static IMAPCheckResult imap_test_connection(const IMAPAccount& account);
static void load_default_imap_config();

// =============================================================================
// LEGACY (DEAD) ENTRY POINT - kept for parity, never called
// =============================================================================
static bool imap_connect() {
    logMessage("IMAP: Old system disabled - use IMAP configuration");
    return false;
}

// =============================================================================
// POLLING (runs on the service worker task)
// =============================================================================
static bool imap_open_session(const IMAPAccount& account, WiFiClientSecure& ssl_client,
                              ReadyMailIMAP::IMAPClient& imap_client, bool read_only, String& error) {
    if (strlen(account.server) == 0 || strlen(account.username) == 0 || strlen(account.password) == 0) {
        error = "Missing server or credentials";
        logMessagef("IMAP: Account %d - %s", account.id, error.c_str());
        return false;
    }

    logMessagef("IMAP: Account %d ('%s') - Connecting to %s:%d", account.id, account.name, account.server, account.port);

    unsigned long connect_start = millis();
    if (!imap_client.connect(account.server, account.port, NULL, account.use_ssl)) {
        error = "Connection or TLS failed after " + String(millis() - connect_start) + " ms";
        logMessagef("IMAP: Account %d - %s", account.id, error.c_str());
        ssl_client.stop();
        return false;
    }

    if (!imap_client.authenticate(account.username, account.password, readymail_auth_password)) {
        error = "Authentication failed";
        logMessagef("IMAP: Account %d - %s", account.id, error.c_str());
        imap_client.close();
        ssl_client.stop();
        return false;
    }

    if (!imap_client.select("INBOX", read_only)) {
        error = "Failed to select INBOX";
        logMessagef("IMAP: Account %d - %s", account.id, error.c_str());
        imap_client.close();
        ssl_client.stop();
        return false;
    }

    return true;
}

static IMAPCheckResult imap_check_account(const IMAPAccount& account) {
    IMAPCheckResult result;

    uint32_t heap_before = ESP.getFreeHeap();
    logMessagef("HEAP: Before IMAP check - Free: %u bytes", heap_before);

    WiFiClientSecure ssl_client;
    tls_configure_client(ssl_client);
    ReadyMailIMAP::IMAPClient imap_client(ssl_client);

    if (!imap_open_session(account, ssl_client, imap_client, false, result.error)) {
        return result;
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

    bool search_ok = imap_client.search("UID SEARCH UNSEEN", 10, true, collection_callback, true);
    message_nums_ptr = nullptr;

    if (!search_ok) {
        result.error = "Search failed";
        logMessagef("IMAP: Account %d - %s", account.id, result.error.c_str());
        imap_client.close();
        ssl_client.stop();
        return result;
    }

    result.unread = message_nums.size();
    logMessagef("IMAP: Account %d - Found %d unread messages", account.id, message_nums.size());

    std::sort(message_nums.begin(), message_nums.end());

    for (uint32_t msg_num : message_nums) {
        if (queue_is_full()) {
            logMessage("IMAP: Queue full, stopping message processing");
            break;
        }

        if (process_message_clean(imap_client, msg_num, account)) {
            result.queued++;
            logMessagef("IMAP: Message %d queued successfully", msg_num);
        }

        yield();
    }

    imap_client.close();
    ssl_client.stop();

    uint32_t heap_after = ESP.getFreeHeap();
    logMessagef("HEAP: After IMAP cleanup - Free: %u bytes (diff: %d)", heap_after, (int32_t)heap_after - (int32_t)heap_before);

    result.success = true;
    return result;
}

static IMAPCheckResult imap_test_connection(const IMAPAccount& account) {
    IMAPCheckResult result;

    WiFiClientSecure ssl_client;
    tls_configure_client(ssl_client);
    ReadyMailIMAP::IMAPClient imap_client(ssl_client);

    if (!imap_open_session(account, ssl_client, imap_client, true, result.error)) {
        return result;
    }

    imap_search_count = 0;
    auto count_callback = [](ReadyMailIMAP::IMAPCallbackData &data) -> void {
        if (data.event() == imap_data_event_search) {
            imap_search_count++;
        }
    };

    bool search_ok = imap_client.search("UID SEARCH UNSEEN", IMAP_TEST_UNREAD_LIMIT, true, count_callback, true);
    imap_client.close();
    ssl_client.stop();

    if (!search_ok) {
        result.error = "Search failed";
        logMessagef("IMAP: Account %d test - %s", account.id, result.error.c_str());
        return result;
    }

    result.unread = imap_search_count;
    result.success = true;
    logMessagef("IMAP: Account %d test passed - %u unread messages", account.id, (unsigned)result.unread);
    return result;
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

    uint64_t capcode = account.capcode > 0 ? account.capcode : settings.default_capcode;
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
// SCHEDULER (state owned by the loop task, network work runs on the service worker)
// =============================================================================
static IMAPAccount* find_imap_account(uint8_t account_id) {
    for (auto& account : imap_config.accounts) {
        if (account.id == account_id) {
            return &account;
        }
    }
    return nullptr;
}

static IMAPScheduleEntry* find_imap_schedule_entry(uint8_t account_id) {
    for (auto& entry : imap_schedule) {
        if (entry.account_id == account_id) {
            return &entry;
        }
    }
    return nullptr;
}

static void build_imap_schedule(uint32_t first_check_delay_ms) {
    imap_schedule.clear();
    imap_schedule_generation++;
    imap_system_enabled = imap_config.enabled;

    if (!imap_system_enabled) {
        return;
    }

    unsigned long current_time = millis();

    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        IMAPScheduleEntry entry;
        entry.account_id = imap_config.accounts[i].id;
        entry.next_check_time = current_time + first_check_delay_ms + (i * 10000UL);
        entry.in_flight = false;
        imap_schedule.push_back(entry);
    }
}

void init_imap_scheduler() {
    build_imap_schedule(IMAP_BOOT_DELAY_MS);
    logMessagef("IMAP: Scheduler initialized with %d accounts", imap_schedule.size());
}

// Rebuilds the schedule from imap_config after a runtime configuration change (toggle/add/edit/delete).
void imap_reload_scheduler() {
    build_imap_schedule(IMAP_RELOAD_DELAY_MS);
    if (imap_system_enabled) {
        logMessagef("IMAP: Scheduler reloaded with %d accounts", imap_schedule.size());
    } else {
        logMessage("IMAP: Scheduler stopped");
    }
}

void imap_reset_suspensions() {
    for (auto& account : imap_config.accounts) {
        account.failed_check_cycles = 0;
        account.suspended = false;
    }
}

static void imap_apply_check_result(uint8_t account_id, uint32_t generation, const IMAPCheckResult& result) {
    if (generation != imap_schedule_generation) {
        logMessagef("IMAP: Account %d result discarded - configuration changed during check", account_id);
        return;
    }

    IMAPScheduleEntry* entry = find_imap_schedule_entry(account_id);
    IMAPAccount* account = find_imap_account(account_id);
    if (!entry || !account) {
        return;
    }

    entry->in_flight = false;
    unsigned long current_time = millis();

    if (result.success) {
        entry->next_check_time = current_time + (unsigned long)account->check_interval_min * 60000UL;
        if (account->failed_check_cycles != 0) {
            account->failed_check_cycles = 0;
            save_imap_config();
        }
        return;
    }

    if (!network_available_cached || imap_suspended_on_gsm) {
        entry->next_check_time = current_time + IMAP_RETRY_STEP_MS;
        logMessagef("IMAP: Account %d check interrupted by a network change (%s) - not counted as a failure",
                    account_id, result.error.c_str());
        return;
    }

    if (account->failed_check_cycles < 255) {
        account->failed_check_cycles++;
    }

    if (account->failed_check_cycles >= IMAP_MAX_FAILED_CHECKS) {
        account->suspended = true;
        logMessagef("IMAP: Account %d suspended after %d consecutive failures (%s)",
                    account_id, account->failed_check_cycles, result.error.c_str());
    } else {
        unsigned long backoff = min((unsigned long)account->failed_check_cycles * IMAP_RETRY_STEP_MS, (unsigned long)IMAP_RETRY_MAX_MS);
        entry->next_check_time = current_time + backoff;
        logMessagef("IMAP: Account %d check failed (%s), attempt %d/%d, retry in %lus",
                    account_id, result.error.c_str(), account->failed_check_cycles, IMAP_MAX_FAILED_CHECKS, backoff / 1000UL);
    }

    save_imap_config();
}

void imap_scheduler_loop() {
    if (!imap_system_enabled || imap_schedule.empty()) {
        return;
    }

    for (const auto& entry : imap_schedule) {
        if (entry.in_flight) {
            return;
        }
    }

    unsigned long current_time = millis();

    for (auto& entry : imap_schedule) {
        IMAPAccount* account = find_imap_account(entry.account_id);
        if (!account || account->suspended || (long)(current_time - entry.next_check_time) < 0) {
            continue;
        }

        logMessagef("IMAP: Processing account %d", entry.account_id);

        IMAPAccount snapshot = *account;
        uint8_t account_id = entry.account_id;
        uint32_t generation = imap_schedule_generation;
        auto result = std::make_shared<IMAPCheckResult>();

        entry.in_flight = true;
        bool queued = service_worker_submit("IMAP check",
            [snapshot, result]() { *result = imap_check_account(snapshot); },
            [account_id, generation, result]() { imap_apply_check_result(account_id, generation, *result); });

        if (!queued) {
            entry.in_flight = false;
            entry.next_check_time = current_time + IMAP_RETRY_STEP_MS;
        }
        return;
    }
}

// =============================================================================
// MANUAL ACCOUNT TEST (POST /imap_test/{index})
// =============================================================================
static void imap_apply_test_result(size_t index, uint8_t account_id, const String& account_name, const IMAPCheckResult& result) {
    IMAPTestStatus& status = imap_test_status[index];

    if (!result.success) {
        status.state = IMAPTestState::Failed;
        status.error = result.error;
        return;
    }

    status.unread = result.unread;
    status.unread_capped = result.unread >= IMAP_TEST_UNREAD_LIMIT;

    IMAPAccount* account = find_imap_account(account_id);
    if (!account || account_name != account->name) {
        status.state = IMAPTestState::Failed;
        status.error = "Account changed while the test was running";
        return;
    }

    String page = "IMAP OK " + account_name + ": " + String(result.unread) + (status.unread_capped ? "+" : "") + " unread";
    uint64_t capcode = account->capcode > 0 ? account->capcode : settings.default_capcode;
    float frequency = account->frequency > 0 ? account->frequency : settings.default_frequency;

    if (!queue_add_message(capcode, frequency, settings.default_txpower, account->mail_drop, page.c_str())) {
        status.state = IMAPTestState::Failed;
        status.error = "Connection OK but the transmission queue is full";
        return;
    }

    status.state = IMAPTestState::Passed;
    logMessagef("IMAP: Account %d test page queued to capcode %llu", account_id, (unsigned long long)capcode);

    if (account->suspended || account->failed_check_cycles != 0) {
        account->suspended = false;
        account->failed_check_cycles = 0;
        logMessagef("IMAP: Account %d reactivated after successful test", account_id);
        save_imap_config();
    }
}

bool imap_start_account_test(size_t index, String& error) {
    if (index >= imap_config.accounts.size() || index >= IMAP_MAX_ACCOUNTS) {
        error = "Account not found";
        return false;
    }

    IMAPTestStatus& status = imap_test_status[index];
    if (status.state == IMAPTestState::Running) {
        error = "A test for this account is already running";
        return false;
    }

    IMAPAccount snapshot = imap_config.accounts[index];
    uint8_t account_id = snapshot.id;
    String account_name = snapshot.name;
    auto result = std::make_shared<IMAPCheckResult>();

    status.state = IMAPTestState::Running;
    status.unread = 0;
    status.unread_capped = false;
    status.error = "";

    logMessagef("IMAP: Account %d test requested", account_id);

    if (!service_worker_submit("IMAP test",
            [snapshot, result]() { *result = imap_test_connection(snapshot); },
            [index, account_id, account_name, result]() { imap_apply_test_result(index, account_id, account_name, *result); })) {
        status.state = IMAPTestState::Failed;
        status.error = "Service worker busy, try again";
        error = status.error;
        return false;
    }

    return true;
}

const IMAPTestStatus& imap_account_test_status(size_t index) {
    static const IMAPTestStatus idle_status;
    if (index >= IMAP_MAX_ACCOUNTS) {
        return idle_status;
    }
    return imap_test_status[index];
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

    JsonDocument doc;
    doc["enabled"] = imap_config.enabled;
    doc["account_count"] = imap_config.account_count;

    JsonArray accounts = doc["accounts"].to<JsonArray>();
    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        JsonObject account = accounts.add<JsonObject>();
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
        account["failed_check_cycles"] = imap_config.accounts[i].failed_check_cycles;
        account["suspended"] = imap_config.accounts[i].suspended;
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

    JsonDocument doc;
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
        temp_account.failed_check_cycles = account["failed_check_cycles"] | 0;
        temp_account.suspended = account["suspended"] | false;

        if (temp_account.suspended) {
            logMessagef("IMAP: Account %d ('%s') is suspended after repeated failures", temp_account.id, temp_account.name);
        }

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

    JsonDocument doc;
    doc["enabled"] = false;
    doc["account_count"] = 0;

    JsonArray accounts = doc["accounts"].to<JsonArray>();

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
