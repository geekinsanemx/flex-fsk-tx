/*
 * FLEX Paging Message Transmitter - v2.5
 * Hardware Abstraction Module
 *
 * Radio (SX1276), OLED, Battery, RTC, RF Amplifier
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <RadioLib.h>
#include <U8g2lib.h>

#if RTC_ENABLED
#include <RTClib.h>
#endif

// =============================================================================
// GLOBAL HARDWARE INSTANCES
// =============================================================================
extern SX1276 radio;
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;

#if RTC_ENABLED
extern RTC_DS3231 rtc;
extern bool rtc_available;
#endif

// =============================================================================
// RADIO MANAGEMENT
// =============================================================================
bool radio_init(float frequency, float power);
void radio_standby();
int radio_set_frequency(float freq);
int radio_set_power(float power);
void radio_set_fifo_callback(void (*callback)(void));

// =============================================================================
// OLED DISPLAY
// =============================================================================
void display_init();
void display_turn_on();
void display_turn_off();
void reset_oled_timeout();

extern bool oled_active;
extern unsigned long last_activity_time;

// =============================================================================
// BATTERY MONITORING
// =============================================================================
void battery_init();
void getBatteryInfo(uint16_t *voltage_mv, int *percentage);
float readBatteryVoltage();

extern bool battery_present;

// =============================================================================
// RTC (DS3231)
// =============================================================================
#if RTC_ENABLED
bool rtc_init();
#endif

extern bool system_time_initialized;
extern bool system_time_from_rtc;
extern float timezone_offset_hours;

// =============================================================================
// RF AMPLIFIER
// =============================================================================
void rfamp_init();
void rfamp_enable();
void rfamp_disable();

// =============================================================================
// VEXT POWER CONTROL (Heltec)
// =============================================================================
void VextON();
void VextOFF();

// =============================================================================
// HEARTBEAT LED
// =============================================================================
void led_heartbeat_init();
void led_heartbeat_update();

extern unsigned long last_heartbeat;
extern bool heartbeat_state;
extern int heartbeat_blink_count;

// =============================================================================
// WATCHDOG TIMER
// =============================================================================
void watchdog_init();
void watchdog_feed();

#endif // HARDWARE_H
