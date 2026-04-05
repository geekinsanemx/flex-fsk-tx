/*
 * FLEX Paging Message Transmitter - v2.5.1
 * AT Command Protocol Implementation + Binary Protocol Processing
 */

#include "at_commands.h"
#include "config.h"
#include "logging.h"
#include "storage.h"
#include "hardware.h"
#include "display.h"
#include "flex_protocol.h"
#include "utils.h"
#include "boards/boards.h"
#include "binary_packet.h"
#include "binary_handlers.h"
#include "binary_events.h"
#include "cobs.h"
#include "crc16.h"

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
char at_buffer[AT_BUFFER_SIZE];
int at_buffer_pos = 0;
bool at_command_ready = false;

int expected_data_length = 0;
unsigned long data_receive_timeout = 0;

uint64_t flex_capcode = 0;
char flex_message_buffer[MAX_FLEX_MESSAGE_LENGTH + 1] = {0};
int flex_message_pos = 0;
unsigned long flex_message_timeout = 0;
bool flex_mail_drop = false;

bool console_loop_enable = true;

uint64_t current_tx_capcode = 0;

// =============================================================================
// INITIALIZATION
// =============================================================================
void at_init() {
    at_reset_state();
    logMessage("AT: Protocol initialized");
}

// =============================================================================
// AT PROTOCOL RESPONSES
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

// =============================================================================
// STATE MANAGEMENT
// =============================================================================
void at_reset_state() {
    device_state = STATE_IDLE;
    expected_data_length = 0;
    data_receive_timeout = 0;
    console_loop_enable = true;

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

bool transmission_guard_active() {
    return (device_state == STATE_TRANSMITTING ||
            device_state == STATE_WAITING_FOR_DATA ||
            device_state == STATE_WAITING_FOR_MSG);
}

// =============================================================================
// COMMAND PARSER
// =============================================================================
bool at_parse_command(char* cmd_buffer) {
    reset_oled_timeout();

    int len = strlen(cmd_buffer);
    while (len > 0 && (cmd_buffer[len-1] == '\r' || cmd_buffer[len-1] == '\n')) {
        cmd_buffer[--len] = '\0';
    }

    if (len == 0) return true;

    if (strncmp(cmd_buffer, "AT", 2) != 0) return false;

    if (strcmp(cmd_buffer, "AT") == 0) {
        at_reset_state();
        display_status();
        at_send_ok();
        return true;
    }

    if (strncmp(cmd_buffer, "AT+", 3) != 0) return false;

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

    if (cmd_name_len >= sizeof(cmd_name) || cmd_name_len <= 0) return false;

    strncpy(cmd_name, cmd_start, cmd_name_len);
    cmd_name[cmd_name_len] = '\0';

    if (transmission_guard_active()) {
        if (!(strcmp(cmd_buffer, "AT") == 0 || strcmp(cmd_name, "STATUS") == 0 ||
              strcmp(cmd_name, "ABORT") == 0)) {
            at_send_error();
            return true;
        }
    }

    // AT+FREQ
    if (strcmp(cmd_name, "FREQ") == 0) {
        if (query_pos != NULL) {
            at_send_response_float("FREQ", current_tx_frequency, 4);
        } else if (equals_pos != NULL) {
            float freq = atof(equals_pos + 1);
            if (freq < 400.0 || freq > 1000.0) {
                at_send_error();
                return true;
            }

            int state = radio_set_frequency(freq);
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

    // AT+FREQPPM
    if (strcmp(cmd_name, "FREQPPM") == 0) {
        if (query_pos != NULL) {
            at_send_response_float("FREQPPM", core_config.frequency_correction_ppm, 2);
        } else if (equals_pos != NULL) {
            float ppm = atof(equals_pos + 1);
            if (ppm < -50.0 || ppm > 50.0) {
                at_send_error();
                return true;
            }

            core_config.frequency_correction_ppm = ppm;
            save_core_config();

            if (current_tx_frequency > 0) {
                float corrected_freq = apply_frequency_correction(current_tx_frequency);
                radio.setFrequency(corrected_freq);
            }

            at_send_ok();
        }
        return true;
    }

    // AT+POWER
    if (strcmp(cmd_name, "POWER") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("POWER", (int)current_tx_power);
        } else if (equals_pos != NULL) {
            int power = atoi(equals_pos + 1);
            if (power < TX_POWER_MIN || power > TX_POWER_MAX) {
                at_send_error();
                return true;
            }

            int state = radio_set_power(power);
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

    // AT+SEND
    if (strcmp(cmd_name, "SEND") == 0) {
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

    // AT+MSG
    if (strcmp(cmd_name, "MSG") == 0) {
        if (equals_pos != NULL) {
            uint64_t capcode;
            if (str2uint64(&capcode, equals_pos + 1) < 0) {
                at_send_error();
                return true;
            }

            if (!validate_flex_capcode(capcode)) {
                at_send_error();
                return true;
            }

            at_reset_state();

            device_state = STATE_WAITING_FOR_MSG;
            flex_capcode = capcode;
            current_tx_capcode = capcode;
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

    // AT+MAILDROP
    if (strcmp(cmd_name, "MAILDROP") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("MAILDROP", flex_mail_drop ? 1 : 0);
        } else if (equals_pos != NULL) {
            int mail_drop = atoi(equals_pos + 1);
            flex_mail_drop = (mail_drop != 0);
            at_send_ok();
        }
        return true;
    }

    // AT+STATUS
    if (strcmp(cmd_name, "STATUS") == 0) {
        const char* status_str;
        switch (device_state) {
            case STATE_IDLE:                status_str = "READY"; break;
            case STATE_WAITING_FOR_DATA:    status_str = "WAITING_DATA"; break;
            case STATE_WAITING_FOR_MSG:     status_str = "WAITING_MSG"; break;
            case STATE_TRANSMITTING:        status_str = "TRANSMITTING"; break;
            case STATE_ERROR:               status_str = "ERROR"; break;
            default:                        status_str = "UNKNOWN"; break;
        }
        at_send_response("STATUS", status_str);
        return true;
    }

    // AT+ABORT
    if (strcmp(cmd_name, "ABORT") == 0) {
        radio_standby();
        digitalWrite(LED_PIN, LOW);
        at_reset_state();
        display_status();
        at_send_ok();
        return true;
    }

    // AT+RESET
    if (strcmp(cmd_name, "RESET") == 0) {
        at_send_ok();
        delay(100);
        ESP.restart();
        return true;
    }

    // AT+DEVICE?
    if (strcmp(cmd_name, "DEVICE") == 0) {
        if (query_pos != NULL) {
            Serial.print("+DEVICE_FIRMWARE: ");
            Serial.print(FIRMWARE_VERSION);
            Serial.print("\r\n");

            uint16_t battery_voltage_mv;
            int battery_percentage;
            getBatteryInfo(&battery_voltage_mv, &battery_percentage);
            Serial.print("+DEVICE_BATTERY: ");
            if (battery_present) {
                Serial.print(battery_percentage);
                Serial.print("%");
            } else {
                Serial.print("N/A");
            }
            Serial.print("\r\n");

            Serial.print("+DEVICE_MEMORY: ");
            Serial.print(ESP.getFreeHeap());
            Serial.print(" bytes\r\n");

            Serial.print("+DEVICE_FLEX_CAPCODE: ");
            Serial.print((unsigned long)settings.default_capcode);
            Serial.print("\r\n");

            Serial.print("+DEVICE_FLEX_FREQUENCY: ");
            Serial.print(settings.default_frequency, 4);
            Serial.print("\r\n");

            Serial.print("+DEVICE_FLEX_POWER: ");
            Serial.print(settings.default_txpower, 1);
            Serial.print("\r\n");

            at_send_ok();
        }
        return true;
    }

    // AT+FLEX
    if (strcmp(cmd_name, "FLEX") == 0) {
        if (query_pos != NULL) {
            Serial.print("+FLEX_CAPCODE: ");
            Serial.print((unsigned long)settings.default_capcode);
            Serial.print("\r\n");
            Serial.print("+FLEX_FREQUENCY: ");
            Serial.print(settings.default_frequency, 4);
            Serial.print("\r\n");
            Serial.print("+FLEX_POWER: ");
            Serial.print(settings.default_txpower, 1);
            Serial.print("\r\n");
            at_send_ok();
        } else if (equals_pos != NULL) {
            String params = String(equals_pos + 1);
            int comma_pos = params.indexOf(',');
            if (comma_pos > 0) {
                String param_name = params.substring(0, comma_pos);
                String value_str = params.substring(comma_pos + 1);
                param_name.toUpperCase();

                if (param_name == "CAPCODE") {
                    uint64_t capcode = strtoull(value_str.c_str(), NULL, 10);
                    if (validate_flex_capcode(capcode)) {
                        settings.default_capcode = capcode;
                        save_runtime_settings();
                        at_send_ok();
                    } else {
                        at_send_error();
                    }
                }
                else if (param_name == "FREQUENCY") {
                    float freq = atof(value_str.c_str());
                    if (freq >= 400.0 && freq <= 1000.0) {
                        settings.default_frequency = freq;
                        save_runtime_settings();
                        at_send_ok();
                    } else {
                        at_send_error();
                    }
                }
                else if (param_name == "POWER") {
                    float power = atof(value_str.c_str());
                    if (power >= TX_POWER_MIN && power <= TX_POWER_MAX) {
                        settings.default_txpower = power;
                        save_runtime_settings();
                        at_send_ok();
                    } else {
                        at_send_error();
                    }
                }
                else {
                    at_send_error();
                }
            } else {
                at_send_error();
            }
        }
        return true;
    }

    // AT+FACTORYRESET
    if (strcmp(cmd_name, "FACTORYRESET") == 0) {
        at_send_ok();
        delay(100);
        logMessage("FACTORY RESET: Initiated via AT command");
        perform_factory_reset();
        return true;
    }

    // AT+LOGS
    if (strcmp(cmd_name, "LOGS") == 0) {
        if (!SPIFFS.exists(LOG_FILE_PATH)) {
            Serial.println("ERROR: No log file found");
            at_send_ok();
            return true;
        }

        int numLines = 25;
        if (query_pos != NULL) {
            numLines = atoi(query_pos + 1);
            if (numLines <= 0) numLines = 25;
        }

        String logs = read_log_tail(numLines);
        Serial.print(logs);
        at_send_ok();
        return true;
    }

    // AT+RMLOG
    if (strcmp(cmd_name, "RMLOG") == 0) {
        if (!SPIFFS.exists(LOG_FILE_PATH)) {
            Serial.println("ERROR: No log file found");
            at_send_ok();
            return true;
        }

        if (delete_log_file()) {
            Serial.println("LOG: File deleted");
            at_send_ok();
        } else {
            Serial.println("ERROR: Failed to delete");
            at_send_ok();
        }
        return true;
    }

    return false;
}

// =============================================================================
// BINARY DATA HANDLER
// =============================================================================
void at_handle_binary_data() {
    reset_oled_timeout();

    if (device_state != STATE_WAITING_FOR_DATA) return;

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
        device_state = STATE_IDLE;

        bool queued = queue_add_message(settings.default_capcode,
                                       current_tx_frequency,
                                       current_tx_power,
                                       false,
                                       (const char*)tx_data_buffer);

        at_reset_state();

        if (queued) {
            at_send_ok();
        } else {
            at_send_error();
        }

        display_status();
    }
}

// =============================================================================
// FLEX MESSAGE HANDLER
// =============================================================================
void at_handle_flex_message() {
    reset_oled_timeout();

    if (device_state != STATE_WAITING_FOR_MSG) return;

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

            bool queued = queue_add_message(flex_capcode,
                                           current_tx_frequency,
                                           current_tx_power,
                                           flex_mail_drop,
                                           flex_message_buffer);

            at_reset_state();

            if (queued) {
                at_send_ok();
            } else {
                at_send_error();
            }

            display_status();
            return;
        }

        if (c >= 32 && c <= 126) {
            flex_message_buffer[flex_message_pos++] = c;
            flex_message_timeout = millis() + FLEX_MSG_TIMEOUT;
        }
    }

    if (flex_message_pos >= MAX_FLEX_MESSAGE_LENGTH) {
        String truncated = truncate_message_with_ellipsis(String(flex_message_buffer));
        strncpy(flex_message_buffer, truncated.c_str(), MAX_FLEX_MESSAGE_LENGTH);
        flex_message_buffer[MAX_FLEX_MESSAGE_LENGTH] = '\0';

        bool queued = queue_add_message(flex_capcode,
                                       current_tx_frequency,
                                       current_tx_power,
                                       flex_mail_drop,
                                       flex_message_buffer);

        at_reset_state();

        if (queued) {
            at_send_ok();
        } else {
            at_send_error();
        }

        display_status();
    }
}

// =============================================================================
// SERIAL PROCESSING
// =============================================================================
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
// BINARY PROTOCOL PROCESSING
// =============================================================================

static uint8_t binary_frame_buffer[512];
size_t binary_frame_pos = 0;

void process_binary_frame() {
    while (Serial.available()) {
        uint8_t byte = Serial.read();

        if (binary_frame_pos >= sizeof(binary_frame_buffer)) {
            logMessage("BINARY: Frame buffer overflow, resetting");
            binary_frame_pos = 0;
            continue;
        }

        binary_frame_buffer[binary_frame_pos++] = byte;

        if (byte == 0x00) {
            // Frame complete, process it
            handle_binary_packet(binary_frame_buffer, binary_frame_pos);
            binary_frame_pos = 0;
            break;
        }
    }
}

void handle_binary_packet(uint8_t *cobs_data, size_t len) {
    uint8_t decoded[512];
    size_t decoded_len = cobs_decode(cobs_data, len, decoded);

    if (decoded_len == 0) {
        logMessage("BINARY: COBS decode failed");
        return;
    }

    if (decoded_len < PACKET_OVERHEAD) {
        logMessagef("BINARY: Packet too short (%d bytes)", decoded_len);
        return;
    }

    // Parse packet
    binary_packet_t pkt;
    memcpy(&pkt, decoded, decoded_len);

    uint16_t calculated_crc = crc16_ccitt(decoded, decoded_len - 2);

    uint16_t recv_crc;
    memcpy(&recv_crc, &decoded[decoded_len - 2], 2);

    if (calculated_crc != recv_crc) {
        logMessagef("BINARY: CRC mismatch (calc=0x%04X, recv=0x%04X)",
                    calculated_crc, recv_crc);
        return;
    }

    binary_protocol_active = true;

    logMessagef("BINARY: Valid packet (type=0x%02X, opcode=0x%02X, msg_id=0x%04X)",
                pkt.type, pkt.opcode, pkt.msg_id);

    if (pkt.type == PKT_TYPE_CMD) {
        dispatch_binary_command(&pkt);
    } else {
        logMessagef("BINARY: Unexpected packet type 0x%02X", pkt.type);
    }
}
