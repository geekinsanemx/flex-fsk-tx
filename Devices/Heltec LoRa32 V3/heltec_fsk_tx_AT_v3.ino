/*
 * FLEX Message Transmitter v3.0 - Heltec WiFi LoRa 32 V3
 * Enhanced FSK transmitter with WiFi, Web Interface and REST API
 * 
 * Features:
 * - FLEX protocol message transmission 
 * - AT command interface over serial (115200 baud)
 * - WiFi connectivity with EEPROM configuration storage
 * - Web interface for sending FLEX messages at <ip>/
 * - Configuration page at <ip>/configuration 
 * - REST API at <ip>:16180 with HTTP Basic Auth
 * - Theme support: 10 themes (5 light, 5 dark) with visual indicators
 * - Character counter and validation
 * - Immediate error detection and detailed error messages
 * - Non-blocking transmission with immediate responses
 * 
 * Hardware-specific features:
 * - SX1262 radio chip support
 * - TX power configuration (-9 to +22 dBm)
 * - Heltec power management (VextON/OFF)
 * - 128x64 OLED display with 5-minute timeout
 * 
 * This code is released into the public domain.
 */

#include <RadioLib.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <HardwareSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// Include tinyflex header (embedded)
#include "tinyflex.h"

// =============================================================================
// CONSTANTS AND DEFAULTS
// =============================================================================

#define HELTEC_SERIAL_BAUD 115200

// Heltec WiFi LoRa 32 V3 pin configuration (SX1262)
#define LORA_NSS    8
#define LORA_NRESET 12
#define LORA_BUSY   13
#define LORA_DIO1   14
#define LORA_SCK    9
#define LORA_MISO   11
#define LORA_MOSI   10

// Battery monitoring for Heltec WiFi LoRa 32 V3
#define VBAT_PIN    1    // ADC pin for battery voltage reading
#define ADC_CTRL    37   // ADC control pin (for V3.1 and earlier: LOW to enable, V3.2+: HIGH to enable)

// Heltec WiFi LoRa 32 V3 specific pin definitions
#define SDA_OLED    17   // I2C SDA for OLED
#define SCL_OLED    18   // I2C SCL for OLED  
#define RST_OLED    21   // OLED Reset pin
#define Vext        36   // External power control pin

// Radio defaults
#define TX_FREQ_DEFAULT 931.9375
#define TX_BITRATE 1.6
#define TX_DEVIATION 5
#define TX_POWER_DEFAULT 2
#define RX_BANDWIDTH 11.7
#define PREAMBLE_LENGTH 0

// AT Protocol constants
#define AT_BUFFER_SIZE 512
#define AT_CMD_TIMEOUT 5000
#define AT_MAX_RETRIES 3
#define AT_INTER_CMD_DELAY 100

// Display constants (Heltec specific)
#define OLED_TIMEOUT_MS (5 * 60 * 1000) // 5 minutes in milliseconds
#define FONT_DEFAULT ArialMT_Plain_10
#define FONT_BOLD ArialMT_Plain_10
#define FONT_LINE_HEIGHT 12
#define FONT_TAB_START 50
#define FONT_BANNER ArialMT_Plain_16  // Larger font for banner
#define BANNER_HEIGHT 18             // Height for banner area

// FLEX Message constants
#define FLEX_MSG_TIMEOUT 30000
#define MAX_FLEX_MESSAGE_LENGTH 240

// WiFi and Web Server constants
#define WEB_SERVER_PORT 80
#define REST_API_PORT 16180
#define WIFI_CONNECT_TIMEOUT 30000
#define WIFI_AP_TIMEOUT 300000  // 5 minutes in AP mode
#define WIFI_RETRY_ATTEMPTS 3   // Retry attempts before AP mode

// LED heartbeat constants
#define HEARTBEAT_INTERVAL 60000  // 1 minute in milliseconds
#define HEARTBEAT_BLINK_DURATION 100  // 100ms blink duration

// Factory reset constants
#define FACTORY_RESET_PIN 0      // Boot button on Heltec (GPIO 0)
#define FACTORY_RESET_HOLD_TIME 30000  // 30 seconds
#define BUTTON_DEBOUNCE_DELAY 50 // 50ms debounce delay

// Default banner message
#define DEFAULT_BANNER "flex-fsk-tx"

// EEPROM Configuration constants
#define EEPROM_SIZE 512
#define EEPROM_MAGIC 0xF1E7  // Magic number to validate EEPROM data
#define CONFIG_VERSION 1

// =============================================================================
// EEPROM CONFIGURATION STRUCTURE
// =============================================================================

struct DeviceConfig {
    uint32_t magic;           // Magic number for validation
    uint8_t version;          // Config version
    
    // WiFi Configuration
    char wifi_ssid[33];       // WiFi SSID (32 chars + null)
    char wifi_password[65];   // WiFi password (64 chars + null)
    bool use_dhcp;            // True for DHCP, false for static IP
    
    // Static IP Configuration
    uint8_t static_ip[4];     // Static IP address
    uint8_t netmask[4];       // Network mask
    uint8_t gateway[4];       // Gateway address
    uint8_t dns[4];           // DNS server
    
    // FLEX Default Configuration
    float default_frequency;  // Default transmission frequency
    uint64_t default_capcode; // Default capcode
    
    // REST API Settings
    char api_username[33];    // API username (32 chars + null)
    char api_password[65];    // API password (64 chars + null)
    uint16_t api_port;        // API listening port
    
    // Device Settings
    float tx_power;           // Transmission power
    bool enable_wifi;         // Enable WiFi functionality
    uint8_t theme;            // UI theme: 0-4=light themes, 5-9=dark themes
    char banner_message[17];  // Custom banner message (16 chars + null)
    
    uint8_t reserved[44];     // Reserved for future use
};

// =============================================================================
// BUILT-IN LED CONTROL
// =============================================================================

#define LED_PIN 35
#define LED_OFF()  digitalWrite(LED_PIN, LOW)
#define LED_ON()   digitalWrite(LED_PIN, HIGH)

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_NRESET, LORA_BUSY);

#ifdef WIRELESS_STICK_V3
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED);
#else
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
#endif

// Web servers
WebServer webServer(WEB_SERVER_PORT);
WebServer* apiServer = nullptr;  // Will be initialized with configured port

// Device configuration
DeviceConfig config;

// Device state management
typedef enum {
    STATE_IDLE,
    STATE_WAITING_FOR_DATA,
    STATE_WAITING_FOR_MSG,
    STATE_TRANSMITTING,
    STATE_ERROR,
    STATE_WIFI_CONNECTING,
    STATE_WIFI_AP_MODE
} device_state_t;

device_state_t device_state = STATE_IDLE;
unsigned long state_timeout = 0;

// AT Protocol variables
char at_buffer[AT_BUFFER_SIZE];
int at_buffer_pos = 0;
bool at_command_ready = false;

// Display timeout control
unsigned long last_activity_time = 0;
bool oled_active = true;

// Global variables for transmission state (from working v2)
volatile bool console_loop_enable = true;
volatile bool fifo_empty = false;
volatile bool transmission_complete = false;
volatile bool transmission_in_progress = false;
volatile bool transmission_processing_complete = false;

// Note: Web responses are sent immediately when transmission starts
// Only immediate error responses if radio.startTransmit() fails

// Transmission data buffer and state variables
uint8_t tx_data_buffer[2048] = {0};
int current_tx_total_length = 0;
int current_tx_remaining_length = 0;
int16_t radio_start_transmit_status = RADIOLIB_ERR_NONE;
int expected_data_length = 0;
unsigned long data_receive_timeout = 0;

// FLEX message variables
uint64_t flex_capcode = 0;
char flex_message_buffer[MAX_FLEX_MESSAGE_LENGTH + 1] = {0};
int flex_message_pos = 0;
unsigned long flex_message_timeout = 0;
bool flex_mail_drop = false;  // Per-message mail drop flag for AT commands

// Radio operation parameters
float current_tx_frequency = TX_FREQ_DEFAULT;
float current_tx_power = TX_POWER_DEFAULT;

// WiFi and network state
bool wifi_connected = false;
bool ap_mode_active = false;
String ap_ssid = "";
IPAddress device_ip;
unsigned long wifi_connect_start = 0;
int wifi_retry_count = 0;

// Heartbeat variables (double blink every minute)
unsigned long last_heartbeat = 0;
bool heartbeat_led_state = false;
int heartbeat_blink_count = 0;
unsigned long heartbeat_blink_start = 0;

// Factory reset variables
unsigned long factory_reset_start = 0;
bool factory_reset_active = false;
unsigned long last_button_debounce_time = 0;
int last_button_state = HIGH;
int button_state = HIGH;

// Serial message logging system
#define SERIAL_LOG_SIZE 50
struct SerialLogEntry {
    unsigned long timestamp;
    char message[100];
};
SerialLogEntry serial_log[SERIAL_LOG_SIZE];
int serial_log_index = 0;
int serial_log_count = 0;

// =============================================================================
// SERIAL MESSAGE LOGGING FUNCTIONS
// =============================================================================

void log_serial_message(const char* message) {
    serial_log[serial_log_index].timestamp = millis();
    strncpy(serial_log[serial_log_index].message, message, sizeof(serial_log[serial_log_index].message) - 1);
    serial_log[serial_log_index].message[sizeof(serial_log[serial_log_index].message) - 1] = '\0';
    
    serial_log_index = (serial_log_index + 1) % SERIAL_LOG_SIZE;
    if (serial_log_count < SERIAL_LOG_SIZE) {
        serial_log_count++;
    }
}

// =============================================================================
// EEPROM CONFIGURATION FUNCTIONS
// =============================================================================

void load_default_config() {
    config.magic = EEPROM_MAGIC;
    config.version = CONFIG_VERSION;
    
    // Default WiFi settings
    strcpy(config.wifi_ssid, "");
    strcpy(config.wifi_password, "");
    config.use_dhcp = true;
    
    // Default static IP settings (192.168.1.100)
    config.static_ip[0] = 192; config.static_ip[1] = 168; config.static_ip[2] = 1; config.static_ip[3] = 100;
    config.netmask[0] = 255; config.netmask[1] = 255; config.netmask[2] = 255; config.netmask[3] = 0;
    config.gateway[0] = 192; config.gateway[1] = 168; config.gateway[2] = 1; config.gateway[3] = 1;
    config.dns[0] = 8; config.dns[1] = 8; config.dns[2] = 8; config.dns[3] = 8;
    
    // Default FLEX settings
    config.default_frequency = TX_FREQ_DEFAULT;
    config.default_capcode = 1234567;
    
    // Default API settings
    strcpy(config.api_username, "admin");
    strcpy(config.api_password, "password");
    config.api_port = REST_API_PORT;
    
    // Default device settings
    config.tx_power = TX_POWER_DEFAULT;
    config.enable_wifi = true;
    config.theme = 0;  // Default theme
    strcpy(config.banner_message, DEFAULT_BANNER);
    
    // Clear reserved space
    memset(config.reserved, 0, sizeof(config.reserved));
}

bool save_config() {
    EEPROM.put(0, config);
    return EEPROM.commit();
}

bool load_config() {
    DeviceConfig temp_config;
    EEPROM.get(0, temp_config);
    
    
    if (temp_config.magic != EEPROM_MAGIC || temp_config.version != CONFIG_VERSION) {
        load_default_config();
        save_config();
        return false;
    }
    
    config = temp_config;
    return true;
}

// =============================================================================
// VEXT CONTROL (Power management for Heltec boards)
// =============================================================================

void VextON(void) {
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
}

void VextOFF(void) {
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH);
}

// Battery voltage reading for Heltec WiFi LoRa 32 V3
float readBatteryVoltage() {
    // Based on debug results: V3.2+ board needs ADC_CTRL HIGH with 0.0025 factor
    pinMode(ADC_CTRL, OUTPUT);
    digitalWrite(ADC_CTRL, HIGH);  // Enable battery reading for V3.2+
    delay(10); // Give it a moment to settle
    
    int raw_value = analogRead(VBAT_PIN);
    
    // Use Method A (0.0025 factor) then apply voltage divider correction
    // Measured 2412mV when battery is fully charged (should be ~4200mV)
    // Scaling factor: 4200/2412 = 1.74
    float voltage = raw_value * 0.0025 * 1.74;
    
    return voltage;
}

// Get battery info with voltage and percentage
void getBatteryInfo(uint16_t *voltage_mv, int *percentage) {
    float voltage_v = readBatteryVoltage();
    *voltage_mv = (uint16_t)(voltage_v * 1000); // Convert to millivolts
    
    // Calculate percentage based on typical LiPo voltage range (3.2V-4.2V)
    *percentage = map(constrain(*voltage_mv, 3200, 4200), 3200, 4200, 0, 100);
}


// =============================================================================
// SPI TIMING CONFIGURATION (ESP32 CORE 3.3.0 FIX) - from working v2
// =============================================================================

void configure_spi_timing() {
    // Use very conservative SPI settings for maximum compatibility
    // Let RadioLib manage the SPI settings, just ensure conservative defaults
    
    
    // Add a small delay to ensure SPI bus is stable
    delay(10);
}

// =============================================================================
// DISPLAY CONTROL
// =============================================================================

void display_turn_off() {
    display.displayOff();
    oled_active = false;
    VextOFF(); // Also turn off VEXT to save power
}

void display_turn_on() {
    VextON(); // Power on VEXT first
    delay(50); // Short delay for power stabilization
    display.displayOn();
    display_current(); // Refresh display
    oled_active = true;
}

void reset_oled_timeout() {
    last_activity_time = millis();
    if (!oled_active) {
        display_turn_on();
    }
}

String generate_ap_ssid() {
    // Initialize WiFi to ensure MAC is available
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Get MAC address for unique identifier
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    // Use last 3 bytes of MAC to create more unique identifier
    String ssid = "HELTEC_FLEX_";
    ssid += String(mac[3], HEX);
    ssid += String(mac[4], HEX);
    ssid += String(mac[5], HEX);
    ssid.toUpperCase();
    
    return ssid;
}

void display_ap_info() {
    if (!oled_active) return;
    
    display.clear();
    
    // No banner in AP mode to save space (matching TTGO v3 behavior)
    // Start from top of screen
    display.setFont(FONT_DEFAULT);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    
    int info_start_y = 12;
    
    display.drawString(0, info_start_y, "AP Mode Active");
    
    info_start_y += 12;
    String ap_str = "SSID: " + ap_ssid;
    display.drawString(0, info_start_y, ap_str);
    
    info_start_y += 12;
    display.drawString(0, info_start_y, "Pass: 12345678");
    
    info_start_y += 12;
    String ip_str = "IP: " + WiFi.softAPIP().toString();
    display.drawString(0, info_start_y, ip_str);
    
    info_start_y += 12;
    // Add battery info in AP mode too
    uint16_t battery_voltage;
    int battery_percentage;
    getBatteryInfo(&battery_voltage, &battery_percentage);
    String battery_str = "Bat: " + String(battery_voltage) + "mV (" + String(battery_percentage) + "%)";
    display.drawString(0, info_start_y, battery_str);

    display.display();
}

void display_panic() {
    const int centerX = display.getWidth() / 2;
    const int centerY = display.getHeight() / 2;
    const char *message = "System halted";

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(FONT_DEFAULT);
    display.drawString(centerX, centerY, message);
    display.display();
}

void display_status() {
    if (!oled_active) return;
    
    display.clear();
    
    // Draw banner at top
    display.setFont(FONT_BANNER);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 0, config.banner_message);
    
    // Set normal font for status info
    display.setFont(FONT_DEFAULT);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    
    String freq_str = "Freq: " + String(current_tx_frequency, 4) + " MHz";
    
    // Get battery info
    uint16_t battery_voltage;
    int battery_percentage;
    getBatteryInfo(&battery_voltage, &battery_percentage);
    
    String power_str = "Pwr: " + String(current_tx_power, 1) + " dBm // " + String(battery_percentage) + "%";
    String status_str = "";
    String wifi_str = "";
    
    // Device state
    switch (device_state) {
        case STATE_IDLE:
            status_str = "State: Ready";
            break;
        case STATE_WAITING_FOR_DATA:
            status_str = "State: Waiting for data...";
            break;
        case STATE_WAITING_FOR_MSG:
            status_str = "State: Waiting for message...";
            break;
        case STATE_TRANSMITTING:
            status_str = "State: Transmitting...";
            break;
        case STATE_ERROR:
            status_str = "State: Error";
            break;
        case STATE_WIFI_CONNECTING:
            status_str = "State: WiFi Connecting...";
            break;
        case STATE_WIFI_AP_MODE:
            status_str = "State: WiFi AP Mode";
            break;
        default:
            status_str = "State: Unknown";
            break;
    }

    // WiFi status
    if (wifi_connected) {
        wifi_str = "IP: " + WiFi.localIP().toString();
    } else if (ap_mode_active) {
        wifi_str = "AP: " + WiFi.softAPIP().toString();
    } else if (!config.enable_wifi) {
        wifi_str = "WiFi: disabled";
    } else {
        wifi_str = "WiFi: Connecting...";
    }
    
    // Truncate long strings to fit display
    if (wifi_str.length() > 21) {
        wifi_str = wifi_str.substring(0, 21);
    }

    int status_start_y = BANNER_HEIGHT + 6; // Increased spacing below banner

    // Draw status info below banner (smaller font to fit WiFi info)
    display.setFont(FONT_DEFAULT);
    
    display.drawString(0, status_start_y, status_str);
    display.drawString(0, status_start_y + 10, freq_str);
    display.drawString(0, status_start_y + 20, power_str);
    display.drawString(0, status_start_y + 30, wifi_str);

    display.display();
}

void display_current() {
    // Smart display function that calls appropriate display based on current mode
    if (ap_mode_active && device_state == STATE_WIFI_AP_MODE) {
        display_ap_info();
    } else {
        display_status();
    }
}

void display_setup() {
    VextON();
    delay(100);
    display.init();
    display.setFont(FONT_DEFAULT);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.clear();
    display.display();
    reset_oled_timeout();
}

// =============================================================================
// FLEX ENCODING FUNCTIONS
// =============================================================================

/**
 * @brief Safe string-to-uint64_t routine for Arduino.
 */
static int str2uint64(uint64_t *out, const char *s) {
    if (!s || s[0] == '\0') return -1;

    uint64_t result = 0;
    const char *p = s;

    while (*p) {
        if (*p < '0' || *p > '9') return -1;

        // Check for overflow
        if (result > (UINT64_MAX - (*p - '0')) / 10) return -1;

        result = result * 10 + (*p - '0');
        p++;
    }

    *out = result;
    return 0;
}

// =============================================================================
// RADIO INITIALIZATION AND CONFIGURATION
// =============================================================================


/**
 * @brief Encode FLEX message and store in tx_data_buffer
 */
bool flex_encode_and_store(uint64_t capcode, const char *message, bool mail_drop) {
    int error = 0;
    
    size_t result = tf_encode_flex_message(message, capcode, tx_data_buffer, sizeof(tx_data_buffer), &error);
    if (result > 0 && error == 0) {
        current_tx_total_length = result;
        current_tx_remaining_length = result;
        return true;
    }
    return false;
}

// =============================================================================
// WiFi AND NETWORK FUNCTIONS
// =============================================================================

void wifi_connect() {
    if (strlen(config.wifi_ssid) == 0) {
        start_ap_mode();
        return;
    }
    
    device_state = STATE_WIFI_CONNECTING;
    display_status();
    
    WiFi.mode(WIFI_STA);
    
    if (!config.use_dhcp) {
        IPAddress ip(config.static_ip[0], config.static_ip[1], config.static_ip[2], config.static_ip[3]);
        IPAddress subnet(config.netmask[0], config.netmask[1], config.netmask[2], config.netmask[3]);
        IPAddress gateway(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]);
        IPAddress dns(config.dns[0], config.dns[1], config.dns[2], config.dns[3]);
        
        WiFi.config(ip, gateway, subnet, dns);
    }
    
    WiFi.begin(config.wifi_ssid, config.wifi_password);
    wifi_connect_start = millis();
    wifi_retry_count = 0;
}

void start_ap_mode() {
    if (ap_ssid.length() == 0) {
        ap_ssid = generate_ap_ssid();
    }
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid.c_str(), "12345678");
    
    device_ip = WiFi.softAPIP();
    display_ap_info();  // Display AP info on OLED
    
    device_state = STATE_WIFI_AP_MODE;
    ap_mode_active = true;
    wifi_connected = false;
}

void check_wifi_connection() {
    if (device_state == STATE_WIFI_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            wifi_connected = true;
            device_ip = WiFi.localIP();
            device_state = STATE_IDLE;
            display_status();
            
            Serial.println("WiFi connected");
            Serial.print("IP: ");
            Serial.println(device_ip);
        } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
            // Authentication failure or SSID not found
            wifi_retry_count++;
            if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
                Serial.println("WiFi authentication failed, starting AP mode");
                start_ap_mode();
            } else {
                Serial.print("WiFi retry attempt: ");
                Serial.println(wifi_retry_count);
                WiFi.begin(config.wifi_ssid, config.wifi_password);
                wifi_connect_start = millis();
            }
        } else if (millis() - wifi_connect_start > WIFI_CONNECT_TIMEOUT) {
            wifi_retry_count++;
            if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
                Serial.println("WiFi connection timeout, starting AP mode");
                start_ap_mode();
            } else {
                Serial.print("WiFi timeout retry attempt: ");
                Serial.println(wifi_retry_count);
                WiFi.begin(config.wifi_ssid, config.wifi_password);
                wifi_connect_start = millis();
            }
        }
    }
}

// =============================================================================
// WEB SERVER FUNCTIONS
// =============================================================================

String get_html_header(String title) {
    String theme_bg = "#FFFFFF";
    String theme_card = "#F2F2F2";
    String theme_text = "#222222";
    String theme_accent = "#222222";
    String theme_button = "#222222";
    String theme_button_hover = "#000000";
    String theme_input = "#FFFFFF";
    String theme_border = "#E0E0E0";
    String theme_nav_active = "#222222";
    String theme_nav_inactive = "#999999";
    
    // Light Themes (0-4)
    if (config.theme == 0) { // Minimal White
        theme_bg = "#FFFFFF";
        theme_card = "#F8F9FA";
        theme_text = "#222222";
        theme_accent = "#333333";
        theme_button = "#333333";
        theme_button_hover = "#000000";
        theme_input = "#FFFFFF";
        theme_border = "#E0E0E0";
        theme_nav_active = "#333333";
        theme_nav_inactive = "#999999";
    } else if (config.theme == 1) { // Aqua Breeze
        theme_bg = "#E6F7F7";
        theme_card = "#D0F0F0";
        theme_text = "#004D4D";
        theme_accent = "#006666";
        theme_button = "#00A3A3";
        theme_button_hover = "#008A8A";
        theme_input = "#FFFFFF";
        theme_border = "#80D4D4";
        theme_nav_active = "#006666";
        theme_nav_inactive = "#66B3B3";
    } else if (config.theme == 2) { // Soft Sand
        theme_bg = "#FAF3E0";
        theme_card = "#F0E6CC";
        theme_text = "#5C3D2E";
        theme_accent = "#8B5A2B";
        theme_button = "#B8860B";
        theme_button_hover = "#A0751A";
        theme_input = "#FFFFFF";
        theme_border = "#D2B48C";
        theme_nav_active = "#8B5A2B";
        theme_nav_inactive = "#CD853F";
    } else if (config.theme == 3) { // Sky Blue
        theme_bg = "#F5FBFF";
        theme_card = "#E8F4FF";
        theme_text = "#00264D";
        theme_accent = "#003D73";
        theme_button = "#0066CC";
        theme_button_hover = "#0052A3";
        theme_input = "#FFFFFF";
        theme_border = "#99C7FF";
        theme_nav_active = "#003D73";
        theme_nav_inactive = "#6699FF";
    } else if (config.theme == 4) { // Minty Fresh
        theme_bg = "#F0FFF4";
        theme_card = "#E6FFE6";
        theme_text = "#0A2F1F";
        theme_accent = "#1A5F3F";
        theme_button = "#22AA55";
        theme_button_hover = "#1E9A4A";
        theme_input = "#FFFFFF";
        theme_border = "#99E6B3";
        theme_nav_active = "#1A5F3F";
        theme_nav_inactive = "#66CC99";
    }
    // Dark Themes (5-9)
    else if (config.theme == 5) { // Carbon Black
        theme_bg = "#121212";
        theme_card = "#1E1E1E";
        theme_text = "#E0E0E0";
        theme_accent = "#FFFFFF";
        theme_button = "#404040";
        theme_button_hover = "#555555";
        theme_input = "#2A2A2A";
        theme_border = "#404040";
        theme_nav_active = "#404040";
        theme_nav_inactive = "#888888";
    } else if (config.theme == 6) { // Neon Purple
        theme_bg = "#1A001A";
        theme_card = "#2D0066";
        theme_text = "#F2E6FF";
        theme_accent = "#CC66FF";
        theme_button = "#7700FF";
        theme_button_hover = "#6600CC";
        theme_input = "#330066";
        theme_border = "#9933FF";
        theme_nav_active = "#CC66FF";
        theme_nav_inactive = "#8855CC";
    } else if (config.theme == 7) { // Cyber Green
        theme_bg = "#0A0F0A";
        theme_card = "#001A00";
        theme_text = "#E6FFE6";
        theme_accent = "#00FF66";
        theme_button = "#00CC44";
        theme_button_hover = "#00AA33";
        theme_input = "#003300";
        theme_border = "#00AA44";
        theme_nav_active = "#00FF66";
        theme_nav_inactive = "#00AA44";
    } else if (config.theme == 8) { // Deep Ocean
        theme_bg = "#001F33";
        theme_card = "#003355";
        theme_text = "#E0F7FA";
        theme_accent = "#66D9EF";
        theme_button = "#0088BB";
        theme_button_hover = "#006699";
        theme_input = "#004466";
        theme_border = "#0099CC";
        theme_nav_active = "#66D9EF";
        theme_nav_inactive = "#4499BB";
    } else if (config.theme == 9) { // Retro Amber
        theme_bg = "#1C1C1C";
        theme_card = "#2A1A00";
        theme_text = "#FFF3E0";
        theme_accent = "#FFB366";
        theme_button = "#CC7700";
        theme_button_hover = "#B36600";
        theme_input = "#333300";
        theme_border = "#FF9933";
        theme_nav_active = "#FFB366";
        theme_nav_inactive = "#CC8844";
    }

    return "<!DOCTYPE html><html><head><title>" + title + "</title>"
           "<meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width, initial-scale=1'>"
           "<style>"
           ":root {"
           "  --theme-bg: " + theme_bg + ";"
           "  --theme-card: " + theme_card + ";"
           "  --theme-text: " + theme_text + ";"
           "  --theme-accent: " + theme_accent + ";"
           "  --theme-button: " + theme_button + ";"
           "  --theme-button-hover: " + theme_button_hover + ";"
           "  --theme-input: " + theme_input + ";"
           "  --theme-border: " + theme_border + ";"
           "  --theme-nav-active: " + theme_nav_active + ";"
           "  --theme-nav-inactive: " + theme_nav_inactive + ";"
           "}"
           "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 20px; background-color: var(--theme-bg); color: var(--theme-text); line-height: 1.6; transition: all 0.3s ease; }"
           ".container { max-width: 800px; margin: 0 auto; background-color: var(--theme-card); padding: 30px; border-radius: 16px; box-shadow: 0 8px 32px rgba(0,0,0,0.1); border: 1px solid var(--theme-border); transition: all 0.3s ease; }"
           ".header { text-align: center; margin-bottom: 30px; }"
           ".header h1 { color: var(--theme-accent); margin: 0; font-size: 2.2em; font-weight: 300; letter-spacing: -0.5px; transition: color 0.3s ease; }"
           ".header p { color: var(--theme-text); margin: 10px 0 0 0; opacity: 0.8; transition: color 0.3s ease; }"
           ".form-group { margin-bottom: 24px; }"
           ".form-group label { display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text); font-size: 14px; transition: color 0.3s ease; }"
           ".form-group input, .form-group select, .form-group textarea { width: 100%; padding: 14px 16px; border: 2px solid var(--theme-border); border-radius: 12px; font-size: 16px; box-sizing: border-box; background-color: var(--theme-input); color: var(--theme-text); transition: all 0.3s ease; }"
           ".form-group input:focus, .form-group select:focus, .form-group textarea:focus { outline: none; border-color: var(--theme-accent); box-shadow: 0 0 0 3px var(--theme-accent)20; transform: translateY(-1px); }"
           ".button { background-color: var(--theme-button); color: white; padding: 14px 28px; border: none; border-radius: 12px; cursor: pointer; font-size: 16px; font-weight: 500; text-decoration: none; display: inline-block; margin: 8px 6px; transition: all 0.3s ease; }"
           ".button:hover { background-color: var(--theme-button-hover); transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.15); }"
           ".button.secondary { background-color: var(--theme-nav-inactive); color: var(--theme-text); }"
           ".button.secondary:hover { background-color: var(--theme-nav-active); color: white; }"
           ".nav { text-align: center; margin-bottom: 30px; padding: 16px; background-color: var(--theme-input); border-radius: 12px; border: 1px solid var(--theme-border); transition: all 0.3s ease; }"
           ".nav a { margin: 0 8px; padding: 12px 20px; border-radius: 8px; transition: all 0.3s ease; text-decoration: none; font-weight: 500; }"
           ".nav a.button { background-color: var(--theme-nav-active); color: white; }"
           ".nav a.button.secondary { background-color: transparent; color: var(--theme-nav-inactive); border: 1px solid var(--theme-border); }"
           ".nav a.button.secondary:hover { background-color: var(--theme-nav-inactive); color: white; border-color: var(--theme-nav-inactive); }"
           ".status { padding: 16px 20px; border-radius: 12px; margin: 24px 0; font-weight: 500; border: none; }"
           ".status.success { background-color: #10B981; color: white; box-shadow: 0 4px 12px rgba(16, 185, 129, 0.3); }"
           ".status.error { background-color: #EF4444; color: white; box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3); }"
           ".char-counter { font-size: 13px; color: var(--theme-nav-inactive); margin-top: 8px; font-weight: 500; transition: color 0.3s ease; }"
           ".progress-bar { width: 100%; height: 8px; background-color: var(--theme-border); border-radius: 6px; margin-top: 8px; overflow: hidden; transition: background-color 0.3s ease; }"
           ".progress-fill { height: 100%; background: linear-gradient(90deg, var(--theme-accent), var(--theme-button)); border-radius: 6px; transition: all 0.4s ease; }"
           "h3 { color: var(--theme-accent); font-weight: 500; font-size: 1.3em; margin-bottom: 16px; transition: color 0.3s ease; }"
           "p { margin-bottom: 12px; color: var(--theme-text); transition: color 0.3s ease; }"
           ".serial-log { background-color: var(--theme-input); border: 2px solid var(--theme-border); border-radius: 12px; padding: 20px; max-height: 300px; overflow-y: auto; font-family: 'Courier New', monospace; font-size: 13px; line-height: 1.4; transition: all 0.3s ease; }"
           ".serial-log div { margin-bottom: 6px; }"
           ".serial-log .timestamp { color: var(--theme-nav-inactive); font-weight: bold; transition: color 0.3s ease; }"
           "#temp-message { position: fixed; top: 20px; right: 20px; z-index: 1000; max-width: 400px; padding: 16px 20px; border-radius: 12px; font-weight: 500; box-shadow: 0 8px 32px rgba(0,0,0,0.2); transform: translateX(450px); transition: transform 0.4s ease; }"
           "#temp-message.show { transform: translateX(0); }"
           "#temp-message.success { background-color: #10B981; color: white; }"
           "#temp-message.info { background-color: #3B82F6; color: white; }"
           "input:-webkit-autofill, input:-webkit-autofill:hover, input:-webkit-autofill:focus, input:-webkit-autofill:active { -webkit-box-shadow: 0 0 0 1000px var(--theme-input) inset !important; -webkit-text-fill-color: var(--theme-text) !important; background-color: var(--theme-input) !important; }"
           "input:-moz-autofill { background-color: var(--theme-input) !important; color: var(--theme-text) !important; }"
           "input[type='text'] { background-color: var(--theme-input) !important; }"
           "select { background-color: var(--theme-input) !important; }"
           "</style>"
           "<script>"
           "function showTempMessage(message, type, duration) {"
           "  var msgDiv = document.getElementById('temp-message');"
           "  if (!msgDiv) {"
           "    msgDiv = document.createElement('div');"
           "    msgDiv.id = 'temp-message';"
           "    document.body.appendChild(msgDiv);"
           "  }"
           "  msgDiv.textContent = message;"
           "  msgDiv.className = type + ' show';"
           "  setTimeout(function() {"
           "    msgDiv.classList.remove('show');"
           "  }, duration || 5000);"
           "}"
           "var themes = ["
           "  {bg:'#FFFFFF',card:'#F8F9FA',text:'#222222',accent:'#333333',button:'#333333',buttonHover:'#000000',input:'#FFFFFF',border:'#E0E0E0',navActive:'#333333',navInactive:'#999999'},"
           "  {bg:'#E6F7F7',card:'#D0F0F0',text:'#004D4D',accent:'#006666',button:'#00A3A3',buttonHover:'#008A8A',input:'#FFFFFF',border:'#80D4D4',navActive:'#006666',navInactive:'#66B3B3'},"
           "  {bg:'#FAF3E0',card:'#F0E6CC',text:'#5C3D2E',accent:'#8B5A2B',button:'#B8860B',buttonHover:'#A0751A',input:'#FFFFFF',border:'#D2B48C',navActive:'#8B5A2B',navInactive:'#CD853F'},"
           "  {bg:'#F5FBFF',card:'#E8F4FF',text:'#00264D',accent:'#003D73',button:'#0066CC',buttonHover:'#0052A3',input:'#FFFFFF',border:'#99C7FF',navActive:'#003D73',navInactive:'#6699FF'},"
           "  {bg:'#F0FFF4',card:'#E6FFE6',text:'#0A2F1F',accent:'#1A5F3F',button:'#22AA55',buttonHover:'#1E9A4A',input:'#FFFFFF',border:'#99E6B3',navActive:'#1A5F3F',navInactive:'#66CC99'},"
           "  {bg:'#121212',card:'#1E1E1E',text:'#E0E0E0',accent:'#FFFFFF',button:'#404040',buttonHover:'#555555',input:'#2A2A2A',border:'#404040',navActive:'#404040',navInactive:'#888888'},"
           "  {bg:'#1A001A',card:'#2D0066',text:'#F2E6FF',accent:'#CC66FF',button:'#7700FF',buttonHover:'#6600CC',input:'#330066',border:'#9933FF',navActive:'#CC66FF',navInactive:'#8855CC'},"
           "  {bg:'#0A0F0A',card:'#001A00',text:'#E6FFE6',accent:'#00FF66',button:'#00CC44',buttonHover:'#00AA33',input:'#003300',border:'#00AA44',navActive:'#00FF66',navInactive:'#00AA44'},"
           "  {bg:'#001F33',card:'#003355',text:'#E0F7FA',accent:'#66D9EF',button:'#0088BB',buttonHover:'#006699',input:'#004466',border:'#0099CC',navActive:'#66D9EF',navInactive:'#4499BB'},"
           "  {bg:'#1C1C1C',card:'#2A1A00',text:'#FFF3E0',accent:'#FFB366',button:'#CC7700',buttonHover:'#B36600',input:'#333300',border:'#FF9933',navActive:'#FFB366',navInactive:'#CC8844'}"
           "];"
           "function applyTheme(themeIndex) {"
           "  var theme = themes[themeIndex];"
           "  var root = document.documentElement;"
           "  root.style.setProperty('--theme-bg', theme.bg);"
           "  root.style.setProperty('--theme-card', theme.card);"
           "  root.style.setProperty('--theme-text', theme.text);"
           "  root.style.setProperty('--theme-accent', theme.accent);"
           "  root.style.setProperty('--theme-button', theme.button);"
           "  root.style.setProperty('--theme-button-hover', theme.buttonHover);"
           "  root.style.setProperty('--theme-input', theme.input);"
           "  root.style.setProperty('--theme-border', theme.border);"
           "  root.style.setProperty('--theme-nav-active', theme.navActive);"
           "  root.style.setProperty('--theme-nav-inactive', theme.navInactive);"
           "}"
           "function onThemeChange() {"
           "  var themeSelect = document.getElementById('theme');"
           "  if (themeSelect) {"
           "    applyTheme(parseInt(themeSelect.value));"
           "  }"
           "}"
           "function submitFormAjax(form, successMsg, restartMsg) {"
           "  var formData = new FormData(form);"
           "  fetch(form.action, { method: 'POST', body: formData })"
           "  .then(response => response.json())"
           "  .then(data => {"
           "    if (data.success) {"
           "      if (data.restart) {"
           "        showTempMessage(restartMsg || 'Settings saved, restarting in 5 seconds...', 'info', 5000);"
           "        setTimeout(() => location.reload(), 5000);"
           "      } else {"
           "        showTempMessage(successMsg || 'Settings saved successfully!', 'success', 5000);"
           "      }"
           "    } else {"
           "      showTempMessage('Error: ' + (data.error || 'Failed to save settings'), 'error', 5000);"
           "    }"
           "  })"
           "  .catch(() => showTempMessage('Network error occurred', 'error', 5000));"
           "  return false;"
           "}"
           "</script>"
           "</head><body><div class='container'>";
}

String get_html_footer() {
    return "</div></body></html>";
}

void handle_root() {
    reset_oled_timeout();
    
    String html = get_html_header("Heltec FLEX Transmitter");
    
    html += "<div class='header'>"
            "<h1>FLEX Message Transmitter</h1>"
            ""
            "</div>";
    
    html += "<div class='nav'>"
            "<a href='/' class='button'>📡 Message</a>"
            "<a href='/configuration' class='button secondary'>⚙️ Configuration</a>"
            "<a href='/status' class='button secondary'>📊 Status</a>"
            "<a href='https://github.com/geekinsanemx/flex-fsk-tx' target='_blank' class='button secondary'>🐙 GitHub</a>"
            "</div>";
    
    html += "<div id='temp-message'></div>"
            "<form id='send-form' action='/send' method='post' onsubmit='return submitSendForm(event)'>"
            "<div style='display:flex;gap:15px;margin-bottom:20px;'>"
            "<div class='form-group' style='flex:1;margin-bottom:0;'>"
            "<label for='capcode'>🎯 Capcode:</label>"
            "<input type='number' id='capcode' name='capcode' value='" + String(config.default_capcode) + "' min='1' max='4294967295' required>"
            "</div>"
            "<div class='form-group' style='flex:1;margin-bottom:0;'>"
            "<label for='frequency'>📻 Frequency (MHz):</label>"
            "<input type='number' id='frequency' name='frequency' value='" + String(config.default_frequency, 4) + "' min='400' max='1000' step='0.0001' required>"
            "</div>"
            "<div class='form-group' style='flex:1;margin-bottom:0;'>"
            "<label for='power'>⚡ TX Power (dBm):</label>"
            "<input type='number' id='power' name='power' value='" + String(config.tx_power) + "' min='0' max='20' required>"
            "</div>"
            "</div>"
            
            "<div class='form-group'>"
            "<label for='message'>💬 Message:</label>"
            "<textarea id='message' name='message' rows='4' maxlength='240' placeholder='Enter your FLEX message here...' required oninput='updateCharCounter()'></textarea>"
            "<div class='char-counter'>Characters: <span id='char-count'>0</span>/240</div>"
            "<div class='progress-bar'><div class='progress-fill' id='char-progress'></div></div>"
            "</div>"
            
            "<div style='display:flex;align-items:center;gap:12px;'>"
            "<button type='submit' class='button'>📤 Send Message</button>"
            "<label style='display:flex;align-items:center;gap:4px;font-size:0.9em;cursor:pointer;'>"
            "<input type='checkbox' id='mail_drop' name='mail_drop' style='transform:scale(1.1);'>"
            "📧 Mail Drop"
            "</label>"
            "</div>"
            "</form>"
            
            "<script>"
            "function updateCharCounter() {"
            "    const message = document.getElementById('message');"
            "    const charCount = document.getElementById('char-count');"
            "    const charProgress = document.getElementById('char-progress');"
            "    const currentLength = message.value.length;"
            "    charCount.textContent = currentLength;"
            "    const percentage = (currentLength / 240) * 100;"
            "    charProgress.style.width = percentage + '%';"
            "    if (currentLength > 200) {"
            "        charProgress.style.backgroundColor = '#ff6b6b';"
            "    } else if (currentLength > 150) {"
            "        charProgress.style.backgroundColor = '#ffa726';"
            "    } else {"
            "        charProgress.style.backgroundColor = '#667eea';"
            "    }"
            "}"
            "updateCharCounter();"
            ""
            "function submitSendForm(event) {"
            "  event.preventDefault();"
            "  var form = document.getElementById('send-form');"
            "  var formData = new FormData(form);"
            "  "
            "  fetch('/send', {"
            "    method: 'POST',"
            "    body: formData"
            "  })"
            "  .then(response => response.json())"
            "  .then(data => {"
            "    if (data.success) {"
            "      showTempMessage('Message sent successfully!', 'success', 5000);"
            "      document.getElementById('message').value = '';"  // Clear message
            "      updateCharCounter();"
            "    } else {"
            "      showTempMessage(data.message, 'error', 5000);"
            "    }"
            "  })"
            "  .catch(error => {"
            "    showTempMessage('Error sending message', 'error', 5000);"
            "  });"
            "  "
            "  return false;"
            "}"
            "</script>";
    
    html += get_html_footer();
    webServer.send(200, "text/html; charset=utf-8", html);
}

void handle_send_message() {
    reset_oled_timeout();
    
    if (!webServer.hasArg("frequency") || !webServer.hasArg("power") || 
        !webServer.hasArg("capcode") || !webServer.hasArg("message")) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Missing required parameters\"}");
        return;
    }
    
    float frequency = webServer.arg("frequency").toFloat();
    int power = webServer.arg("power").toInt();
    uint64_t capcode = strtoull(webServer.arg("capcode").c_str(), NULL, 10);
    String message = webServer.arg("message");
    bool mail_drop = webServer.hasArg("mail_drop");  // Checkbox sends value only if checked
    
    // Validate parameters
    if (frequency < 400.0 || frequency > 1000.0) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Frequency must be between 400.0 and 1000.0 MHz\"}");
        return;
    }
    
    if (power < 0 || power > 20) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"TX Power must be between 0 and 20 dBm\"}");
        return;
    }
    
    if (capcode < 1 || capcode > 4294967295ULL) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Capcode must be between 1 and 4294967295\"}");
        return;
    }
    
    if (message.length() > 240) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Message too long (max 240 characters)\"}");
        return;
    }
    
    // Configure radio
    current_tx_frequency = frequency;
    current_tx_power = power;
    radio.setFrequency(frequency);
    radio.setOutputPower(power);
    
    // Try to encode and transmit
    if (flex_encode_and_store(capcode, message.c_str(), mail_drop)) {
        // Start transmission using working v2 method
        device_state = STATE_TRANSMITTING;
        transmission_in_progress = true;
        LED_ON();
        display_status();
        
        // Log transmission start
        char tx_log[100];
        snprintf(tx_log, sizeof(tx_log), "TX: Capcode %llu, %.1fMHz, %ddBm", capcode, frequency, power);
        log_serial_message(tx_log);

        bool tx_success = at_transmit_data();

        if (tx_success) {
            webServer.send(200, "application/json", "{\"success\":true,\"message\":\"Message sent successfully!\"}");
            
            device_state = STATE_IDLE;
            transmission_in_progress = false;
            LED_OFF();
            display_status();
        } else {
            device_state = STATE_IDLE;
            transmission_in_progress = false;
            LED_OFF();
            display_status();
            
            webServer.send(500, "application/json", "{\"success\":false,\"message\":\"Transmission failed - see serial output for details\"}");
        }
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to encode FLEX message\"}");     
    }
}

void handle_configuration() {
    reset_oled_timeout();
    
    String html = get_html_header("Configuration");
    
    html += "<div class='header'>"
            "<h1>⚙️ Device Configuration</h1>"
            "</div>";
    
    html += "<div class='nav'>"
            "<a href='/' class='button secondary'>📡 Message</a>"
            "<a href='/configuration' class='button'>⚙️ Configuration</a>"
            "<a href='/status' class='button secondary'>📊 Status</a>"
            "<a href='https://github.com/geekinsanemx/flex-fsk-tx' target='_blank' class='button secondary'>🐙 GitHub</a>"
            "</div>";
    
    html += "<form action='/save_config' method='post' onsubmit='return submitFormAjax(this, \"Settings saved successfully!\", \"Settings saved, restarting in 5 seconds...\")'>"
            
            "<h3 >🎨 Interface Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='banner_message'>Device Banner (16 chars max):</label>"
            "<input type='text' id='banner_message' name='banner_message' value='" + String(config.banner_message) + "' maxlength='16' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='theme'>UI Theme:</label>"
            "<select id='theme' name='theme' onchange='onThemeChange()' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<option value='0'" + (config.theme == 0 ? " selected" : "") + ">🌞 Minimal White</option>"
            "<option value='1'" + (config.theme == 1 ? " selected" : "") + ">🌞 Aqua Breeze</option>"
            "<option value='2'" + (config.theme == 2 ? " selected" : "") + ">🌞 Soft Sand</option>"
            "<option value='3'" + (config.theme == 3 ? " selected" : "") + ">🌞 Sky Blue</option>"
            "<option value='4'" + (config.theme == 4 ? " selected" : "") + ">🌞 Minty Fresh</option>"
            "<option value='5'" + (config.theme == 5 ? " selected" : "") + ">🌙 Carbon Black</option>"
            "<option value='6'" + (config.theme == 6 ? " selected" : "") + ">🌙 Neon Purple</option>"
            "<option value='7'" + (config.theme == 7 ? " selected" : "") + ">🌙 Cyber Green</option>"
            "<option value='8'" + (config.theme == 8 ? " selected" : "") + ">🌙 Deep Ocean</option>"
            "<option value='9'" + (config.theme == 9 ? " selected" : "") + ">🌙 Retro Amber</option>"
            "</select>"
            "</div>"
            "</div>"
            
            "<h3 >📶 WiFi Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='enable_wifi'>WiFi Enable:</label>"
            "<select id='enable_wifi' name='enable_wifi' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<option value='1'" + (config.enable_wifi ? " selected" : "") + ">Enabled</option>"
            "<option value='0'" + (!config.enable_wifi ? " selected" : "") + ">Disabled</option>"
            "</select>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='use_dhcp'>Use DHCP:</label>"
            "<select id='use_dhcp' name='use_dhcp' onchange='toggleStaticIP()' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<option value='1'" + (config.use_dhcp ? " selected" : "") + ">Yes (Automatic IP)</option>"
            "<option value='0'" + (!config.use_dhcp ? " selected" : "") + ">No (Static IP)</option>"
            "</select>"
            "</div>"
            "</div>"
            "<div style='display:flex;gap:15px;margin-top:15px;'>"
            "<div style='flex:1;'>"
            "<label for='wifi_ssid'>SSID:</label>"
            "<input type='text' id='wifi_ssid' name='wifi_ssid' value='" + String(config.wifi_ssid) + "' maxlength='32' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='wifi_password'>Password:</label>"
            "<input type='password' id='wifi_password' name='wifi_password' value='" + String(config.wifi_password) + "' maxlength='64' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            
            "<h3>🌐 IP Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='static_ip'>IP Address:</label>"
            "<input type='text' id='static_ip' name='static_ip' value='" + 
            (wifi_connected && config.use_dhcp ? WiFi.localIP().toString() : 
             String(config.static_ip[0]) + "." + String(config.static_ip[1]) + "." + 
             String(config.static_ip[2]) + "." + String(config.static_ip[3])) + 
            "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='192.168.1.100' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='netmask'>Netmask:</label>"
            "<input type='text' id='netmask' name='netmask' value='" + 
            (wifi_connected && config.use_dhcp ? WiFi.subnetMask().toString() : 
             String(config.netmask[0]) + "." + String(config.netmask[1]) + "." + 
             String(config.netmask[2]) + "." + String(config.netmask[3])) + 
            "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='255.255.255.0' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            "<div style='display:flex;gap:15px;margin-top:15px;'>"
            "<div style='flex:1;'>"
            "<label for='gateway'>Gateway:</label>"
            "<input type='text' id='gateway' name='gateway' value='" + 
            (wifi_connected && config.use_dhcp ? WiFi.gatewayIP().toString() : 
             String(config.gateway[0]) + "." + String(config.gateway[1]) + "." + 
             String(config.gateway[2]) + "." + String(config.gateway[3])) + 
            "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='192.168.1.1' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='dns'>DNS Server:</label>"
            "<input type='text' id='dns' name='dns' value='" + 
            (wifi_connected && config.use_dhcp ? WiFi.dnsIP().toString() : 
             String(config.dns[0]) + "." + String(config.dns[1]) + "." + 
             String(config.dns[2]) + "." + String(config.dns[3])) + 
            "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='8.8.8.8' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            
            "<h3>📻 Default FLEX Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='default_frequency'>Default Frequency (MHz):</label>"
            "<input type='number' id='default_frequency' name='default_frequency' step='0.0001' value='" + String(config.default_frequency, 4) + "' min='400' max='1000' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='tx_power'>Default TX Power (0-20 dBm):</label>"
            "<input type='number' id='tx_power' name='tx_power' value='" + String((int)config.tx_power) + "' min='0' max='20' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            "<div style='display:flex;gap:15px;margin-top:15px;'>"
            "<div style='flex:1;'>"
            "<label for='default_capcode'>Default Capcode:</label>"
            "<input type='number' id='default_capcode' name='default_capcode' value='" + String(config.default_capcode) + "' min='1' max='4294967295' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div style='flex:1;'>"
            "</div>"
            "</div>"
            
            "<h3>🔌 API Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='api_port'>API Listening Port:</label>"
            "<input type='number' id='api_port' name='api_port' value='" + String(config.api_port) + "' min='1024' max='65535' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div style='flex:1;'>"
            "</div>"
            "</div>"
            "<div style='display:flex;gap:15px;margin-top:15px;'>"
            "<div style='flex:1;'>"
            "<label for='api_username'>API Username:</label>"
            "<input type='text' id='api_username' name='api_username' value='" + String(config.api_username) + "' maxlength='32' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='api_password'>API Password:</label>"
            "<input type='password' id='api_password' name='api_password' value='" + String(config.api_password) + "' maxlength='64' style='width:100%;padding:14px 16px;border:2px solid var(--theme-border);border-radius:12px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            
            "<div style='margin-top:30px;text-align:center;'>"
            "<button type='submit' class='button'>💾 Save Configuration</button>"
            "</div>"
            "</form>"
            "<script>"
            "function toggleStaticIP() {"
            "  var useDhcp = document.getElementById('use_dhcp').value === '1';"
            "  var staticFields = ['static_ip', 'netmask', 'gateway', 'dns'];"
            "  for (var i = 0; i < staticFields.length; i++) {"
            "    var field = document.getElementById(staticFields[i]);"
            "    field.disabled = useDhcp;"
            "    if (useDhcp) {"
            "      field.style.backgroundColor = '#f5f5f5';"
            "      field.style.color = '#999';"
            "    } else {"
            "      field.style.backgroundColor = 'var(--theme-input)';"
            "      field.style.color = 'var(--theme-text)';"
            "    }"
            "  }"
            "}"
            "window.onload = function() { toggleStaticIP(); validateConfigPower(); };"
            "function validateConfigPower() {"
            "  var p = document.getElementById('tx_power');"
            "  var val = parseInt(p.value);"
            "  if (val < 0 || val > 20) {"
            "    p.style.borderColor = '#e74c3c';"
            "    p.style.backgroundColor = '#fdf2f2';"
            "  } else {"
            "    p.style.borderColor = 'var(--theme-border)';"
            "    p.style.backgroundColor = 'var(--theme-input)';"
            "  }"
            "}"
            "document.getElementById('tx_power').addEventListener('input', validateConfigPower);"
            "document.getElementById('tx_power').addEventListener('keyup', validateConfigPower);"
            "document.getElementById('configForm').addEventListener('submit', function(e) {"
            "  var p = parseInt(document.getElementById('tx_power').value);"
            "  if (p < 0 || p > 20) {"
            "    e.preventDefault();"
            "    alert('TX Power must be between 0 and 20 dBm');"
            "    return false;"
            "  }"
            "});"
            "</script>";
    
    html += get_html_footer();
    webServer.send(200, "text/html", html);
}

void handle_save_config() {
    reset_oled_timeout();
    
    // Track what changes to determine if restart is needed
    bool need_restart = false;
    DeviceConfig old_config = config;
    
    // Update WiFi settings
    if (webServer.hasArg("wifi_ssid")) {
        String ssid = webServer.arg("wifi_ssid");
        ssid.trim();
        strncpy(config.wifi_ssid, ssid.c_str(), sizeof(config.wifi_ssid) - 1);
        config.wifi_ssid[sizeof(config.wifi_ssid) - 1] = '\0';
    }
    
    if (webServer.hasArg("wifi_password")) {
        String password = webServer.arg("wifi_password");
        password.trim();
        strncpy(config.wifi_password, password.c_str(), sizeof(config.wifi_password) - 1);
        config.wifi_password[sizeof(config.wifi_password) - 1] = '\0';
    }
    
    if (webServer.hasArg("use_dhcp")) {
        config.use_dhcp = (webServer.arg("use_dhcp") == "1");
    }
    
    // Update static IP settings
    if (webServer.hasArg("static_ip")) {
        String ip = webServer.arg("static_ip");
        IPAddress parsed_ip;
        if (parsed_ip.fromString(ip)) {
            config.static_ip[0] = parsed_ip[0];
            config.static_ip[1] = parsed_ip[1];
            config.static_ip[2] = parsed_ip[2];
            config.static_ip[3] = parsed_ip[3];
        }
    }
    
    if (webServer.hasArg("netmask")) {
        String netmask = webServer.arg("netmask");
        IPAddress parsed_netmask;
        if (parsed_netmask.fromString(netmask)) {
            config.netmask[0] = parsed_netmask[0];
            config.netmask[1] = parsed_netmask[1];
            config.netmask[2] = parsed_netmask[2];
            config.netmask[3] = parsed_netmask[3];
        }
    }
    
    if (webServer.hasArg("gateway")) {
        String gateway = webServer.arg("gateway");
        IPAddress parsed_gateway;
        if (parsed_gateway.fromString(gateway)) {
            config.gateway[0] = parsed_gateway[0];
            config.gateway[1] = parsed_gateway[1];
            config.gateway[2] = parsed_gateway[2];
            config.gateway[3] = parsed_gateway[3];
        }
    }
    
    if (webServer.hasArg("dns")) {
        String dns = webServer.arg("dns");
        IPAddress parsed_dns;
        if (parsed_dns.fromString(dns)) {
            config.dns[0] = parsed_dns[0];
            config.dns[1] = parsed_dns[1];
            config.dns[2] = parsed_dns[2];
            config.dns[3] = parsed_dns[3];
        }
    }
    
    // Update FLEX settings
    if (webServer.hasArg("default_frequency")) {
        config.default_frequency = webServer.arg("default_frequency").toFloat();
    }
    
    if (webServer.hasArg("default_capcode")) {
        config.default_capcode = strtoull(webServer.arg("default_capcode").c_str(), NULL, 10);
    }
    
    // Update API settings
    if (webServer.hasArg("api_username")) {
        String username = webServer.arg("api_username");
        username.trim();
        strncpy(config.api_username, username.c_str(), sizeof(config.api_username) - 1);
        config.api_username[sizeof(config.api_username) - 1] = '\0';
    }
    
    if (webServer.hasArg("api_password")) {
        String password = webServer.arg("api_password");
        password.trim();
        strncpy(config.api_password, password.c_str(), sizeof(config.api_password) - 1);
        config.api_password[sizeof(config.api_password) - 1] = '\0';
    }
    
    if (webServer.hasArg("api_port")) {
        config.api_port = webServer.arg("api_port").toInt();
    }
    
    // Update device settings
    if (webServer.hasArg("tx_power")) {
        float power = webServer.arg("tx_power").toFloat();
        if (power >= 0.0 && power <= 20.0) {
            config.tx_power = power;
            current_tx_power = config.tx_power;
            radio.setOutputPower(current_tx_power);
        }
    }
    
    if (webServer.hasArg("banner_message")) {
        String banner = webServer.arg("banner_message");
        banner.trim();
        if (banner.length() == 0) banner = DEFAULT_BANNER;
        strncpy(config.banner_message, banner.c_str(), sizeof(config.banner_message) - 1);
        config.banner_message[sizeof(config.banner_message) - 1] = '\0';
    }
    
    if (webServer.hasArg("theme")) {
        config.theme = webServer.arg("theme").toInt();
    }
    
    // Update WiFi enable
    if (webServer.hasArg("enable_wifi")) {
        config.enable_wifi = (webServer.arg("enable_wifi") == "1");
    }
    
    // Check what changed to determine if restart is needed
    need_restart = (strcmp(old_config.wifi_ssid, config.wifi_ssid) != 0) ||
                   (strcmp(old_config.wifi_password, config.wifi_password) != 0) ||
                   (old_config.use_dhcp != config.use_dhcp) ||
                   (memcmp(old_config.static_ip, config.static_ip, 4) != 0) ||
                   (memcmp(old_config.netmask, config.netmask, 4) != 0) ||
                   (memcmp(old_config.gateway, config.gateway, 4) != 0) ||
                   (memcmp(old_config.dns, config.dns, 4) != 0) ||
                   (old_config.enable_wifi != config.enable_wifi) ||
                   (old_config.api_port != config.api_port) ||
                   (strcmp(old_config.api_username, config.api_username) != 0) ||
                   (strcmp(old_config.api_password, config.api_password) != 0);
    
    // Save configuration to EEPROM
    if (save_config()) {
        log_serial_message("CONFIG: Settings saved to EEPROM");
        display_current(); // Update display with new banner
        
        // Return JSON response
        if (need_restart) {
            webServer.send(200, "application/json", "{\"success\":true,\"restart\":true}");
            delay(5000);
            ESP.restart();
        } else {
            webServer.send(200, "application/json", "{\"success\":true,\"restart\":false}");
        }
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save configuration to EEPROM\"}");
    }
}

void handle_device_status() {
    reset_oled_timeout();
    
    String html = get_html_header("Device Status");
    
    html += "<div class='header'>"
            "<h1>📊 Device Status</h1>"
            "</div>";
    
    html += "<div class='nav'>"
            "<a href='/' class='button secondary'>📡 Message</a>"
            "<a href='/configuration' class='button secondary'>⚙️ Configuration</a>"
            "<a href='/status' class='button'>📊 Status</a>"
            "<a href='https://github.com/geekinsanemx/flex-fsk-tx' target='_blank' class='button secondary'>🐙 GitHub</a>"
            "</div>";
    
    // Two-column layout container
    html += "<div style='display:flex;gap:30px;flex-wrap:wrap;'>";
    
    // Left column - Device Information
    html += "<div style='flex:1;min-width:300px;'>";
    html += "<h3 >📡 Device Information</h3>";
    html += "<p><strong>Banner:</strong> " + String(config.banner_message) + "</p>";
    html += "<p><strong>Frequency:</strong> " + String(current_tx_frequency, 4) + " MHz</p>";
    html += "<p><strong>TX Power:</strong> " + String(current_tx_power, 1) + " dBm</p>";
    html += "<p><strong>Default Capcode:</strong> " + String(config.default_capcode) + "</p>";
    html += "<p><strong>Uptime:</strong> " + String(millis() / 60000) + " minutes</p>";
    html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    String theme_name = "";
    switch(config.theme) {
        case 0: theme_name = "🌞 Minimal White"; break;
        case 1: theme_name = "🌞 Aqua Breeze"; break;
        case 2: theme_name = "🌞 Soft Sand"; break;
        case 3: theme_name = "🌞 Sky Blue"; break;
        case 4: theme_name = "🌞 Minty Fresh"; break;
        case 5: theme_name = "🌙 Carbon Black"; break;
        case 6: theme_name = "🌙 Neon Purple"; break;
        case 7: theme_name = "🌙 Cyber Green"; break;
        case 8: theme_name = "🌙 Deep Ocean"; break;
        case 9: theme_name = "🌙 Retro Amber"; break;
        default: theme_name = "Unknown"; break;
    }
    html += "<p><strong>Theme:</strong> " + theme_name + "</p>";
    String wifi_status = config.enable_wifi ? "Enabled" : "Disabled";
    html += "<p><strong>WiFi Status:</strong> " + wifi_status + "</p>";
    html += "<p><strong>API Port:</strong> " + String(config.api_port) + "</p>";
    html += "<p><strong>Chip Model:</strong> " + String(ESP.getChipModel()) + "</p>";
    html += "<p><strong>CPU Frequency:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>";
    
    // Battery voltage for Heltec
    uint16_t battery_voltage;
    int battery_percentage;
    getBatteryInfo(&battery_voltage, &battery_percentage);
    html += "<p><strong>Battery:</strong> " + String(battery_voltage) + "mV (" + String(battery_percentage) + "%)</p>";
    html += "</div>";
    
    // Right column - Network Information
    html += "<div style='flex:1;min-width:300px;'>";
    html += "<h3 >📶 Network Information</h3>";
    
    if (wifi_connected) {
        html += "<p><strong>WiFi SSID:</strong> " + String(config.wifi_ssid) + "</p>";
        html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
        html += "<p><strong>Subnet Mask:</strong> " + WiFi.subnetMask().toString() + "</p>";
        html += "<p><strong>Gateway:</strong> " + WiFi.gatewayIP().toString() + "</p>";
        html += "<p><strong>DNS Server:</strong> " + WiFi.dnsIP().toString() + "</p>";
        html += "<p><strong>MAC Address:</strong> " + WiFi.macAddress() + "</p>";
        html += "<p><strong>RSSI:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
        String dhcp_status = config.use_dhcp ? "Enabled" : "Static IP";
        html += "<p><strong>DHCP:</strong> " + dhcp_status + "</p>";
    } else if (ap_mode_active) {
        html += "<p><strong>Mode:</strong> Access Point (Configuration Mode)</p>";
        html += "<p><strong>AP SSID:</strong> " + ap_ssid + "</p>";
        html += "<p><strong>AP IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
        html += "<p><strong>AP MAC:</strong> " + WiFi.softAPmacAddress() + "</p>";
        html += "<p><strong>Connected Clients:</strong> " + String(WiFi.softAPgetStationNum()) + "</p>";
    } else {
        html += "<p><strong>Status:</strong> WiFi Disabled</p>";
        html += "<p><strong>MAC Address:</strong> " + WiFi.macAddress() + "</p>";
    }
    html += "</div>"; // Close right column
    html += "</div>"; // Close two-column container
    
    // Serial Messages Log
    html += "<h3>📡 Recent Serial Messages</h3>";
    if (serial_log_count == 0) {
        html += "<p>No serial messages logged yet.</p>";
    } else {
        html += "<div class='serial-log'>";
        
        // Display messages in reverse chronological order (newest first)
        for (int i = 0; i < serial_log_count; i++) {
            int index = (serial_log_index - 1 - i + SERIAL_LOG_SIZE) % SERIAL_LOG_SIZE;
            if (index < 0) index += SERIAL_LOG_SIZE;
            
            unsigned long seconds = serial_log[index].timestamp / 1000;
            unsigned long minutes = seconds / 60;
            seconds = seconds % 60;
            
            html += "<div>";
            html += "<span class='timestamp'>[" + String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds) + "]</span> ";
            html += String(serial_log[index].message);
            html += "</div>";
        }
        html += "</div>";
    }
    
    html += "<h3>⚠️ Factory Reset</h3>";
    html += "<p>Reset device to factory defaults (this will clear all configuration):</p>";
    html += "<form action='/factory_reset' method='post' onsubmit='return confirm(\"Are you sure you want to reset to factory defaults? This will clear all configuration and restart the device.\")'>";
    html += "<button type='submit' class='button' style='background-color:#dc3545;'>🔄 Factory Reset</button>";
    html += "</form>";
    
    html += get_html_footer();
    webServer.send(200, "text/html; charset=utf-8", html);
}

void handle_factory_reset() {
    reset_oled_timeout();
    
    load_default_config();
    save_config();
    
    String html = get_html_header("Factory Reset");
    html += "<div class='status success'>✅ Factory reset completed! Device will restart in 3 seconds...</div>";
    html += "<script>setTimeout(function() { window.location.href = '/'; }, 3000);</script>";
    html += get_html_footer();
    webServer.send(200, "text/html; charset=utf-8", html);
    
    delay(1000);
    ESP.restart();
}

// =============================================================================
// BASE64 DECODE FUNCTION
// =============================================================================

String base64_decode(String input) {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String result = "";
    int val = 0, valb = -8;
    
    for (char c : input) {
        if (c == '=') break;
        const char* pos = strchr(chars, c);
        if (!pos) continue;
        
        val = (val << 6) + (pos - chars);
        valb += 6;
        if (valb >= 0) {
            result += (char)((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return result;
}

// =============================================================================
// REST API FUNCTIONS
// =============================================================================

bool authenticate_api_request() {
    if (!apiServer->hasHeader("Authorization")) {
        return false;
    }
    
    String auth_header = apiServer->header("Authorization");
    if (!auth_header.startsWith("Basic ")) {
        return false;
    }
    
    String encoded_credentials = auth_header.substring(6);
    String decoded_credentials = base64_decode(encoded_credentials);
    
    int colon_index = decoded_credentials.indexOf(':');
    if (colon_index == -1) {
        return false;
    }
    
    String username = decoded_credentials.substring(0, colon_index);
    String password = decoded_credentials.substring(colon_index + 1);
    
    return (username == config.api_username && password == config.api_password);
}

// =============================================================================
// API SERVER FUNCTIONS
// =============================================================================

void handle_api_message() {
    reset_oled_timeout();
    
    // Check authentication
    if (!authenticate_api_request()) {
        apiServer->sendHeader("WWW-Authenticate", "Basic realm=\"Heltec FLEX API\"");
        apiServer->send(401, "application/json", "{\"error\":\"Authentication required\"}");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, apiServer->arg("plain"));
    
    if (error) {
        apiServer->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc["message"].is<String>()) {
        apiServer->send(400, "application/json", "{\"error\":\"Missing required field: message\"}");
        return;
    }
    
    uint64_t capcode = doc["capcode"].is<uint64_t>() ? doc["capcode"] : config.default_capcode;  // Optional capcode, use default if not provided
    float frequency = doc["frequency"].is<float>() ? doc["frequency"] : config.default_frequency;  // Optional frequency, use default if not provided
    String message = doc["message"].as<String>();
    int power = doc["tx_power"].is<int>() ? doc["tx_power"] : config.tx_power;  // Optional tx_power, use default if not provided
    bool mail_drop = doc["mail_drop"].is<bool>() ? doc["mail_drop"] : false;  // Optional mail drop flag
    
    // Convert frequency: if > 1000, assume Hz and convert to MHz
    if (frequency > 1000.0) {
        frequency = frequency / 1000000.0;
    }
    
    // Validate parameters
    if (frequency < 400.0 || frequency > 1000.0) {
        apiServer->send(400, "application/json", "{\"error\":\"Frequency must be between 400.0 and 1000.0 MHz\"}");
        return;
    }
    
    if (power < 0 || power > 20) {
        apiServer->send(400, "application/json", "{\"error\":\"TX Power must be between 0 and 20 dBm\"}");
        return;
    }
    
    if (capcode < 1 || capcode > 4294967295ULL) {
        apiServer->send(400, "application/json", "{\"error\":\"Capcode must be between 1 and 4294967295\"}");
        return;
    }
    
    if (message.length() > 240) {
        apiServer->send(400, "application/json", "{\"error\":\"Message too long (max 240 characters)\"}");
        return;
    }
    
    if (device_state == STATE_TRANSMITTING) {
        apiServer->send(503, "application/json", "{\"error\":\"Device is busy\"}");
        return;
    }
    
    // Configure radio
    current_tx_frequency = frequency;
    current_tx_power = power;
    radio.setFrequency(frequency);
    radio.setOutputPower(power);
    
    // Try to encode and transmit
    if (flex_encode_and_store(capcode, message.c_str(), mail_drop)) {
        // Start transmission using working v2 method
        device_state = STATE_TRANSMITTING;
        transmission_in_progress = true;
        LED_ON();
        display_status();
        
        // Log transmission start
        char tx_log[100];
        snprintf(tx_log, sizeof(tx_log), "TX: Capcode %llu, %.1fMHz, %ddBm", capcode, frequency, power);
        log_serial_message(tx_log);

        bool tx_success = at_transmit_data();

        if (tx_success) {
            // Transmission completed successfully
            JsonDocument response;
            response["status"] = "success";
            response["message"] = "Transmission completed successfully";
            response["frequency"] = frequency;
            response["power"] = power;
            response["capcode"] = capcode;
            response["text"] = message;
            
            String response_str;
            serializeJson(response, response_str);
            apiServer->send(200, "application/json", response_str);
            
            device_state = STATE_IDLE;
            transmission_in_progress = false;
            LED_OFF();
            display_status();
        } else {
            // Transmission failed
            device_state = STATE_IDLE;
            transmission_in_progress = false;
            LED_OFF();
            display_status();
            
            JsonDocument response;
            response["status"] = "error";
            response["message"] = "Transmission failed - see serial output for details";
            response["frequency"] = frequency;
            response["power"] = power;
            response["capcode"] = capcode;
            response["text"] = message;
            
            String response_str;
            serializeJson(response, response_str);
            apiServer->send(500, "application/json", response_str);
            return;
        }
    } else {
        apiServer->send(500, "application/json", "{\"error\":\"Failed to encode FLEX message\"}");
    }
}

// =============================================================================
// IMPROVED TRANSMISSION FUNCTION (ESP32 CORE 3.3.0 FIX) - from working v2
// =============================================================================

bool at_transmit_data() {
    reset_oled_timeout();


    const int CHUNK_SIZE = 255;  // SX1262 FIFO limitation
    int chunks = (current_tx_total_length + CHUNK_SIZE - 1) / CHUNK_SIZE;


    for (int i = 0; i < chunks; i++) {
        int chunk_start = i * CHUNK_SIZE;
        int chunk_size = min(CHUNK_SIZE, current_tx_total_length - chunk_start);


        // Test SPI communication before each chunk (but only for first chunk to avoid spam)
        if (i == 0) {
            int spi_test = radio.standby();
            if (spi_test != RADIOLIB_ERR_NONE) {
                return false;
            }
        }

        // Reset transmission complete flag
        transmission_complete = false;

        // Add small delay before transmission to ensure radio is ready
        delay(10);

        int tx_state = radio.startTransmit(tx_data_buffer + chunk_start, chunk_size);

        if (tx_state != RADIOLIB_ERR_NONE) {
            return false;
        }

        // Wait for this chunk to complete with improved timeout calculation
        unsigned long chunk_timeout = millis() + (chunk_size * 10 + 3000); // Dynamic timeout based on chunk size
        while (!transmission_complete && millis() < chunk_timeout) {
            yield();
            delay(1); // Use delay instead of delayMicroseconds for better stability
        }

        if (!transmission_complete) {
            return false;
        }

        // Inter-chunk delay for radio stability (ESP32 core 3.3.0 compatibility)
        if (i < chunks - 1) { // Don't delay after last chunk
            delay(50); // Increased delay between chunks for stability
        }
    }

    return true;
}

// =============================================================================
// RADIO ISR HANDLERS
// =============================================================================

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void on_transmit_complete() {
    // Add memory barrier to prevent compiler optimization issues
    __asm__ __volatile__ ("" ::: "memory");
    
    transmission_complete = true;
    transmission_in_progress = false;
    fifo_empty = true;
    transmission_processing_complete = true;
    
    // Another memory barrier
    __asm__ __volatile__ ("" ::: "memory");
}

// =============================================================================
// AT COMMAND PROCESSING
// =============================================================================

void at_send_ok() {
    Serial.println("OK");
    log_serial_message("OK");
}

void at_send_error() {
    Serial.println("ERROR");
    log_serial_message("ERROR");
}

void at_send_response(const char *name, const char *value) {
    Serial.print("+");
    Serial.print(name);
    Serial.print(": ");
    Serial.println(value);
    
    // Log the response
    char log_msg[100];
    snprintf(log_msg, sizeof(log_msg), "+%s: %s", name, value);
    log_serial_message(log_msg);
    
    at_send_ok();
}

void at_send_response_int(const char *name, int value) {
    Serial.print("+");
    Serial.print(name);
    Serial.print(": ");
    Serial.println(value);
    
    // Log the response
    char log_msg[100];
    snprintf(log_msg, sizeof(log_msg), "+%s: %d", name, value);
    log_serial_message(log_msg);
    
    at_send_ok();
}

void at_send_response_float(const char *name, float value, int decimals) {
    Serial.print("+");
    Serial.print(name);
    Serial.print(": ");
    Serial.println(value, decimals);
    
    // Log the response
    char log_msg[100];
    snprintf(log_msg, sizeof(log_msg), "+%s: %.4f", name, value);
    log_serial_message(log_msg);
    
    at_send_ok();
}

void at_reset_state() {
    device_state = STATE_IDLE;
    state_timeout = 0;
    at_buffer_pos = 0;
    at_command_ready = false;
    expected_data_length = 0;
    data_receive_timeout = 0;
    flex_capcode = 0;
    flex_message_pos = 0;
    flex_message_timeout = 0;
    flex_mail_drop = false;
    memset(flex_message_buffer, 0, sizeof(flex_message_buffer));
}

void at_parse_command(const char *command) {
    reset_oled_timeout();
    
    if (strlen(command) == 0) {
        at_send_error();
        return;
    }
    
    // Convert command to uppercase for case-insensitive processing
    char cmd_upper[AT_BUFFER_SIZE];
    strncpy(cmd_upper, command, sizeof(cmd_upper) - 1);
    cmd_upper[sizeof(cmd_upper) - 1] = '\0';
    
    for (int i = 0; cmd_upper[i]; i++) {
        cmd_upper[i] = toupper(cmd_upper[i]);
    }
    
    // Parse command
    char *cmd_name = strtok(cmd_upper, "=?");
    char *equals_pos = strchr(command, '=');
    char *query_pos = strchr(command, '?');
    
    if (cmd_name == NULL) {
        at_send_error();
        return;
    }
    
    // Basic AT command
    if (strcmp(cmd_name, "AT") == 0) {
        at_reset_state();
        at_send_ok();
    }
    // Frequency commands
    else if (strcmp(cmd_name, "AT+FREQ") == 0) {
        if (query_pos != NULL) {
            at_send_response_float("FREQ", current_tx_frequency, 4);
        } else if (equals_pos != NULL) {
            float freq = atof(equals_pos + 1);
            if (freq >= 400.0 && freq <= 1000.0) {
                current_tx_frequency = freq;
                
                int state = radio.setFrequency(freq);
                if (state != RADIOLIB_ERR_NONE) {
                    Serial.println(state);
                    at_send_error();
                    return;
                }
                
                at_send_ok();
                display_status();
            } else {
                at_send_error();
            }
        } else {
            at_send_error();
        }
    }
    // Power commands
    else if (strcmp(cmd_name, "AT+POWER") == 0) {
        if (query_pos != NULL) {
            at_send_response_float("POWER", current_tx_power, 1);
        } else if (equals_pos != NULL) {
            float power = atof(equals_pos + 1);
            if (power >= 0.0 && power <= 20.0) {
                current_tx_power = power;
                
                int state = radio.setOutputPower(power);
                if (state != RADIOLIB_ERR_NONE) {
                    at_send_error();
                    return;
                }
                
                at_send_ok();
                display_status();
            } else {
                at_send_error();
            }
        } else {
            at_send_error();
        }
    }
    // Data transmission command
    else if (strcmp(cmd_name, "AT+SEND") == 0) {
        if (equals_pos != NULL) {
            int length = atoi(equals_pos + 1);
            if (length > 0 && length <= 2048) {
                expected_data_length = length;
                data_receive_timeout = millis() + 15000; // 15 second timeout
                device_state = STATE_WAITING_FOR_DATA;
                at_buffer_pos = 0;
                Serial.println("+SEND: READY");
                display_status();
            } else {
                at_send_error();
            }
        } else {
            at_send_error();
        }
    }
    // FLEX message command (v2+ feature)
    else if (strcmp(cmd_name, "AT+MSG") == 0) {
        if (equals_pos != NULL) {
            if (str2uint64(&flex_capcode, equals_pos + 1) == 0) {
                flex_message_pos = 0;
                flex_message_timeout = millis() + FLEX_MSG_TIMEOUT;
                device_state = STATE_WAITING_FOR_MSG;
                memset(flex_message_buffer, 0, sizeof(flex_message_buffer));
                Serial.println("+MSG: READY");
                display_status();
            } else {
                at_send_error();
            }
        } else {
            at_send_error();
        }
    }
    // Mail drop flag command (v2+ feature)
    else if (strcmp(cmd_name, "AT+MAILDROP") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("MAILDROP", flex_mail_drop ? 1 : 0);
        } else if (equals_pos != NULL) {
            int mail_drop = atoi(equals_pos + 1);
            flex_mail_drop = (mail_drop != 0);
            at_send_ok();
        } else {
            at_send_error();
        }
    }
    // Status query
    else if (strcmp(cmd_name, "AT+STATUS") == 0) {
        if (query_pos != NULL) {
            const char *status_str = "UNKNOWN";
            switch (device_state) {
                case STATE_IDLE: status_str = "READY"; break;
                case STATE_WAITING_FOR_DATA: status_str = "WAITING_DATA"; break;
                case STATE_WAITING_FOR_MSG: status_str = "WAITING_MSG"; break;
                case STATE_TRANSMITTING: status_str = "TRANSMITTING"; break;
                case STATE_ERROR: status_str = "ERROR"; break;
                case STATE_WIFI_CONNECTING: status_str = "WIFI_CONNECTING"; break;
                case STATE_WIFI_AP_MODE: status_str = "WIFI_AP_MODE"; break;
            }
            at_send_response("STATUS", status_str);
        } else {
            at_send_error();
        }
    }
    // Abort command
    else if (strcmp(cmd_name, "AT+ABORT") == 0) {
        at_reset_state();
        at_send_ok();
        display_status();
    }
    // Reset command
    else if (strcmp(cmd_name, "AT+RESET") == 0) {
        at_send_ok();
        delay(1000);
        ESP.restart();
    }

    // New WiFi-related AT commands
    else if (strcmp(cmd_name, "AT+WIFI") == 0) {
        if (query_pos != NULL) {
            String status = "DISCONNECTED";
            if (wifi_connected) {
                status = "CONNECTED," + WiFi.localIP().toString();
            } else if (ap_mode_active) {
                status = "AP_MODE," + WiFi.softAPIP().toString();
            }
            at_send_response("WIFI", status.c_str());
        } else if (equals_pos != NULL) {
            // Parse: AT+WIFI=ssid,password[,dhcp][,ip,mask,gw,dns]
            String params = String(equals_pos + 1);
            // Basic implementation - enhanced parsing needed
            int comma1 = params.indexOf(',');
            if (comma1 > 0) {
                String ssid = params.substring(0, comma1);
                String remaining = params.substring(comma1 + 1);
                int comma2 = remaining.indexOf(',');
                String password = (comma2 > 0) ? remaining.substring(0, comma2) : remaining;
                
                strncpy(config.wifi_ssid, ssid.c_str(), sizeof(config.wifi_ssid) - 1);
                strncpy(config.wifi_password, password.c_str(), sizeof(config.wifi_password) - 1);
                config.wifi_ssid[sizeof(config.wifi_ssid) - 1] = '\0';
                config.wifi_password[sizeof(config.wifi_password) - 1] = '\0';
                
                // Connect to WiFi
                wifi_connect();
                at_send_ok();
            } else {
                at_send_error();
            }
        } else {
            at_send_error();
        }
    }
    else if (strcmp(cmd_name, "AT+BANNER") == 0) {
        if (query_pos != NULL) {
            at_send_response("BANNER", config.banner_message);
        } else if (equals_pos != NULL) {
            String banner = String(equals_pos + 1);
            banner.trim();
            if (banner.length() == 0) banner = DEFAULT_BANNER;
            if (banner.length() > 16) banner = banner.substring(0, 16);
            strncpy(config.banner_message, banner.c_str(), sizeof(config.banner_message) - 1);
            config.banner_message[sizeof(config.banner_message) - 1] = '\0';
            display_status(); // Update display immediately
            at_send_ok();
        } else {
            at_send_error();
        }
    }
    else if (strcmp(cmd_name, "AT+APIPORT") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("APIPORT", config.api_port);
        } else if (equals_pos != NULL) {
            int port = atoi(equals_pos + 1);
            if (port >= 1024 && port <= 65535) {
                config.api_port = port;
                at_send_ok();
            } else {
                at_send_error();
            }
        } else {
            at_send_error();
        }
    }
    else if (strcmp(cmd_name, "AT+APIUSER") == 0) {
        if (query_pos != NULL) {
            at_send_response("APIUSER", config.api_username);
        } else if (equals_pos != NULL) {
            String username = String(equals_pos + 1);
            username.trim();
            strncpy(config.api_username, username.c_str(), sizeof(config.api_username) - 1);
            config.api_username[sizeof(config.api_username) - 1] = '\0';
            at_send_ok();
        } else {
            at_send_error();
        }
    }
    else if (strcmp(cmd_name, "AT+APIPASS") == 0) {
        if (query_pos != NULL) {
            at_send_response("APIPASS", "***");  // Don't show actual password
        } else if (equals_pos != NULL) {
            String password = String(equals_pos + 1);
            password.trim();
            strncpy(config.api_password, password.c_str(), sizeof(config.api_password) - 1);
            config.api_password[sizeof(config.api_password) - 1] = '\0';
            at_send_ok();
        } else {
            at_send_error();
        }
    }
    else if (strcmp(cmd_name, "AT+WIFIENABLE") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("WIFIENABLE", config.enable_wifi ? 1 : 0);
        } else if (equals_pos != NULL) {
            int enable = atoi(equals_pos + 1);
            config.enable_wifi = (enable != 0);
            at_send_ok();
        } else {
            at_send_error();
        }
    }
    else if (strcmp(cmd_name, "AT+BATTERY") == 0) {
        if (query_pos != NULL) {
            uint16_t battery_voltage;
            int battery_percentage;
            getBatteryInfo(&battery_voltage, &battery_percentage);
            String battery_str = String(battery_voltage) + "mV " + String(battery_percentage) + "%";
            at_send_response("BATTERY", battery_str.c_str());
        } else {
            at_send_error();
        }
    }
    else if (strcmp(cmd_name, "AT+SAVE") == 0) {
        if (save_config()) {
            at_send_response("CONFIG", "Configuration saved to EEPROM");
        } else {
            at_send_error();
        }
    }
    else if (strcmp(cmd_name, "AT+FACTORYRESET") == 0) {
        load_default_config();
        save_config();
        at_send_response("RESET", "Factory reset initiated");
        at_send_response("RESET", "EEPROM cleared");
        at_send_response("RESET", "Restarting device...");
        delay(1000);
        ESP.restart();
    }
    else if (strcmp(cmd_name, "AT+WIFICONFIG") == 0) {
        at_send_response("WIFICONFIG", "Interactive WiFi Configuration:");
        at_send_response("WIFICONFIG", ("Current SSID: " + String(config.wifi_ssid)).c_str());
        at_send_response("WIFICONFIG", ("Use DHCP: " + String(config.use_dhcp ? "YES" : "NO")).c_str());
        if (!config.use_dhcp) {
            String ip_str = String(config.static_ip[0]) + "." + String(config.static_ip[1]) + "." + 
                           String(config.static_ip[2]) + "." + String(config.static_ip[3]);
            at_send_response("WIFICONFIG", ("Static IP: " + ip_str).c_str());
        }
        at_send_response("WIFICONFIG", "Use individual AT commands to configure:");
        at_send_response("WIFICONFIG", "AT+WIFI=<ssid>,<password>[,dhcp][,ip,mask,gw,dns]");
        at_send_ok();
    }
    else if (strcmp(cmd_name, "AT+RESET") == 0) {
        at_send_response("SYSTEM", "Restarting device...");
        delay(1000);
        ESP.restart();
    }
    // Unknown command
    else {
        at_send_error();
    }
}

void at_process_serial() {
    while (Serial.available()) {
        char c = Serial.read();
        
        if (device_state == STATE_WAITING_FOR_DATA) {
            // Binary data mode - collect bytes until we have expected length
            if (at_buffer_pos < expected_data_length && at_buffer_pos < AT_BUFFER_SIZE) {
                tx_data_buffer[at_buffer_pos] = c;
                at_buffer_pos++;
                
                if (at_buffer_pos >= expected_data_length) {
                    // Got all data - start transmission
                    current_tx_total_length = expected_data_length;
                    current_tx_remaining_length = expected_data_length;
                    
                    // Start transmission using working v2 method
                    device_state = STATE_TRANSMITTING;
                    transmission_in_progress = true;
                    LED_ON();
                    display_status();
                    
                    bool tx_success = at_transmit_data();

                    if (tx_success) {
                        at_send_ok();
                        device_state = STATE_IDLE;
                        transmission_in_progress = false;
                        LED_OFF();
                        display_status();
                    } else {
                        device_state = STATE_IDLE;
                        transmission_in_progress = false;
                        LED_OFF();
                        display_status();
                        at_send_error();
                    }
                    
                    at_buffer_pos = 0;
                    expected_data_length = 0;
                }
            }
        } else if (device_state == STATE_WAITING_FOR_MSG) {
            // Text message mode for FLEX
            if (c == '\r' || c == '\n') {
                if (flex_message_pos > 0) {
                    flex_message_buffer[flex_message_pos] = '\0';
                    
                    // Encode FLEX message and transmit
                    if (flex_encode_and_store(flex_capcode, flex_message_buffer, flex_mail_drop)) {
                        // Start transmission using working v2 method
                        device_state = STATE_TRANSMITTING;
                        transmission_in_progress = true;
                        LED_ON();
                        display_status();
                        
                        bool tx_success = at_transmit_data();

                        if (tx_success) {
                            at_send_ok();
                            device_state = STATE_IDLE;
                            transmission_in_progress = false;
                            LED_OFF();
                            display_status();
                        } else {
                            device_state = STATE_IDLE;
                            transmission_in_progress = false;
                            LED_OFF();
                            display_status();
                            at_send_error();
                        }
                    } else {
                        device_state = STATE_IDLE;
                        display_status();
                        at_send_error();
                    }
                    
                    // Reset for next message
                    flex_message_pos = 0;
                    flex_capcode = 0;
                    flex_message_timeout = 0;
                }
            } else if (c >= 32 && c <= 126) { // Printable ASCII only
                if (flex_message_pos < MAX_FLEX_MESSAGE_LENGTH) {
                    flex_message_buffer[flex_message_pos] = c;
                    flex_message_pos++;
                }
            }
        } else {
            // Command mode
            if (c == '\r' || c == '\n') {
                if (at_buffer_pos > 0) {
                    at_buffer[at_buffer_pos] = '\0';
                    at_parse_command(at_buffer);
                    at_buffer_pos = 0;
                }
            } else if (at_buffer_pos < AT_BUFFER_SIZE - 1) {
                at_buffer[at_buffer_pos] = c;
                at_buffer_pos++;
            }
        }
    }
}

// =============================================================================
// SETUP AND MAIN LOOP
// =============================================================================
// PANIC FUNCTION (CRITICAL ERROR HANDLING) - from working v2
// =============================================================================

void panic() {
    display_panic();
    Serial.print("ERROR\r\n");
    while (true) {
        delay(100000);
    }
}

// =============================================================================

void setup() {
    // Initialize serial communication
    Serial.begin(HELTEC_SERIAL_BAUD);
    delay(100); // Short delay for serial to initialize
    log_serial_message("STARTUP: FLEX Message Transmitter v3.0");

    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Load configuration
    load_config();
    
    // Apply loaded configuration
    current_tx_frequency = config.default_frequency;
    current_tx_power = config.tx_power;

    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    LED_OFF();  // Start with LED off

    // Initialize factory reset button with pullup resistor (GPIO 0)
    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
    button_state = digitalRead(FACTORY_RESET_PIN);
    last_button_state = button_state;

    // Initialize battery monitoring ADC (control handled in reading function)
    pinMode(ADC_CTRL, OUTPUT);
    digitalWrite(ADC_CTRL, LOW);  // Default off to save power

    // Initialize display
    display_setup();
    
    // Show startup message
    display.clear();
    display.setFont(FONT_BANNER);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 20, "Starting...");
    display.display();
    delay(2000);

    // CRITICAL: Add delays for hardware stabilization (from working v2)
    delay(200); // Increased initial delay

    // Initialize radio module in FSK mode
    int radio_init_state = radio.beginFSK();

    if (radio_init_state != RADIOLIB_ERR_NONE) {
        panic();
    }

    // Add stabilization delay after radio init (from working v2)
    delay(100);

    // Configure SPI timing (minimal intervention)
    configure_spi_timing();

    // Put radio in standby before configuration (from working v2)
    radio_init_state = radio.standby();
    if (radio_init_state != RADIOLIB_ERR_NONE) {
        panic();
    }
    delay(20);
    
    radio_init_state = radio.setFrequency(TX_FREQ_DEFAULT);
    if (radio_init_state != RADIOLIB_ERR_NONE) {
        Serial.println(radio_init_state);
        panic();
    }
    delay(20);

    radio_init_state = radio.setBitRate(TX_BITRATE);
    if (radio_init_state != RADIOLIB_ERR_NONE) {
        Serial.println(radio_init_state);
        panic();
    }
    delay(20);

    radio_init_state = radio.setFrequencyDeviation(TX_DEVIATION);
    if (radio_init_state != RADIOLIB_ERR_NONE) {
        Serial.println(radio_init_state);
        panic();
    }
    delay(20);
    
    radio_init_state = radio.setRxBandwidth(RX_BANDWIDTH);
    if (radio_init_state != RADIOLIB_ERR_NONE) {
        Serial.println(radio_init_state);
        panic();
    }
    delay(20);
    
    radio_init_state = radio.setOutputPower(TX_POWER_DEFAULT);
    if (radio_init_state != RADIOLIB_ERR_NONE) {
        Serial.println(radio_init_state);
        panic();
    }
    delay(20);
    
    radio_init_state = radio.setPreambleLength(PREAMBLE_LENGTH);
    if (radio_init_state != RADIOLIB_ERR_NONE) {
        Serial.println(radio_init_state);
        panic();
    }
    delay(20);
    
    radio.setDataShaping(RADIOLIB_SHAPING_NONE);
    radio.setEncoding(RADIOLIB_ENCODING_NRZ);

    uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    int sync_state = radio.setSyncWord(syncWord, 8);
    if (sync_state != RADIOLIB_ERR_NONE) {
        Serial.println(sync_state);
        panic();
    }
    delay(10);
    
    radio.setCRC(false);

    int packet_mode_state = radio.fixedPacketLengthMode(0);
    if (packet_mode_state != RADIOLIB_ERR_NONE) {
        Serial.println(packet_mode_state);
        panic();
    }

    // Final radio standby to ensure clean state (from working v2)
    radio.standby();

    // Set interrupt for transmission complete
    radio.setDio1Action(on_transmit_complete);

    // Generate unique AP SSID
    ap_ssid = generate_ap_ssid();
    
    // Initialize WiFi if enabled
    if (config.enable_wifi) {
        wifi_connect();
        
        // Initialize web server
        webServer.on("/", handle_root);
        webServer.on("/send", HTTP_POST, handle_send_message);
        webServer.on("/configuration", handle_configuration);
        webServer.on("/save_config", HTTP_POST, handle_save_config);
        webServer.on("/status", handle_device_status);
        webServer.on("/factory_reset", HTTP_POST, handle_factory_reset);
        webServer.begin();
        
        // Initialize API server with configured port
        apiServer = new WebServer(config.api_port);
        apiServer->on("/", HTTP_POST, handle_api_message);
        apiServer->begin();
    }

    // Show ready message
    display_status();
    
    Serial.println("AT READY");
}

void loop() {
    // Handle AT commands from serial
    at_process_serial();

    // Handle WiFi connection
    if (config.enable_wifi) {
        check_wifi_connection();
        webServer.handleClient();
        if (apiServer) {
            apiServer->handleClient();
        }
    }

    // Handle transmission completion
    if (device_state == STATE_TRANSMITTING && transmission_processing_complete) {
        LED_OFF();
        device_state = STATE_IDLE;
        display_status();
        transmission_processing_complete = false;
        fifo_empty = false;
    }

    // Handle timeouts
    unsigned long current_time = millis();
    
    // Data receive timeout
    if (device_state == STATE_WAITING_FOR_DATA && data_receive_timeout > 0 && current_time > data_receive_timeout) {
        at_reset_state();
        at_send_error();
        display_status();
    }
    
    // FLEX message timeout
    if (device_state == STATE_WAITING_FOR_MSG && flex_message_timeout > 0 && current_time > flex_message_timeout) {
        at_reset_state();
        at_send_error();
        display_status();
    }
    
    // OLED timeout
    if (oled_active && (current_time - last_activity_time) > OLED_TIMEOUT_MS) {
        display_turn_off();
    }
    
    // Heartbeat LED - double blink every minute
    if (current_time - last_heartbeat > HEARTBEAT_INTERVAL) {
        if (device_state == STATE_IDLE && heartbeat_blink_count == 0) {
            // Start double blink sequence
            LED_ON();
            heartbeat_led_state = true;
            heartbeat_blink_count = 1;
            heartbeat_blink_start = current_time;
        }
    }
    
    // Handle double blink sequence
    if (heartbeat_blink_count > 0) {
        unsigned long blink_elapsed = current_time - heartbeat_blink_start;
        
        if (heartbeat_blink_count == 1) {
            // First blink on
            if (blink_elapsed >= HEARTBEAT_BLINK_DURATION) {
                LED_OFF();
                heartbeat_led_state = false;
                heartbeat_blink_count = 2;
                heartbeat_blink_start = current_time;
            }
        } else if (heartbeat_blink_count == 2) {
            // Gap between blinks
            if (blink_elapsed >= HEARTBEAT_BLINK_DURATION) {
                LED_ON();
                heartbeat_led_state = true;
                heartbeat_blink_count = 3;
                heartbeat_blink_start = current_time;
            }
        } else if (heartbeat_blink_count == 3) {
            // Second blink on
            if (blink_elapsed >= HEARTBEAT_BLINK_DURATION) {
                LED_OFF();
                heartbeat_led_state = false;
                heartbeat_blink_count = 0;
                last_heartbeat = current_time;
            }
        }
    }
    
    // Factory reset button check with proper debouncing (GPIO 0)
    int reading = digitalRead(FACTORY_RESET_PIN);
    
    // Check if button state changed (for debouncing)
    if (reading != last_button_state) {
        last_button_debounce_time = current_time;
    }
    
    // Only process button state after debounce delay
    if ((current_time - last_button_debounce_time) > BUTTON_DEBOUNCE_DELAY) {
        // If button state has changed after debounce
        if (reading != button_state) {
            button_state = reading;
            
            // Button pressed (LOW due to pullup)
            if (button_state == LOW) {
                if (!factory_reset_active) {
                    factory_reset_start = current_time;
                    factory_reset_active = true;
                }
            } else {
                // Button released
                if (factory_reset_active) {
                    factory_reset_active = false;
                }
            }
        }
    }
    
    // Check if button held long enough for factory reset
    if (factory_reset_active && button_state == LOW) {
        if (current_time - factory_reset_start > FACTORY_RESET_HOLD_TIME) {
            // Factory reset triggered
            Serial.println("Factory Reset triggered, resetting to defaults...");
            load_default_config();
            save_config();
            
            display.clear();
            display.setFont(FONT_DEFAULT);
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.drawString(64, 20, "Factory Reset");
            display.drawString(64, 35, "Restarting...");
            display.display();
            
            delay(2000);
            ESP.restart();
        }
    }
    
    last_button_state = reading;

    // Small delay to prevent excessive CPU usage
    delay(10);
}