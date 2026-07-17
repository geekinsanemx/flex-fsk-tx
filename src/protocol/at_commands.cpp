#include "../protocol/at_commands.h"

#include <WiFi.h>
#include <SPIFFS.h>

#include "../version.h"
#include "../core/config.h"
#include "../../include/boards/boards.h"
#include "../core/storage.h"
#include "../core/logging.h"
#include "../core/hardware.h"
#include "../core/display.h"
#include "../protocol/flex_protocol.h"
#include "../protocol/transmission.h"
#include "../network/wifi.h"
#include "../network/network.h"
#include "../services/mqtt.h"
#include "../services/imap.h"

static char at_buffer[AT_BUFFER_SIZE];
static int at_buffer_pos = 0;
static bool at_command_ready = false;

static volatile bool console_loop_enable = true;
static volatile bool transmission_processing_complete = false;

static int expected_data_length = 0;
static unsigned long data_receive_timeout = 0;

static uint64_t flex_capcode = 0;
static char flex_message_buffer[MAX_FLEX_MESSAGE_LENGTH + 1] = {0};
static int flex_message_pos = 0;
static unsigned long flex_message_timeout = 0;
static bool flex_mail_drop = false;

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

// =============================================================================
// RESPONSE HELPERS
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


    flex_capcode = 0;
    current_tx_capcode = 0;
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

// =============================================================================
// COMMAND PARSER
// =============================================================================
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

    if (transmission_guard_active()) {
        if (!(strcmp(cmd_buffer, "AT") == 0 || strcmp(cmd_name, "STATUS") == 0 || strcmp(cmd_name, "ABORT") == 0)) {
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

            int state = radio.setFrequency(apply_frequency_correction(freq));
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

    if (strcmp(cmd_name, "FREQPPM") == 0) {
        if (query_pos != NULL) {
            at_send_response_float("FREQPPM", settings.frequency_correction_ppm, 2);
        } else if (equals_pos != NULL) {
            float ppm = atof(equals_pos + 1);
            if (ppm < -50.0 || ppm > 50.0) {
                at_send_error();
                return true;
            }

            settings.frequency_correction_ppm = ppm;
            save_runtime_settings();

            if (current_tx_frequency > 0) {
                float corrected_freq = apply_frequency_correction(current_tx_frequency);
                radio.setFrequency(corrected_freq);
            }

            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "POWER") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("POWER", (int)tx_power);
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

            tx_power = power;
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
            case STATE_NTP_SYNC:
                status_str = "NTP_SYNC";
                break;
            case STATE_MQTT_CONNECTING:
                status_str = "MQTT_CONNECTING";
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
        mqtt_reboot_count = 0;
        save_mqtt_reboot_count();
        at_send_ok();
        delay(100);
        ESP.restart();
        return true;
    }

    else if (strcmp(cmd_name, "NETWORK") == 0) {
        if (query_pos != NULL) {
            const char* mode_str;
            switch (network_mode) {
                case NETWORK_MODE_AUTO: mode_str = "AUTO"; break;
                case NETWORK_MODE_WIFI: mode_str = "WIFI"; break;
                case NETWORK_MODE_GSM:  mode_str = "GSM";  break;
                case NETWORK_MODE_AP:   mode_str = "AP";   break;
                default: mode_str = "UNKNOWN"; break;
            }
            at_send_response("NETWORK", mode_str);
        } else if (equals_pos != NULL) {
            String mode_param = String(equals_pos + 1);
            mode_param.trim();
            mode_param.toUpperCase();

            NetworkMode requested_mode;
            if (mode_param == "AUTO") {
                requested_mode = NETWORK_MODE_AUTO;
            } else if (mode_param == "WIFI") {
                requested_mode = NETWORK_MODE_WIFI;
#ifdef ENABLE_GSM
            } else if (mode_param == "GSM") {
                requested_mode = NETWORK_MODE_GSM;
#endif
            } else if (mode_param == "AP") {
                requested_mode = NETWORK_MODE_AP;
            } else {
                at_send_error();
                return true;
            }

            network_mode_switch(requested_mode);
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "WIFI") == 0) {
        if (query_pos != NULL) {
            String status = "DISCONNECTED";
            if (wifi_connected) {
                status = "CONNECTED," + WiFi.localIP().toString();
            } else if (ap_mode_active) {
                status = "AP_MODE," + WiFi.softAPIP().toString();
            }
            at_send_response("WIFI", status.c_str());
        } else if (equals_pos != NULL) {
            // AT+WIFI=<ssid>,<password> - Add or update network
            String params = String(equals_pos + 1);
            int comma_pos = params.indexOf(',');
            if (comma_pos > 0) {
                String ssid = params.substring(0, comma_pos);
                String password = params.substring(comma_pos + 1);
                ssid.trim();
                password.trim();

                if (ssid.length() > 0 && ssid.length() <= 32 && password.length() <= 64) {
                    int network_idx = -1;

                    // Find existing network or add new one
                    for (int i = 0; i < stored_networks_count; i++) {
                        if (String(stored_networks[i].ssid) == ssid) {
                            network_idx = i;
                            break;
                        }
                    }

                    if (network_idx == -1 && stored_networks_count < MAX_WIFI_NETWORKS) {
                        network_idx = stored_networks_count;
                        stored_networks_count++;
                    }

                    if (network_idx >= 0) {
                        strlcpy(stored_networks[network_idx].ssid, ssid.c_str(), sizeof(stored_networks[network_idx].ssid));
                        strlcpy(stored_networks[network_idx].password, password.c_str(), sizeof(stored_networks[network_idx].password));
                        stored_networks[network_idx].use_dhcp = true;  // Default to DHCP

                        if (save_runtime_settings()) {
                            at_send_ok();
                        } else {
                            at_send_error();
                        }
                    } else {
                        at_send_error();  // Max networks reached
                    }
                } else {
                    at_send_error();  // Invalid SSID/password length
                }
            } else {
                at_send_error();  // Invalid format
            }
        }
        return true;
    }




    else if (strcmp(cmd_name, "DEVICE") == 0) {
        if (query_pos != NULL) {
            Serial.print("+DEVICE_FIRMWARE: ");
            Serial.print(FIRMWARE_VERSION);
            Serial.print("\r\n");

            uint16_t battery_voltage_mv;
            int battery_percentage_temp;
            getBatteryInfo(&battery_voltage_mv, &battery_percentage_temp);
            Serial.print("+DEVICE_BATTERY: ");
            if (battery_present) {
                Serial.print(battery_percentage_temp);
                Serial.print("%");
            } else {
                Serial.print("N/A");
            }
            Serial.print("\r\n");

            String wifi_status = "Disconnected";
            if (wifi_connected) {
                wifi_status = "Connected";
            } else if (ap_mode_active) {
                wifi_status = "AP_Mode";
            }
            Serial.print("+DEVICE_WIFI: ");
            Serial.print(wifi_status);
            Serial.print("\r\n");

            String mqtt_status = settings.mqtt_enabled ? (mqttClient.connected() ? "Connected" : "Disconnected") : "Disabled";
            Serial.print("+DEVICE_MQTT: ");
            Serial.print(mqtt_status);
            Serial.print("\r\n");

#ifdef ENABLE_IMAP
            String imap_status = imap_config.enabled ? "Active" : "Disabled";
            Serial.print("+DEVICE_IMAP: ");
            Serial.print(imap_status);
            Serial.print("\r\n");

            for (int i = 0; i < imap_config.account_count; i++) {
                String account_status = !imap_config.accounts[i].suspended ? "Active" : "Suspended";
                Serial.print("+DEVICE_IMAP_ACCOUNT");
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(account_status);
                Serial.print("\r\n");
            }
#endif // ENABLE_IMAP

            String api_status = settings.api_enabled ? "Enabled" : "Disabled";
            Serial.print("+DEVICE_API: ");
            Serial.print(api_status);
            Serial.print("\r\n");

            String grafana_status = settings.grafana_enabled ? "Enabled" : "Disabled";
            Serial.print("+DEVICE_GRAFANA: ");
            Serial.print(grafana_status);
            Serial.print("\r\n");

            Serial.print("+DEVICE_MEMORY: ");
            Serial.print(ESP.getFreeHeap());
            Serial.print(" bytes\r\n");

            Serial.print("+DEVICE_FLEX_CAPCODE: ");
            Serial.print(settings.default_capcode);
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

    else if (strcmp(cmd_name, "FLEX") == 0) {
        if (query_pos != NULL) {
            Serial.print("+FLEX_CAPCODE: ");
            Serial.print(settings.default_capcode);
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
                    if (capcode > 0) {
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
                    if (power >= 0.0 && power <= 20.0) {
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

    else if (strcmp(cmd_name, "FACTORYRESET") == 0) {
        at_send_ok();
        delay(100);

        logMessage("SYSTEM: Factory reset initiated via AT command");
        perform_factory_reset();
        return true;
    }

    else if (strcmp(cmd_name, "LOGS") == 0) {
        if (!SPIFFS.exists("/serial.log")) {
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

    else if (strcmp(cmd_name, "RMLOG") == 0) {
        if (!SPIFFS.exists("/serial.log")) {
            Serial.println("ERROR: No log file found");
            at_send_ok();
            return true;
        }

        if (SPIFFS.remove("/serial.log")) {
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
// BINARY / FLEX MESSAGE MODES
// =============================================================================
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

        fifo_empty = true;
        current_tx_remaining_length = current_tx_total_length;
        radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);

        display_status();
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

            if (queue_add_message(flex_capcode, current_tx_frequency, tx_power, flex_mail_drop, flex_message_buffer)) {
                at_reset_state();
                at_send_ok();
                display_status();
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
        String truncated_message = truncate_message_with_ellipsis(String(flex_message_buffer));
        strncpy(flex_message_buffer, truncated_message.c_str(), MAX_FLEX_MESSAGE_LENGTH);
        flex_message_buffer[MAX_FLEX_MESSAGE_LENGTH] = '\0';

        if (queue_add_message(flex_capcode, current_tx_frequency, tx_power, flex_mail_drop, flex_message_buffer)) {
            at_reset_state();
            at_send_ok();
            display_status();
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
