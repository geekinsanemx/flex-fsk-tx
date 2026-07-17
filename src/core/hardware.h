/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Hardware Module - battery, heartbeat LED, watchdog, boot failure tracking, factory reset button
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>

// =============================================================================
// STRUCTS
// =============================================================================
struct BootFailureTracker {
    uint8_t consecutive_resets;
    uint32_t last_reset_phase;
};

enum BootPhase {
    BOOT_INIT,
    BOOT_NETWORK_PENDING,
    BOOT_NETWORK_READY,
    BOOT_NTP_SYNCING,
    BOOT_NTP_FAILED,
    BOOT_WATCHDOG_ACTIVE,
    BOOT_MQTT_PENDING,
    BOOT_MQTT_READY,
    BOOT_SERVICES_PENDING,
    BOOT_COMPLETE,
    BOOT_AP_COMPLETE
};

// =============================================================================
// GLOBALS
// =============================================================================
extern BootPhase boot_phase;
extern unsigned long boot_phase_start;
extern bool watchdog_task_registered;
extern bool watchdog_initialized;
extern const uint32_t WATCHDOG_TIMEOUT_MS;

extern bool battery_present;

extern unsigned long last_heartbeat;
extern bool heartbeat_state;
extern int heartbeat_blink_count;

extern unsigned long factory_reset_start;
extern bool factory_reset_pressed;

extern BootFailureTracker boot_tracker;

// =============================================================================
// WATCHDOG
// =============================================================================
void setup_watchdog();
void feed_watchdog();
void check_heap_health();

// =============================================================================
// BATTERY
// =============================================================================
void getBatteryInfo(uint16_t *voltage_mv, int *percentage);
float readBatteryVoltage();
void check_low_battery_alert(int battery_pct);
void check_power_disconnect_alert(bool power_connected, bool charging_active);

// =============================================================================
// VEXT POWER RAIL
// =============================================================================
void VextON(void);
void VextOFF(void);

// =============================================================================
// RF AMPLIFIER
// =============================================================================
void rfamp_init();
void rfamp_enable();
void rfamp_disable();

// =============================================================================
// LED HEARTBEAT
// =============================================================================
void handle_led_heartbeat();

// =============================================================================
// BOOT FAILURE TRACKING (NVS)
// =============================================================================
void load_boot_tracker();
void save_boot_tracker();
void check_boot_failure_history();
void mark_boot_success();

// =============================================================================
// FACTORY RESET BUTTON
// =============================================================================
void handle_factory_reset();

// =============================================================================
// PANIC
// =============================================================================
void panic();

#endif // HARDWARE_H
