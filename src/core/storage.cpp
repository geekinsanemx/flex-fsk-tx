#include "../core/storage.h"

#include <SPIFFS.h>

#include "../core/tx_lock.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

#include "../core/config.h"
#include "../version.h"
#include "../core/logging.h"
#include "../core/utils.h"
#include "../network/wifi.h"
#include "../network/gsm.h"
#ifdef ENABLE_CHATGPT
#include "../services/chatgpt.h"
#endif // ENABLE_CHATGPT
#include "../core/display.h"
#include "../network/ntp_time.h"
#include "../../include/boards/boards.h"

CoreConfig core_config;
DeviceSettings settings;
Preferences preferences;

// =============================================================================
// CERTIFICATE STORAGE (SPIFFS)
// =============================================================================
String loadCertificateFromSPIFFS(const char* filename) {
    FlashGuard fg;
    if (!fg.ok()) return "";

    File file = SPIFFS.open(filename, "r");
    if (!file) {
        return "";
    }

    String content = file.readString();
    file.close();
    return content;
}

bool saveCertificateToSPIFFS(const char* filename, const String& cert) {
    FlashGuard fg;
    if (!fg.ok()) return false;

    File file = SPIFFS.open(filename, "w");
    if (!file) {
        return false;
    }

    size_t bytesWritten = file.print(cert);
    file.flush();
    file.close();
    return (bytesWritten > 0);
}

bool certificateExistsInSPIFFS(const char* filename) {
    FlashGuard fg;
    if (!fg.ok()) return false;

    return SPIFFS.exists(filename);
}

String getCertificateStatusFromSPIFFS(const char* filename) {
    FlashGuard fg;
    if (!fg.ok()) return "Unknown";

    if (certificateExistsInSPIFFS(filename)) {
        File file = SPIFFS.open(filename, "r");
        if (file) {
            size_t size = file.size();
            file.close();
            return "<span class='text-success'>✅ Uploaded (" + String(size) + " bytes)</span>";
        }
    }
    return "<span class='text-danger'>❌ Not uploaded</span>";
}

void deleteAllCertificatesFromSPIFFS() {
    FlashGuard fg;
    if (!fg.ok()) return;

    SPIFFS.remove(MQTT_CA_CERT_FILE);
    SPIFFS.remove(MQTT_DEVICE_CERT_FILE);
    SPIFFS.remove(MQTT_DEVICE_KEY_FILE);
}

String getCertificateFilename(const String& certType) {
    if (certType == "mqtt_ca") return MQTT_CA_CERT_FILE;
    if (certType == "mqtt_cert") return MQTT_DEVICE_CERT_FILE;
    if (certType == "mqtt_key") return MQTT_DEVICE_KEY_FILE;
    return "";
}

// =============================================================================
// NVS CORE CONFIG
// =============================================================================
void load_default_core_config() {
    core_config.magic = CONFIG_MAGIC;
    core_config.version = CONFIG_VERSION;
    core_config.frequency_correction_ppm = 0.0;
    memset(core_config.reserved, 0, sizeof(core_config.reserved));

    deleteAllCertificatesFromSPIFFS();
}

bool save_core_config() {
    FlashGuard fg;
    if (!fg.ok()) return false;

    logMessagef("CONFIG: Saving core config - magic=0x%X, version=%d",
                  core_config.magic, core_config.version);
    logMessagef("CONFIG: CoreConfig struct size: %d bytes", sizeof(CoreConfig));

    if (!preferences.begin("flex-fsk", false)) {
        logMessage("CONFIG: Failed to open preferences for writing");
        return false;
    }

    size_t bytes_written = preferences.putBytes("config", &core_config, sizeof(CoreConfig));
    bool result = (bytes_written == sizeof(CoreConfig));

    preferences.end();

    logMessagef("CONFIG: Preferences write: %s (%d bytes)", result ? "SUCCESS" : "FAILED", bytes_written);

    return result;
}

bool load_core_config() {
    FlashGuard fg;
    if (!fg.ok()) return false;

    if (!preferences.begin("flex-fsk", true)) {
        logMessage("CONFIG: Failed to open preferences for reading, using defaults");
        load_default_core_config();
        save_core_config();
        return false;
    }

    CoreConfig temp_core_config;
    size_t bytes_read = preferences.getBytes("config", &temp_core_config, sizeof(CoreConfig));
    preferences.end();

    logMessagef("CONFIG: Loading core config - bytes_read=%d, expected=%d", bytes_read, sizeof(CoreConfig));

    if (bytes_read != sizeof(CoreConfig)) {
        logMessage("CONFIG: Core config size mismatch, using defaults");
        load_default_core_config();
        save_core_config();
        return false;
    }

    logMessagef("CONFIG: Loaded core config - magic=0x%X (expected 0x%X), version=%d, freq_correction=%.2f ppm",
                  temp_core_config.magic, CONFIG_MAGIC, temp_core_config.version, temp_core_config.frequency_correction_ppm);

    if (temp_core_config.magic != CONFIG_MAGIC) {
        logMessage("CONFIG: Invalid magic number, using defaults");
        load_default_core_config();
        save_core_config();
        return false;
    }

    if (temp_core_config.version != CONFIG_VERSION) {
        logMessagef("CONFIG: Core config version mismatch (found %d, expected %d), using factory defaults", temp_core_config.version, CONFIG_VERSION);
        load_default_core_config();
        save_core_config();
        return false;
    }

    core_config = temp_core_config;

    if (core_config.frequency_correction_ppm != 0.0) {
        settings.frequency_correction_ppm = core_config.frequency_correction_ppm;
        logMessagef("CONFIG: Using NVS frequency_correction_ppm: %.2f", core_config.frequency_correction_ppm);
    }

    logMessage("CONFIG: Core config loaded successfully");
    return true;
}

// =============================================================================
// SPIFFS RUNTIME SETTINGS
// =============================================================================
bool save_runtime_settings() {
    FlashGuard fg;
    if (!fg.ok()) return false;

    logMessage("SETTINGS: Saving configuration to /settings.json");

    File file = SPIFFS.open("/settings.json", "w");
    if (!file) {
        logMessage("SETTINGS: Failed to create settings file");
        return false;
    }

    DynamicJsonDocument doc(4096);

    JsonObject device = doc.createNestedObject("device");
    device["theme"] = settings.theme;
    device["banner_message"] = settings.banner_message;
    device["timezone_offset_hours"] = settings.timezone_offset_hours;
    device["ntp_server"] = settings.ntp_server;

    JsonObject alerts = doc.createNestedObject("alerts");
    alerts["low_battery"] = settings.enable_low_battery_alert;
    alerts["power_disconnect"] = settings.enable_power_disconnect_alert;

    JsonObject flex = doc.createNestedObject("flex");
    flex["default_frequency"] = settings.default_frequency;
    flex["default_capcode"] = String(settings.default_capcode);
    flex["default_txpower"] = settings.default_txpower;
    flex["frequency_correction_ppm"] = settings.frequency_correction_ppm;

    JsonObject api = doc.createNestedObject("api");
    api["enabled"] = settings.api_enabled;
    api["http_port"] = settings.http_port;
    api["username"] = settings.api_username;
    api["password"] = base64_encode_string(String(settings.api_password));

    JsonObject services = doc.createNestedObject("services");
    services["mqtt_enabled"] = settings.mqtt_enabled;
    services["mqtt_boot_delay_ms"] = settings.mqtt_boot_delay_ms;
    services["mqtt_notify_failures"] = settings.mqtt_notify_failures;
    services["mqtt_retry_interval_mins"] = settings.mqtt_retry_interval_mins;
    services["imap_enabled"] = settings.imap_enabled;
    services["grafana_enabled"] = settings.grafana_enabled;

    JsonObject mqtt = doc.createNestedObject("mqtt");
    mqtt["server"] = settings.mqtt_server;
    mqtt["port"] = settings.mqtt_port;
    mqtt["thing_name"] = settings.mqtt_thing_name;
    mqtt["subscribe_topic"] = settings.mqtt_subscribe_topic;
    mqtt["publish_topic"] = settings.mqtt_publish_topic;
    mqtt["notify_failures"] = settings.mqtt_notify_failures;
    mqtt["retry_interval_mins"] = settings.mqtt_retry_interval_mins;

    JsonObject rsyslog = doc.createNestedObject("rsyslog");
    rsyslog["enabled"] = settings.rsyslog_enabled;
    rsyslog["server"] = settings.rsyslog_server;
    rsyslog["port"] = settings.rsyslog_port;
    rsyslog["use_tcp"] = settings.rsyslog_use_tcp;
    rsyslog["min_severity"] = settings.rsyslog_min_severity;

    JsonObject rf_amplifier = doc.createNestedObject("rf_amplifier");
    rf_amplifier["enabled"] = settings.enable_rf_amplifier;
    rf_amplifier["power_pin"] = settings.rf_amplifier_power_pin;
    rf_amplifier["delay_ms"] = settings.rf_amplifier_delay_ms;
    rf_amplifier["active_high"] = settings.rf_amplifier_active_high;

#ifdef ENABLE_GSM
    JsonObject gsm = doc.createNestedObject("gsm");
    gsm["enabled"] = gsm_config.enable_gsm;
    gsm["apn"] = gsm_config.apn;
    gsm["apn_user"] = gsm_config.apn_user;
    gsm["apn_pass"] = base64_encode_string(String(gsm_config.apn_pass));
    gsm["pin"] = base64_encode_string(String(gsm_config.pin));
    gsm["tx_pin"] = gsm_config.tx_pin;
    gsm["rx_pin"] = gsm_config.rx_pin;
    gsm["power_pin"] = gsm_config.power_pin;
    gsm["baudrate"] = gsm_config.baudrate;
    gsm["connection_timeout"] = gsm_config.connection_timeout;
    gsm["require_cell_signal"] = gsm_config.require_cell_signal;
    gsm["min_signal_quality"] = gsm_config.min_signal_quality;
#endif

    JsonObject wifi = doc.createNestedObject("wifi");
    for (int i = 0; i < stored_networks_count; i++) {
        JsonObject net = wifi.createNestedObject(stored_networks[i].ssid);
        net["password"] = base64_encode_string(String(stored_networks[i].password));
        net["use_dhcp"] = stored_networks[i].use_dhcp;
        net["static_ip"] = String(stored_networks[i].static_ip[0]) + "." + String(stored_networks[i].static_ip[1]) + "." + String(stored_networks[i].static_ip[2]) + "." + String(stored_networks[i].static_ip[3]);
        net["netmask"] = String(stored_networks[i].netmask[0]) + "." + String(stored_networks[i].netmask[1]) + "." + String(stored_networks[i].netmask[2]) + "." + String(stored_networks[i].netmask[3]);
        net["gateway"] = String(stored_networks[i].gateway[0]) + "." + String(stored_networks[i].gateway[1]) + "." + String(stored_networks[i].gateway[2]) + "." + String(stored_networks[i].gateway[3]);
        net["dns"] = String(stored_networks[i].dns[0]) + "." + String(stored_networks[i].dns[1]) + "." + String(stored_networks[i].dns[2]) + "." + String(stored_networks[i].dns[3]);
    }

    core_config.frequency_correction_ppm = settings.frequency_correction_ppm;
    save_core_config();

    if (serializeJson(doc, file) == 0) {
        logMessage("SETTINGS: Failed to write settings JSON");
        file.flush();
        file.close();
        return false;
    }

    file.flush();
    file.close();
    logMessage("SETTINGS: Configuration saved successfully");
    return true;
}

bool load_runtime_settings() {
    FlashGuard fg;
    if (!fg.ok()) return false;

    logMessage("SETTINGS: Loading configuration from /settings.json");

    if (!SPIFFS.exists("/settings.json")) {
        logMessage("SETTINGS: Config file not found, using defaults");
        load_default_settings();
        save_runtime_settings();
        return false;
    }

    File file = SPIFFS.open("/settings.json", "r");
    if (!file) {
        logMessage("SETTINGS: Failed to open config file");
        load_default_settings();
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        logMessage("SETTINGS: Failed to parse config JSON, using defaults");
        load_default_settings();
        save_runtime_settings();
        return false;
    }

    settings.mqtt_boot_delay_ms = 0;

    if (doc.containsKey("device")) {
        JsonObject device = doc["device"];
        settings.theme = device["theme"] | 0;
        strlcpy(settings.banner_message, device["banner_message"] | "flex-fsk-tx", sizeof(settings.banner_message));
        settings.timezone_offset_hours = device["timezone_offset_hours"] | 0.0;
        strlcpy(settings.ntp_server, device["ntp_server"] | "pool.ntp.org", sizeof(settings.ntp_server));
    }

    if (doc.containsKey("alerts")) {
        JsonObject alerts = doc["alerts"];
        settings.enable_low_battery_alert = alerts["low_battery"] | true;
        settings.enable_power_disconnect_alert = alerts["power_disconnect"] | true;
    }

    if (doc.containsKey("flex")) {
        JsonObject flex = doc["flex"];
        settings.default_frequency = flex["default_frequency"] | 931.9375;
        settings.default_capcode = strtoull(flex["default_capcode"] | "37137", nullptr, 10);
        settings.default_txpower = flex["default_txpower"] | 10.0;
        settings.frequency_correction_ppm = flex["frequency_correction_ppm"] | 0.0;
    }

    if (doc.containsKey("api")) {
        JsonObject api = doc["api"];
        settings.api_enabled = api["enabled"] | true;
        settings.http_port = api["http_port"] | 80;
        strlcpy(settings.api_username, api["username"] | "admin", sizeof(settings.api_username));
        String decoded_password = base64_decode_string(api["password"] | "");
        strlcpy(settings.api_password, decoded_password.c_str(), sizeof(settings.api_password));
    }

    if (doc.containsKey("services")) {
        JsonObject services = doc["services"];
        settings.mqtt_enabled = services["mqtt_enabled"] | false;
        settings.mqtt_boot_delay_ms = services["mqtt_boot_delay_ms"] | 0;
        settings.mqtt_notify_failures = services["mqtt_notify_failures"] | true;
        settings.mqtt_retry_interval_mins = services["mqtt_retry_interval_mins"] | 60;
        settings.imap_enabled = services["imap_enabled"] | true;
        settings.grafana_enabled = services["grafana_enabled"] | true;
    }

    if (doc.containsKey("mqtt")) {
        JsonObject mqtt = doc["mqtt"];
        strlcpy(settings.mqtt_server, mqtt["server"] | "", sizeof(settings.mqtt_server));
        settings.mqtt_port = mqtt["port"] | 8883;
        strlcpy(settings.mqtt_thing_name, mqtt["thing_name"] | "", sizeof(settings.mqtt_thing_name));
        strlcpy(settings.mqtt_subscribe_topic, mqtt["subscribe_topic"] | "", sizeof(settings.mqtt_subscribe_topic));
        strlcpy(settings.mqtt_publish_topic, mqtt["publish_topic"] | "", sizeof(settings.mqtt_publish_topic));
    }

    if (doc.containsKey("rsyslog")) {
        JsonObject rsyslog = doc["rsyslog"];
        settings.rsyslog_enabled = rsyslog["enabled"] | false;
        strlcpy(settings.rsyslog_server, rsyslog["server"] | "", sizeof(settings.rsyslog_server));
        settings.rsyslog_port = rsyslog["port"] | 514;
        settings.rsyslog_use_tcp = rsyslog["use_tcp"] | false;
        settings.rsyslog_min_severity = rsyslog["min_severity"] | 6;
    }

    if (doc.containsKey("rf_amplifier")) {
        JsonObject rf_amplifier = doc["rf_amplifier"];
        settings.enable_rf_amplifier = rf_amplifier["enabled"] | false;
        settings.rf_amplifier_power_pin = rf_amplifier["power_pin"] | RFAMP_PWR_PIN;
        settings.rf_amplifier_delay_ms = rf_amplifier["delay_ms"] | 200;
        settings.rf_amplifier_active_high = rf_amplifier["active_high"] | true;
    } else {
        settings.enable_rf_amplifier = false;
        settings.rf_amplifier_power_pin = RFAMP_PWR_PIN;
        settings.rf_amplifier_delay_ms = 200;
        settings.rf_amplifier_active_high = true;
    }

    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];

        int net_index = 0;
        for (JsonPair kv : wifi) {
            if (net_index >= MAX_WIFI_NETWORKS) break;

            const char* ssid = kv.key().c_str();
            JsonObject network = kv.value();

            strlcpy(stored_networks[net_index].ssid, ssid, sizeof(stored_networks[net_index].ssid));

            String password = base64_decode_string(network["password"] | "");
            strlcpy(stored_networks[net_index].password, password.c_str(), sizeof(stored_networks[net_index].password));

            stored_networks[net_index].use_dhcp = network["use_dhcp"] | true;

            String static_ip_str = network["static_ip"] | "192.168.1.100";
            sscanf(static_ip_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[net_index].static_ip[0],
                   &stored_networks[net_index].static_ip[1],
                   &stored_networks[net_index].static_ip[2],
                   &stored_networks[net_index].static_ip[3]);

            String netmask_str = network["netmask"] | "255.255.255.0";
            sscanf(netmask_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[net_index].netmask[0],
                   &stored_networks[net_index].netmask[1],
                   &stored_networks[net_index].netmask[2],
                   &stored_networks[net_index].netmask[3]);

            String gateway_str = network["gateway"] | "192.168.1.1";
            sscanf(gateway_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[net_index].gateway[0],
                   &stored_networks[net_index].gateway[1],
                   &stored_networks[net_index].gateway[2],
                   &stored_networks[net_index].gateway[3]);

            String dns_str = network["dns"] | "8.8.8.8";
            sscanf(dns_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[net_index].dns[0],
                   &stored_networks[net_index].dns[1],
                   &stored_networks[net_index].dns[2],
                   &stored_networks[net_index].dns[3]);

            net_index++;
        }
        stored_networks_count = net_index;

        logMessagef("WIFI: Loaded %d network(s) from settings", stored_networks_count);
    }

#ifdef ENABLE_GSM
    if (doc.containsKey("gsm")) {
        JsonObject gsm = doc["gsm"];
        gsm_config.enable_gsm = gsm["enabled"] | false;
        strlcpy(gsm_config.apn, gsm["apn"] | "internet.itelcel.com", sizeof(gsm_config.apn));
        strlcpy(gsm_config.apn_user, gsm["apn_user"] | "webgprs", sizeof(gsm_config.apn_user));

        String apn_pass_b64 = gsm["apn_pass"] | "";
        String apn_pass = base64_decode_string(apn_pass_b64);
        strlcpy(gsm_config.apn_pass, apn_pass.c_str(), sizeof(gsm_config.apn_pass));

        String pin_b64 = gsm["pin"] | "";
        String pin = base64_decode_string(pin_b64);
        strlcpy(gsm_config.pin, pin.c_str(), sizeof(gsm_config.pin));

        gsm_config.tx_pin = gsm["tx_pin"] | GSM_TX_PIN;
        gsm_config.rx_pin = gsm["rx_pin"] | GSM_RX_PIN;
        gsm_config.power_pin = gsm["power_pin"] | GSM_PWR_PIN;
        gsm_config.baudrate = gsm["baudrate"] | 115200;
        gsm_config.connection_timeout = gsm["connection_timeout"] | 30000;
        gsm_config.require_cell_signal = gsm["require_cell_signal"] | true;
        gsm_config.min_signal_quality = gsm["min_signal_quality"] | 0;
    }
#endif

    logMessage("SETTINGS: Configuration loaded successfully");
    return true;
}

void load_default_settings() {
    logMessage("SETTINGS: Loading default settings configuration");

    settings.theme = 0;
    strlcpy(settings.banner_message, "flex-fsk-tx", sizeof(settings.banner_message));
    settings.timezone_offset_hours = 0.0;
    strlcpy(settings.ntp_server, "pool.ntp.org", sizeof(settings.ntp_server));

    settings.enable_low_battery_alert = true;
    settings.enable_power_disconnect_alert = true;

    settings.default_frequency = 931.9375;
    settings.default_capcode = 37137;
    settings.default_txpower = 10.0;
    settings.frequency_correction_ppm = (core_config.frequency_correction_ppm != 0.0)
        ? core_config.frequency_correction_ppm
        : 0.0;

    settings.api_enabled = true;
    settings.http_port = 80;
    strlcpy(settings.api_username, "admin", sizeof(settings.api_username));
    strlcpy(settings.api_password, "passw0rd", sizeof(settings.api_password));

    settings.mqtt_enabled = false;
    settings.mqtt_boot_delay_ms = 0;
    settings.mqtt_notify_failures = true;
    settings.mqtt_retry_interval_mins = 60;
    settings.imap_enabled = true;
    settings.grafana_enabled = true;

    strlcpy(settings.mqtt_server, "", sizeof(settings.mqtt_server));
    settings.mqtt_port = 8883;
    strlcpy(settings.mqtt_thing_name, "", sizeof(settings.mqtt_thing_name));
    strlcpy(settings.mqtt_subscribe_topic, "", sizeof(settings.mqtt_subscribe_topic));
    strlcpy(settings.mqtt_publish_topic, "", sizeof(settings.mqtt_publish_topic));

    settings.rsyslog_enabled = false;
    strlcpy(settings.rsyslog_server, "", sizeof(settings.rsyslog_server));
    settings.rsyslog_port = 514;
    settings.rsyslog_use_tcp = false;
    settings.rsyslog_min_severity = 6;

    settings.enable_rf_amplifier = false;
    settings.rf_amplifier_power_pin = RFAMP_PWR_PIN;
    settings.rf_amplifier_delay_ms = 200;
    settings.rf_amplifier_active_high = true;

#ifdef ENABLE_GSM
    gsm_config.enable_gsm = false;
    strlcpy(gsm_config.apn, "internet.itelcel.com", sizeof(gsm_config.apn));
    strlcpy(gsm_config.apn_user, "webgprs", sizeof(gsm_config.apn_user));
    strlcpy(gsm_config.apn_pass, "webgprs2002", sizeof(gsm_config.apn_pass));
    strlcpy(gsm_config.pin, "", sizeof(gsm_config.pin));
    gsm_config.tx_pin = GSM_TX_PIN;
    gsm_config.rx_pin = GSM_RX_PIN;
    gsm_config.power_pin = GSM_PWR_PIN;
    gsm_config.baudrate = 115200;
    gsm_config.connection_timeout = 30000;
    gsm_config.require_cell_signal = true;
    gsm_config.min_signal_quality = 0;
#endif
}

// =============================================================================
// BACKUP / RESTORE
// =============================================================================
String export_user_backup() {
    FlashGuard fg;
    if (!fg.ok()) return "";

    DynamicJsonDocument doc(8192);

    doc["version"] = FIRMWARE_VERSION;
    doc["timestamp"] = getLocalTimestamp();

    JsonObject cfg = doc.createNestedObject("config");

    JsonObject device = cfg.createNestedObject("device");
    device["theme"] = settings.theme;
    device["banner_message"] = String(settings.banner_message);
    device["timezone_offset_hours"] = settings.timezone_offset_hours;
    device["ntp_server"] = String(settings.ntp_server);

    JsonObject wifi = cfg.createNestedObject("wifi");
    for (int i = 0; i < stored_networks_count; i++) {
        JsonObject net = wifi.createNestedObject(stored_networks[i].ssid);
        net["password"] = base64_encode_string(String(stored_networks[i].password));
        net["use_dhcp"] = stored_networks[i].use_dhcp;
        net["static_ip"] = String(stored_networks[i].static_ip[0]) + "." + String(stored_networks[i].static_ip[1]) + "." + String(stored_networks[i].static_ip[2]) + "." + String(stored_networks[i].static_ip[3]);
        net["netmask"] = String(stored_networks[i].netmask[0]) + "." + String(stored_networks[i].netmask[1]) + "." + String(stored_networks[i].netmask[2]) + "." + String(stored_networks[i].netmask[3]);
        net["gateway"] = String(stored_networks[i].gateway[0]) + "." + String(stored_networks[i].gateway[1]) + "." + String(stored_networks[i].gateway[2]) + "." + String(stored_networks[i].gateway[3]);
        net["dns"] = String(stored_networks[i].dns[0]) + "." + String(stored_networks[i].dns[1]) + "." + String(stored_networks[i].dns[2]) + "." + String(stored_networks[i].dns[3]);
    }

    JsonObject alerts = cfg.createNestedObject("alerts");
    alerts["low_battery"] = settings.enable_low_battery_alert;
    alerts["power_disconnect"] = settings.enable_power_disconnect_alert;

    JsonObject flex = cfg.createNestedObject("flex");
    flex["default_frequency"] = settings.default_frequency;
    flex["default_capcode"] = String(settings.default_capcode);
    flex["default_txpower"] = settings.default_txpower;
    flex["frequency_correction_ppm"] = settings.frequency_correction_ppm;

    JsonObject rf_amplifier = cfg.createNestedObject("rf_amplifier");
    rf_amplifier["enabled"] = settings.enable_rf_amplifier;
    rf_amplifier["power_pin"] = settings.rf_amplifier_power_pin;
    rf_amplifier["delay_ms"] = settings.rf_amplifier_delay_ms;
    rf_amplifier["active_high"] = settings.rf_amplifier_active_high;

    JsonObject api = cfg.createNestedObject("api");
    api["enable"] = settings.api_enabled;
    api["http_port"] = settings.http_port;
    api["username"] = String(settings.api_username);
    api["password"] = base64_encode_string(String(settings.api_password));

    JsonObject grafana = cfg.createNestedObject("grafana");
    grafana["enable"] = settings.grafana_enabled;

    JsonObject mqtt = cfg.createNestedObject("mqtt");
    mqtt["enabled"] = settings.mqtt_enabled;
    mqtt["server"] = String(settings.mqtt_server);
    mqtt["port"] = settings.mqtt_port;
    mqtt["thing_name"] = String(settings.mqtt_thing_name);
    mqtt["subscribe_topic"] = String(settings.mqtt_subscribe_topic);
    mqtt["publish_topic"] = String(settings.mqtt_publish_topic);
    mqtt["notify_failures"] = settings.mqtt_notify_failures;
    mqtt["retry_interval_mins"] = settings.mqtt_retry_interval_mins;

    JsonObject certs = mqtt.createNestedObject("certificates_b64");

    certs["root_ca"] = base64_encode_string(loadCertificateFromSPIFFS(MQTT_CA_CERT_FILE));
    certs["device_cert"] = base64_encode_string(loadCertificateFromSPIFFS(MQTT_DEVICE_CERT_FILE));
    certs["device_key"] = base64_encode_string(loadCertificateFromSPIFFS(MQTT_DEVICE_KEY_FILE));

    JsonObject rsyslog = cfg.createNestedObject("rsyslog");
    rsyslog["enabled"] = settings.rsyslog_enabled;
    rsyslog["server"] = String(settings.rsyslog_server);
    rsyslog["port"] = settings.rsyslog_port;
    rsyslog["use_tcp"] = settings.rsyslog_use_tcp;
    rsyslog["min_severity"] = settings.rsyslog_min_severity;

#ifdef ENABLE_GSM
    JsonObject gsm = cfg.createNestedObject("gsm");
    gsm["enabled"] = gsm_config.enable_gsm;
    gsm["apn"] = String(gsm_config.apn);
    gsm["apn_user"] = String(gsm_config.apn_user);
    gsm["apn_pass"] = base64_encode_string(String(gsm_config.apn_pass));
    gsm["pin"] = base64_encode_string(String(gsm_config.pin));
    gsm["tx_pin"] = gsm_config.tx_pin;
    gsm["rx_pin"] = gsm_config.rx_pin;
    gsm["power_pin"] = gsm_config.power_pin;
    gsm["baudrate"] = gsm_config.baudrate;
    gsm["connection_timeout"] = gsm_config.connection_timeout;
    gsm["require_cell_signal"] = gsm_config.require_cell_signal;
    gsm["min_signal_quality"] = gsm_config.min_signal_quality;
#endif

    JsonObject imap = cfg.createNestedObject("imap");

    File imap_file = SPIFFS.open("/imap_settings.json", "r");
    if (imap_file) {
        String imap_content = imap_file.readString();
        imap_file.close();

        DynamicJsonDocument imap_doc(8192);
        DeserializationError error = deserializeJson(imap_doc, imap_content);
        if (!error) {
            JsonObject source = imap_doc.as<JsonObject>();
            for (JsonPair kv : source) {
                imap[kv.key()] = kv.value();
            }
            logMessage("BACKUP: IMAP settings packed directly in backup");
        } else {
            logMessage("BACKUP ERROR: Failed to parse IMAP settings JSON");
        }
    } else {
        logMessage("BACKUP: No IMAP settings file found");
    }

    JsonObject chatgpt = cfg.createNestedObject("chatgpt");

    File chatgpt_file = SPIFFS.open("/chatgpt_settings.json", "r");
    if (chatgpt_file) {
        String chatgpt_content = chatgpt_file.readString();
        chatgpt_file.close();

        DynamicJsonDocument chatgpt_doc(8192);
        DeserializationError error = deserializeJson(chatgpt_doc, chatgpt_content);
        if (!error) {
            JsonObject source = chatgpt_doc.as<JsonObject>();
            for (JsonPair kv : source) {
                chatgpt[kv.key()] = kv.value();
            }
            logMessage("BACKUP: ChatGPT prompts packed directly in backup");
        } else {
            logMessage("BACKUP ERROR: Failed to parse ChatGPT prompts JSON");
        }
    } else {
        logMessage("BACKUP: No ChatGPT prompts file found");
    }

    String temp_json;
    serializeJson(doc, temp_json);
    String checksum = calculate_crc32(temp_json);
    doc["checksum"] = checksum;

    String json_string;
    serializeJsonPretty(doc, json_string);

    return json_string;
}

bool import_user_backup(const String& json_string, String& error_msg) {
    FlashGuard fg;
    if (!fg.ok()) {
        error_msg = "Flash busy - RF transmission in progress";
        return false;
    }

    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, json_string);

    if (error) {
        error_msg = "Invalid JSON format: " + String(error.c_str());
        return false;
    }

    if (!doc.containsKey("checksum") || !doc.containsKey("config")) {
        error_msg = "Missing required fields in backup file";
        return false;
    }

    String stored_checksum = doc["checksum"].as<String>();
    doc.remove("checksum");

    String temp_json;
    serializeJson(doc, temp_json);
    String calculated_checksum = calculate_crc32(temp_json);

    if (stored_checksum != calculated_checksum) {
        error_msg = "Checksum validation failed - file may be corrupted";
        return false;
    }

    JsonObject cfg = doc["config"];
    CoreConfig temp_core_config = core_config;
    DeviceSettings temp_settings = settings;
#ifdef ENABLE_GSM
    GSMConfig temp_gsm = gsm_config;
#endif

    if (cfg.containsKey("device")) {
        JsonObject device = cfg["device"];
        if (device.containsKey("theme"))
            temp_settings.theme = device["theme"];
        if (device.containsKey("banner_message"))
            strncpy(temp_settings.banner_message, device["banner_message"].as<String>().c_str(), sizeof(temp_settings.banner_message) - 1);
        if (device.containsKey("timezone_offset_hours"))
            temp_settings.timezone_offset_hours = device["timezone_offset_hours"];
        if (device.containsKey("ntp_server"))
            strncpy(temp_settings.ntp_server, device["ntp_server"].as<String>().c_str(), sizeof(temp_settings.ntp_server) - 1);
    }

    if (cfg.containsKey("wifi")) {
        JsonObject wifi = cfg["wifi"];

        stored_networks_count = 0;
        for (JsonPair kv : wifi) {
            if (stored_networks_count >= MAX_WIFI_NETWORKS) break;

            const char* ssid = kv.key().c_str();
            JsonObject network = kv.value();

            strlcpy(stored_networks[stored_networks_count].ssid, ssid, sizeof(stored_networks[stored_networks_count].ssid));

            String password = base64_decode_string(network["password"] | "");
            strlcpy(stored_networks[stored_networks_count].password, password.c_str(), sizeof(stored_networks[stored_networks_count].password));

            stored_networks[stored_networks_count].use_dhcp = network["use_dhcp"] | true;

            String static_ip_str = network["static_ip"] | "192.168.1.100";
            sscanf(static_ip_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[stored_networks_count].static_ip[0],
                   &stored_networks[stored_networks_count].static_ip[1],
                   &stored_networks[stored_networks_count].static_ip[2],
                   &stored_networks[stored_networks_count].static_ip[3]);

            String netmask_str = network["netmask"] | "255.255.255.0";
            sscanf(netmask_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[stored_networks_count].netmask[0],
                   &stored_networks[stored_networks_count].netmask[1],
                   &stored_networks[stored_networks_count].netmask[2],
                   &stored_networks[stored_networks_count].netmask[3]);

            String gateway_str = network["gateway"] | "192.168.1.1";
            sscanf(gateway_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[stored_networks_count].gateway[0],
                   &stored_networks[stored_networks_count].gateway[1],
                   &stored_networks[stored_networks_count].gateway[2],
                   &stored_networks[stored_networks_count].gateway[3]);

            String dns_str = network["dns"] | "8.8.8.8";
            sscanf(dns_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[stored_networks_count].dns[0],
                   &stored_networks[stored_networks_count].dns[1],
                   &stored_networks[stored_networks_count].dns[2],
                   &stored_networks[stored_networks_count].dns[3]);

            stored_networks_count++;
        }

        logMessagef("RESTORE: Loaded %d WiFi network(s)", stored_networks_count);
    }

#ifdef ENABLE_GSM
    if (cfg.containsKey("gsm")) {
        JsonObject gsm = cfg["gsm"];
        if (gsm.containsKey("enabled"))
            temp_gsm.enable_gsm = gsm["enabled"];
        if (gsm.containsKey("apn"))
            strlcpy(temp_gsm.apn, gsm["apn"].as<String>().c_str(), sizeof(temp_gsm.apn));
        if (gsm.containsKey("apn_user"))
            strlcpy(temp_gsm.apn_user, gsm["apn_user"].as<String>().c_str(), sizeof(temp_gsm.apn_user));
        if (gsm.containsKey("apn_pass")) {
            String pass = base64_decode_string(gsm["apn_pass"].as<String>());
            strlcpy(temp_gsm.apn_pass, pass.c_str(), sizeof(temp_gsm.apn_pass));
        }
        if (gsm.containsKey("pin")) {
            String decoded_pin = base64_decode_string(gsm["pin"].as<String>());
            strlcpy(temp_gsm.pin, decoded_pin.c_str(), sizeof(temp_gsm.pin));
        }
        if (gsm.containsKey("tx_pin"))
            temp_gsm.tx_pin = gsm["tx_pin"];
        if (gsm.containsKey("rx_pin"))
            temp_gsm.rx_pin = gsm["rx_pin"];
        if (gsm.containsKey("power_pin"))
            temp_gsm.power_pin = gsm["power_pin"];
        if (gsm.containsKey("baudrate"))
            temp_gsm.baudrate = gsm["baudrate"];
        if (gsm.containsKey("connection_timeout"))
            temp_gsm.connection_timeout = gsm["connection_timeout"];
        if (gsm.containsKey("require_cell_signal"))
            temp_gsm.require_cell_signal = gsm["require_cell_signal"];
        if (gsm.containsKey("min_signal_quality"))
            temp_gsm.min_signal_quality = gsm["min_signal_quality"];

        logMessagef("RESTORE: GSM APN='%s', User='%s', Pass='%s'",
                    temp_gsm.apn, temp_gsm.apn_user, temp_gsm.apn_pass);
    }
#endif

    if (cfg.containsKey("alerts")) {
        JsonObject alerts = cfg["alerts"];
        if (alerts.containsKey("low_battery"))
            temp_settings.enable_low_battery_alert = alerts["low_battery"];
        if (alerts.containsKey("power_disconnect"))
            temp_settings.enable_power_disconnect_alert = alerts["power_disconnect"];
    }

    if (cfg.containsKey("flex")) {
        JsonObject flex = cfg["flex"];
        if (flex.containsKey("default_frequency"))
            temp_settings.default_frequency = flex["default_frequency"];
        if (flex.containsKey("default_capcode"))
            temp_settings.default_capcode = strtoull(flex["default_capcode"].as<String>().c_str(), NULL, 10);
        if (flex.containsKey("default_txpower"))
            temp_settings.default_txpower = flex["default_txpower"];
        if (flex.containsKey("frequency_correction_ppm")) {
            float backup_freq_corr = flex["frequency_correction_ppm"];
            if (core_config.frequency_correction_ppm != 0.0) {
                temp_settings.frequency_correction_ppm = core_config.frequency_correction_ppm;
                logMessagef("RESTORE: Using NVS frequency_correction_ppm: %.2f (ignoring backup)", core_config.frequency_correction_ppm);
            } else if (backup_freq_corr != 0.0) {
                temp_settings.frequency_correction_ppm = backup_freq_corr;
                temp_core_config.frequency_correction_ppm = backup_freq_corr;
                logMessagef("RESTORE: Using backup frequency_correction_ppm: %.2f", backup_freq_corr);
            } else {
                temp_settings.frequency_correction_ppm = 0.0;
            }
        }
    }

    if (cfg.containsKey("rf_amplifier")) {
        JsonObject rf_amplifier = cfg["rf_amplifier"];
        if (rf_amplifier.containsKey("enabled"))
            temp_settings.enable_rf_amplifier = rf_amplifier["enabled"];
        if (rf_amplifier.containsKey("power_pin"))
            temp_settings.rf_amplifier_power_pin = rf_amplifier["power_pin"];
        if (rf_amplifier.containsKey("delay_ms"))
            temp_settings.rf_amplifier_delay_ms = rf_amplifier["delay_ms"];
        if (rf_amplifier.containsKey("active_high"))
            temp_settings.rf_amplifier_active_high = rf_amplifier["active_high"];
    }

    if (cfg.containsKey("api")) {
        JsonObject api = cfg["api"];
        if (api.containsKey("enable"))
            temp_settings.api_enabled = api["enable"];
        if (api.containsKey("http_port"))
            temp_settings.http_port = api["http_port"];
        if (api.containsKey("username"))
            strncpy(temp_settings.api_username, api["username"].as<String>().c_str(), sizeof(temp_settings.api_username) - 1);
        if (api.containsKey("password")) {
            String decoded_password = base64_decode_string(api["password"].as<String>());
            strncpy(temp_settings.api_password, decoded_password.c_str(), sizeof(temp_settings.api_password) - 1);
        }
    }

    if (cfg.containsKey("grafana")) {
        JsonObject grafana = cfg["grafana"];
        if (grafana.containsKey("enable"))
            temp_settings.grafana_enabled = grafana["enable"];
    }

    if (cfg.containsKey("mqtt")) {
        JsonObject mqtt = cfg["mqtt"];
        if (mqtt.containsKey("enabled"))
            temp_settings.mqtt_enabled = mqtt["enabled"];
        if (mqtt.containsKey("server"))
            strncpy(temp_settings.mqtt_server, mqtt["server"].as<String>().c_str(), sizeof(temp_settings.mqtt_server) - 1);
        if (mqtt.containsKey("port"))
            temp_settings.mqtt_port = mqtt["port"];
        if (mqtt.containsKey("thing_name"))
            strncpy(temp_settings.mqtt_thing_name, mqtt["thing_name"].as<String>().c_str(), sizeof(temp_settings.mqtt_thing_name) - 1);
        if (mqtt.containsKey("subscribe_topic"))
            strncpy(temp_settings.mqtt_subscribe_topic, mqtt["subscribe_topic"].as<String>().c_str(), sizeof(temp_settings.mqtt_subscribe_topic) - 1);
        if (mqtt.containsKey("publish_topic"))
            strncpy(temp_settings.mqtt_publish_topic, mqtt["publish_topic"].as<String>().c_str(), sizeof(temp_settings.mqtt_publish_topic) - 1);
        if (mqtt.containsKey("notify_failures"))
            temp_settings.mqtt_notify_failures = mqtt["notify_failures"];
        if (mqtt.containsKey("retry_interval_mins"))
            temp_settings.mqtt_retry_interval_mins = mqtt["retry_interval_mins"];

        if (mqtt.containsKey("certificates_b64")) {
            JsonObject certs = mqtt["certificates_b64"];
            if (certs.containsKey("root_ca") && !certs["root_ca"].as<String>().isEmpty()) {
                String decoded = base64_decode_string(certs["root_ca"].as<String>());
                if (saveCertificateToSPIFFS(MQTT_CA_CERT_FILE, decoded)) {
                    logMessage("RESTORE: Root CA certificate restored to SPIFFS (" + String(decoded.length()) + " bytes)");
                } else {
                    logMessage("RESTORE ERROR: Failed to save Root CA to SPIFFS");
                }
            }
            if (certs.containsKey("device_cert") && !certs["device_cert"].as<String>().isEmpty()) {
                String decoded = base64_decode_string(certs["device_cert"].as<String>());
                if (saveCertificateToSPIFFS(MQTT_DEVICE_CERT_FILE, decoded)) {
                    logMessage("RESTORE: Device certificate restored to SPIFFS (" + String(decoded.length()) + " bytes)");
                } else {
                    logMessage("RESTORE ERROR: Failed to save Device certificate to SPIFFS");
                }
            }
            if (certs.containsKey("device_key") && !certs["device_key"].as<String>().isEmpty()) {
                String decoded = base64_decode_string(certs["device_key"].as<String>());
                if (saveCertificateToSPIFFS(MQTT_DEVICE_KEY_FILE, decoded)) {
                    logMessage("RESTORE: Device private key restored to SPIFFS (" + String(decoded.length()) + " bytes)");
                } else {
                    logMessage("RESTORE ERROR: Failed to save Device private key to SPIFFS");
                }
            }
        }
    }

    if (cfg.containsKey("rsyslog")) {
        JsonObject rsyslog = cfg["rsyslog"];
        if (rsyslog.containsKey("enabled"))
            temp_settings.rsyslog_enabled = rsyslog["enabled"];
        if (rsyslog.containsKey("server"))
            strncpy(temp_settings.rsyslog_server, rsyslog["server"].as<String>().c_str(), sizeof(temp_settings.rsyslog_server) - 1);
        if (rsyslog.containsKey("port"))
            temp_settings.rsyslog_port = rsyslog["port"];
        if (rsyslog.containsKey("use_tcp"))
            temp_settings.rsyslog_use_tcp = rsyslog["use_tcp"];
        if (rsyslog.containsKey("min_severity"))
            temp_settings.rsyslog_min_severity = rsyslog["min_severity"];
    }

    temp_settings.imap_enabled = false;

    if (cfg.containsKey("chatgpt")) {
        JsonObject chatgpt = cfg["chatgpt"];

#ifdef ENABLE_CHATGPT
        if (chatgpt.containsKey("enabled")) {
            chatgpt_config.enabled = chatgpt["enabled"];
        }
        if (chatgpt.containsKey("api_key_b64")) {
            String api_key_b64 = chatgpt["api_key_b64"].as<String>();
            strncpy(chatgpt_config.api_key_b64, api_key_b64.c_str(), sizeof(chatgpt_config.api_key_b64) - 1);
        }
#endif // ENABLE_CHATGPT

        String chatgpt_json;
        serializeJsonPretty(chatgpt, chatgpt_json);

        File chatgpt_file = SPIFFS.open("/chatgpt_settings.json", "w");
        if (chatgpt_file) {
            size_t written = chatgpt_file.print(chatgpt_json);
            chatgpt_file.flush();
            chatgpt_file.close();

            if (written == 0) {
                logMessage("RESTORE ERROR: ChatGPT prompts write failed (0 bytes)");
            } else {
#ifdef ENABLE_CHATGPT
                chatgpt_load_config();
#endif // ENABLE_CHATGPT
                logMessage("RESTORE: ChatGPT prompts restored to SPIFFS (" + String(written) + " bytes)");
            }
        } else {
            logMessage("RESTORE ERROR: Failed to save ChatGPT prompts to SPIFFS");
        }
    }

    if (cfg.containsKey("imap")) {
        JsonObject imap = cfg["imap"];

        String imap_json;
        serializeJsonPretty(imap, imap_json);

        File imap_file = SPIFFS.open("/imap_settings.json", "w");
        if (imap_file) {
            size_t written = imap_file.print(imap_json);
            imap_file.flush();
            imap_file.close();

            if (written == 0) {
                logMessage("RESTORE ERROR: IMAP settings write failed (0 bytes)");
            } else {
                logMessage("RESTORE: IMAP settings restored to SPIFFS (" + String(written) + " bytes)");
            }
        } else {
            logMessage("RESTORE ERROR: Failed to save IMAP settings to SPIFFS");
        }
    }

    core_config = temp_core_config;
    settings = temp_settings;
#ifdef ENABLE_GSM
    gsm_config = temp_gsm;
#endif

    return true;
}

// =============================================================================
// FACTORY RESET
// =============================================================================
void perform_factory_reset() {
    FlashGuard fg;
    if (!fg.ok()) return;

    display_turn_on();
    const int centerX = display.getWidth() / 2;
    const int centerY = display.getHeight() / 2;
    const char *message = "FACTORY RESET";

    display.clearBuffer();
    display.setFont(u8g2_font_nokiafc22_tr);
    int width = display.getStrWidth(message);
    display.drawStr(centerX - (width / 2), centerY, message);
    display.sendBuffer();

    esp_task_wdt_deinit();
    logMessage("SYSTEM: Formatting SPIFFS...");
    SPIFFS.format();

    delay(1000);
    ESP.restart();
}
