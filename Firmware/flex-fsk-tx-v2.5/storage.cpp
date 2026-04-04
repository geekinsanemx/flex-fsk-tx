/*
 * FLEX Paging Message Transmitter - v2.5
 * Storage Management Module Implementation
 */

#include "storage.h"
#include "config.h"
#include "logging.h"
#include <ArduinoJson.h>
#include "esp_task_wdt.h"

CoreConfig core_config;
DeviceSettings settings;
BootTracker boot_tracker;
Preferences preferences;

// =============================================================================
// SPIFFS INITIALIZATION
// =============================================================================
bool storage_init() {
    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR: SPIFFS mount failed");
        return false;
    }

    logMessage("STORAGE: SPIFFS mounted successfully");
    return true;
}

// =============================================================================
// NVS CORE CONFIG (Minimal, survives factory reset)
// =============================================================================
void load_default_core_config() {
    core_config.magic = CONFIG_MAGIC;
    core_config.version = CONFIG_VERSION;
    core_config.frequency_correction_ppm = FREQUENCY_CORRECTION_PPM;
    memset(core_config.reserved, 0, sizeof(core_config.reserved));
}

bool save_core_config() {
    logMessagef("CONFIG: Saving core config - magic=0x%X, version=%d, ppm=%.2f",
                core_config.magic, core_config.version, core_config.frequency_correction_ppm);

    if (!preferences.begin(NVS_NAMESPACE, false)) {
        logMessage("CONFIG: Failed to open NVS for writing");
        return false;
    }

    size_t bytes_written = preferences.putBytes("config", &core_config, sizeof(CoreConfig));
    bool result = (bytes_written == sizeof(CoreConfig));

    preferences.end();

    logMessagef("CONFIG: NVS write %s (%d bytes)", result ? "SUCCESS" : "FAILED", bytes_written);
    return result;
}

bool load_core_config() {
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        logMessage("CONFIG: Failed to open NVS for reading");
        load_default_core_config();
        save_core_config();
        return false;
    }

    size_t bytes_read = preferences.getBytes("config", &core_config, sizeof(CoreConfig));
    preferences.end();

    if (bytes_read != sizeof(CoreConfig)) {
        logMessage("CONFIG: No valid config in NVS, using defaults");
        load_default_core_config();
        save_core_config();
        return false;
    }

    if (core_config.magic != CONFIG_MAGIC) {
        logMessagef("CONFIG: Invalid magic 0x%X, expected 0x%X", core_config.magic, CONFIG_MAGIC);
        load_default_core_config();
        save_core_config();
        return false;
    }

    if (core_config.version != CONFIG_VERSION) {
        logMessagef("CONFIG: Version mismatch %d, expected %d", core_config.version, CONFIG_VERSION);

        float saved_ppm = core_config.frequency_correction_ppm;
        load_default_core_config();
        core_config.frequency_correction_ppm = saved_ppm;
        save_core_config();
        return false;
    }

    logMessage("CONFIG: Core config loaded from NVS");
    return true;
}

// =============================================================================
// SPIFFS RUNTIME SETTINGS
// =============================================================================
void load_default_settings() {
    logMessage("SETTINGS: Loading defaults");

    strlcpy(settings.banner_message, DEFAULT_BANNER, sizeof(settings.banner_message));
    settings.timezone_offset_hours = 0.0;

    settings.enable_low_battery_alert = true;
    settings.enable_power_disconnect_alert = true;

    settings.enable_rf_amplifier = false;
    settings.rf_amplifier_power_pin = RFAMP_PWR_PIN_DEFAULT;
    settings.rf_amplifier_delay_ms = RFAMP_DELAY_DEFAULT;
    settings.rf_amplifier_active_high = true;

    settings.default_frequency = TX_FREQ_DEFAULT;
    settings.default_capcode = 37137;
    settings.default_txpower = TX_POWER_DEFAULT;
}

bool save_runtime_settings() {
    logMessage("SETTINGS: Saving to SPIFFS");

    StaticJsonDocument<1024> doc;

    doc["banner"] = settings.banner_message;
    doc["timezone_offset"] = settings.timezone_offset_hours;

    JsonObject alerts = doc.createNestedObject("alerts");
    alerts["low_battery"] = settings.enable_low_battery_alert;
    alerts["power_disconnect"] = settings.enable_power_disconnect_alert;

    JsonObject rf_amp = doc.createNestedObject("rf_amplifier");
    rf_amp["enabled"] = settings.enable_rf_amplifier;
    rf_amp["power_pin"] = settings.rf_amplifier_power_pin;
    rf_amp["delay_ms"] = settings.rf_amplifier_delay_ms;
    rf_amp["active_high"] = settings.rf_amplifier_active_high;

    JsonObject flex = doc.createNestedObject("flex");
    flex["frequency"] = settings.default_frequency;
    flex["capcode"] = String(settings.default_capcode);
    flex["txpower"] = settings.default_txpower;

    File file = SPIFFS.open(SETTINGS_FILE_PATH, "w");
    if (!file) {
        logMessage("SETTINGS: Failed to open file for writing");
        return false;
    }

    size_t bytes_written = serializeJson(doc, file);
    file.flush();
    file.close();

    if (bytes_written == 0) {
        logMessage("SETTINGS: Failed to write JSON");
        return false;
    }

    logMessagef("SETTINGS: Saved successfully (%d bytes)", bytes_written);

    save_core_config();

    return true;
}

bool load_runtime_settings() {
    logMessage("SETTINGS: Loading from SPIFFS");

    if (!SPIFFS.exists(SETTINGS_FILE_PATH)) {
        logMessage("SETTINGS: File not found, using defaults");
        load_default_settings();
        save_runtime_settings();
        return false;
    }

    File file = SPIFFS.open(SETTINGS_FILE_PATH, "r");
    if (!file) {
        logMessage("SETTINGS: Failed to open file");
        load_default_settings();
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        logMessagef("SETTINGS: JSON parse error: %s", error.c_str());
        load_default_settings();
        save_runtime_settings();
        return false;
    }

    strlcpy(settings.banner_message, doc["banner"] | DEFAULT_BANNER, sizeof(settings.banner_message));
    settings.timezone_offset_hours = doc["timezone_offset"] | 0.0;

    if (doc.containsKey("alerts")) {
        JsonObject alerts = doc["alerts"];
        settings.enable_low_battery_alert = alerts["low_battery"] | true;
        settings.enable_power_disconnect_alert = alerts["power_disconnect"] | true;
    }

    if (doc.containsKey("rf_amplifier")) {
        JsonObject rf_amp = doc["rf_amplifier"];
        settings.enable_rf_amplifier = rf_amp["enabled"] | false;
        settings.rf_amplifier_power_pin = rf_amp["power_pin"] | RFAMP_PWR_PIN_DEFAULT;
        settings.rf_amplifier_delay_ms = rf_amp["delay_ms"] | RFAMP_DELAY_DEFAULT;
        settings.rf_amplifier_active_high = rf_amp["active_high"] | true;
    }

    if (doc.containsKey("flex")) {
        JsonObject flex = doc["flex"];
        settings.default_frequency = flex["frequency"] | TX_FREQ_DEFAULT;
        settings.default_capcode = strtoull(flex["capcode"] | "37137", nullptr, 10);
        settings.default_txpower = flex["txpower"] | TX_POWER_DEFAULT;
    }

    logMessage("SETTINGS: Loaded successfully");
    return true;
}

// =============================================================================
// BOOT FAILURE TRACKING
// =============================================================================
void load_boot_tracker() {
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        boot_tracker.failure_count = 0;
        boot_tracker.last_failure_time = 0;
        return;
    }

    boot_tracker.failure_count = preferences.getUChar("boot_fails", 0);
    boot_tracker.last_failure_time = preferences.getULong("boot_time", 0);
    preferences.end();
}

void save_boot_tracker() {
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return;
    }

    preferences.putUChar("boot_fails", boot_tracker.failure_count);
    preferences.putULong("boot_time", boot_tracker.last_failure_time);
    preferences.end();
}

void check_boot_failure_history() {
    load_boot_tracker();

    unsigned long now = millis();

    if (boot_tracker.failure_count > 0) {
        if ((now - boot_tracker.last_failure_time) < BOOT_FAILURE_WINDOW_MS) {
            boot_tracker.failure_count++;
            logMessagef("BOOT: Failure count: %d", boot_tracker.failure_count);

            if (boot_tracker.failure_count >= MAX_BOOT_FAILURES) {
                logMessage("BOOT: Maximum failures reached - device may be unstable");
            }
        } else {
            logMessage("BOOT: Failure window expired, resetting counter");
            boot_tracker.failure_count = 0;
        }
    }

    boot_tracker.last_failure_time = now;
    save_boot_tracker();
}

void mark_boot_success() {
    boot_tracker.failure_count = 0;
    boot_tracker.last_failure_time = 0;
    save_boot_tracker();
    logMessage("BOOT: Marked successful");
}

// =============================================================================
// FACTORY RESET
// =============================================================================
void perform_factory_reset() {
    logMessage("FACTORY RESET: Starting...");

    esp_task_wdt_delete(NULL);

    extern void display_factory_reset();
    display_factory_reset();

    delay(2000);

    logMessage("FACTORY RESET: Formatting SPIFFS...");
    SPIFFS.format();

    delay(1000);

    logMessage("FACTORY RESET: Rebooting...");
    ESP.restart();
}
