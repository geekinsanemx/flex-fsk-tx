/*
 * FLEX Paging Message Transmitter - v2.5
 * Hardware Abstraction Implementation
 */

#include "config.h"
#include "hardware.h"
#include "storage.h"
#include "logging.h"
#include "boards/boards.h"
#include "esp_task_wdt.h"
#include <Wire.h>
#include <SPI.h>
#include <time.h>

#if RTC_ENABLED
#include <RTClib.h>
#endif

// =============================================================================
// GLOBAL HARDWARE INSTANCES
// =============================================================================
SX1276 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, LORA_RST_PIN, LORA_GPIO_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

#if RTC_ENABLED
  RTC_DS3231 rtc;
  bool rtc_available = false;
#endif

// =============================================================================
// GLOBAL STATE VARIABLES
// =============================================================================
bool oled_active = true;
unsigned long last_activity_time = 0;
bool battery_present = false;
bool system_time_initialized = false;
bool system_time_from_rtc = false;
unsigned long last_heartbeat = 0;
bool heartbeat_state = false;
int heartbeat_blink_count = 0;
float timezone_offset_hours = 0.0;

// =============================================================================
// VEXT POWER CONTROL (Heltec)
// =============================================================================
void VextON() {
    if (VEXT_PIN >= 0) {
        pinMode(VEXT_PIN, OUTPUT);
        digitalWrite(VEXT_PIN, LOW);
    }
}

void VextOFF() {
    if (VEXT_PIN >= 0) {
        pinMode(VEXT_PIN, OUTPUT);
        digitalWrite(VEXT_PIN, HIGH);
    }
}

// =============================================================================
// RADIO MANAGEMENT (SX1276)
// =============================================================================
bool radio_init(float frequency, float power) {
    logMessage("RADIO: Initializing SX1276");

    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_CS_PIN);

    float corrected_freq = frequency * (1.0 + core_config.frequency_correction_ppm / 1000000.0);

    int state = radio.beginFSK(corrected_freq,
                               TX_BITRATE,
                               TX_DEVIATION,
                               RX_BANDWIDTH,
                               power,
                               PREAMBLE_LENGTH,
                               false);

    if (state != RADIOLIB_ERR_NONE) {
        logMessagef("RADIO: Init failed (error=%d), trying fallback power", state);

        state = radio.beginFSK(corrected_freq,
                               TX_BITRATE,
                               TX_DEVIATION,
                               RX_BANDWIDTH,
                               TX_POWER_DEFAULT,
                               PREAMBLE_LENGTH,
                               false);

        if (state != RADIOLIB_ERR_NONE) {
            logMessagef("RADIO: Init failed even with fallback (error=%d)", state);
            return false;
        }

        settings.default_txpower = TX_POWER_DEFAULT;
    }

    int packet_mode_state = radio.fixedPacketLengthMode(0);
    if (packet_mode_state != RADIOLIB_ERR_NONE) {
        logMessage("RADIO: Failed to set variable packet mode");
        return false;
    }

    logMessagef("RADIO: Initialized at %.4f MHz (%.2f ppm), power %.1f dBm",
                frequency, core_config.frequency_correction_ppm, power);

    return true;
}

void radio_standby() {
    radio.standby();
}

int radio_set_frequency(float freq) {
    float corrected = freq * (1.0 + core_config.frequency_correction_ppm / 1000000.0);
    return radio.setFrequency(corrected);
}

int radio_set_power(float power) {
    return radio.setOutputPower(power);
}

void radio_set_fifo_callback(void (*callback)(void)) {
    radio.setFifoEmptyAction(callback);
}

// =============================================================================
// OLED DISPLAY
// =============================================================================
void display_init() {
    logMessage("DISPLAY: Initializing OLED");

    VextON();
    delay(10);

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    if (OLED_RST_PIN >= 0) {
        pinMode(OLED_RST_PIN, OUTPUT);
        digitalWrite(OLED_RST_PIN, LOW);
        delay(50);
        digitalWrite(OLED_RST_PIN, HIGH);
        delay(50);
    }

    display.begin();
    display.clearBuffer();

    logMessage("DISPLAY: OLED initialized");
}

void display_turn_off() {
    if (oled_active) {
        display.setPowerSave(1);
        VextOFF();
        oled_active = false;
    }
}

void display_turn_on() {
    if (!oled_active) {
        VextON();
        delay(10);
        display.setPowerSave(0);
        oled_active = true;
    }
}

void reset_oled_timeout() {
    last_activity_time = millis();
    display_turn_on();
}

// =============================================================================
// BATTERY MONITORING
// =============================================================================
void battery_init() {
    pinMode(BATTERY_ADC_PIN, INPUT);
    logMessage("BATTERY: Monitoring initialized");
}

void getBatteryInfo(uint16_t *voltage_mv, int *percentage) {
    int adc_value = analogRead(BATTERY_ADC_PIN);
    float battery_voltage = (float)(adc_value) / 4095.0 * 2.0 * 3.3 * 1.1;

    battery_present = (battery_voltage > 2.5);

    int battery_percentage = 0;
    if (battery_present) {
        if (battery_voltage >= BATTERY_VOLTAGE_MAX) {
            battery_percentage = 100;
        } else {
            float voltage_clamped = constrain(battery_voltage, BATTERY_VOLTAGE_MIN, BATTERY_VOLTAGE_MAX);
            battery_percentage = map((int)(voltage_clamped * 100),
                                   (int)(BATTERY_VOLTAGE_MIN * 100),
                                   (int)(BATTERY_VOLTAGE_MAX * 100),
                                   0, 100);
            battery_percentage = constrain(battery_percentage, 0, 100);
        }
    }

    *voltage_mv = (uint16_t)(battery_voltage * 1000);
    *percentage = battery_percentage;
}

float readBatteryVoltage() {
    int adc_value = analogRead(BATTERY_ADC_PIN);
    return (float)(adc_value) / 4095.0 * 2.0 * 3.3 * 1.1;
}

// =============================================================================
// RTC (DS3231)
// =============================================================================
#if RTC_ENABLED
static void rtc_sync_system_time() {
    if (!rtc_available) return;

    DateTime now = rtc.now();
    struct timeval tv;
    tv.tv_sec = now.unixtime();
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);

    system_time_initialized = true;
    system_time_from_rtc = true;

    logMessagef("RTC: System time set: %04d-%02d-%02d %02d:%02d:%02d",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
}

bool rtc_init() {
    logMessage("RTC: Initializing DS3231");

    if (rtc.begin()) {
        rtc_available = true;
        logMessage("RTC: DS3231 detected");

        if (!rtc.lostPower()) {
            rtc_sync_system_time();
            return true;
        } else {
            logMessage("RTC: Clock lost power, time invalid");
            return false;
        }
    } else {
        logMessage("RTC: DS3231 not detected");
        return false;
    }
}
#endif

// =============================================================================
// RF AMPLIFIER
// =============================================================================
void rfamp_init() {
    uint8_t pin = (settings.rf_amplifier_power_pin == 0)
                  ? RFAMP_PWR_PIN_DEFAULT
                  : settings.rf_amplifier_power_pin;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, settings.rf_amplifier_active_high ? LOW : HIGH);

    if (settings.enable_rf_amplifier) {
        logMessagef("RFAMP: Initialized on GPIO%d (%s, %dms delay)",
                    pin,
                    settings.rf_amplifier_active_high ? "active-high" : "active-low",
                    settings.rf_amplifier_delay_ms);
    } else {
        logMessage("RFAMP: Disabled");
    }
}

void rfamp_enable() {
    if (!settings.enable_rf_amplifier) return;

    uint8_t pin = (settings.rf_amplifier_power_pin == 0)
                  ? RFAMP_PWR_PIN_DEFAULT
                  : settings.rf_amplifier_power_pin;

    digitalWrite(pin, settings.rf_amplifier_active_high ? HIGH : LOW);
    delay(settings.rf_amplifier_delay_ms);
}

void rfamp_disable() {
    if (!settings.enable_rf_amplifier) return;

    uint8_t pin = (settings.rf_amplifier_power_pin == 0)
                  ? RFAMP_PWR_PIN_DEFAULT
                  : settings.rf_amplifier_power_pin;

    digitalWrite(pin, settings.rf_amplifier_active_high ? LOW : HIGH);
}

// =============================================================================
// HEARTBEAT LED
// =============================================================================
void led_heartbeat_init() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    last_heartbeat = millis();
    heartbeat_state = false;
    heartbeat_blink_count = 0;

    logMessage("LED: Heartbeat initialized");
}

void led_heartbeat_update() {
    unsigned long current_time = millis();

    if ((current_time - last_heartbeat) >= HEARTBEAT_INTERVAL) {
        heartbeat_blink_count = 0;
        heartbeat_state = true;
        digitalWrite(LED_PIN, HIGH);
        last_heartbeat = current_time;
    }

    if (heartbeat_state) {
        static unsigned long last_blink = 0;

        if (current_time - last_blink >= HEARTBEAT_BLINK_DURATION) {
            if (heartbeat_blink_count < HEARTBEAT_BLINK_COUNT) {
                if (heartbeat_blink_count % 2 == 0) {
                    digitalWrite(LED_PIN, HIGH);
                } else {
                    digitalWrite(LED_PIN, LOW);
                }
                heartbeat_blink_count++;
                last_blink = current_time;
            } else {
                digitalWrite(LED_PIN, LOW);
                heartbeat_state = false;
            }
        }
    }
}

// =============================================================================
// WATCHDOG TIMER
// =============================================================================
void watchdog_init() {
    logMessage("WATCHDOG: Initializing");

    esp_task_wdt_deinit();

    esp_task_wdt_config_t config = {
        .timeout_ms = WATCHDOG_TIMEOUT_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };

    esp_task_wdt_init(&config);
    esp_task_wdt_add(NULL);

    logMessagef("WATCHDOG: Initialized (%ds timeout)", WATCHDOG_TIMEOUT_S);
}

void watchdog_feed() {
    esp_task_wdt_reset();
}
