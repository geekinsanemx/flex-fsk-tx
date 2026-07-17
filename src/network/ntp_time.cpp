/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * NTP/RTC Time Module - non-blocking NTP sync, RTC mirroring, timestamp helpers
 */

#include "../network/ntp_time.h"
#include "../core/config.h"
#include "../core/storage.h"
#include "../core/logging.h"
#include "../core/hardware.h"
#include "../protocol/transmission.h"

#ifdef ENABLE_RTC
RTC_DS3231 rtc;
bool rtc_available = false;
#endif

bool system_time_initialized = false;
bool system_time_from_rtc = false;
static bool system_time_from_ntp = false;

unsigned long last_ntp_sync = 0;
bool ntp_synced = false;
bool ntp_sync_in_progress = false;
unsigned long ntp_sync_last_attempt = 0;
int ntp_sync_attempts = 0;
const int NTP_MAX_ATTEMPTS = 10;
const unsigned long NTP_SYNC_TIMEOUT_MS = 1000;
const unsigned long NTP_SYNC_INTERVAL_MS = 3600000UL;

static device_state_t ntp_previous_state = STATE_IDLE;
static bool ntp_state_active = false;

static inline void restore_ntp_state() {
    if (ntp_state_active) {
        change_device_state(ntp_previous_state);
        ntp_state_active = false;
    }
}

void ntp_sync_start() {
    if (ntp_sync_in_progress) {
        return;
    }

    logMessage("NTP: Starting non-blocking time synchronization...");

    String ntp_server1 = (strlen(settings.ntp_server) > 0) ? String(settings.ntp_server) : "pool.ntp.org";

    if (!ntp_state_active) {
        ntp_previous_state = device_state;
    }
    change_device_state(STATE_NTP_SYNC);
    ntp_state_active = true;

    delay(100);
    configTime(0, 0, ntp_server1.c_str(), "time.nist.gov", "216.239.35.4");
    delay(500);

    feed_watchdog();

    ntp_sync_in_progress = true;
    ntp_sync_last_attempt = 0;
    ntp_sync_attempts = 0;

    logMessagef("NTP: Using server: %s", ntp_server1.c_str());
}

void ntp_sync_process() {
    if (!ntp_sync_in_progress) {
        return;
    }

    if (millis() - ntp_sync_last_attempt < NTP_SYNC_TIMEOUT_MS) {
        return;
    }


    feed_watchdog();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        time_t now;
        time(&now);
        ntp_synced = true;
        last_ntp_sync = millis();
        ntp_sync_in_progress = false;
        system_time_initialized = true;
        system_time_from_ntp = true;

        restore_ntp_state();

        logMessagef("NTP: Time synchronized! Attempts: %d, Timestamp: %ld", ntp_sync_attempts + 1, (long)now);
        logMessagef("NTP: Current time: %s", asctime(&timeinfo));

#ifdef ENABLE_RTC
        rtc_sync_from_ntp();
#endif

    } else {
        ntp_sync_attempts++;
        ntp_sync_last_attempt = millis();
        logMessagef("NTP: Sync attempt %d/%d...", ntp_sync_attempts, NTP_MAX_ATTEMPTS);

        if (ntp_sync_attempts >= NTP_MAX_ATTEMPTS) {
            logMessage("NTP: Time sync failed after maximum attempts");
            ntp_synced = false;
            ntp_sync_in_progress = false;

            restore_ntp_state();
        }
    }
}

bool ntp_sync_time() {
    ntp_sync_start();

    for (int i = 0; i < 3 && ntp_sync_in_progress; i++) {
        ntp_sync_process();
        yield();
        delay(10);
    }

    return ntp_synced;
}

#ifdef ENABLE_RTC
void rtc_sync_from_ntp() {
    if (!rtc_available) {
        return;
    }

    time_t now;
    time(&now);
    rtc.adjust(DateTime(now));
    logMessage("RTC: Updated from NTP sync");
}
#endif

unsigned long getUnixTimestamp() {
    time_t now;
    time(&now);
    return (unsigned long)now;
}

time_t getLocalTimestamp() {
    time_t now;
    time(&now);
    return now + (long)(settings.timezone_offset_hours * 3600);
}
