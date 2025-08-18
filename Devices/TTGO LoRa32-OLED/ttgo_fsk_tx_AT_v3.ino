/*
 * ttgo_fsk_tx_AT_v3: Enhanced TTGO LoRa32 FSK transmitter with WiFi, Web Interface and REST API
 * Based on ttgo_fsk_tx_AT_v3 with added WiFi capabilities
 * 
 * VERSION 6: Enhanced Error Detection + Theme Support (Latest)
 * 
 * This version adds immediate error detection for transmission start failures
 * while maintaining immediate responses to prevent HTTP timeouts.
 * 
 * Features:
 * - Immediate failure detection if radio.startTransmit() fails
 * - Immediate success response: "Message transmission started successfully"
 * - Detailed error messages with specific error codes
 * - No blocking waits or timing interference
 * - WiFi connectivity with EEPROM configuration storage
 * - Web interface for sending FLEX messages at <ip>/message
 * - Configuration page at <ip>/configuration 
 * - REST API at <ip>:16180 with HTTP Basic Auth
 * - TX power configuration (0-20 dBm)
 * - Character counter and validation
 * - Theme support: Default (Blue), Light (Clear), Dark themes
 * - All AT command functionality preserved
 * 
 * This code is released into the public domain.
 */

#define RADIO_BOARD_AUTO

#include <RadioLib.h>
#include <RadioBoards.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// Include tinyflex header (embedded)
#include "tinyflex.h"

// =============================================================================
// CONSTANTS AND DEFAULTS
// =============================================================================

#define TTGO_SERIAL_BAUD 115200

// Radio defaults
#define TX_FREQ_DEFAULT 931.9375
#define TX_BITRATE 1.6
#define TX_DEVIATION 5
#define TX_POWER_DEFAULT 2
#define RX_BANDWIDTH 10.4
#define PREAMBLE_LENGTH 0

// AT Protocol constants
#define AT_BUFFER_SIZE 512
#define AT_CMD_TIMEOUT 5000
#define AT_MAX_RETRIES 3
#define AT_INTER_CMD_DELAY 100

// Display constants
#define OLED_TIMEOUT_MS (5 * 60 * 1000) // 5 minutes in milliseconds
#define FONT_BANNER u8g2_font_10x20_tr
#define BANNER_HEIGHT 16
#define BANNER_MARGIN 2
#define FONT_DEFAULT u8g2_font_7x13_tr
#define FONT_BOLD u8g2_font_7x13B_tr
#define FONT_LINE_HEIGHT 14
#define FONT_TAB_START 42

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
#define FACTORY_RESET_PIN 0      // Boot button on TTGO
#define FACTORY_RESET_HOLD_TIME 30000  // 30 seconds

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
    uint8_t theme;            // UI theme: 0=default(blue), 1=light, 2=dark
    char banner_message[17];  // Custom banner message (16 chars + null)
    
    uint8_t reserved[44];     // Reserved for future use
};

// =============================================================================
// BUILT-IN LED CONTROL
// =============================================================================

#define LED_PIN 25
#define LED_OFF()  digitalWrite(LED_PIN, LOW)
#define LED_ON()   digitalWrite(LED_PIN, HIGH)

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

Radio radio = new RadioModule();
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

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

// Global variables for transmission state
volatile bool console_loop_enable = true;
volatile bool fifo_empty = false;
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
unsigned long wifi_connect_start = 0;
int wifi_retry_count = 0;
IPAddress device_ip;
String ap_ssid = "";  // Will be generated based on MAC address

// LED heartbeat variables
unsigned long last_heartbeat = 0;
bool heartbeat_state = false;
int heartbeat_blink_count = 0;

// Factory reset variables
unsigned long factory_reset_start = 0;
bool factory_reset_pressed = false;

// Battery monitoring
float battery_voltage = 0.0;
int battery_percentage = 0;
bool battery_present = false;

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
    
    // Default static IP (192.168.1.100)
    config.static_ip[0] = 192; config.static_ip[1] = 168; 
    config.static_ip[2] = 1; config.static_ip[3] = 100;
    config.netmask[0] = 255; config.netmask[1] = 255; 
    config.netmask[2] = 255; config.netmask[3] = 0;
    config.gateway[0] = 192; config.gateway[1] = 168; 
    config.gateway[2] = 1; config.gateway[3] = 1;
    config.dns[0] = 8; config.dns[1] = 8; 
    config.dns[2] = 8; config.dns[3] = 8;
    
    // Default FLEX settings
    config.default_frequency = TX_FREQ_DEFAULT;
    config.default_capcode = 1234567;
    
    // Default API settings
    strcpy(config.api_username, "admin");
    strcpy(config.api_password, "passw0rd");
    config.api_port = REST_API_PORT;
    
    // Default device settings
    config.tx_power = TX_POWER_DEFAULT;
    config.enable_wifi = true;
    config.theme = 0; // Default blue theme
    strcpy(config.banner_message, DEFAULT_BANNER);
    
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
// DISPLAY FUNCTIONS
// =============================================================================

void display_turn_off() {
    if (oled_active) {
        display.setPowerSave(1);
        oled_active = false;
    }
}

void display_turn_on() {
    if (!oled_active) {
        display.setPowerSave(0);
        oled_active = true;
    }
}

void read_battery_status() {
    // Read battery voltage from ADC (adjust pin based on your TTGO board)
    int adc_value = analogRead(35);  // Pin 35 is commonly used for battery monitoring
    
    // Convert ADC reading to voltage using official LilyGO formula for TTGO LoRa32 V2.1_1.6
    battery_voltage = (float)(adc_value) / 4095.0 * 2.0 * 3.3 * 1.1;
    
    // Check if battery is present (voltage > threshold)
    battery_present = (battery_voltage > 2.5);
    
    if (battery_present) {
        // Calculate battery percentage for LiPo using actual TTGO board voltage ranges
        // 3.3V = 0% (safe minimum), 4.15V = 100% (fully charged - charging LED off)
        float voltage_clamped = constrain(battery_voltage, 3.3, 4.15);
        battery_percentage = map(voltage_clamped * 100, 330, 415, 0, 100);
    } else {
        battery_percentage = 0;
    }
}

void handle_led_heartbeat() {
    unsigned long current_time = millis();
    
    // Check if it's time for heartbeat (every minute)
    if (current_time - last_heartbeat >= HEARTBEAT_INTERVAL) {
        // Start heartbeat sequence
        heartbeat_blink_count = 0;
        heartbeat_state = true;
        LED_ON();
        last_heartbeat = current_time;
    }
    
    // Handle blinking during heartbeat
    if (heartbeat_state) {
        static unsigned long last_blink = 0;
        
        if (current_time - last_blink >= HEARTBEAT_BLINK_DURATION) {
            if (heartbeat_blink_count < 4) {  // 2 blinks = 4 state changes (on-off-on-off)
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

String generate_ap_ssid() {
    // Initialize WiFi to ensure MAC is available
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Get MAC address for unique identifier
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    // Use last 3 bytes of MAC to create more unique identifier
    uint32_t unique_id = (mac[3] << 16) | (mac[4] << 8) | mac[5];
    
    // Convert to hexadecimal for better uniqueness (last 4 hex digits)
    String suffix = String(unique_id & 0xFFFF, HEX);
    suffix.toUpperCase();
    while (suffix.length() < 4) {
        suffix = "0" + suffix;
    }
    
    Serial.print("Device MAC: ");
    for (int i = 0; i < 6; i++) {
        if (i > 0) Serial.print(":");
        if (mac[i] < 16) Serial.print("0");
        Serial.print(mac[i], HEX);
    }
    Serial.println();
    Serial.println("Generated AP SSID: TTGO_FLEX_" + suffix);
    
    return "TTGO_FLEX_" + suffix;
}

void handle_factory_reset() {
    bool button_pressed = (digitalRead(FACTORY_RESET_PIN) == LOW);
    unsigned long current_time = millis();
    
    if (button_pressed && !factory_reset_pressed) {
        // Button just pressed
        factory_reset_pressed = true;
        factory_reset_start = current_time;
        Serial.println("Factory reset button pressed - hold for 30 seconds");
    } else if (!button_pressed && factory_reset_pressed) {
        // Button released before timeout
        factory_reset_pressed = false;
        Serial.println("Factory reset cancelled");
    } else if (button_pressed && factory_reset_pressed) {
        // Button held - check timeout
        if (current_time - factory_reset_start >= FACTORY_RESET_HOLD_TIME) {
            // Perform factory reset
            Serial.println("Factory reset initiated!");
            
            // Show reset message on display
            display.clearBuffer();
            display.setFont(u8g2_font_6x10_tr);
            display.drawStr(10, 20, "FACTORY RESET");
            display.drawStr(10, 35, "Restoring defaults...");
            display.sendBuffer();
            
            // Load defaults and save
            load_default_config();
            save_config();
            
            delay(2000);
            
            // Restart device
            ESP.restart();
        } else {
            // Show countdown on display every 5 seconds
            unsigned long remaining = FACTORY_RESET_HOLD_TIME - (current_time - factory_reset_start);
            if ((current_time - factory_reset_start) % 5000 < 100) {
                Serial.print("Factory reset in: ");
                Serial.print(remaining / 1000);
                Serial.println(" seconds");
            }
        }
    }
}

void reset_oled_timeout() {
    last_activity_time = millis();
    display_turn_on();
}

void display_panic() {
    const int centerX = display.getWidth() / 2;
    const int centerY = display.getHeight() / 2;
    const char *message = "System halted";

    display.clearBuffer();
    display.setFont(u8g2_font_open_iconic_check_4x_t);
    display.drawGlyph(centerX - (32 / 2), centerY + (32 / 2), 66);
    display.setFont(u8g2_font_nokiafc22_tr);
    int width = display.getStrWidth(message);
    display.drawStr(centerX - (width / 2), centerY + 30, message);
    display.sendBuffer();
}

void display_setup() {
    display.begin();
    display.clearBuffer();
}

void display_ap_info() {
    if (!oled_active) return;

    display.clearBuffer();

    // No banner in AP mode to save space
    // Start from top of screen
    int info_start_y = 12;

    // Draw AP mode info with larger font for better readability
    display.setFont(u8g2_font_7x13_tr);
    
    display.drawStr(0, info_start_y, "AP Mode Active");
    
    info_start_y += 14;
    // Use smaller font for SSID to fit on screen
    display.setFont(u8g2_font_6x10_tr);
    String ssid_display = "SSID: " + ap_ssid;
    display.drawStr(0, info_start_y, ssid_display.c_str());
    
    // Back to normal font for other info
    display.setFont(u8g2_font_7x13_tr);
    info_start_y += 12;
    display.drawStr(0, info_start_y, "Pass: 12345678");
    
    info_start_y += 12;
    String ip_str = "IP: " + WiFi.softAPIP().toString();
    display.drawStr(0, info_start_y, ip_str.c_str());

    display.sendBuffer();
}

void display_status() {
    if (!oled_active) return;
    
    read_battery_status();  // Ensure fresh battery reading for display

    String tx_power_str;
    if (battery_present) {
        tx_power_str = String(current_tx_power, 1) + "dBm // " + String(battery_percentage) + "%";
    } else {
        tx_power_str = String(current_tx_power, 1) + "dBm";
    }
    String tx_frequency_str = String(current_tx_frequency, 4) + " MHz";
    String status_str;
    String wifi_str;

    // Determine status based on device state
    switch (device_state) {
        case STATE_IDLE:
            status_str = "Ready";
            break;
        case STATE_WAITING_FOR_DATA:
            status_str = "Receiving Data...";
            break;
        case STATE_WAITING_FOR_MSG:
            status_str = "Receiving Msg...";
            break;
        case STATE_TRANSMITTING:
            status_str = "Transmitting...";
            break;
        case STATE_ERROR:
            status_str = "Error";
            break;
        case STATE_WIFI_CONNECTING:
            status_str = "WiFi Connecting...";
            break;
        case STATE_WIFI_AP_MODE:
            status_str = "WiFi AP Mode";
            break;
        default:
            status_str = "Unknown";
            break;
    }

    // WiFi status
    if (wifi_connected) {
        wifi_str = "IP: " + WiFi.localIP().toString();
    } else if (ap_mode_active) {
        wifi_str = "AP: " + WiFi.softAPIP().toString();
    } else if (!config.enable_wifi) {
        wifi_str = "IP: disabled wifi";
    } else {
        wifi_str = "WiFi: Connecting...";
    }

    display.clearBuffer();

    // Draw banner at top
    display.setFont(FONT_BANNER);
    int banner_width = display.getStrWidth(config.banner_message);
    int banner_x = (display.getWidth() - banner_width) / 2;
    display.drawStr(banner_x, BANNER_HEIGHT, config.banner_message);

    // Calculate starting position for status info
    int status_start_y = BANNER_HEIGHT + BANNER_MARGIN + 10;

    // Draw status info below banner (smaller font to fit WiFi info)
    display.setFont(u8g2_font_6x10_tr);
    
    display.drawStr(0, status_start_y, "State: ");
    display.drawStr(40, status_start_y, status_str.c_str());

    status_start_y += 10;
    display.drawStr(0, status_start_y, "Pwr: ");
    display.drawStr(30, status_start_y, tx_power_str.c_str());

    status_start_y += 10;
    display.drawStr(0, status_start_y, "Freq: ");
    display.drawStr(35, status_start_y, tx_frequency_str.c_str());

    status_start_y += 10;
    display.drawStr(0, status_start_y, wifi_str.c_str());

    display.sendBuffer();
}

// =============================================================================
// FLEX ENCODING FUNCTIONS
// =============================================================================

static int str2uint64(uint64_t *out, const char *s) {
    if (!s || s[0] == '\0') return -1;
    
    uint64_t result = 0;
    const char *p = s;
    
    while (*p) {
        if (*p < '0' || *p > '9') return -1;
        if (result > (UINT64_MAX - (*p - '0')) / 10) return -1;
        result = result * 10 + (*p - '0');
        p++;
    }
    
    *out = result;
    return 0;
}

bool flex_encode_and_store(uint64_t capcode, const char *message, bool mail_drop) {
    uint8_t flex_buffer[FLEX_BUFFER_SIZE];
    struct tf_message_config config = {0};
    config.mail_drop = mail_drop ? 1 : 0;
    
    int error = 0;
    size_t encoded_size = tf_encode_flex_message_ex(message, capcode, flex_buffer, 
                                                   sizeof(flex_buffer), &error, &config);
    
    if (error < 0 || encoded_size == 0 || encoded_size > sizeof(tx_data_buffer)) {
        return false;
    }
    
    memcpy(tx_data_buffer, flex_buffer, encoded_size);
    current_tx_total_length = encoded_size;
    current_tx_remaining_length = encoded_size;
    
    return true;
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
        IPAddress gateway(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]);
        IPAddress subnet(config.netmask[0], config.netmask[1], config.netmask[2], config.netmask[3]);
        IPAddress dns(config.dns[0], config.dns[1], config.dns[2], config.dns[3]);
        
        WiFi.config(ip, gateway, subnet, dns);
    }
    
    WiFi.begin(config.wifi_ssid, config.wifi_password);
    wifi_connect_start = millis();
    wifi_retry_count = 0;
}

void start_ap_mode() {
    device_state = STATE_WIFI_AP_MODE;
    ap_mode_active = true;
    
    // Generate unique SSID if not already done
    if (ap_ssid == "") {
        ap_ssid = generate_ap_ssid();
    }
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid.c_str(), "12345678");
    
    device_ip = WiFi.softAPIP();
    display_ap_info();  // Display AP info on OLED
    
    Serial.println("AP Mode started");
    Serial.print("AP SSID: ");
    Serial.println(ap_ssid);
    Serial.println("AP Password: 12345678");
    Serial.print("AP IP: ");
    Serial.println(device_ip);
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

String get_html_header(const String& title) {
    String theme_css;
    
    // Generate CSS based on selected theme
    switch(config.theme) {
        case 1: // Light theme
            theme_css = "body{background:linear-gradient(135deg,#f5f7fa 0%,#c3cfe2 100%);}"
                       ".container{background:white;box-shadow:0 10px 30px rgba(0,0,0,0.1);}"
                       ".header{background:linear-gradient(45deg,#74b9ff,#0984e3);}"
                       "input:focus,textarea:focus{border-color:#74b9ff;}"
                       ".btn{background:linear-gradient(45deg,#74b9ff,#0984e3);}"
                       ".btn:hover:not(:disabled){box-shadow:0 5px 15px rgba(116,185,255,0.4);}"
                       ".nav a{color:#0984e3;}";
            break;
        case 2: // Dark theme
            theme_css = "body{background:#000000;}"
                       ".container{background:#1a1a1a;box-shadow:0 10px 30px rgba(0,0,0,0.8);}"
                       ".header{background:linear-gradient(45deg,#2c5aa0,#1e3d72);}"
                       ".content{color:#ddd;}"
                       "label{color:#ddd;}"
                       "input,textarea,select{background:#2a2a2a;border-color:#2c5aa0;color:#ddd;}"
                       "input:focus,textarea:focus,select:focus{border-color:#4a90e2;}"
                       ".btn{background:linear-gradient(45deg,#2c5aa0,#1e3d72);}"
                       ".btn:hover:not(:disabled){box-shadow:0 5px 15px rgba(44,90,160,0.4);}"
                       ".nav{background:#2a2a2a;border-color:#2c5aa0;}"
                       ".nav a{color:#4a90e2;}";
            break;
        default: // Default blue theme (case 0)
            theme_css = "body{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);}"
                       ".container{background:white;box-shadow:0 10px 30px rgba(0,0,0,0.2);}"
                       ".header{background:linear-gradient(45deg,#667eea,#764ba2);}"
                       "input:focus,textarea:focus{border-color:#667eea;}"
                       ".btn{background:linear-gradient(45deg,#667eea,#764ba2);}"
                       ".btn:hover:not(:disabled){box-shadow:0 5px 15px rgba(102,126,234,0.4);}"
                       ".nav a{color:#667eea;}";
            break;
    }
    
    return "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>" + title + "</title>"
           "<meta name='viewport' content='width=device-width, initial-scale=1'>"
           "<style>"
           "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;margin:0;padding:20px;min-height:100vh;}"
           ".container{max-width:600px;margin:0 auto;border-radius:12px;overflow:hidden;}"
           ".header{color:white;padding:20px;text-align:center;}"
           ".header h2{margin:0;font-size:24px;font-weight:300;}"
           ".content{padding:30px;}"
           ".form-group{margin-bottom:20px;}"
           "label{display:block;margin-bottom:8px;font-weight:500;color:#333;}"
           "input,textarea,select{width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;transition:border-color 0.3s;box-sizing:border-box;}"
           "textarea{resize:vertical;min-height:100px;font-family:inherit;}"
           ".char-counter{text-align:right;margin-top:5px;font-size:14px;color:#666;}"
           ".char-counter.error{color:#e74c3c;font-weight:bold;}"
           ".btn{width:100%;padding:15px;color:white;border:none;border-radius:8px;font-size:16px;font-weight:500;cursor:pointer;transition:all 0.3s;}"
           ".btn:disabled{background:#ccc;cursor:not-allowed;transform:none;box-shadow:none;}"
           ".status{margin:20px 0;padding:15px;border-radius:8px;display:none;}"
           ".status.success{background:#d4edda;color:#155724;border:1px solid #c3e6cb;}"
           ".status.error{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb;}"
           ".status.info{background:#d1ecf1;color:#0c5460;border:1px solid #bee5eb;}"
           ".nav{padding:15px;text-align:center;border-top:1px solid #e9ecef;}"
           ".nav a{text-decoration:none;margin:0 15px;font-weight:500;}"
           ".nav a:hover{text-decoration:underline;}"
           + theme_css +
           "</style></head><body>"
           "<div class='container'>"
           "<div class='header'><h2>" + title + "</h2></div>"
           "<div class='content'>";
}

String get_html_footer() {
    return "</div><div class='nav'><a href='/'>Message</a> | <a href='/configuration'>Configuration</a> | <a href='/status'>Status</a> | <a href='https://github.com/geekinsanemx/flex-fsk-tx' target='_blank'>GitHub</a></div>"
           "</div></body></html>";
}

void handle_root() {
    // Send response in chunks to avoid memory issues
    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html", "");
    
    // Send header
    String header = get_html_header("FLEX Message Transmitter");
    webServer.sendContent(header);
    
    // Send form
    String form = "<div id='statusDiv' class='status'></div>"
                  "<form id='messageForm'>"
                  "<div style='display:flex;gap:15px;'>"
                  "<div style='flex:2;'>"
                  "<label for='frequency'>Frequency (MHz):</label>"
                  "<input type='number' id='frequency' name='frequency' step='0.0001' value='" + String(config.default_frequency, 4) + "' required style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
                  "</div>"
                  "<div style='flex:1;'>"
                  "<label for='power'>TX Power (0-20 dBm):</label>"
                  "<input type='number' id='power' name='power' min='0' max='20' value='" + String((int)config.tx_power) + "' required style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
                  "</div>"
                  "</div>"
                  "<div style='display:flex;gap:15px;align-items:end;margin-top:20px;'>"
                  "<div style='flex:2;'>"
                  "<label for='capcode'>Capcode:</label>"
                  "<input type='number' id='capcode' name='capcode' value='" + String(config.default_capcode) + "' required style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
                  "</div>"
                  "<div style='flex:1;padding-bottom:14px;'>"
                  "<label style='display:flex;justify-content:space-between;align-items:center;font-weight:500;'>"
                  "Mail Drop <input type='checkbox' id='mail_drop' name='mail_drop' style='transform:scale(1.2);'>"
                  "</label>"
                  "</div>"
                  "</div>"
                  "<div class='form-group' style='margin-top:20px;'>"
                  "<label for='message'>Message:</label>"
                  "<textarea id='message' name='message' rows='4' maxlength='240' placeholder='Enter your FLEX message here...' required></textarea>"
                  "<div id='charCounter' class='char-counter'>0 / 240 characters</div>"
                  "</div>"
                  "<button type='submit' id='sendBtn' class='btn' style='margin-top:20px;'>Send Message</button>"
                  "</form>";
    webServer.sendContent(form);
    
    // Send JavaScript in separate chunk
    String js = "<script>"
                "window.onload=function(){"
                "var m=document.getElementById('message');"
                "var c=document.getElementById('charCounter');"
                "var b=document.getElementById('sendBtn');"
                "var s=document.getElementById('statusDiv');"
                "var f=document.getElementById('messageForm');"
                "function updateCounter(){"
                "var l=m.value.length;"
                "c.textContent=l+' / 240 characters';"
                "if(l>240){c.className='char-counter error';b.disabled=true;}"
                "else{c.className='char-counter';b.disabled=false;}"
                "}"
                "function showStatus(msg,type){"
                "s.textContent=msg;s.className='status '+type;s.style.display='block';"
                "}"
                "m.addEventListener('input',updateCounter);"
                "m.addEventListener('keyup',updateCounter);"
                "var p=document.getElementById('power');"
                "function validatePower(){"
                "var val=parseInt(p.value);"
                "if(val<0||val>20){p.style.borderColor='#e74c3c';p.style.backgroundColor='#fdf2f2';}"
                "else{p.style.borderColor='#e1e5e9';p.style.backgroundColor='';}"
                "}"
                "p.addEventListener('input',validatePower);"
                "p.addEventListener('keyup',validatePower);";
    webServer.sendContent(js);
    
    // Send remaining JavaScript
    String js2 = "f.addEventListener('submit',function(e){"
                 "e.preventDefault();"
                 "s.style.display='none';"
                 "var p=parseInt(document.getElementById('power').value);"
                 "if(p<0||p>20){showStatus('ERROR: TX Power must be between 0 and 20 dBm','error');return;}"
                 "b.disabled=true;b.textContent='Sending...';"
                 "var fd=new FormData(f);"
                 "var xhr=new XMLHttpRequest();"
                 "xhr.open('POST','/send',true);"
                 "xhr.onreadystatechange=function(){"
                 "if(xhr.readyState===4){"
                 "b.disabled=false;b.textContent='Send Message';updateCounter();"
                 "if(xhr.status===200){showStatus('SUCCESS: Message sent!','success');"
                 "setTimeout(function(){s.style.display='none';},5000);}"
                 "else{showStatus('ERROR: '+xhr.responseText,'error');}"
                 "}};"
                 "xhr.send(fd);"
                 "});"
                 "updateCounter();"
                 "};"
                 "</script>";
    webServer.sendContent(js2);
    
    // Send footer
    String footer = get_html_footer();
    webServer.sendContent(footer);
    webServer.sendContent("");
}

void handle_send_message() {
    reset_oled_timeout();
    
    // Add CORS headers
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    webServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
    if (!webServer.hasArg("frequency") || !webServer.hasArg("power") || !webServer.hasArg("capcode") || !webServer.hasArg("message")) {
        webServer.send(400, "text/plain", "Missing required parameters");
        return;
    }
    
    float frequency = webServer.arg("frequency").toFloat();
    int power = webServer.arg("power").toInt();
    uint64_t capcode;
    String message = webServer.arg("message");
    bool mail_drop = webServer.hasArg("mail_drop");  // Checkbox sends value only if checked
    
    if (str2uint64(&capcode, webServer.arg("capcode").c_str()) < 0) {
        webServer.send(400, "text/plain", "Invalid capcode format");
        return;
    }
    
    if (frequency < 400.0 || frequency > 1000.0) {
        webServer.send(400, "text/plain", "Frequency must be between 400.0 and 1000.0 MHz");
        return;
    }
    
    if (power < 0 || power > 20) {
        webServer.send(400, "text/plain", "TX Power must be between 0 and 20 dBm");
        return;
    }
    
    if (message.length() == 0 || message.length() > MAX_FLEX_MESSAGE_LENGTH) {
        webServer.send(400, "text/plain", "Message length must be between 1 and " + String(MAX_FLEX_MESSAGE_LENGTH) + " characters");
        return;
    }
    
    // Check if device is busy
    if (device_state != STATE_IDLE) {
        webServer.send(503, "text/plain", "Device is busy, please try again later");
        return;
    }
    
    // Set frequency
    if (abs(frequency - current_tx_frequency) > 0.0001) {
        int state = radio.setFrequency(frequency);
        if (state != RADIOLIB_ERR_NONE) {
            webServer.send(500, "text/plain", "Failed to set frequency");
            return;
        }
        current_tx_frequency = frequency;
    }
    
    // Set power
    if (abs(power - current_tx_power) > 0.1) {
        int state = radio.setOutputPower(power);
        if (state != RADIOLIB_ERR_NONE) {
            webServer.send(500, "text/plain", "Failed to set TX power");
            return;
        }
        current_tx_power = power;
    }
    
    // Encode and transmit FLEX message
    if (flex_encode_and_store(capcode, message.c_str(), mail_drop)) {
        device_state = STATE_TRANSMITTING;
        LED_ON();
        display_status();
        
        fifo_empty = true;
        current_tx_remaining_length = current_tx_total_length;
        radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);
        
        // Check if transmission started successfully
        if (radio_start_transmit_status != RADIOLIB_ERR_NONE) {
            // Failed to start transmission
            device_state = STATE_ERROR;
            LED_OFF();
            at_reset_state();
            display_status();
            webServer.send(500, "text/plain", "Failed to start transmission: " + String(radio_start_transmit_status));
            return;
        }
        
        // Transmission started successfully - send immediate response
        webServer.send(200, "text/plain", "Message transmission started successfully");
    } else {
        webServer.send(500, "text/plain", "Failed to encode FLEX message");
    }
}

void handle_configuration() {
    String html = get_html_header("Device Configuration");
    
    html += "<form action='/save_config' method='POST' id='configForm'>"
            
            "<h3 style='color:#667eea;border-bottom:2px solid #e1e5e9;padding-bottom:10px;'>Device Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='banner_message'>Banner Message (1-16 chars):</label>"
            "<input type='text' id='banner_message' name='banner_message' value='" + String(config.banner_message) + "' maxlength='16' minlength='1' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='theme'>Color Theme:</label>"
            "<select id='theme' name='theme' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "<option value='0'" + (config.theme == 0 ? " selected" : "") + ">Default (Blue)</option>"
            "<option value='1'" + (config.theme == 1 ? " selected" : "") + ">Light (Clear)</option>"
            "<option value='2'" + (config.theme == 2 ? " selected" : "") + ">Dark</option>"
            "</select>"
            "</div>"
            "</div>"
            
            "<h3 style='color:#667eea;border-bottom:2px solid #e1e5e9;padding-bottom:10px;margin-top:30px;'>WiFi Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='enable_wifi'>WiFi Enable:</label>"
            "<select id='enable_wifi' name='enable_wifi' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "<option value='1'" + (config.enable_wifi ? " selected" : "") + ">Enabled</option>"
            "<option value='0'" + (!config.enable_wifi ? " selected" : "") + ">Disabled</option>"
            "</select>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='use_dhcp'>Use DHCP:</label>"
            "<select id='use_dhcp' name='use_dhcp' onchange='toggleStaticIP()' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "<option value='1'" + (config.use_dhcp ? " selected" : "") + ">Yes (Automatic IP)</option>"
            "<option value='0'" + (!config.use_dhcp ? " selected" : "") + ">No (Static IP)</option>"
            "</select>"
            "</div>"
            "</div>"
            "<div style='display:flex;gap:15px;margin-top:15px;'>"
            "<div style='flex:1;'>"
            "<label for='wifi_ssid'>SSID:</label>"
            "<input type='text' id='wifi_ssid' name='wifi_ssid' value='" + String(config.wifi_ssid) + "' maxlength='32' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='wifi_password'>Password:</label>"
            "<input type='password' id='wifi_password' name='wifi_password' value='" + String(config.wifi_password) + "' maxlength='64' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "</div>"
            
            "<h3 style='color:#667eea;border-bottom:2px solid #e1e5e9;padding-bottom:10px;margin-top:30px;'>IP Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='static_ip'>IP Address:</label>"
            "<input type='text' id='static_ip' name='static_ip' value='" + 
            (wifi_connected && config.use_dhcp ? WiFi.localIP().toString() : 
             String(config.static_ip[0]) + "." + String(config.static_ip[1]) + "." + 
             String(config.static_ip[2]) + "." + String(config.static_ip[3])) + 
            "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='192.168.1.100' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='netmask'>Netmask:</label>"
            "<input type='text' id='netmask' name='netmask' value='" + 
            (wifi_connected && config.use_dhcp ? WiFi.subnetMask().toString() : 
             String(config.netmask[0]) + "." + String(config.netmask[1]) + "." + 
             String(config.netmask[2]) + "." + String(config.netmask[3])) + 
            "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='255.255.255.0' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "</div>"
            "<div style='display:flex;gap:15px;margin-top:15px;'>"
            "<div style='flex:1;'>"
            "<label for='gateway'>Gateway:</label>"
            "<input type='text' id='gateway' name='gateway' value='" + 
            (wifi_connected && config.use_dhcp ? WiFi.gatewayIP().toString() : 
             String(config.gateway[0]) + "." + String(config.gateway[1]) + "." + 
             String(config.gateway[2]) + "." + String(config.gateway[3])) + 
            "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='192.168.1.1' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='dns'>DNS Server:</label>"
            "<input type='text' id='dns' name='dns' value='" + 
            (wifi_connected && config.use_dhcp ? WiFi.dnsIP().toString() : 
             String(config.dns[0]) + "." + String(config.dns[1]) + "." + 
             String(config.dns[2]) + "." + String(config.dns[3])) + 
            "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='8.8.8.8' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "</div>"
            
            "<h3 style='color:#667eea;border-bottom:2px solid #e1e5e9;padding-bottom:10px;margin-top:30px;'>Default FLEX Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='default_frequency'>Default Frequency (MHz):</label>"
            "<input type='number' id='default_frequency' name='default_frequency' step='0.0001' value='" + String(config.default_frequency, 4) + "' min='400' max='1000' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='tx_power'>Default TX Power (0-20 dBm):</label>"
            "<input type='number' id='tx_power' name='tx_power' value='" + String((int)config.tx_power) + "' min='0' max='20' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "</div>"
            "<div style='display:flex;gap:15px;margin-top:15px;'>"
            "<div style='flex:1;'>"
            "<label for='default_capcode'>Default Capcode:</label>"
            "<input type='number' id='default_capcode' name='default_capcode' value='" + String(config.default_capcode) + "' min='1' max='4294967295' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "<div style='flex:1;'>"
            "</div>"
            "</div>"
            
            "<h3 style='color:#667eea;border-bottom:2px solid #e1e5e9;padding-bottom:10px;margin-top:30px;'>API Settings</h3>"
            "<div style='display:flex;gap:15px;'>"
            "<div style='flex:1;'>"
            "<label for='api_port'>API Listening Port:</label>"
            "<input type='number' id='api_port' name='api_port' value='" + String(config.api_port) + "' min='1024' max='65535' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "<div style='flex:1;'>"
            "</div>"
            "</div>"
            "<div style='display:flex;gap:15px;margin-top:15px;'>"
            "<div style='flex:1;'>"
            "<label for='api_username'>API Username:</label>"
            "<input type='text' id='api_username' name='api_username' value='" + String(config.api_username) + "' maxlength='32' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "<div style='flex:1;'>"
            "<label for='api_password'>API Password:</label>"
            "<input type='password' id='api_password' name='api_password' value='" + String(config.api_password) + "' maxlength='64' style='width:100%;padding:12px;border:2px solid #e1e5e9;border-radius:8px;font-size:16px;box-sizing:border-box;'>"
            "</div>"
            "</div>"
            
            
            "<button type='submit' class='btn' style='margin-top:30px;'>Save Configuration</button>"
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
            "      field.style.backgroundColor = '';"
            "      field.style.color = '';"
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
            "    p.style.borderColor = '#e1e5e9';"
            "    p.style.backgroundColor = '';"
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

void parse_ip_string(const String& ip_str, uint8_t ip[4]) {
    int parts[4];
    int part_count = sscanf(ip_str.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);
    
    if (part_count == 4) {
        for (int i = 0; i < 4; i++) {
            if (parts[i] >= 0 && parts[i] <= 255) {
                ip[i] = parts[i];
            }
        }
    }
}

void handle_device_status() {
    reset_oled_timeout();
    
    read_battery_status();
    
    String html = get_html_header("Device Status");
    
    html += "<h3 style='color:#667eea;border-bottom:2px solid #e1e5e9;padding-bottom:10px;'>Device Information</h3>";
    html += "<div style='display:flex;gap:30px;'>";
    html += "<div style='flex:1;'>";
    html += "<p><strong>Banner:</strong> " + String(config.banner_message) + "</p>";
    html += "<p><strong>Frequency:</strong> " + String(current_tx_frequency, 4) + " MHz</p>";
    html += "<p><strong>TX Power:</strong> " + String(current_tx_power, 1) + " dBm</p>";
    html += "<p><strong>Default Capcode:</strong> " + String(config.default_capcode) + "</p>";
    html += "<p><strong>Uptime:</strong> " + String(millis() / 1000 / 60) + " minutes</p>";
    html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    html += "</div>";
    html += "<div style='flex:1;'>";
    
    if (battery_present) {
        html += "<p><strong>Battery:</strong> " + String(battery_voltage, 2) + "V (" + String(battery_percentage) + "%)</p>";
    } else {
        html += "<p><strong>Power:</strong> External / USB</p>";
    }
    
    String theme_name = (config.theme == 0 ? "Default (Blue)" : (config.theme == 1 ? "Light" : "Dark"));
    html += "<p><strong>Theme:</strong> " + theme_name + "</p>";
    String wifi_status = config.enable_wifi ? "Enabled" : "Disabled";
    html += "<p><strong>WiFi Status:</strong> " + wifi_status + "</p>";
    html += "<p><strong>API Port:</strong> " + String(config.api_port) + "</p>";
    html += "<p><strong>Chip Model:</strong> " + String(ESP.getChipModel()) + "</p>";
    html += "<p><strong>CPU Frequency:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>";
    html += "</div>";
    html += "</div>";
    
    html += "<h3 style='color:#667eea;border-bottom:2px solid #e1e5e9;padding-bottom:10px;margin-top:30px;'>Network Information</h3>";
    
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
    
    html += "<h3 style='color:#667eea;border-bottom:2px solid #e1e5e9;padding-bottom:10px;margin-top:30px;'>Factory Reset</h3>";
    html += "<p style='color:#e74c3c;'><strong>Warning:</strong> This will reset all settings to defaults!</p>";
    html += "<form action='/factory_reset' method='POST' onsubmit='return confirm(\"Are you sure you want to reset all settings? This action cannot be undone!\");'>";
    html += "<div style='display:flex;align-items:center;gap:15px;'>";
    html += "<button type='submit' id='resetButton' disabled style='background:#ccc;color:#666;border:none;padding:15px 30px;border-radius:8px;font-size:16px;cursor:not-allowed;transition:all 0.3s ease;'>Factory Reset</button>";
    html += "<span style='color:#e74c3c;font-weight:bold;font-size:14px;'>I know what I'm doing!</span>";
    html += "<input type='checkbox' id='confirmReset' style='transform:scale(1.3);' onchange='toggleResetButton()'>";
    html += "</div>";
    html += "</form>";
    html += "<script>";
    html += "function toggleResetButton() {";
    html += "  var checkbox = document.getElementById('confirmReset');";
    html += "  var button = document.getElementById('resetButton');";
    html += "  if (checkbox.checked) {";
    html += "    button.disabled = false;";
    html += "    button.style.background = '#e74c3c';";
    html += "    button.style.color = 'white';";
    html += "    button.style.cursor = 'pointer';";
    html += "  } else {";
    html += "    button.disabled = true;";
    html += "    button.style.background = '#ccc';";
    html += "    button.style.color = '#666';";
    html += "    button.style.cursor = 'not-allowed';";
    html += "  }";
    html += "}";
    html += "</script>";
    
    html += get_html_footer();
    webServer.send(200, "text/html", html);
}

void handle_web_factory_reset() {
    reset_oled_timeout();
    
    String html = get_html_header("Factory Reset");
    html += "<p><strong>Factory reset initiated!</strong></p>";
    html += "<p>The device will restart with default settings in 3 seconds.</p>";
    html += get_html_footer();
    
    webServer.send(200, "text/html", html);
    
    delay(1000);
    
    Serial.println("Factory reset initiated via web interface");
    load_default_config();
    save_config();
    
    delay(2000);
    ESP.restart();
}

void handle_save_config() {
    reset_oled_timeout();
    
    // Update WiFi settings
    if (webServer.hasArg("wifi_ssid")) {
        String ssid = webServer.arg("wifi_ssid");
        ssid.toCharArray(config.wifi_ssid, sizeof(config.wifi_ssid));
    }
    
    if (webServer.hasArg("wifi_password")) {
        String password = webServer.arg("wifi_password");
        password.toCharArray(config.wifi_password, sizeof(config.wifi_password));
    }
    
    if (webServer.hasArg("use_dhcp")) {
        config.use_dhcp = (webServer.arg("use_dhcp") == "1");
    }
    
    // Update static IP settings
    if (webServer.hasArg("static_ip")) {
        parse_ip_string(webServer.arg("static_ip"), config.static_ip);
    }
    if (webServer.hasArg("netmask")) {
        parse_ip_string(webServer.arg("netmask"), config.netmask);
    }
    if (webServer.hasArg("gateway")) {
        parse_ip_string(webServer.arg("gateway"), config.gateway);
    }
    if (webServer.hasArg("dns")) {
        parse_ip_string(webServer.arg("dns"), config.dns);
    }
    
    // Update FLEX settings
    if (webServer.hasArg("default_frequency")) {
        config.default_frequency = webServer.arg("default_frequency").toFloat();
    }
    if (webServer.hasArg("tx_power")) {
        int power = webServer.arg("tx_power").toInt();
        if (power >= 0 && power <= 20) {
            config.tx_power = power;
        }
    }
    if (webServer.hasArg("default_capcode")) {
        config.default_capcode = webServer.arg("default_capcode").toInt();
    }
    
    // Update API settings
    if (webServer.hasArg("api_port")) {
        int port = webServer.arg("api_port").toInt();
        if (port >= 1024 && port <= 65535) {
            config.api_port = port;
        }
    }
    if (webServer.hasArg("api_username")) {
        String username = webServer.arg("api_username");
        username.toCharArray(config.api_username, sizeof(config.api_username));
    }
    if (webServer.hasArg("api_password")) {
        String password = webServer.arg("api_password");
        password.toCharArray(config.api_password, sizeof(config.api_password));
    }
    
    // Update theme
    if (webServer.hasArg("theme")) {
        int theme = webServer.arg("theme").toInt();
        if (theme >= 0 && theme <= 2) {
            config.theme = theme;
        }
    }
    
    // Update banner message
    if (webServer.hasArg("banner_message")) {
        String banner = webServer.arg("banner_message");
        if (banner.length() >= 1 && banner.length() <= 16) {
            banner.toCharArray(config.banner_message, sizeof(config.banner_message));
        }
    }
    
    // Update WiFi enable
    if (webServer.hasArg("enable_wifi")) {
        config.enable_wifi = (webServer.arg("enable_wifi") == "1");
    }
    
    // Save configuration to EEPROM
    if (save_config()) {
        String html = get_html_header("Configuration Saved");
        html += "<div class='container'>"
                "<p><strong>Configuration saved successfully!</strong></p>"
                "<p>The device will restart to apply new settings.</p>"
                "</div>";
        html += get_html_footer();
        
        webServer.send(200, "text/html", html);
        delay(2000);
        ESP.restart();
    } else {
        webServer.send(500, "text/html", get_html_header("Error") + 
                      "<p>Failed to save configuration</p>" + get_html_footer());
    }
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

void handle_api_message() {
    reset_oled_timeout();
    
    if (!authenticate_api_request()) {
        apiServer->sendHeader("WWW-Authenticate", "Basic realm=\"TTGO FLEX API\"");
        apiServer->send(401, "application/json", "{\"error\":\"Authentication required\"}");
        return;
    }
    
    if (apiServer->method() != HTTP_POST) {
        apiServer->send(405, "application/json", "{\"error\":\"Method not allowed\"}");
        return;
    }
    
    if (!apiServer->hasArg("plain")) {
        apiServer->send(400, "application/json", "{\"error\":\"No JSON payload\"}");
        return;
    }
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, apiServer->arg("plain"));
    
    if (error) {
        apiServer->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc.containsKey("capcode") || !doc.containsKey("frequency") || !doc.containsKey("message")) {
        apiServer->send(400, "application/json", "{\"error\":\"Missing required fields: capcode, frequency, message\"}");
        return;
    }
    
    uint64_t capcode = doc["capcode"];
    float frequency = doc["frequency"];
    String message = doc["message"].as<String>();
    int power = doc.containsKey("power") ? doc["power"] : config.tx_power;  // Optional power, use default if not provided
    bool mail_drop = doc.containsKey("mail_drop") ? doc["mail_drop"] : false;  // Optional mail drop flag
    
    // Convert frequency: if > 1000, assume Hz and convert to MHz
    if (frequency > 1000.0) {
        frequency = frequency / 1000000.0;  // Convert Hz to MHz
    }
    
    // Validate parameters (now in MHz)
    if (frequency < 400.0 || frequency > 1000.0) {
        apiServer->send(400, "application/json", "{\"error\":\"Frequency must be between 400.0-1000.0 MHz or 400000000-1000000000 Hz\"}");
        return;
    }
    
    if (power < 0 || power > 20) {
        apiServer->send(400, "application/json", "{\"error\":\"TX Power must be between 0 and 20 dBm\"}");
        return;
    }
    
    if (message.length() == 0 || message.length() > MAX_FLEX_MESSAGE_LENGTH) {
        apiServer->send(400, "application/json", "{\"error\":\"Message length must be between 1 and " + String(MAX_FLEX_MESSAGE_LENGTH) + " characters\"}");
        return;
    }
    
    // Check if device is busy
    if (device_state != STATE_IDLE) {
        apiServer->send(503, "application/json", "{\"error\":\"Device is busy\"}");
        return;
    }
    
    // Set frequency
    if (abs(frequency - current_tx_frequency) > 0.0001) {
        int state = radio.setFrequency(frequency);
        if (state != RADIOLIB_ERR_NONE) {
            apiServer->send(500, "application/json", "{\"error\":\"Failed to set frequency\"}");
            return;
        }
        current_tx_frequency = frequency;
    }
    
    // Set power
    if (abs(power - current_tx_power) > 0.1) {
        int state = radio.setOutputPower(power);
        if (state != RADIOLIB_ERR_NONE) {
            apiServer->send(500, "application/json", "{\"error\":\"Failed to set TX power\"}");
            return;
        }
        current_tx_power = power;
    }
    
    // Encode and transmit FLEX message
    if (flex_encode_and_store(capcode, message.c_str(), mail_drop)) {
        device_state = STATE_TRANSMITTING;
        LED_ON();
        display_status();
        
        fifo_empty = true;
        current_tx_remaining_length = current_tx_total_length;
        radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);
        
        // Check if transmission started successfully
        if (radio_start_transmit_status != RADIOLIB_ERR_NONE) {
            // Failed to start transmission
            device_state = STATE_ERROR;
            LED_OFF();
            at_reset_state();
            display_status();
            
            DynamicJsonDocument response(512);
            response["status"] = "error";
            response["message"] = "Failed to start transmission: " + String(radio_start_transmit_status);
            response["frequency"] = frequency;
            response["power"] = power;
            response["capcode"] = capcode;
            response["text"] = message;
            
            String response_str;
            serializeJson(response, response_str);
            apiServer->send(500, "application/json", response_str);
            return;
        }
        
        // Transmission started successfully - send immediate response
        DynamicJsonDocument response(512);
        response["status"] = "success";
        response["message"] = "Transmission started";
        response["frequency"] = frequency;
        response["power"] = power;
        response["capcode"] = capcode;
        response["text"] = message;
        
        String response_str;
        serializeJson(response, response_str);
        apiServer->send(200, "application/json", response_str);
    } else {
        apiServer->send(500, "application/json", "{\"error\":\"Failed to encode FLEX message\"}");
    }
}

// =============================================================================
// AT PROTOCOL FUNCTIONS (preserved from original)
// =============================================================================

void at_send_ok() {
    Serial.print("OK\r\n");
    Serial.flush();
    delay(AT_INTER_CMD_DELAY);
}

void at_send_error() {
    Serial.print("ERROR\r\n");
    Serial.flush();
    delay(AT_INTER_CMD_DELAY);
}

void at_send_response(const char* cmd, const char* value) {
    Serial.print("+");
    Serial.print(cmd);
    Serial.print(": ");
    Serial.print(value);
    Serial.print("\r\n");
    at_send_ok();
}

void at_send_response_float(const char* cmd, float value, int decimals) {
    Serial.print("+");
    Serial.print(cmd);
    Serial.print(": ");
    Serial.print(value, decimals);
    Serial.print("\r\n");
    at_send_ok();
}

void at_send_response_int(const char* cmd, int value) {
    Serial.print("+");
    Serial.print(cmd);
    Serial.print(": ");
    Serial.print(value);
    Serial.print("\r\n");
    at_send_ok();
}

void at_reset_state() {
    if (device_state != STATE_WIFI_CONNECTING && device_state != STATE_WIFI_AP_MODE) {
        device_state = STATE_IDLE;
    }
    current_tx_total_length = 0;
    current_tx_remaining_length = 0;
    expected_data_length = 0;
    data_receive_timeout = 0;
    state_timeout = 0;
    transmission_processing_complete = false;
    console_loop_enable = true;
    
    // Web responses are handled immediately, no tracking needed
    
    flex_capcode = 0;
    flex_message_pos = 0;
    flex_message_timeout = 0;
    flex_mail_drop = false;
    memset(flex_message_buffer, 0, sizeof(flex_message_buffer));
}

void at_flush_serial_buffers() {
    while (Serial.available()) {
        Serial.read();
        delay(1);
    }
    delay(50);
}

bool at_parse_command(char* cmd_buffer) {
    reset_oled_timeout();

    int len = strlen(cmd_buffer);
    while (len > 0 && (cmd_buffer[len-1] == '\r' || cmd_buffer[len-1] == '\n')) {
        cmd_buffer[--len] = '\0';
    }

    if (len == 0) {
        return true;
    }

    if (strncmp(cmd_buffer, "AT", 2) != 0) {
        return false;
    }

    if (strcmp(cmd_buffer, "AT") == 0) {
        at_reset_state();
        display_status();
        at_send_ok();
        return true;
    }

    if (strncmp(cmd_buffer, "AT+", 3) != 0) {
        return false;
    }

    char* cmd_start = cmd_buffer + 3;
    char* equals_pos = strchr(cmd_start, '=');
    char* query_pos = strchr(cmd_start, '?');

    char cmd_name[32];
    int cmd_name_len;

    if (equals_pos != NULL) {
        cmd_name_len = equals_pos - cmd_start;
    } else if (query_pos != NULL) {
        cmd_name_len = query_pos - cmd_start;
    } else {
        cmd_name_len = strlen(cmd_start);
    }

    if (cmd_name_len >= sizeof(cmd_name) || cmd_name_len <= 0) {
        return false;
    }

    strncpy(cmd_name, cmd_start, cmd_name_len);
    cmd_name[cmd_name_len] = '\0';

    if (device_state == STATE_WAITING_FOR_DATA || device_state == STATE_WAITING_FOR_MSG) {
        if (strcmp(cmd_name, "STATUS") != 0 && strcmp(cmd_buffer, "AT") != 0) {
            at_send_error();
            return true;
        }
    }

    if (strcmp(cmd_name, "FREQ") == 0) {
        if (query_pos != NULL) {
            at_send_response_float("FREQ", current_tx_frequency, 4);
        } else if (equals_pos != NULL) {
            float freq = atof(equals_pos + 1);
            if (freq < 400.0 || freq > 1000.0) {
                at_send_error();
                return true;
            }

            int state = radio.setFrequency(freq);
            if (state != RADIOLIB_ERR_NONE) {
                at_send_error();
                return true;
            }

            current_tx_frequency = freq;
            display_status();
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "POWER") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("POWER", (int)current_tx_power);
        } else if (equals_pos != NULL) {
            int power = atoi(equals_pos + 1);
            if (power < -9 || power > 20) {
                at_send_error();
                return true;
            }

            int state = radio.setOutputPower(power);
            if (state != RADIOLIB_ERR_NONE) {
                at_send_error();
                return true;
            }

            current_tx_power = power;
            display_status();
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "SEND") == 0) {
        if (equals_pos != NULL) {
            int bytes_to_read = atoi(equals_pos + 1);

            if (bytes_to_read <= 0 || bytes_to_read > 2048) {
                at_send_error();
                return true;
            }

            at_reset_state();

            device_state = STATE_WAITING_FOR_DATA;
            expected_data_length = bytes_to_read;
            current_tx_total_length = 0;
            data_receive_timeout = millis() + 15000;
            console_loop_enable = false;

            at_flush_serial_buffers();

            Serial.print("+SEND: READY\r\n");
            Serial.flush();

            display_status();
        }
        return true;
    }

    else if (strcmp(cmd_name, "MSG") == 0) {
        if (equals_pos != NULL) {
            uint64_t capcode;
            if (str2uint64(&capcode, equals_pos + 1) < 0) {
                at_send_error();
                return true;
            }
            
            at_reset_state();
            
            device_state = STATE_WAITING_FOR_MSG;
            flex_capcode = capcode;
            flex_message_pos = 0;
            flex_message_timeout = millis() + FLEX_MSG_TIMEOUT;
            console_loop_enable = false;
            memset(flex_message_buffer, 0, sizeof(flex_message_buffer));
            
            at_flush_serial_buffers();
            
            Serial.print("+MSG: READY\r\n");
            Serial.flush();
            
            display_status();
        }
        return true;
    }

    else if (strcmp(cmd_name, "MAILDROP") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("MAILDROP", flex_mail_drop ? 1 : 0);
        } else if (equals_pos != NULL) {
            int mail_drop = atoi(equals_pos + 1);
            flex_mail_drop = (mail_drop != 0);
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "STATUS") == 0) {
        const char* status_str;
        switch (device_state) {
            case STATE_IDLE:
                status_str = "READY";
                break;
            case STATE_WAITING_FOR_DATA:
                status_str = "WAITING_DATA";
                break;
            case STATE_WAITING_FOR_MSG:
                status_str = "WAITING_MSG";
                break;
            case STATE_TRANSMITTING:
                status_str = "TRANSMITTING";
                break;
            case STATE_ERROR:
                status_str = "ERROR";
                break;
            case STATE_WIFI_CONNECTING:
                status_str = "WIFI_CONNECTING";
                break;
            case STATE_WIFI_AP_MODE:
                status_str = "WIFI_AP_MODE";
                break;
            default:
                status_str = "UNKNOWN";
                break;
        }
        at_send_response("STATUS", status_str);
        return true;
    }

    else if (strcmp(cmd_name, "ABORT") == 0) {
        radio.standby();
        LED_OFF();
        at_reset_state();
        display_status();
        at_send_ok();
        return true;
    }

    else if (strcmp(cmd_name, "RESET") == 0) {
        at_send_ok();
        delay(100);
        ESP.restart();
        return true;
    }

    // New WiFi-related AT commands
    else if (strcmp(cmd_name, "WIFI") == 0) {
        if (query_pos != NULL) {
            String status = "DISCONNECTED";
            if (wifi_connected) {
                status = "CONNECTED," + WiFi.localIP().toString();
            } else if (ap_mode_active) {
                status = "AP_MODE," + WiFi.softAPIP().toString();
            }
            at_send_response("WIFI", status.c_str());
        }
        return true;
    }

    else if (strcmp(cmd_name, "WIFISSID") == 0) {
        if (query_pos != NULL) {
            at_send_response("WIFISSID", config.wifi_ssid);
        } else if (equals_pos != NULL) {
            String ssid = String(equals_pos + 1);
            if (ssid.length() <= 32) {
                ssid.toCharArray(config.wifi_ssid, sizeof(config.wifi_ssid));
                save_config();
                at_send_ok();
            } else {
                at_send_error();
            }
        }
        return true;
    }

    else if (strcmp(cmd_name, "WIFIPASS") == 0) {
        if (query_pos != NULL) {
            at_send_response("WIFIPASS", "****");  // Don't show password
        } else if (equals_pos != NULL) {
            String password = String(equals_pos + 1);
            if (password.length() <= 64) {
                password.toCharArray(config.wifi_password, sizeof(config.wifi_password));
                save_config();
                at_send_ok();
            } else {
                at_send_error();
            }
        }
        return true;
    }

    else if (strcmp(cmd_name, "WIFIDHCP") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("WIFIDHCP", config.use_dhcp ? 1 : 0);
        } else if (equals_pos != NULL) {
            int dhcp = atoi(equals_pos + 1);
            config.use_dhcp = (dhcp != 0);
            save_config();
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "WIFIIP") == 0) {
        if (query_pos != NULL) {
            String ip_str = String(config.static_ip[0]) + "." + String(config.static_ip[1]) + "." + 
                           String(config.static_ip[2]) + "." + String(config.static_ip[3]);
            at_send_response("WIFIIP", ip_str.c_str());
        } else if (equals_pos != NULL) {
            String ip_str = String(equals_pos + 1);
            parse_ip_string(ip_str, config.static_ip);
            save_config();
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "WIFIMASK") == 0) {
        if (query_pos != NULL) {
            String mask_str = String(config.netmask[0]) + "." + String(config.netmask[1]) + "." + 
                             String(config.netmask[2]) + "." + String(config.netmask[3]);
            at_send_response("WIFIMASK", mask_str.c_str());
        } else if (equals_pos != NULL) {
            String mask_str = String(equals_pos + 1);
            parse_ip_string(mask_str, config.netmask);
            save_config();
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "WIFIGW") == 0) {
        if (query_pos != NULL) {
            String gw_str = String(config.gateway[0]) + "." + String(config.gateway[1]) + "." + 
                           String(config.gateway[2]) + "." + String(config.gateway[3]);
            at_send_response("WIFIGW", gw_str.c_str());
        } else if (equals_pos != NULL) {
            String gw_str = String(equals_pos + 1);
            parse_ip_string(gw_str, config.gateway);
            save_config();
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "WIFICONNECT") == 0) {
        if (strlen(config.wifi_ssid) > 0) {
            // Disconnect current WiFi if connected
            if (wifi_connected) {
                WiFi.disconnect();
                wifi_connected = false;
            }
            // Restart WiFi connection with new settings
            wifi_connect();
            at_send_ok();
        } else {
            at_send_error();
        }
        return true;
    }

    else if (strcmp(cmd_name, "BANNER") == 0) {
        if (query_pos != NULL) {
            at_send_response("BANNER", config.banner_message);
        } else if (equals_pos != NULL) {
            String banner = String(equals_pos + 1);
            if (banner.length() >= 1 && banner.length() <= 16) {
                banner.toCharArray(config.banner_message, sizeof(config.banner_message));
                save_config();
                display_status();
                at_send_ok();
            } else {
                at_send_error();
            }
        }
        return true;
    }

    else if (strcmp(cmd_name, "WIFIENABLE") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("WIFIENABLE", config.enable_wifi ? 1 : 0);
        } else if (equals_pos != NULL) {
            int enable = atoi(equals_pos + 1);
            config.enable_wifi = (enable != 0);
            save_config();
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "BATTERY") == 0) {
        read_battery_status();
        if (battery_present) {
            String battery_info = String(battery_voltage, 2) + "V," + String(battery_percentage) + "%";
            at_send_response("BATTERY", battery_info.c_str());
        } else {
            at_send_response("BATTERY", "NOT_PRESENT");
        }
        return true;
    }

    else if (strcmp(cmd_name, "FACTORYRESET") == 0) {
        at_send_ok();
        delay(100);
        
        Serial.println("Factory reset initiated via AT command");
        load_default_config();
        save_config();
        delay(1000);
        ESP.restart();
        return true;
    }

    return false;
}

void at_handle_binary_data() {
    reset_oled_timeout();

    if (device_state != STATE_WAITING_FOR_DATA) {
        return;
    }

    if (millis() > data_receive_timeout) {
        at_reset_state();
        display_status();
        at_send_error();
        return;
    }

    while (Serial.available() && current_tx_total_length < expected_data_length) {
        tx_data_buffer[current_tx_total_length++] = Serial.read();
        data_receive_timeout = millis() + 5000;
    }

    if (current_tx_total_length >= expected_data_length) {
        device_state = STATE_TRANSMITTING;
        LED_ON();
        display_status();

        fifo_empty = true;
        current_tx_remaining_length = current_tx_total_length;
        radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);
    }
}

void at_handle_flex_message() {
    reset_oled_timeout();

    if (device_state != STATE_WAITING_FOR_MSG) {
        return;
    }
    
    if (millis() > flex_message_timeout) {
        at_reset_state();
        display_status();
        at_send_error();
        return;
    }
    
    while (Serial.available() && flex_message_pos < MAX_FLEX_MESSAGE_LENGTH) {
        char c = Serial.read();
        
        if (c == '\r' || c == '\n') {
            flex_message_buffer[flex_message_pos] = '\0';
            
            if (flex_encode_and_store(flex_capcode, flex_message_buffer, flex_mail_drop)) {
                device_state = STATE_TRANSMITTING;
                LED_ON();
                display_status();

                fifo_empty = true;
                radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);
            } else {
                device_state = STATE_ERROR;
                at_send_error();
                at_reset_state();
                display_status();
            }
            return;
        }
        
        if (c >= 32 && c <= 126) {
            flex_message_buffer[flex_message_pos++] = c;
            flex_message_timeout = millis() + FLEX_MSG_TIMEOUT;
        }
    }
    
    if (flex_message_pos >= MAX_FLEX_MESSAGE_LENGTH) {
        flex_message_buffer[MAX_FLEX_MESSAGE_LENGTH] = '\0';
        
        if (flex_encode_and_store(flex_capcode, flex_message_buffer, flex_mail_drop)) {
            device_state = STATE_TRANSMITTING;
            LED_ON();
            display_status();

            fifo_empty = true;
            radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);
        } else {
            device_state = STATE_ERROR;
            at_send_error();
            at_reset_state();
            display_status();
        }
    }
}

void at_process_serial() {
    if (device_state == STATE_WAITING_FOR_DATA) {
        at_handle_binary_data();
        return;
    }
    
    if (device_state == STATE_WAITING_FOR_MSG) {
        at_handle_flex_message();
        return;
    }

    while (Serial.available()) {
        char c = Serial.read();

        if (at_buffer_pos >= AT_BUFFER_SIZE - 1) {
            at_buffer_pos = 0;
            at_send_error();
            continue;
        }

        at_buffer[at_buffer_pos++] = c;

        if (c == '\n' || (c == '\r' && at_buffer_pos > 1)) {
            at_buffer[at_buffer_pos] = '\0';
            at_command_ready = true;
            break;
        }
    }

    if (at_command_ready) {
        if (!at_parse_command(at_buffer)) {
            at_send_error();
        }

        at_buffer_pos = 0;
        at_command_ready = false;
    }
}

// =============================================================================
// PANIC FUNCTION
// =============================================================================

void panic() {
    display_panic();
    Serial.print("ERROR\r\n");
    while (true) {
        delay(100000);
    }
}

// =============================================================================
// INTERRUPT SERVICE ROUTINE
// =============================================================================

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void on_interrupt_fifo_has_space() {
    fifo_empty = true;
}

// =============================================================================
// ARDUINO SETUP AND LOOP
// =============================================================================

void setup() {
    Serial.begin(TTGO_SERIAL_BAUD);

    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Load configuration
    load_config();
    
    // Initialize with configured defaults
    current_tx_frequency = config.default_frequency;
    current_tx_power = config.tx_power;

    display_setup();
    display_status();

    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    LED_OFF();
    
    // Initialize factory reset button
    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
    
    // Initialize battery monitoring
    read_battery_status();
    
    // Initialize heartbeat timer
    last_heartbeat = millis();

    // Initialize radio module
    int radio_init_state = radio.beginFSK(current_tx_frequency,
                                         TX_BITRATE,
                                         TX_DEVIATION,
                                         RX_BANDWIDTH,
                                         current_tx_power,
                                         PREAMBLE_LENGTH,
                                         false);

    if (radio_init_state != RADIOLIB_ERR_NONE) {
        panic();
    }

    radio.setFifoEmptyAction(on_interrupt_fifo_has_space);

    int packet_mode_state = radio.fixedPacketLengthMode(0);
    if (packet_mode_state != RADIOLIB_ERR_NONE) {
        panic();
    }

    // Initialize AT state
    at_reset_state();

    // Initialize activity timer
    reset_oled_timeout();

    // Generate unique AP SSID based on MAC address
    ap_ssid = generate_ap_ssid();
    
    // Initialize WiFi if enabled
    if (config.enable_wifi) {
        wifi_connect();
        
        // Setup web server routes
        webServer.on("/", handle_root);
        webServer.on("/message", handle_root);
        webServer.on("/send", HTTP_POST, handle_send_message);
        webServer.on("/test", []() { webServer.send(200, "text/plain", "Test OK"); });
        webServer.on("/configuration", handle_configuration);
        webServer.on("/save_config", HTTP_POST, handle_save_config);
        webServer.on("/status", handle_device_status);
        webServer.on("/factory_reset", HTTP_POST, handle_web_factory_reset);
        
        // Initialize API server with configured port
        apiServer = new WebServer(config.api_port);
        apiServer->on("/", HTTP_POST, handle_api_message);
        
        webServer.begin();
        apiServer->begin();
        
        Serial.println("Web servers started");
    }

    // Send ready message
    Serial.print("AT READY\r\n");
    if (config.enable_wifi) {
        Serial.print("WIFI ENABLED\r\n");
    }
    Serial.flush();
}

void loop() {
    // Handle AT commands
    at_process_serial();

    // Handle WiFi connection
    if (config.enable_wifi) {
        check_wifi_connection();
        
        // Handle web server requests
        if (wifi_connected || ap_mode_active) {
            webServer.handleClient();
            if (apiServer) {
                apiServer->handleClient();
            }
        }
    }

    // Handle OLED timeout
    if (oled_active && (millis() - last_activity_time > OLED_TIMEOUT_MS)) {
        display_turn_off();
    }
    
    // Handle LED heartbeat (only when not transmitting)
    if (device_state != STATE_TRANSMITTING) {
        handle_led_heartbeat();
    }
    
    // Handle factory reset button
    handle_factory_reset();
    
    // Update battery status every 30 seconds
    static unsigned long last_battery_update = 0;
    if (millis() - last_battery_update > 30000) {
        read_battery_status();
        last_battery_update = millis();
    }

    // Handle radio transmission
    if (fifo_empty && current_tx_remaining_length > 0) {
        fifo_empty = false;
        transmission_processing_complete = radio.fifoAdd(tx_data_buffer, current_tx_total_length, &current_tx_remaining_length);
    }

    if (transmission_processing_complete) {
        transmission_processing_complete = false;

        if (radio_start_transmit_status == RADIOLIB_ERR_NONE) {
            at_send_ok();
        } else {
            device_state = STATE_ERROR;
            at_send_error();
        }

        radio.standby();
        LED_OFF();

        at_reset_state();
        display_status();
    }

    delay(1);
}
