#include "../core/hardware.h"

#include <esp_task_wdt.h>
#include <Preferences.h>

#include "../core/config.h"
#include "../../include/boards/boards.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../protocol/transmission.h"
#include "../services/imap.h"
#include "../core/display.h"
#include "../network/wifi.h"

static bool low_battery_alert_sent = false;
static bool power_disconnect_alert_sent = false;

bool watchdog_task_registered = false;
bool watchdog_initialized = false;
const uint32_t WATCHDOG_TIMEOUT_MS = 120000UL;

bool battery_present = false;

unsigned long last_heartbeat = 0;
bool heartbeat_state = false;
int heartbeat_blink_count = 0;

unsigned long factory_reset_start = 0;
bool factory_reset_pressed = false;

BootFailureTracker boot_tracker = {0, 0};

BootPhase boot_phase = BOOT_INIT;
unsigned long boot_phase_start = 0;

// =============================================================================
// WATCHDOG
// =============================================================================
void setup_watchdog() {
    esp_task_wdt_config_t config = {
        .timeout_ms = WATCHDOG_TIMEOUT_MS,
        .idle_core_mask = 0,
        .trigger_panic = true
    };

    esp_err_t init_result = esp_task_wdt_init(&config);
    if (init_result == ESP_OK) {
        watchdog_initialized = true;
    } else {
        watchdog_initialized = false;
        logMessagef("WATCHDOG: Init failed (err=%d), using default configuration", init_result);
    }

    esp_err_t add_result = esp_task_wdt_add(NULL);
    if (add_result == ESP_OK) {
        logMessagef("WATCHDOG: Loop task registered with %lus timeout",
                    (unsigned long)(WATCHDOG_TIMEOUT_MS / 1000UL));
        watchdog_task_registered = true;
    } else {
        logMessagef("WATCHDOG: Failed to register loop task (err=%d)", add_result);
        watchdog_task_registered = false;
    }
}

void feed_watchdog() {
    if (watchdog_task_registered) {
        esp_task_wdt_reset();
    }
}

void check_heap_health() {
    static unsigned long last_warning = 0;
    static uint32_t last_min_heap = 0;
    static bool first_run = true;

    uint32_t free_heap = ESP.getFreeHeap();
    uint32_t min_heap = ESP.getMinFreeHeap();

    if (first_run) {
        last_min_heap = min_heap;
        first_run = false;
        return;
    }

    bool show_warning = false;
    char warning_msg[128];


    if (free_heap < 30000) {
        snprintf(warning_msg, sizeof(warning_msg), "HEAP CRITICAL: Free: %lu bytes", free_heap);
        show_warning = true;
    }

    else if (free_heap < 50000) {
        snprintf(warning_msg, sizeof(warning_msg), "HEAP WARNING: Free: %lu bytes", free_heap);
        show_warning = true;
    }

    else if (last_min_heap > min_heap && (last_min_heap - min_heap) > 10000) {
        snprintf(warning_msg, sizeof(warning_msg), "HEAP: Min dropped by %lu bytes (now: %lu)", last_min_heap - min_heap, min_heap);
        show_warning = true;
        last_min_heap = min_heap;
    }


    if (show_warning && ((unsigned long)(millis() - last_warning) > 30000)) {
        logMessage(warning_msg);
        last_warning = millis();
    }
}

// =============================================================================
// BATTERY
// =============================================================================
void getBatteryInfo(uint16_t *voltage_mv, int *percentage) {
    int adc_value = analogRead(BATTERY_ADC_PIN);

    float battery_voltage_local = (float)(adc_value) / 4095.0 * 2.0 * 3.3 * 1.1;

    battery_present = (battery_voltage_local > 2.5);

    int battery_percentage_local = 0;
    if (battery_present) {
        if (battery_voltage_local >= 4.15) {
            battery_percentage_local = 100;
        } else {
            float voltage_clamped = constrain(battery_voltage_local, 3.2, 4.15);
            battery_percentage_local = map(voltage_clamped * 100, 320, 415, 0, 100);
            battery_percentage_local = constrain(battery_percentage_local, 0, 100);
        }
    }

    *voltage_mv = (uint16_t)(battery_voltage_local * 1000);
    *percentage = battery_percentage_local;
}

float readBatteryVoltage() {
    int adc_value = analogRead(BATTERY_ADC_PIN);
    return (float)(adc_value) / 4095.0 * 2.0 * 3.3 * 1.1;
}

void check_low_battery_alert(int battery_pct) {
    if (battery_pct <= 10 && !low_battery_alert_sent) {
        String alert_msg = "LOW BATTERY: " + String(battery_pct) + "% remaining";

        if (queue_add_message(
            settings.default_capcode,
            settings.default_frequency,
            settings.default_txpower,
            false,
            alert_msg.c_str()
        )) {
            low_battery_alert_sent = true;
            logMessage("ALERT: Low battery warning queued (" + String(battery_pct) + "%)");
        }
    }
    else if (battery_pct > 15) {
        low_battery_alert_sent = false;
    }
}

void check_power_disconnect_alert(bool power_connected, bool charging_active) {
    static bool was_power_connected = true;
    static uint8_t disconnect_confirm_count = 0;

    if (was_power_connected && !power_connected) {
        disconnect_confirm_count++;
        if (disconnect_confirm_count >= 3 && !power_disconnect_alert_sent) {
            if (queue_add_message(
                settings.default_capcode,
                settings.default_frequency,
                settings.default_txpower,
                false,
                "POWER DISCONNECTED: Battery discharging"
            )) {
                power_disconnect_alert_sent = true;
                logMessage("ALERT: Power disconnect warning queued");
            }
        }
    } else if (power_connected) {
        disconnect_confirm_count = 0;
        power_disconnect_alert_sent = false;
        was_power_connected = true;
    } else if (!was_power_connected) {
        disconnect_confirm_count = 0;
    }
}

// =============================================================================
// VEXT POWER RAIL
// =============================================================================
void VextON(void) {
    if (VEXT_PIN >= 0) {
        pinMode(VEXT_PIN, OUTPUT);
        digitalWrite(VEXT_PIN, LOW);
    }
}

void VextOFF(void) {
    if (VEXT_PIN >= 0) {
        pinMode(VEXT_PIN, OUTPUT);
        digitalWrite(VEXT_PIN, HIGH);
    }
}

// =============================================================================
// RF AMPLIFIER
// =============================================================================
void rfamp_init() {
    int pin = (settings.rf_amplifier_power_pin == 0) ? RFAMP_PWR_PIN : settings.rf_amplifier_power_pin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, settings.rf_amplifier_active_high ? LOW : HIGH);
}

void rfamp_enable() {
    if (!settings.enable_rf_amplifier) return;
    int pin = (settings.rf_amplifier_power_pin == 0) ? RFAMP_PWR_PIN : settings.rf_amplifier_power_pin;
    digitalWrite(pin, settings.rf_amplifier_active_high ? HIGH : LOW);
    delay(settings.rf_amplifier_delay_ms);
}

void rfamp_disable() {
    if (!settings.enable_rf_amplifier) return;
    int pin = (settings.rf_amplifier_power_pin == 0) ? RFAMP_PWR_PIN : settings.rf_amplifier_power_pin;
    digitalWrite(pin, settings.rf_amplifier_active_high ? LOW : HIGH);
}

// =============================================================================
// LED HEARTBEAT
// =============================================================================
void handle_led_heartbeat() {
    unsigned long current_time = millis();
    unsigned long heartbeat_interval = ap_mode_active ? (HEARTBEAT_INTERVAL / 4) : HEARTBEAT_INTERVAL;

    if ((unsigned long)(current_time - last_heartbeat) >= heartbeat_interval) {
        heartbeat_blink_count = 0;
        heartbeat_state = true;
        LED_ON();
        last_heartbeat = current_time;
    }

    if (heartbeat_state) {
        static unsigned long last_blink = 0;

        if (current_time - last_blink >= HEARTBEAT_BLINK_DURATION) {
            if (heartbeat_blink_count < 4) {
                if (heartbeat_blink_count % 2 == 0) {
                    LED_OFF();
                } else {
                    LED_ON();
                }
                heartbeat_blink_count++;
                last_blink = current_time;
            } else {
                LED_OFF();
                heartbeat_state = false;
                heartbeat_blink_count = 0;
            }
        }
    }
}

// =============================================================================
// BOOT FAILURE TRACKING (NVS)
// =============================================================================
void load_boot_tracker() {
    Preferences prefs;
    prefs.begin("boot_tracker", true);
    boot_tracker.consecutive_resets = prefs.getUChar("resets", 0);
    boot_tracker.last_reset_phase = prefs.getUInt("phase", 0);
    prefs.end();
}

void save_boot_tracker() {
    Preferences prefs;
    prefs.begin("boot_tracker", false);
    prefs.putUChar("resets", boot_tracker.consecutive_resets);
    prefs.putUInt("phase", boot_tracker.last_reset_phase);
    prefs.end();
}

void check_boot_failure_history() {
    load_boot_tracker();

    if (boot_tracker.consecutive_resets >= 3 &&
        boot_tracker.last_reset_phase == BOOT_SERVICES_PENDING) {

        logMessage("BOOT: Detected 3 consecutive failures during IMAP init");
        logMessage("BOOT: Auto-disabling IMAP for safe mode");

#ifdef ENABLE_IMAP
        imap_config.enabled = false;
        save_imap_config();
#endif // ENABLE_IMAP

        boot_tracker.consecutive_resets = 0;
        save_boot_tracker();
    } else {
        boot_tracker.last_reset_phase = boot_phase;
        boot_tracker.consecutive_resets++;
        save_boot_tracker();
    }
}

void mark_boot_success() {
    boot_tracker.consecutive_resets = 0;
    save_boot_tracker();
    logMessage("BOOT: Complete - boot failure tracker reset");
}

// =============================================================================
// FACTORY RESET BUTTON
// =============================================================================
void handle_factory_reset() {
    bool button_pressed = (digitalRead(FACTORY_RESET_PIN) == LOW);
    unsigned long current_time = millis();

    if (button_pressed && !factory_reset_pressed) {
        factory_reset_pressed = true;
        factory_reset_start = current_time;
        logMessage("SYSTEM: Factory reset button pressed - hold for 30 seconds");
    } else if (!button_pressed && factory_reset_pressed) {
        factory_reset_pressed = false;
        logMessage("SYSTEM: Factory reset cancelled");
    } else if (button_pressed && factory_reset_pressed) {
        if (current_time - factory_reset_start >= FACTORY_RESET_HOLD_TIME) {
            logMessage("SYSTEM: Factory reset initiated!");

            display.clearBuffer();
            display.setFont(u8g2_font_6x10_tr);
            display.drawStr(10, 20, "FACTORY RESET");
            display.drawStr(10, 35, "Restoring defaults...");
            display.sendBuffer();

            delay(2000);

            perform_factory_reset();
        } else {
            unsigned long remaining = FACTORY_RESET_HOLD_TIME - (current_time - factory_reset_start);
            if ((current_time - factory_reset_start) % 5000 < 100) {
                logMessagef("SYSTEM: Factory reset in: %d seconds", remaining / 1000);
            }
        }
    }
}

// =============================================================================
// PANIC
// =============================================================================
void panic() {
    display_panic();
    Serial.print("ERROR\r\n");
    while (true) {
        delay(100000);
    }
}
