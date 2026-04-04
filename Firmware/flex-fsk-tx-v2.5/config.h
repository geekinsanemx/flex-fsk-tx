/*
 * FLEX Paging Message Transmitter - v2.5
 * Configuration Header
 *
 * All constants and configuration options centralized here
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define FIRMWARE_VERSION "v2.5.0"

// =============================================================================
// BOARD SELECTION
// =============================================================================
#if !defined(TTGO_LORA32_V21) && !defined(HELTEC_WIFI_LORA32_V2)
  #define TTGO_LORA32_V21
#endif

// =============================================================================
// COMPILATION FLAGS
// =============================================================================
#define RTC_ENABLED false       // Enable DS3231 RTC support
// #define ENABLE_DEBUG         // Uncomment for verbose debug output

// =============================================================================
// SERIAL COMMUNICATION
// =============================================================================
#define SERIAL_BAUD 115200

// =============================================================================
// RADIO DEFAULTS (SX1276)
// =============================================================================
#define TX_FREQ_DEFAULT 931.9375
#define TX_BITRATE 1.6
#define TX_DEVIATION 5
#define TX_POWER_DEFAULT 2
#define TX_POWER_MIN 2
#define TX_POWER_MAX 20
#define RX_BANDWIDTH 10.4
#define PREAMBLE_LENGTH 0
#define FREQUENCY_CORRECTION_PPM 0.0

// =============================================================================
// AT PROTOCOL
// =============================================================================
#define AT_BUFFER_SIZE 512
#define AT_CMD_TIMEOUT 5000
#define AT_MAX_RETRIES 3
#define AT_INTER_CMD_DELAY 100

// =============================================================================
// FLEX PROTOCOL
// =============================================================================
#define FLEX_MSG_TIMEOUT 30000
#define MAX_FLEX_MESSAGE_LENGTH 248
#define MESSAGE_QUEUE_SIZE 10

// Valid FLEX capcode ranges
#define FLEX_CAPCODE_MIN_1 1
#define FLEX_CAPCODE_MAX_1 1933312
#define FLEX_CAPCODE_MIN_2 1998849
#define FLEX_CAPCODE_MAX_2 2031614
#define FLEX_CAPCODE_MIN_3 2101249
#define FLEX_CAPCODE_MAX_3 4291000000

// =============================================================================
// EMR (Emergency Message Resynchronization)
// =============================================================================
#define EMR_TIMEOUT_MS 600000  // 10 minutes
#define EMR_PATTERN_SIZE 4
static const uint8_t EMR_PATTERN[EMR_PATTERN_SIZE] = {0xA5, 0x5A, 0xA5, 0x5A};

// =============================================================================
// OLED DISPLAY
// =============================================================================
#define OLED_TIMEOUT_MS (5 * 60 * 1000)  // 5 minutes
#define FONT_BANNER u8g2_font_10x20_tr
#define BANNER_HEIGHT 16
#define BANNER_MARGIN 2
#define FONT_DEFAULT u8g2_font_7x13_tr
#define FONT_BOLD u8g2_font_7x13B_tr
#define FONT_LINE_HEIGHT 14
#define FONT_TAB_START 42
#define DEFAULT_BANNER "flex-fsk-tx"

// =============================================================================
// BATTERY MONITORING
// =============================================================================
#define BATTERY_CHECK_INTERVAL 60000      // 60 seconds
#define BATTERY_LOW_THRESHOLD 10          // 10%
#define BATTERY_LOW_HYSTERESIS 15         // 15%
#define BATTERY_VOLTAGE_MIN 3.2           // 3.2V = 0%
#define BATTERY_VOLTAGE_MAX 4.15          // 4.15V = 100%
#define BATTERY_VOLTAGE_CONNECTED 4.17    // >4.17V = power connected
#define BATTERY_VOLTAGE_CHARGING 4.20     // >4.20V = actively charging
#define BATTERY_DISCONNECT_THRESHOLD_LOW 4.08
#define BATTERY_DISCONNECT_THRESHOLD_HIGH 4.12
#define BATTERY_DISCONNECT_CONFIRM_COUNT 3

// =============================================================================
// HEARTBEAT LED
// =============================================================================
#define HEARTBEAT_INTERVAL 60000          // 60 seconds
#define HEARTBEAT_BLINK_DURATION 100      // 100ms
#define HEARTBEAT_BLINK_COUNT 4           // 4 blinks

// =============================================================================
// FACTORY RESET
// =============================================================================
#define FACTORY_RESET_PIN 0               // GPIO 0 button
#define FACTORY_RESET_HOLD_TIME 30000     // 30 seconds

// =============================================================================
// WATCHDOG TIMER
// =============================================================================
#define WATCHDOG_TIMEOUT_S 60
#define BOOT_GRACE_PERIOD_S 120

// =============================================================================
// LOGGING SYSTEM
// =============================================================================
#define MAX_LOG_FILE_SIZE 32768           // 32KB max file
#define LOG_TRUNCATE_SIZE 8192            // Keep 8KB on rotate
#define LOG_BUFFER_SIZE 2048              // 2KB RAM buffer
#define LOG_FLUSH_INTERVAL_MS 1000        // Flush every 1s
#define LOG_FLUSH_THRESHOLD (LOG_BUFFER_SIZE * 3 / 4)
#define LOG_FILE_PATH "/serial.log"

// =============================================================================
// STORAGE (NVS + SPIFFS)
// =============================================================================
#define CONFIG_MAGIC 0xF257
#define CONFIG_VERSION 1
#define NVS_NAMESPACE "flex-fsk"
#define SETTINGS_FILE_PATH "/settings.json"

// =============================================================================
// BOOT FAILURE TRACKING
// =============================================================================
#define MAX_BOOT_FAILURES 3
#define BOOT_FAILURE_WINDOW_MS 300000     // 5 minutes

// =============================================================================
// RF AMPLIFIER DEFAULTS (Board-specific)
// =============================================================================
#ifdef TTGO_LORA32_V21
  #define RFAMP_PWR_PIN_DEFAULT 32
#else
  #define RFAMP_PWR_PIN_DEFAULT 22
#endif

#define RFAMP_DELAY_MIN 20
#define RFAMP_DELAY_MAX 5000
#define RFAMP_DELAY_DEFAULT 200

#endif // CONFIG_H
