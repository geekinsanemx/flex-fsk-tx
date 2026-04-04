/*
 * FLEX Paging Message Transmitter - v2.5
 * Storage Management Module
 *
 * Handles NVS (Preferences) and SPIFFS configuration storage
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <Preferences.h>
#include <SPIFFS.h>

// =============================================================================
// CORE CONFIGURATION (NVS/Preferences)
// Minimal critical settings that survive factory reset
// =============================================================================
struct CoreConfig {
    uint32_t magic;
    uint8_t version;
    float frequency_correction_ppm;
    uint8_t reserved[200];
};

// =============================================================================
// DEVICE SETTINGS (SPIFFS /settings.json)
// Application settings, cleared on factory reset
// =============================================================================
struct DeviceSettings {
    char banner_message[17];
    float timezone_offset_hours;

    bool enable_low_battery_alert;
    bool enable_power_disconnect_alert;

    bool enable_rf_amplifier;
    uint8_t rf_amplifier_power_pin;
    uint16_t rf_amplifier_delay_ms;
    bool rf_amplifier_active_high;

    float default_frequency;
    uint64_t default_capcode;
    float default_txpower;
};

// =============================================================================
// BOOT FAILURE TRACKING (NVS)
// =============================================================================
struct BootTracker {
    uint8_t failure_count;
    unsigned long last_failure_time;
};

// =============================================================================
// GLOBAL INSTANCES
// =============================================================================
extern CoreConfig core_config;
extern DeviceSettings settings;
extern BootTracker boot_tracker;
extern Preferences preferences;

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

// NVS Core Config
bool storage_init();
bool save_core_config();
bool load_core_config();
void load_default_core_config();

// SPIFFS Settings
bool save_runtime_settings();
bool load_runtime_settings();
void load_default_settings();

// Boot Tracking
void load_boot_tracker();
void save_boot_tracker();
void check_boot_failure_history();
void mark_boot_success();

// Factory Reset
void perform_factory_reset();

#endif // STORAGE_H
