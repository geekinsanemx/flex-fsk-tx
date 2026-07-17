/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Configuration Header
 *
 * All constants and configuration options centralized here.
 * Version information lives in version.h, not here.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// BOARD SELECTION
// =============================================================================
// Uncomment ONE of the following board defines, or pass it via
// --build-property "build.extra_flags=-DHELTEC_WIFI_LORA32_V2" at compile time.
// Defaults to TTGO_LORA32_V21 if neither is defined.
#if !defined(TTGO_LORA32_V21) && !defined(HELTEC_WIFI_LORA32_V2)
  #define TTGO_LORA32_V21
#endif

// =============================================================================
// COMPILATION FLAGS
// =============================================================================
// The following optional subsystems are DISABLED by default. Enable each via a
// compiler command-line define — scripts/flex-build-upload.sh's --enable-rtc /
// --enable-imap / --enable-chatgpt / --enable-gsm flags set these automatically:
//   -DENABLE_RTC         Enable DS3231 RTC support
//   -DENABLE_IMAP        Enable IMAP email-to-page polling
//   -DENABLE_CHATGPT     Enable scheduled ChatGPT prompts
//   -DENABLE_GSM         Enable GSM/cellular failover support
#define ENABLE_DEBUG              // Uncomment for verbose debug output

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
#define EMR_PATTERN_SIZE 4
static const uint8_t EMR_PATTERN[EMR_PATTERN_SIZE] = {0xA5, 0x5A, 0xA5, 0x5A};

// =============================================================================
// DISPLAY (U8G2)
// =============================================================================
#define OLED_TIMEOUT_MS (5 * 60 * 1000)
#define FONT_BANNER u8g2_font_10x20_tr
#define BANNER_HEIGHT 16
#define BANNER_MARGIN 2
#define FONT_DEFAULT u8g2_font_7x13_tr
#define FONT_BOLD u8g2_font_7x13B_tr
#define FONT_LINE_HEIGHT 14
#define FONT_TAB_START 42

// =============================================================================
// IMAP
// =============================================================================
#define IMAP_BATCH_SIZE 10
#define IMAP_CONTENT_LIMIT 248
#define IMAP_RECONNECT_INTERVAL 30000
#define IMAP_CHECK_INTERVAL 60000
#define IMAP_MAX_ACCOUNTS 5
#define IMAP_MIN_CHECK_INTERVAL 5
#define IMAP_JITTER_OFFSET 60000

// =============================================================================
// CHATGPT
// =============================================================================
#define MAX_CHATGPT_PROMPTS 10
#define CHATGPT_CHECK_INTERVAL 60000UL

// =============================================================================
// GSM
// =============================================================================
#define GSM_AT_READY_TIMEOUT_MS 2000
#define GSM_BOOT_STABILIZE_MS 5000
#define GSM_SIM_READY_RETRIES 20
#define GSM_SIM_POLL_DELAY_MS 100
#define GSM_REG_LOG_INTERVAL_MS 5000

// =============================================================================
// WEB SERVER / WIFI
// =============================================================================
#define WEB_SERVER_PORT 80
#define WIFI_CONNECT_TIMEOUT 30000
#define WIFI_AP_TIMEOUT 300000
#define WIFI_RETRY_ATTEMPTS 3
#define MAX_WIFI_NETWORKS 10

// =============================================================================
// HEARTBEAT / FACTORY RESET
// =============================================================================
#define HEARTBEAT_INTERVAL 60000
#define HEARTBEAT_BLINK_DURATION 100
#define FACTORY_RESET_PIN 0
#define FACTORY_RESET_HOLD_TIME 30000

// =============================================================================
// LED (requires LED_PIN from include/boards/boards.h at point of use)
// =============================================================================
#define LED_OFF()  digitalWrite(LED_PIN, LOW)
#define LED_ON()   digitalWrite(LED_PIN, HIGH)

// =============================================================================
// MISC
// =============================================================================
#define DEFAULT_BANNER "flex-fsk-tx"
#define CONFIG_MAGIC 0xF1E7
#define CONFIG_VERSION 3

// =============================================================================
// LOGGING / SPIFFS STORAGE
// =============================================================================
#define MAX_LOG_FILE_SIZE 65536
#define LOG_TRUNCATE_SIZE 32768
#define LOG_BUFFER_SIZE 2048
#define LOG_FLUSH_INTERVAL_MS 1000
#define LOG_FLUSH_THRESHOLD (LOG_BUFFER_SIZE * 3 / 4)

#define MQTT_CA_CERT_FILE "/mqtt_ca.pem"
#define MQTT_DEVICE_CERT_FILE "/mqtt_cert.pem"
#define MQTT_DEVICE_KEY_FILE "/mqtt_key.pem"

// =============================================================================
// QUEUE
// =============================================================================
#define MAX_QUEUE_SIZE 25

// =============================================================================
// HELPERS
// =============================================================================
#define CHECK_HEAP(size) (ESP.getFreeHeap() > (size + 8192))

#endif // CONFIG_H
