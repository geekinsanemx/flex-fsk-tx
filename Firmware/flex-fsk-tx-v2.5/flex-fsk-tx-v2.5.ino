/*
 * ============================================================================
 * FLEX Paging Message Transmitter - v2.5
 * ============================================================================
 *
 * UART/Serial AT Command Interface with Enhanced Features
 *
 * Features:
 * - AT command protocol for serial communication
 * - FIFO-based efficient transmission
 * - FLEX message encoding on device
 * - Core 0 isolated transmission task
 * - Message queue (10 messages)
 * - EMR (Emergency Message Resynchronization)
 * - Persistent configuration (NVS + SPIFFS)
 * - Persistent logging system
 * - Battery monitoring with alerts
 * - RTC support (DS3231)
 * - RF amplifier control
 * - Factory reset capability
 * - Watchdog timer
 * - Heartbeat LED
 * - OLED display with timeout
 *
 * AT Commands:
 * - AT                         : Basic AT command
 * - AT+FREQ=xxx / AT+FREQ?     : Set/query frequency (400-1000 MHz)
 * - AT+FREQPPM=xxx / AT+FREQPPM? : Set/query frequency correction PPM
 * - AT+POWER=xxx / AT+POWER?   : Set/query power (2-20 dBm)
 * - AT+SEND=xxx                : Send binary data
 * - AT+MSG=capcode             : Send FLEX message (followed by text)
 * - AT+MAILDROP=x / AT+MAILDROP? : Set/query mail drop flag
 * - AT+STATUS?                 : Query device status
 * - AT+DEVICE?                 : Query comprehensive device info
 * - AT+FLEX=<param>,<value> / AT+FLEX? : Configure FLEX defaults
 * - AT+LOGS?N                  : Query last N log lines (default 25)
 * - AT+RMLOG                   : Delete log file
 * - AT+FACTORYRESET            : Factory reset (clear SPIFFS)
 * - AT+ABORT                   : Abort current operation
 * - AT+RESET                   : Reset device
 *
 * Hardware Support:
 * - TTGO LoRa32-OLED (ESP32 + SX1276)
 * - Heltec WiFi LoRa 32 V2 (ESP32 + SX1276)
 *
 * ============================================================================
 */

#include <WiFi.h>  // WiFi stack init only
#include "config.h"
#include "boards/boards.h"
#include "storage.h"
#include "logging.h"
#include "hardware.h"
#include "display.h"
#include "flex_protocol.h"
#include "transmission.h"
#include "at_commands.h"
#include "utils.h"

// =============================================================================
// BATTERY MONITORING STATE
// =============================================================================
static unsigned long last_battery_check = 0;
static bool last_power_connected = true;
static bool last_charging_active = false;
static bool battery_first_check = true;
static int last_percent_bracket = -1;
static bool low_battery_alert_sent = false;
static bool power_disconnect_alert_sent = false;

// =============================================================================
// FACTORY RESET STATE
// =============================================================================
static unsigned long button_press_start = 0;
static bool button_was_pressed = false;

// =============================================================================
// BATTERY ALERT FUNCTIONS
// =============================================================================
void check_low_battery_alert(int battery_pct) {
    if (!settings.enable_low_battery_alert) return;
    if (!battery_present) return;

    if (battery_pct <= BATTERY_LOW_THRESHOLD && !low_battery_alert_sent) {
        String alert_msg = "LOW BATTERY: " + String(battery_pct) + "% remaining";

        if (queue_add_message(settings.default_capcode,
                             settings.default_frequency,
                             settings.default_txpower,
                             false,
                             alert_msg.c_str())) {
            low_battery_alert_sent = true;
            logMessagef("ALERT: Low battery queued (%d%%)", battery_pct);
        }
    } else if (battery_pct > BATTERY_LOW_HYSTERESIS) {
        low_battery_alert_sent = false;
    }
}

void check_power_disconnect_alert(bool power_connected, bool charging_active) {
    if (!settings.enable_power_disconnect_alert) return;
    if (!battery_present) return;
    if (battery_first_check) return;

    static bool was_power_connected = true;
    static uint8_t disconnect_confirm_count = 0;

    if (was_power_connected && !power_connected) {
        disconnect_confirm_count++;
        if (disconnect_confirm_count >= BATTERY_DISCONNECT_CONFIRM_COUNT &&
            !power_disconnect_alert_sent) {
            if (queue_add_message(settings.default_capcode,
                                 settings.default_frequency,
                                 settings.default_txpower,
                                 false,
                                 "POWER DISCONNECTED: Battery discharging")) {
                power_disconnect_alert_sent = true;
                logMessage("ALERT: Power disconnect queued");
            }
        }
    } else if (power_connected) {
        disconnect_confirm_count = 0;
        power_disconnect_alert_sent = false;
    }

    was_power_connected = power_connected;
}

// =============================================================================
// FACTORY RESET HANDLER
// =============================================================================
void handle_factory_reset() {
    if (digitalRead(FACTORY_RESET_PIN) == LOW) {
        if (!button_was_pressed) {
            button_press_start = millis();
            button_was_pressed = true;
        }

        if (millis() - button_press_start >= FACTORY_RESET_HOLD_TIME) {
            perform_factory_reset();
        }
    } else {
        button_was_pressed = false;
    }
}

// =============================================================================
// ARDUINO SETUP
// =============================================================================
void setup() {
    Serial.begin(SERIAL_BAUD);

    // Initialize WiFi stack but do NOT connect
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Initialize SPIFFS first (needed for logging)
    if (!storage_init()) {
        Serial.println("FATAL: SPIFFS init failed");
        while (true) delay(1000);
    }

    // Initialize logging system
    logging_init();
    logMessage("========================================");
    logMessage("FLEX Paging Transmitter " FIRMWARE_VERSION);
    logMessage("========================================");

    // Load configuration
    load_core_config();
    load_runtime_settings();

    // Copy timezone for logging timestamps
    timezone_offset_hours = settings.timezone_offset_hours;

    // Initialize hardware
    display_init();
    battery_init();

#if RTC_ENABLED
    rtc_init();
#endif

    rfamp_init();
    led_heartbeat_init();

    // Initialize radio
    if (!radio_init(settings.default_frequency, settings.default_txpower)) {
        logMessage("FATAL: Radio init failed");
        display_factory_reset();
        while (true) delay(1000);
    }

    current_tx_frequency = settings.default_frequency;
    current_tx_power = settings.default_txpower;

    // Initialize factory reset button
    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);

    // Check boot failure history
    check_boot_failure_history();

    // Initialize FLEX protocol
    queue_init();

    // Initialize AT protocol
    at_init();

    // Initialize transmission task (Core 0)
    transmission_init();

    // Start watchdog
    watchdog_init();

    // Mark boot success after delay
    delay(5000);
    mark_boot_success();

    // Display status
    display_status();
    reset_oled_timeout();

    // Send ready message
    Serial.print("AT READY\r\n");
    Serial.print("FLEX-FSK-TX " FIRMWARE_VERSION "\r\n");
    Serial.flush();

    logMessage("SYSTEM: Boot complete, ready for commands");
}

// =============================================================================
// ARDUINO LOOP (Core 1)
// =============================================================================
void loop() {
    // Feed watchdog
    watchdog_feed();

    // Process AT commands
    at_process_serial();

    // Flush log buffer if needed
    flush_log_buffer_if_due();

    // Update display if requested by Core 0
    display_update_if_needed();

    // Handle OLED timeout
    if (oled_active && (millis() - last_activity_time > OLED_TIMEOUT_MS)) {
        display_turn_off();
    }

    // Handle factory reset button
    handle_factory_reset();

    // Battery monitoring (every 60 seconds)
    if (millis() - last_battery_check > BATTERY_CHECK_INTERVAL) {
        uint16_t battery_voltage_mv;
        int battery_percentage;
        getBatteryInfo(&battery_voltage_mv, &battery_percentage);

        int adc_raw = analogRead(BATTERY_ADC_PIN);
        float battery_voltage_v = battery_voltage_mv / 1000.0;

        bool is_power_connected = last_power_connected;
        if (battery_first_check) {
            is_power_connected = (battery_voltage_v > BATTERY_DISCONNECT_THRESHOLD_HIGH);
        } else if (last_power_connected) {
            if (battery_voltage_v < BATTERY_DISCONNECT_THRESHOLD_LOW) {
                is_power_connected = false;
            }
        } else {
            if (battery_voltage_v > BATTERY_DISCONNECT_THRESHOLD_HIGH) {
                is_power_connected = true;
            }
        }

        bool is_actively_charging = (battery_voltage_v > BATTERY_VOLTAGE_CHARGING);
        int current_percent_bracket = battery_percentage / 10;

        // Log battery status if significant change
        if (battery_first_check ||
            is_power_connected != last_power_connected ||
            is_actively_charging != last_charging_active ||
            current_percent_bracket != last_percent_bracket) {

            logMessagef("BATTERY: V=%dmV (%.2fV), ADC=%d, %%=%d, Power=%s, Charging=%s, Present=%s",
                       battery_voltage_mv, battery_voltage_v, adc_raw, battery_percentage,
                       is_power_connected ? "Connected" : "Battery",
                       is_actively_charging ? "Yes" : "No",
                       battery_present ? "Yes" : "No");

            last_power_connected = is_power_connected;
            last_charging_active = is_actively_charging;
            last_percent_bracket = current_percent_bracket;
            battery_first_check = false;
        }

        // Update display if active
        if (oled_active) {
            display_status();
        }

        // Check alerts
        check_low_battery_alert(battery_percentage);
        check_power_disconnect_alert(is_power_connected, is_actively_charging);

        last_battery_check = millis();
    }

    // Heartbeat LED
    if (!transmission_guard_active()) {
        led_heartbeat_update();
    }

    // Check transmission task health
    static unsigned long last_health_check = 0;
    if (millis() - last_health_check > 30000) {  // Every 30 seconds
        check_transmission_task_health();
        last_health_check = millis();
    }

    delay(1);
}
