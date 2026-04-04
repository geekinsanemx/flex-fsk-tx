/*
 * FLEX Paging Message Transmitter - v2.5
 * Display Management Implementation
 */

#include "display.h"
#include "config.h"
#include "hardware.h"
#include "storage.h"

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
volatile device_state_t device_state = STATE_IDLE;
volatile bool display_update_requested = false;
float current_tx_frequency = TX_FREQ_DEFAULT;
float current_tx_power = TX_POWER_DEFAULT;

// =============================================================================
// DISPLAY STATUS
// =============================================================================
void display_status() {
    if (!oled_active) return;

    uint16_t battery_voltage_mv;
    int battery_percentage;
    getBatteryInfo(&battery_voltage_mv, &battery_percentage);

    String tx_power_str;
    if (battery_present) {
        tx_power_str = String(current_tx_power, 1) + "dBm || " + String(battery_percentage) + "%";
    } else {
        tx_power_str = String(current_tx_power, 1) + "dBm";
    }

    String tx_frequency_str = String(current_tx_frequency, 4) + " MHz";
    String status_str;

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
        default:
            status_str = "Unknown";
            break;
    }

    display.clearBuffer();

    display.setFont(FONT_BANNER);
    int banner_width = display.getStrWidth(settings.banner_message);
    int banner_x = (display.getWidth() - banner_width) / 2;
    display.drawStr(banner_x, BANNER_HEIGHT, settings.banner_message);

    int status_start_y = BANNER_HEIGHT + BANNER_MARGIN + FONT_LINE_HEIGHT;

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
// FACTORY RESET DISPLAY
// =============================================================================
void display_factory_reset() {
    display.clearBuffer();
    display.setFont(u8g2_font_10x20_tr);

    String message = "FACTORY RESET...";
    int width = display.getStrWidth(message.c_str());
    int x = (display.getWidth() - width) / 2;

    display.drawStr(x, 32, message.c_str());
    display.sendBuffer();
}

// =============================================================================
// DISPLAY UPDATE (For Core 0 synchronization)
// =============================================================================
void display_update_if_needed() {
    if (display_update_requested) {
        reset_oled_timeout();
        display_status();
        display_update_requested = false;
    }
}
