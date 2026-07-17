/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Display Module - SSD1306 OLED status/AP-info screens
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>

// =============================================================================
// GLOBALS
// =============================================================================
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;

extern unsigned long last_activity_time;
extern bool oled_active;

// =============================================================================
// FUNCTIONS
// =============================================================================
void display_setup();
void display_turn_on();
void display_turn_off();
void reset_oled_timeout();
void display_status();
void display_ap_info();
void display_panic();

#endif // DISPLAY_H
