/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * NTP/RTC Time Module
 */

#ifndef NTP_TIME_H
#define NTP_TIME_H

#include <Arduino.h>
#include "../core/config.h"

#ifdef ENABLE_RTC
#include <RTClib.h>
#endif

// =============================================================================
// GLOBALS
// =============================================================================
#ifdef ENABLE_RTC
extern RTC_DS3231 rtc;
extern bool rtc_available;
#endif

extern bool system_time_initialized;
extern bool system_time_from_rtc;

extern unsigned long last_ntp_sync;
extern bool ntp_synced;
extern bool ntp_sync_in_progress;
extern unsigned long ntp_sync_last_attempt;
extern int ntp_sync_attempts;
extern const int NTP_MAX_ATTEMPTS;
extern const unsigned long NTP_SYNC_TIMEOUT_MS;
extern const unsigned long NTP_SYNC_INTERVAL_MS;

// =============================================================================
// FUNCTIONS
// =============================================================================
unsigned long getUnixTimestamp();
time_t getLocalTimestamp();

// Implemented in Batch 3
void ntp_sync_start();
void ntp_sync_process();
bool ntp_sync_time();
void rtc_sync_from_ntp();

#endif // NTP_TIME_H
