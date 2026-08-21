#include "../core/logging.h"

#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <stdarg.h>

#include "../core/config.h"
#include "../core/storage.h"
#include "../core/tx_lock.h"
#include "../network/wifi.h"
#include "../network/ntp_time.h"

static char     log_buffer[LOG_BUFFER_SIZE];
static size_t   log_buffer_len    = 0;
static uint32_t log_last_flush_ms = 0;
static uint32_t log_lines_dropped = 0;

// =============================================================================
// SYSLOG
// =============================================================================
uint8_t detectSeverity(const String& message) {
    if (message.startsWith("ERROR:") || message.startsWith("FATAL:")) return 3;
    if (message.startsWith("WARN:") || message.startsWith("WARNING:")) return 4;
    if (message.startsWith("SYSTEM:") || message.startsWith("CRITICAL:")) return 2;
    if (message.startsWith("IMAP:") || message.startsWith("MQTT:")) return 5;
    if (message.startsWith("DEBUG:")) return 7;
    return 6;
}

String formatSyslogMessage(const String& message, uint8_t severity) {
    uint8_t priority = (16 * 8) + severity;

    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timestamp[16];
    strftime(timestamp, sizeof(timestamp), "%b %d %H:%M:%S", &timeinfo);

    String hostname = String(settings.banner_message);
    if (hostname.length() == 0) hostname = "FLEX";

    return "<" + String(priority) + ">" + String(timestamp) + " " + hostname + " " + mac_suffix + ": " + message;
}

void sendSyslog(const String& message) {
    if (!settings.rsyslog_enabled) return;
    if (strlen(settings.rsyslog_server) == 0) return;
    if (!wifi_connected) return;

    uint8_t severity = detectSeverity(message);
    if (severity > settings.rsyslog_min_severity) return;

    String syslogMsg = formatSyslogMessage(message, severity);

    if (settings.rsyslog_use_tcp) {
        WiFiClient client;
        if (client.connect(settings.rsyslog_server, settings.rsyslog_port)) {
            client.print(syslogMsg);
            client.stop();
        }
    } else {
        WiFiUDP udp;
        udp.beginPacket(settings.rsyslog_server, settings.rsyslog_port);
        udp.print(syslogMsg);
        udp.endPacket();
    }
}

// =============================================================================
// LOG RING BUFFER
// =============================================================================
static size_t count_lines(const char* start, size_t len) {
    size_t lines = 0;
    for (size_t i = 0; i < len; i++) {
        if (start[i] == '\n') lines++;
    }
    return lines;
}

// Caller must hold log_guard. Returns true when a SPIFFS flush is due.
static bool buffer_log_line(const char* message) {
    char logLine[256];

    if (!system_time_initialized) {
        unsigned long uptime_seconds = millis() / 1000;
        unsigned long hours = (uptime_seconds / 3600) % 24;
        unsigned long minutes = (uptime_seconds / 60) % 60;
        unsigned long seconds = uptime_seconds % 60;
        snprintf(logLine, sizeof(logLine), "0000-00-00 %02lu:%02lu:%02lu %s\n",
                 hours, minutes, seconds, message);
    } else {
        time_t now = getLocalTimestamp();
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        snprintf(logLine, sizeof(logLine), "%04d-%02d-%02d %02d:%02d:%02d %s\n",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, message);
    }

    size_t lineLen = strlen(logLine);
    if (lineLen == 0 || lineLen >= LOG_BUFFER_SIZE) {
        log_lines_dropped++;
        return false;
    }

    // The buffer can only be flushed when no RF transmission holds the flash
    // guard. While it is held the buffer keeps filling, so make room by
    // discarding whole lines from the oldest end instead of losing the newest.
    if (log_buffer_len + lineLen >= LOG_BUFFER_SIZE) {
        size_t drop = (log_buffer_len + lineLen) - LOG_BUFFER_SIZE + 1;
        while (drop < log_buffer_len && log_buffer[drop - 1] != '\n') drop++;

        if (drop >= log_buffer_len) {
            log_lines_dropped += count_lines(log_buffer, log_buffer_len);
            log_buffer_len = 0;
        } else {
            log_lines_dropped += count_lines(log_buffer, drop);
            memmove(log_buffer, log_buffer + drop, log_buffer_len - drop);
            log_buffer_len -= drop;
        }
    }

    memcpy(log_buffer + log_buffer_len, logLine, lineLen);
    log_buffer_len += lineLen;

    return log_buffer_len >= LOG_FLUSH_THRESHOLD;
}

// =============================================================================
// LOGGING
// =============================================================================
void logMessage(const char* message) {
    bool flush_due = false;

    if (log_guard_take(LOG_GUARD_TIMEOUT_MS)) {
        Serial.println(message);
        flush_due = buffer_log_line(message);
        log_guard_give();
    }

    if (flush_due) {
        flush_log_buffer_to_spiffs();
    }

    sendSyslog(String(message));
}

void logMessage(const String& message) {
    logMessage(message.c_str());
}

void logMessagef(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[200];
    vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[sizeof(buffer) - 1] = '\0';

    va_end(args);

    logMessage(buffer);
}

// =============================================================================
// SPIFFS LOG FILE
// =============================================================================
void flush_log_buffer_to_spiffs() {
    if (log_buffer_len == 0) return;

    // Never block here: this runs on Core 1 and the guard is held for the whole
    // RF-active window. Skipping keeps the lines buffered for the next attempt.
    if (!flash_guard_try()) return;

    if (!log_guard_take(LOG_GUARD_TIMEOUT_MS)) {
        flash_guard_give();
        return;
    }

    if (log_buffer_len > 0) {
        File file = SPIFFS.open("/serial.log", "a");
        if (file) {
            file.write((const uint8_t*)log_buffer, log_buffer_len);
            file.close();

            File rf = SPIFFS.open("/serial.log", "r");
            if (rf) {
                size_t fileSize = rf.size();
                rf.close();
                if (fileSize > MAX_LOG_FILE_SIZE) {
                    trim_log_file();
                }
            }
        }

        log_buffer_len = 0;
    }

    log_last_flush_ms = millis();

    uint32_t dropped  = log_lines_dropped;
    log_lines_dropped = 0;

    log_guard_give();
    flash_guard_give();

    if (dropped > 0) {
        logMessagef("LOG: %lu line(s) dropped while the RF transmission guard was held",
                    (unsigned long)dropped);
    }
}

void flush_log_buffer_if_due() {
    if (log_buffer_len > 0 && (millis() - log_last_flush_ms) >= LOG_FLUSH_INTERVAL_MS) {
        flush_log_buffer_to_spiffs();
    }
}

void trim_log_file() {
    if (!flash_guard_take(FLASH_GUARD_TIMEOUT_MS)) return;

    File file = SPIFFS.open("/serial.log", "r");
    if (!file) {
        flash_guard_give();
        return;
    }

    size_t fileSize = file.size();
    if (fileSize <= MAX_LOG_FILE_SIZE) {
        file.close();
        flash_guard_give();
        return;
    }

    size_t keepPosition = fileSize - LOG_TRUNCATE_SIZE;
    file.seek(keepPosition);

    while (file.available() && file.read() != '\n');

    File tmpFile = SPIFFS.open("/serial.tmp", "w");
    uint8_t buffer[512];

    while (file.available()) {
        size_t len = file.readBytes((char*)buffer, 512);
        tmpFile.write(buffer, len);
        yield();
    }

    file.close();
    tmpFile.close();

    SPIFFS.remove("/serial.log");
    SPIFFS.rename("/serial.tmp", "/serial.log");

    Serial.printf("LOG: File rotated - kept last %d KB from %d KB total\n",
                  LOG_TRUNCATE_SIZE / 1024, fileSize / 1024);

    flash_guard_give();
}

void append_to_log_file(const char* message) {
    bool flush_due = false;

    if (log_guard_take(LOG_GUARD_TIMEOUT_MS)) {
        flush_due = buffer_log_line(message);
        log_guard_give();
    }

    if (flush_due) {
        flush_log_buffer_to_spiffs();
    }
}

String read_log_tail(int max_lines) {
    if (!flash_guard_take(FLASH_GUARD_TIMEOUT_MS)) return "";

    File file = SPIFFS.open("/serial.log", "r");
    if (!file) {
        flash_guard_give();
        return "";
    }

    size_t fileSize = file.size();
    if (fileSize == 0) {
        file.close();
        flash_guard_give();
        return "";
    }

    size_t readSize = min((size_t)(max_lines * 100), fileSize);
    size_t startPos = fileSize - readSize;

    file.seek(startPos);
    if (startPos > 0) {
        while (file.available() && file.read() != '\n');
    }

    String content = "";
    while (file.available()) {
        content += file.readStringUntil('\n') + "\n";
    }
    file.close();

    flash_guard_give();

    int lineCount = 0;

    for (int i = content.length() - 1; i >= 0; i--) {
        if (content[i] == '\n') {
            lineCount++;
            if (lineCount == max_lines) {
                return content.substring(i + 1);
            }
        }
    }

    return content;
}
