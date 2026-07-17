#include "../core/display.h"

#include <Wire.h>
#include <WiFi.h>

#include "../core/config.h"
#include "../../include/boards/boards.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../core/hardware.h"
#include "../network/ntp_time.h"
#include "../network/wifi.h"
#include "../network/gsm.h"
#include "../network/network.h"
#include "../services/mqtt.h"
#include "../protocol/transmission.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

unsigned long last_activity_time = 0;
bool oled_active = true;

// =============================================================================
// DISPLAY POWER
// =============================================================================
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
// DISPLAY SCREENS
// =============================================================================
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

#ifdef ENABLE_RTC
    if (rtc.begin()) {
        rtc_available = true;
        logMessage("RTC: DS3231 initialized successfully");

        if (!rtc.lostPower()) {
            DateTime now = rtc.now();
            struct timeval tv;
            tv.tv_sec = now.unixtime();
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            system_time_initialized = true;
            system_time_from_rtc = true;
            logMessagef("RTC: System time set from RTC: %04d-%02d-%02d %02d:%02d:%02d",
                       now.year(), now.month(), now.day(),
                       now.hour(), now.minute(), now.second());
        } else {
            logMessage("RTC: WARNING - RTC lost power, time may be incorrect");
        }
    } else {
        rtc_available = false;
        logMessage("RTC: Failed to initialize DS3231");
    }
#endif
}

void display_ap_info() {
    if (!oled_active) return;

    display.clearBuffer();

    int info_start_y = 12;

    display.setFont(u8g2_font_7x13_tr);

    display.drawStr(0, info_start_y, "AP Mode Active");

    info_start_y += 14;
    display.setFont(u8g2_font_6x10_tr);
    String ssid_display = "SSID: " + ap_ssid;
    display.drawStr(0, info_start_y, ssid_display.c_str());

    display.setFont(u8g2_font_7x13_tr);
    info_start_y += 12;
    String pass_display = "Pass: " + ap_password;
    display.drawStr(0, info_start_y, pass_display.c_str());

    info_start_y += 12;
    String ip_str = "AP: " + WiFi.softAPIP().toString();
    display.drawStr(0, info_start_y, ip_str.c_str());

    display.sendBuffer();
}

void display_status() {
    if (!oled_active) return;

    if (ap_mode_active && WiFi.softAPgetStationNum() == 0) {
        display_ap_info();
        return;
    }

    uint16_t battery_voltage_mv;
    int battery_percentage_temp;
    getBatteryInfo(&battery_voltage_mv, &battery_percentage_temp);

    String tx_power_str;
    if (battery_present) {
        tx_power_str = String(tx_power, 1) + "dBm || " + String(battery_percentage_temp) + "%";
    } else {
        tx_power_str = String(tx_power, 1) + "dBm";
    }
    String tx_frequency_str = String(current_tx_frequency, 4) + " MHz";
    String status_str;
    String wifi_str;

    switch (device_state) {
        case STATE_IDLE:
            status_str = mqttClient.connected() ? "Ready (M)" : "Ready";
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
        case STATE_GSM_INITIALIZING:
        case STATE_GSM_CONNECTING:
        case STATE_GSM_REGISTERING:
            status_str = "Connecting...";
            break;
        case STATE_WIFI_AP_MODE:
            status_str = "AP Mode";
            break;
        case STATE_IMAP_PROCESSING:
            status_str = "IMAP Sync...";
            break;
        case STATE_NTP_SYNC:
            status_str = "NTP Sync...";
            break;
        case STATE_MQTT_CONNECTING:
            if (mqtt_connection_attempt > 0) {
                status_str = "MQTT [" + String(mqtt_connection_attempt) + "]...";
            } else {
                status_str = "MQTT Connecting...";
            }
            break;
        default:
            status_str = "Unknown";
            break;
    }

#ifdef ENABLE_GSM
    if (gsm_connected && active_network == NETWORK_GSM_ACTIVE) {
        String prefix = "GSM: ";
        if (gsm_internet_test_attempt > 0) {
            wifi_str = prefix + "Connecting [" + String(gsm_internet_test_attempt) + "]...";
        } else if (gsm_internet_verified) {
            wifi_str = prefix + gsm_ip_address;
        } else {
            wifi_str = prefix + "No Internet";
        }
    } else
#endif
    if (wifi_connected) {
        String prefix = "WiFi: ";
        wifi_str = prefix + WiFi.localIP().toString();
    } else if (ap_mode_active) {
        String prefix = "AP: ";
        wifi_str = prefix + WiFi.softAPIP().toString();
    } else if (device_state == STATE_WIFI_CONNECTING) {
        wifi_str = "WiFi: Connecting...";
#ifdef ENABLE_GSM
    } else if (device_state == STATE_GSM_INITIALIZING) {
        wifi_str = "GSM: Initializing...";
    } else if (device_state == STATE_GSM_CONNECTING) {
        wifi_str = "GSM: Connecting...";
    } else if (device_state == STATE_GSM_REGISTERING) {
        wifi_str = "GSM: Registering...";
    } else if (!gsm_config.enable_gsm) {
        wifi_str = "GSM: disabled";
#endif
    } else {
        wifi_str = "No Network";
    }

    display.clearBuffer();

    display.setFont(FONT_BANNER);
    int banner_width = display.getStrWidth(settings.banner_message);
    int banner_x = (display.getWidth() - banner_width) / 2;
    display.drawStr(banner_x, BANNER_HEIGHT, settings.banner_message);

    int status_start_y = BANNER_HEIGHT + BANNER_MARGIN + 10;

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
