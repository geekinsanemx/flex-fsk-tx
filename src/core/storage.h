/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Storage Module - NVS core config, SPIFFS settings, certificates, backup/restore
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <Preferences.h>

// =============================================================================
// STRUCTS
// =============================================================================
struct CoreConfig {
    uint32_t magic;
    uint8_t version;
    float frequency_correction_ppm;
    uint8_t reserved[200];
};

struct DeviceSettings {
    uint8_t theme;
    char banner_message[17];
    float timezone_offset_hours;
    char ntp_server[64];
    bool enable_low_battery_alert;
    bool enable_power_disconnect_alert;
    float default_frequency;
    uint64_t default_capcode;
    float default_txpower;
    float frequency_correction_ppm;
    bool api_enabled;
    uint16_t http_port;
    char api_username[33];
    char api_password[65];
    bool mqtt_enabled;
    uint32_t mqtt_boot_delay_ms;
    bool mqtt_notify_failures;
    uint32_t mqtt_retry_interval_mins;
    bool imap_enabled;
    bool grafana_enabled;
    char mqtt_server[128];
    uint16_t mqtt_port;
    char mqtt_thing_name[32];
    char mqtt_subscribe_topic[64];
    char mqtt_publish_topic[64];
    bool rsyslog_enabled;
    char rsyslog_server[51];
    uint16_t rsyslog_port;
    bool rsyslog_use_tcp;
    uint8_t rsyslog_min_severity;
    bool enable_rf_amplifier;
    uint8_t rf_amplifier_power_pin;
    uint16_t rf_amplifier_delay_ms;
    bool rf_amplifier_active_high;
};

// =============================================================================
// GLOBALS
// =============================================================================
extern CoreConfig core_config;
extern DeviceSettings settings;
extern Preferences preferences;

// =============================================================================
// CERTIFICATE STORAGE (SPIFFS)
// =============================================================================
String loadCertificateFromSPIFFS(const char* filename);
bool saveCertificateToSPIFFS(const char* filename, const String& cert);
bool certificateExistsInSPIFFS(const char* filename);
String getCertificateStatusFromSPIFFS(const char* filename);
void deleteAllCertificatesFromSPIFFS();
String getCertificateFilename(const String& certType);

// =============================================================================
// NVS CORE CONFIG
// =============================================================================
void load_default_core_config();
bool save_core_config();
bool load_core_config();

// =============================================================================
// SPIFFS RUNTIME SETTINGS
// =============================================================================
bool save_runtime_settings();
bool load_runtime_settings();
void load_default_settings();

// =============================================================================
// BACKUP / RESTORE
// =============================================================================
String export_user_backup();
bool import_user_backup(const String& json_string, String& error_msg);

// =============================================================================
// FACTORY RESET
// =============================================================================
void perform_factory_reset();

#endif // STORAGE_H
