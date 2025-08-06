/*
 * flex-fsk-tx: Send FLEX packets over serial using AT commands.
 * Original send_ttgo firmware Written by Davidson Francis (aka Theldus) and Rodrigo Laneth - 2025.
 * > https://github.com/rlaneth/ttgo-fsk-tx/
 * send_ttgo adaptation for heltec v3 and standarized AT command renamed to flex-fsk-tx @ geekinsanemx
 * > https://github.com/geekinsanemx/flex-fsk-tx
 * Improved AT Protocol implementation with better retry logic and error handling.
 *
 * This is free and unencumbered software released into the public domain.
 */

#include <RadioLib.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <HardwareSerial.h>

// =============================================================================
// CONSTANTS AND DEFAULTS
// =============================================================================

#define SERIAL_BAUD 115200

// Heltec WiFi LoRa 32 V3 pin configuration
#define LORA_NSS    8
#define LORA_NRESET 12
#define LORA_BUSY   13
#define LORA_DIO1   14
#define LORA_SCK    9
#define LORA_MISO   11
#define LORA_MOSI   10

#define TX_FREQ_DEFAULT 931.9375
#define TX_BITRATE 1.6
#define TX_DEVIATION 5
#define TX_POWER_DEFAULT 2
#define RX_BANDWIDTH 10.4
#define PREAMBLE_LENGTH 0

// Display constants
#define OLED_TIMEOUT_MS (5 * 60 * 1000) // 5 minutes in milliseconds
#define FONT_DEFAULT ArialMT_Plain_10
#define FONT_BOLD ArialMT_Plain_10
#define FONT_LINE_HEIGHT 12
#define FONT_TAB_START 50

// AT Protocol constants
#define AT_BUFFER_SIZE 512
#define AT_CMD_TIMEOUT 5000
#define AT_MAX_RETRIES 3
#define AT_INTER_CMD_DELAY 100

// BANNER DISPLAY
#define BANNER "GeekInsaneMX"
#define FONT_BANNER ArialMT_Plain_16  // Larger font for banner
#define BANNER_HEIGHT 18             // Height for banner area

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_NRESET, LORA_BUSY);

#ifdef WIRELESS_STICK_V3
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED);
#else
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
#endif

// AT Protocol variables
char at_buffer[AT_BUFFER_SIZE];
int at_buffer_pos = 0;
bool at_command_ready = false;

// Device state management
typedef enum {
    STATE_IDLE,
    STATE_WAITING_FOR_DATA,
    STATE_TRANSMITTING,
    STATE_ERROR
} device_state_t;

device_state_t device_state = STATE_IDLE;
unsigned long state_timeout = 0;

// Global variables for transmission state
volatile bool transmission_complete = false;
volatile bool transmission_in_progress = false;

// Transmission data buffer and state variables
uint8_t tx_data_buffer[2048] = {0};
int current_tx_total_length = 0;
int expected_data_length = 0;
unsigned long data_receive_timeout = 0;

// Radio operation parameters
float current_tx_frequency = TX_FREQ_DEFAULT;
float current_tx_power = TX_POWER_DEFAULT;


// =============================================================================
// BUILT-IN LED CONTROL
// =============================================================================

// LED control macros (active-low)
#define LED_PIN 35
#define LED_OFF()  digitalWrite(LED_PIN, LOW)
#define LED_ON()   digitalWrite(LED_PIN, HIGH)

// =============================================================================
// DISPLAY CONTROL
// =============================================================================

unsigned long last_activity_time = 0;
bool oled_active = true;

void oled_turn_off() {
    display.displayOff();
    oled_active = false;
    VextOFF(); // Also turn off VEXT to save power
}

void oled_turn_on() {
    VextON(); // Power on VEXT first
    delay(50); // Short delay for power stabilization
    display.displayOn();
    display_status(); // Refresh display
    oled_active = true;
}

void reset_oled_timeout() {
    last_activity_time = millis();
    if (!oled_active) {
        oled_turn_on();
    }
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
    expected_data_length = 0;
    data_receive_timeout = 0;
    state_timeout = 0;
    transmission_in_progress = false;
    transmission_complete = false;
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

    /*
    // Debug: show what command we received
    Serial.print("DEBUG: Received command: '");
    Serial.print(cmd_buffer);
    Serial.println("'");
    */

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

    // Only allow certain commands when waiting for data
    if (device_state == STATE_WAITING_FOR_DATA) {
        if (strcmp(cmd_name, "STATUS") != 0 && strcmp(cmd_buffer, "AT") != 0) {
            // Serial.println("DEBUG: Device is waiting for binary data");
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
                Serial.print("DEBUG: Radio setFrequency failed: ");
                Serial.println(state);
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
            if (power < -9 || power > 22) {
                at_send_error();
                return true;
            }

            int state = radio.setOutputPower(power);
            if (state != RADIOLIB_ERR_NONE) {
                Serial.print("DEBUG: Radio setOutputPower failed: ");
                Serial.println(state);
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
                Serial.print("DEBUG: Invalid data length: ");
                Serial.println(bytes_to_read);
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

            // Clear any pending serial data
            at_flush_serial_buffers();

            // Send ready response
            Serial.print("+SEND: READY\r\n");
            Serial.flush();

            display_status();
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
        Serial.print("DEBUG: Binary data receive timeout, got ");
        Serial.print(current_tx_total_length);
        Serial.print(" of ");
        Serial.print(expected_data_length);
        Serial.println(" bytes");

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
      /*
        Serial.print("DEBUG: Received all ");
        Serial.print(current_tx_total_length);
        Serial.println(" bytes, starting transmission");
      */

        // Start transmission
        device_state = STATE_TRANSMITTING;
        transmission_in_progress = true;
        display_status();

        bool tx_success = at_transmit_data();

        if (tx_success) {
            at_send_ok();
        } else {
            device_state = STATE_ERROR;
            at_send_error();
        }

        // Reset state after transmission
        at_reset_state();
        display_status();
    }
}

bool at_transmit_data() {
    reset_oled_timeout();

    const int CHUNK_SIZE = 255;
    int chunks = (current_tx_total_length + CHUNK_SIZE - 1) / CHUNK_SIZE;

    /*
    Serial.print("DEBUG: Transmitting ");
    Serial.print(current_tx_total_length);
    Serial.print(" bytes in ");
    Serial.print(chunks);
    Serial.println(" chunks");
    */

    for (int i = 0; i < chunks; i++) {
        int chunk_start = i * CHUNK_SIZE;
        int chunk_size = min(CHUNK_SIZE, current_tx_total_length - chunk_start);

        /*
        Serial.print("DEBUG: Transmitting chunk ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.print(chunks);
        Serial.print(" (");
        Serial.print(chunk_size);
        Serial.println(" bytes)");
        */

        // Reset transmission complete flag
        transmission_complete = false;

        int tx_state = radio.startTransmit(tx_data_buffer + chunk_start, chunk_size);

        if (tx_state != RADIOLIB_ERR_NONE) {
            Serial.print("DEBUG: Chunk ");
            Serial.print(i + 1);
            Serial.print(" TX start failed with code ");
            Serial.println(tx_state);
            return false;
        }

        // Wait for this chunk to complete
        unsigned long chunk_timeout = millis() + 8000; // 8 second timeout per chunk
        while (!transmission_complete && millis() < chunk_timeout) {
            yield();
            delay(1);
        }

        if (!transmission_complete) {
            Serial.print("DEBUG: Chunk ");
            Serial.print(i + 1);
            Serial.println(" transmission timeout");
            return false;
        }

        /*
        Serial.print("DEBUG: Chunk ");
        Serial.print(i + 1);
        Serial.println(" transmitted successfully");
        */

        // Small delay between chunks
        // delay(100);
    }

    Serial.println("DEBUG: All chunks transmitted successfully");

    return true;
}

void at_process_serial() {
    if (device_state == STATE_WAITING_FOR_DATA) {
        at_handle_binary_data();
        return;
    }

    while (Serial.available()) {
        char c = Serial.read();

        // Handle buffer overflow
        if (at_buffer_pos >= AT_BUFFER_SIZE - 1) {
            // Buffer overflow - reset and send error
            Serial.println("DEBUG: AT buffer overflow");
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

// =============================================================================
// DISPLAY FUNCTIONS
// =============================================================================

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

void display_setup() {
    VextON();
    delay(100);
    display.init();
    display.setFont(FONT_DEFAULT);
    display.clear();
    display.display();
}

void display_status() {
    if (!oled_active) return;

    String tx_power_str = String(current_tx_power, 1) + " dBm";
    String tx_frequency_str = String(current_tx_frequency, 4) + " MHz";
    String status_str;
    LED_OFF();

    switch (device_state) {
        case STATE_IDLE:
            status_str = "Ready";
            break;
        case STATE_WAITING_FOR_DATA:
            status_str = "Receving Data...";
            break;
        case STATE_TRANSMITTING:
            LED_ON();
            status_str = "Transmitting...";
            break;
        case STATE_ERROR:
            status_str = "Error";
            break;
        default:
            status_str = "Unknown";
            break;
    }

    display.clear();

    // Draw banner at top
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(FONT_BANNER);
    display.drawString(display.getWidth()/2, 0, BANNER);

    // Draw status info below banner
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(FONT_DEFAULT);

    int height_ptr = BANNER_HEIGHT + 4;  // Start below banner

    display.drawString(0, height_ptr, "State:");
    display.drawString(FONT_TAB_START, height_ptr, status_str);

    height_ptr += FONT_LINE_HEIGHT;
    display.drawString(0, height_ptr, "TX Pwr:");
    display.drawString(FONT_TAB_START, height_ptr, tx_power_str);

    height_ptr += FONT_LINE_HEIGHT;
    display.drawString(0, height_ptr, "Freq:");
    display.drawString(FONT_TAB_START, height_ptr, tx_frequency_str);

    display.display();
}

// =============================================================================
// INTERRUPT SERVICE ROUTINE
// =============================================================================

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void on_transmission_complete() {
    transmission_complete = true;
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
// ARDUINO SETUP AND LOOP
// =============================================================================

void setup() {
    Serial.begin(SERIAL_BAUD);

    display_setup();
    display_status();

    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    LED_OFF();  // Start with LED off

    // Initialize radio module in FSK mode
    int radio_init_state = radio.beginFSK();

    if (radio_init_state != RADIOLIB_ERR_NONE) {
        panic();
    }

    radio_init_state = radio.setFrequency(TX_FREQ_DEFAULT);
    radio_init_state = radio.setBitRate(TX_BITRATE);
    radio_init_state = radio.setFrequencyDeviation(TX_DEVIATION);
    radio_init_state = radio.setRxBandwidth(RX_BANDWIDTH);
    radio_init_state = radio.setOutputPower(TX_POWER_DEFAULT);
    radio_init_state = radio.setPreambleLength(PREAMBLE_LENGTH);

    if (radio_init_state != RADIOLIB_ERR_NONE) {
        panic();
    }

    radio.setDio1Action(on_transmission_complete);
    radio.setDataShaping(RADIOLIB_SHAPING_NONE);
    radio.setEncoding(RADIOLIB_ENCODING_NRZ);

    uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    radio.setSyncWord(syncWord, 8);
    radio.setCRC(false);

    int packet_mode_state = radio.fixedPacketLengthMode(0);
    if (packet_mode_state != RADIOLIB_ERR_NONE) {
        panic();
    }

    // Initialize state
    at_reset_state();

    // Send ready message
    Serial.print("AT READY\r\n");
    Serial.flush();
}

void loop() {
    at_process_serial();

    // Handle OLED timeout
    if (oled_active && (millis() - last_activity_time > OLED_TIMEOUT_MS)) {
        oled_turn_off();
    }

    delay(1);
}
