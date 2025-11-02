/*
 * pocsag_fsk_tx_v2: FLEX + POCSAG transmitter with AT command protocol
 * ESP32 LoRa32 dual-protocol transmitter with AT command protocol
 *
 * Features:
 * - AT command protocol for serial communication
 * - FIFO-based efficient transmission
 * - FLEX message encoding on device
 * - POCSAG message encoding on device (512/1200/2400 baud)
 * - Protocol selection via AT+MSG command
 * - OLED display with banner and status
 * - LED transmission indicator
 * - 5-minute display timeout for power saving
 * - Unified support for TTGO and Heltec boards via boards.h
 *
 * AT Commands:
 * - AT                    : Basic AT command
 * - AT+FREQ=xxx / AT+FREQ?: Set/query frequency (400-1000 MHz)
 * - AT+POWER=xxx / AT+POWER?: Set/query power (-9 to 22 dBm)
 * - AT+SEND=xxx           : Send xxx bytes (followed by binary data)
 * - AT+MSG=capcode        : Send FLEX message (followed by text message)
 * - AT+MSG=FLEX,capcode   : Send FLEX message (explicit protocol)
 * - AT+MSG=POCSAG,capcode : Send POCSAG message (explicit protocol)
 * - AT+POCSAGRATE=xxx / AT+POCSAGRATE?: Set/query POCSAG baud rate (512/1200/2400)
 * - AT+MAILDROP=x / AT+MAILDROP?: Set/query mail drop flag (0/1, FLEX only)
 * - AT+STATUS?            : Query device status
 * - AT+ABORT              : Abort current operation
 * - AT+RESET              : Reset device
 *
 * This code is released into the public domain.
 */

#define TTGO_LORA32_V21

#include <RadioLib.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>

#include "boards/boards.h"

#include "boards/boards.h"
#include "tinyflex/tinyflex.h"
#include "pocsag/pocsag.h"

// =============================================================================
// CONSTANTS AND DEFAULTS
// =============================================================================

#define SERIAL_BAUD 115200

// Radio defaults for FLEX
#define TX_FREQ_DEFAULT 931.9375
#define TX_BITRATE_FLEX 1.6
#define TX_DEVIATION_FLEX 8.0
#define TX_POWER_DEFAULT 2
#define RX_BANDWIDTH 10.4
#define PREAMBLE_LENGTH 0

// POCSAG defaults
#define DEFAULT_POCSAG_RATE 1200  // 512, 1200, or 2400
#define TX_DEVIATION_POCSAG 4.5

// AT Protocol constants
#define AT_BUFFER_SIZE 512
#define AT_CMD_TIMEOUT 5000
#define AT_MAX_RETRIES 3
#define AT_INTER_CMD_DELAY 100

// Display constants
#define OLED_TIMEOUT_MS (5 * 60 * 1000) // 5 minutes in milliseconds
#define BANNER "pocsag-fsk-tx"
#define FONT_BANNER u8g2_font_10x20_tr  // Larger font for banner
#define BANNER_HEIGHT 16                // Reduced height to move everything up
#define BANNER_MARGIN 2                 // Reduced margin to save space
#define FONT_DEFAULT u8g2_font_7x13_tr
#define FONT_BOLD u8g2_font_7x13B_tr
#define FONT_LINE_HEIGHT 14
#define FONT_TAB_START 42

// Message constants
#define FLEX_MSG_TIMEOUT 30000  // 30 seconds timeout for AT+MSG
#define MAX_FLEX_MESSAGE_LENGTH 248
#define MAX_POCSAG_MESSAGE_LENGTH 240

// =============================================================================
// PROTOCOL SELECTION
// =============================================================================

typedef enum {
    PROTO_FLEX,
    PROTO_POCSAG
} protocol_t;

// =============================================================================
// BUILT-IN LED CONTROL
// =============================================================================

#define LED_OFF()  digitalWrite(LED_PIN, LOW)
#define LED_ON()   digitalWrite(LED_PIN, HIGH)

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

SX1276 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, LORA_RST_PIN, LORA_GPIO_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// Device state management
typedef enum {
    STATE_IDLE,
    STATE_WAITING_FOR_DATA,
    STATE_WAITING_FOR_MSG,     // For AT+MSG
    STATE_TRANSMITTING,
    STATE_ERROR
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
volatile bool console_loop_enable = true;                // Flag to enable/disable console input loop
volatile bool fifo_empty = false;                        // Flag set by ISR when FIFO has space for more data
volatile bool transmission_processing_complete = false;  // Flag set by fifoAdd when all data of the current transmission is sent

// Transmission data buffer and state variables
uint8_t tx_data_buffer[4096] = {0};                      // Buffer to hold the entire message data (increased for POCSAG)
int     current_tx_total_length = 0;                     // Total length of the current message being transmitted
int     current_tx_remaining_length = 0;                 // Number of bytes remaining to be loaded into FIFO for the current message
int16_t radio_start_transmit_status = RADIOLIB_ERR_NONE; // Stores the result of the radio.startTransmit() call
int     expected_data_length = 0;                        // Expected data length for SEND command
unsigned long data_receive_timeout = 0;                  // Timeout for binary data reception

// Message variables
protocol_t current_protocol = PROTO_FLEX;  // Current protocol for message
uint64_t message_capcode = 0;
char message_buffer[MAX_FLEX_MESSAGE_LENGTH + 1] = {0};
int message_pos = 0;
unsigned long message_timeout = 0;
bool flex_mail_drop = false;

// POCSAG configuration
int current_pocsag_rate = DEFAULT_POCSAG_RATE;

// Radio operation parameters
float current_tx_frequency = TX_FREQ_DEFAULT;            // Current transmission frequency
float current_tx_power = TX_POWER_DEFAULT;               // Current transmission power

// =============================================================================
// DISPLAY FUNCTIONS
// =============================================================================

void display_turn_off() {
    if (oled_active) {
        display.setPowerSave(1); // Turn off display
        oled_active = false;
    }
}

void display_turn_on() {
    if (!oled_active) {
        display.setPowerSave(0); // Turn on display
        oled_active = true;
    }
}

void reset_oled_timeout() {
    last_activity_time = millis();
    display_turn_on();
}

void display_panic()
{
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

void display_setup()
{
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
}

void display_status()
{
    if (!oled_active) return; // Don't update if display is off

    String tx_power_str = String(current_tx_power, 1) + " dBm";
    String tx_frequency_str = String(current_tx_frequency, 4) + " MHz";
    String status_str;

    // Determine status based on device state
    switch (device_state) {
        case STATE_IDLE:
            status_str = "Ready";
            break;
        case STATE_WAITING_FOR_DATA:
            status_str = "Receiving Data...";
            break;
        case STATE_WAITING_FOR_MSG:
            status_str = (current_protocol == PROTO_FLEX) ? "RX FLEX Msg..." : "RX POCSAG Msg...";
            break;
        case STATE_TRANSMITTING:
            status_str = "Transmitting...";
            break;
        case STATE_ERROR:
            status_str = "Error";
            break;
        default:
            status_str = "Unknown";
            break;
    }

    display.clearBuffer();

    // Draw banner at top - positioned properly with font ascent
    display.setFont(FONT_BANNER);
    int banner_width = display.getStrWidth(BANNER);
    int banner_x = (display.getWidth() - banner_width) / 2;
    display.drawStr(banner_x, BANNER_HEIGHT, BANNER);

    // Calculate starting position for status info (banner height + margin)
    int status_start_y = BANNER_HEIGHT + BANNER_MARGIN + FONT_LINE_HEIGHT;

    // Draw status info below banner
    display.setFont(FONT_DEFAULT);

    display.drawStr(0, status_start_y, "State:");
    display.drawStr(FONT_TAB_START, status_start_y, status_str.c_str());

    status_start_y += FONT_LINE_HEIGHT;
    display.drawStr(0, status_start_y, "Pwr:");
    display.drawStr(FONT_TAB_START, status_start_y, tx_power_str.c_str());

    status_start_y += FONT_LINE_HEIGHT;
    display.drawStr(0, status_start_y, "Freq:");
    display.drawStr(FONT_TAB_START, status_start_y, tx_frequency_str.c_str());

    display.sendBuffer();
}

// =============================================================================
// RADIO CONFIGURATION FUNCTIONS
// =============================================================================

void apply_flex_radio_settings() {
    radio.setBitRate(TX_BITRATE_FLEX);
    radio.setFrequencyDeviation(TX_DEVIATION_FLEX);
    radio.setDataShaping(RADIOLIB_SHAPING_0_3);  // Gaussian BT=0.3 for FLEX
}

void apply_pocsag_radio_settings() {
    float pocsag_bitrate = (float)current_pocsag_rate / 1000.0;  // Convert to kbps
    radio.setBitRate(pocsag_bitrate);
    radio.setFrequencyDeviation(TX_DEVIATION_POCSAG);
    radio.setDataShaping(RADIOLIB_SHAPING_NONE);  // No shaping for POCSAG
}

// =============================================================================
// ENCODING FUNCTIONS
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

/**
 * @brief Encode FLEX message and store in tx_data_buffer
 */
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

    // Copy to transmission buffer
    memcpy(tx_data_buffer, flex_buffer, encoded_size);
    current_tx_total_length = encoded_size;
    current_tx_remaining_length = encoded_size;

    return true;
}

/**
 * @brief Encode POCSAG message and store in tx_data_buffer
 */
bool pocsag_encode_and_store(uint32_t capcode, const char *message) {
    // Encode POCSAG message to 32-bit words (includes preamble)
    uint32_t pocsag_words[POCSAG_MAX_MESSAGE_CODEWORDS];
    size_t word_count = pocsag_encode_message(capcode, message, pocsag_words);

    if (word_count == 0 || word_count > POCSAG_MAX_MESSAGE_CODEWORDS) {
        return false;
    }

    // Convert words to bytes (MSB first)
    // Note: preamble is already included in pocsag_words by pocsag_encode_message()
    size_t message_size = pocsag_words_to_bytes(pocsag_words, word_count, tx_data_buffer);

    current_tx_total_length = message_size;
    current_tx_remaining_length = current_tx_total_length;

    return true;
}

// =============================================================================
// AT PROTOCOL FUNCTIONS
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
    device_state = STATE_IDLE;
    current_tx_total_length = 0;
    current_tx_remaining_length = 0;
    expected_data_length = 0;
    data_receive_timeout = 0;
    state_timeout = 0;
    transmission_processing_complete = false;
    console_loop_enable = true;

    // Reset message state
    current_protocol = PROTO_FLEX;  // Default to FLEX
    message_capcode = 0;
    message_pos = 0;
    message_timeout = 0;
    flex_mail_drop = false;
    memset(message_buffer, 0, sizeof(message_buffer));
}

void at_flush_serial_buffers() {
    // Clear any pending data
    while (Serial.available()) {
        Serial.read();
        delay(1);
    }
    delay(50); // Give some time for any remaining data
}

bool at_parse_command(char* cmd_buffer) {
    reset_oled_timeout();

    // Remove \r\n from end
    int len = strlen(cmd_buffer);
    while (len > 0 && (cmd_buffer[len-1] == '\r' || cmd_buffer[len-1] == '\n')) {
        cmd_buffer[--len] = '\0';
    }

    // Skip empty commands
    if (len == 0) {
        return true;
    }

    // Check for AT prefix
    if (strncmp(cmd_buffer, "AT", 2) != 0) {
        return false;
    }

    // Handle basic AT command
    if (strcmp(cmd_buffer, "AT") == 0) {
        // Reset state on basic AT command
        at_reset_state();
        display_status();
        at_send_ok();
        return true;
    }

    // Check for AT+ commands
    if (strncmp(cmd_buffer, "AT+", 3) != 0) {
        return false;
    }

    char* cmd_start = cmd_buffer + 3;
    char* equals_pos = strchr(cmd_start, '=');
    char* query_pos = strchr(cmd_start, '?');

    // Parse command name
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

    // Only allow certain commands when waiting for data or message
    if (device_state == STATE_WAITING_FOR_DATA || device_state == STATE_WAITING_FOR_MSG) {
        if (strcmp(cmd_name, "STATUS") != 0 && strcmp(cmd_buffer, "AT") != 0) {
            at_send_error();
            return true;
        }
    }

    // Handle different commands
    if (strcmp(cmd_name, "FREQ") == 0) {
        if (query_pos != NULL) {
            // Query frequency
            at_send_response_float("FREQ", current_tx_frequency, 4);
        } else if (equals_pos != NULL) {
            // Set frequency
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
            // Query power
            at_send_response_int("POWER", (int)current_tx_power);
        } else if (equals_pos != NULL) {
            // Set power
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

    else if (strcmp(cmd_name, "POCSAGRATE") == 0) {
        if (query_pos != NULL) {
            // Query POCSAG rate
            at_send_response_int("POCSAGRATE", current_pocsag_rate);
        } else if (equals_pos != NULL) {
            // Set POCSAG rate
            int rate = atoi(equals_pos + 1);
            if (rate != 512 && rate != 1200 && rate != 2400) {
                at_send_error();
                return true;
            }
            current_pocsag_rate = rate;
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "SEND") == 0) {
        if (equals_pos != NULL) {
            int bytes_to_read = atoi(equals_pos + 1);

            if (bytes_to_read <= 0 || bytes_to_read > 4096) {
                at_send_error();
                return true;
            }

            // Reset transmission state
            at_reset_state();

            // Set up for receiving binary data
            device_state = STATE_WAITING_FOR_DATA;
            expected_data_length = bytes_to_read;
            current_tx_total_length = 0;
            data_receive_timeout = millis() + 15000; // 15 second timeout
            console_loop_enable = false; // Disable console loop during data reception

            // Clear any pending serial data
            at_flush_serial_buffers();

            // Send ready response
            Serial.print("+SEND: READY\r\n");
            Serial.flush();

            display_status();
        }
        return true;
    }

    else if (strcmp(cmd_name, "MSG") == 0) {
        if (equals_pos != NULL) {
            char* params = equals_pos + 1;
            char* comma_pos = strchr(params, ',');

            // Parse protocol and capcode BEFORE reset
            protocol_t protocol_to_use = PROTO_FLEX;
            uint64_t capcode_to_use = 0;

            if (comma_pos != NULL) {
                // Format: AT+MSG=PROTOCOL,capcode
                *comma_pos = '\0';  // Null-terminate protocol string
                char* protocol_str = params;
                char* capcode_str = comma_pos + 1;

                // Determine protocol
                if (strcasecmp(protocol_str, "FLEX") == 0) {
                    protocol_to_use = PROTO_FLEX;
                } else if (strcasecmp(protocol_str, "POCSAG") == 0) {
                    protocol_to_use = PROTO_POCSAG;
                } else {
                    at_send_error();
                    return true;
                }

                // Parse capcode
                if (str2uint64(&capcode_to_use, capcode_str) < 0) {
                    at_send_error();
                    return true;
                }

            } else {
                // Format: AT+MSG=capcode (default to FLEX)
                protocol_to_use = PROTO_FLEX;
                if (str2uint64(&capcode_to_use, params) < 0) {
                    at_send_error();
                    return true;
                }
            }

            // Reset transmission state
            at_reset_state();

            // Set protocol and capcode AFTER reset
            current_protocol = protocol_to_use;
            message_capcode = capcode_to_use;

            // Set up for receiving message
            device_state = STATE_WAITING_FOR_MSG;
            message_pos = 0;
            message_timeout = millis() + FLEX_MSG_TIMEOUT; // 30 second timeout
            console_loop_enable = false; // Disable console loop during message reception
            memset(message_buffer, 0, sizeof(message_buffer));

            // Clear any pending serial data
            at_flush_serial_buffers();

            // Send ready response
            Serial.print("+MSG: READY\r\n");
            Serial.flush();

            display_status();
        }
        return true;
    }

    else if (strcmp(cmd_name, "MAILDROP") == 0) {
        if (query_pos != NULL) {
            // Query mail drop setting
            at_send_response_int("MAILDROP", flex_mail_drop ? 1 : 0);
        } else if (equals_pos != NULL) {
            // Set mail drop flag
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
            default:
                status_str = "UNKNOWN";
                break;
        }
        at_send_response("STATUS", status_str);
        return true;
    }

    else if (strcmp(cmd_name, "ABORT") == 0) {
        // Allow aborting current operation
        radio.standby();
        LED_OFF(); // Turn off LED on abort

        // Restore FLEX settings on abort
        apply_flex_radio_settings();

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

    return false;
}

void at_handle_binary_data() {
    reset_oled_timeout();

    if (device_state != STATE_WAITING_FOR_DATA) {
        return;
    }

    // Check timeout
    if (millis() > data_receive_timeout) {
        at_reset_state();
        display_status();
        at_send_error();
        return;
    }

    // Read available binary data
    while (Serial.available() && current_tx_total_length < expected_data_length) {
        tx_data_buffer[current_tx_total_length++] = Serial.read();
        // Reset timeout on successful data receive
        data_receive_timeout = millis() + 5000; // 5 second timeout for continuous data
    }

    // Check if we have received all expected data
    if (current_tx_total_length >= expected_data_length) {
        // Start transmission
        device_state = STATE_TRANSMITTING;
        LED_ON(); // Turn on LED during transmission
        display_status();

        // Initialize transmission variables
        fifo_empty = true;
        current_tx_remaining_length = current_tx_total_length;
        radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);

        // Note: The actual transmission handling is done in the main loop
        // using the existing FIFO mechanism
    }
}

void at_handle_message() {
    reset_oled_timeout();

    if (device_state != STATE_WAITING_FOR_MSG) {
        return;
    }

    // Check timeout
    if (millis() > message_timeout) {
        at_reset_state();
        display_status();
        at_send_error();
        return;
    }

    // Read available message data
    int max_len = (current_protocol == PROTO_FLEX) ? MAX_FLEX_MESSAGE_LENGTH : MAX_POCSAG_MESSAGE_LENGTH;
    while (Serial.available() && message_pos < max_len) {
        char c = Serial.read();

        // Check for message termination
        if (c == '\r' || c == '\n') {
            // Message complete
            message_buffer[message_pos] = '\0';

            // Apply protocol-specific radio settings
            if (current_protocol == PROTO_FLEX) {
                apply_flex_radio_settings();
            } else {
                apply_pocsag_radio_settings();
            }

            // Encode message based on protocol
            bool encode_success = false;
            if (current_protocol == PROTO_FLEX) {
                encode_success = flex_encode_and_store(message_capcode, message_buffer, flex_mail_drop);
            } else if (current_protocol == PROTO_POCSAG) {
                encode_success = pocsag_encode_and_store((uint32_t)message_capcode, message_buffer);
            }

            if (encode_success) {
                // Start transmission
                device_state = STATE_TRANSMITTING;
                LED_ON(); // Turn on LED during transmission
                display_status();

                // Initialize transmission variables
                fifo_empty = true;
                radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);

                // Note: The actual transmission handling is done in the main loop
                // using the existing FIFO mechanism
            } else {
                device_state = STATE_ERROR;
                at_send_error();

                // Restore FLEX settings after error
                apply_flex_radio_settings();

                at_reset_state();
                display_status();
            }
            return;
        }

        // Add character to message buffer
        if (c >= 32 && c <= 126) { // Printable ASCII characters only
            message_buffer[message_pos++] = c;
            // Reset timeout on successful data receive
            message_timeout = millis() + FLEX_MSG_TIMEOUT;
        }
    }

    // Check if buffer is full
    if (message_pos >= max_len) {
        // Message too long, terminate it
        message_buffer[max_len] = '\0';

        // Apply protocol-specific radio settings
        if (current_protocol == PROTO_FLEX) {
            apply_flex_radio_settings();
        } else {
            apply_pocsag_radio_settings();
        }

        // Encode message based on protocol
        bool encode_success = false;
        if (current_protocol == PROTO_FLEX) {
            encode_success = flex_encode_and_store(message_capcode, message_buffer, flex_mail_drop);
        } else if (current_protocol == PROTO_POCSAG) {
            encode_success = pocsag_encode_and_store((uint32_t)message_capcode, message_buffer);
        }

        if (encode_success) {
            // Start transmission
            device_state = STATE_TRANSMITTING;
            LED_ON(); // Turn on LED during transmission
            display_status();

            // Initialize transmission variables
            fifo_empty = true;
            radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);

            // Note: The actual transmission handling is done in the main loop
            // using the existing FIFO mechanism
        } else {
            device_state = STATE_ERROR;
            at_send_error();

            // Restore FLEX settings after error
            apply_flex_radio_settings();

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
        at_handle_message();
        return;
    }

    while (Serial.available()) {
        char c = Serial.read();

        // Handle buffer overflow
        if (at_buffer_pos >= AT_BUFFER_SIZE - 1) {
            at_buffer_pos = 0;
            at_send_error();
            continue;
        }

        at_buffer[at_buffer_pos++] = c;

        // Check for command terminator
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

        // Reset buffer
        at_buffer_pos = 0;
        at_command_ready = false;
    }
}

// =============================================================================
// PANIC FUNCTION
// =============================================================================

void panic()
{
  display_panic();
  Serial.print("ERROR\r\n");
  while (true)
  {
    delay(100000);
  }
}

// =============================================================================
// INTERRUPT SERVICE ROUTINE
// =============================================================================

// Interrupt Service Routine (ISR) called when radio's transmit FIFO has space.
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void on_interrupt_fifo_has_space()
{
  fifo_empty = true;
}

// =============================================================================
// ARDUINO SETUP AND LOOP
// =============================================================================

void setup()
{
  Serial.begin(SERIAL_BAUD);

  SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_CS_PIN);

  display_setup();    // Initialize display
  display_status();   // Show initial status on display

  pinMode(LED_PIN, OUTPUT);
  LED_OFF();

  // Initialize radio module in FSK mode with FLEX parameters (default)
  int radio_init_state = radio.beginFSK(current_tx_frequency,
                                     TX_BITRATE_FLEX,
                                     TX_DEVIATION_FLEX,
                                     RX_BANDWIDTH,
                                     current_tx_power,
                                     PREAMBLE_LENGTH,
                                     false);

  if (radio_init_state != RADIOLIB_ERR_NONE)
  {
    panic();
  }

  // Set the callback function for when the FIFO is empty (has space)
  radio.setFifoEmptyAction(on_interrupt_fifo_has_space);

  // Configure packet mode: 0 for variable length (required for streaming)
  int packet_mode_state = radio.fixedPacketLengthMode(0);
  if (packet_mode_state != RADIOLIB_ERR_NONE) {
    panic();
  }

  // Initialize AT state
  at_reset_state();

  // Initialize activity timer
  reset_oled_timeout();

  // Send ready message
  Serial.print("AT READY\r\n");
  Serial.flush();
}

void loop()
{
  // Handle AT commands
  at_process_serial();

  // Handle OLED timeout
  if (oled_active && (millis() - last_activity_time > OLED_TIMEOUT_MS)) {
      display_turn_off();
  }

  // Check if ISR indicated FIFO has space AND there's data remaining for the current transmission
  if (fifo_empty && current_tx_remaining_length > 0)
  {
    fifo_empty = false; // Reset ISR flag
    transmission_processing_complete = radio.fifoAdd(tx_data_buffer, current_tx_total_length, &current_tx_remaining_length);
  }

  if (transmission_processing_complete)
  {
    transmission_processing_complete = false; // Reset flag for the next transmission cycle

    if (radio_start_transmit_status == RADIOLIB_ERR_NONE)
    {
      at_send_ok();
    }
    else
    {
      device_state = STATE_ERROR;
      at_send_error();
    }

    // After transmission, put the radio in standby mode
    radio.standby();
    LED_OFF(); // Turn off LED after transmission

    // Restore FLEX settings after transmission
    apply_flex_radio_settings();

    // Reset state after transmission
    at_reset_state();
    display_status();
  }

  delay(1);
}
