/*
 * FLEX Paging Message Transmitter - v2.5
 * Logging System Implementation
 */

#include "logging.h"
#include "config.h"
#include <SPIFFS.h>
#include <time.h>

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
static char log_buffer[LOG_BUFFER_SIZE];
static size_t log_buffer_len = 0;
static uint32_t log_last_flush_ms = 0;

extern bool system_time_initialized;
extern float timezone_offset_hours;

// =============================================================================
// INITIALIZATION
// =============================================================================
void logging_init() {
    log_buffer_len = 0;
    log_last_flush_ms = millis();
    memset(log_buffer, 0, LOG_BUFFER_SIZE);
}

// =============================================================================
// TIMESTAMP FORMATTING
// =============================================================================
String get_timestamp() {
    char timestamp[32];

    if (system_time_initialized) {
        time_t now;
        time(&now);
        now += (int)(timezone_offset_hours * 3600);
        struct tm* timeinfo = gmtime(&now);

        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo->tm_year + 1900,
                 timeinfo->tm_mon + 1,
                 timeinfo->tm_mday,
                 timeinfo->tm_hour,
                 timeinfo->tm_min,
                 timeinfo->tm_sec);
    } else {
        unsigned long uptime = millis() / 1000;
        unsigned long hours = uptime / 3600;
        unsigned long minutes = (uptime % 3600) / 60;
        unsigned long seconds = uptime % 60;

        snprintf(timestamp, sizeof(timestamp), "0000-00-00 %02lu:%02lu:%02lu",
                 hours, minutes, seconds);
    }

    return String(timestamp);
}

// =============================================================================
// CORE LOGGING FUNCTIONS
// =============================================================================
void logMessage(const char* message) {
    String timestamped = get_timestamp() + " " + String(message);

    Serial.println(timestamped);

    append_to_log_file(timestamped.c_str());
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
// SPIFFS FILE MANAGEMENT
// =============================================================================
void append_to_log_file(const char* message) {
    size_t msg_len = strlen(message);

    if (log_buffer_len + msg_len + 1 >= LOG_BUFFER_SIZE) {
        flush_log_buffer_to_spiffs();
    }

    if (msg_len < LOG_BUFFER_SIZE - log_buffer_len - 1) {
        strcpy(log_buffer + log_buffer_len, message);
        log_buffer_len += msg_len;
        log_buffer[log_buffer_len++] = '\n';
        log_buffer[log_buffer_len] = '\0';
    }

    if (log_buffer_len >= LOG_FLUSH_THRESHOLD) {
        flush_log_buffer_to_spiffs();
    }
}

void flush_log_buffer_to_spiffs() {
    if (log_buffer_len == 0) return;

    File file = SPIFFS.open(LOG_FILE_PATH, FILE_APPEND);
    if (!file) {
        log_buffer_len = 0;
        return;
    }

    file.write((const uint8_t*)log_buffer, log_buffer_len);
    file.flush();
    file.close();

    log_buffer_len = 0;
    log_last_flush_ms = millis();

    if (SPIFFS.exists(LOG_FILE_PATH)) {
        File check_file = SPIFFS.open(LOG_FILE_PATH, "r");
        if (check_file) {
            size_t file_size = check_file.size();
            check_file.close();

            if (file_size > MAX_LOG_FILE_SIZE) {
                trim_log_file();
            }
        }
    }
}

void flush_log_buffer_if_due() {
    if (log_buffer_len > 0 && (millis() - log_last_flush_ms) >= LOG_FLUSH_INTERVAL_MS) {
        flush_log_buffer_to_spiffs();
    }
}

void trim_log_file() {
    if (!SPIFFS.exists(LOG_FILE_PATH)) return;

    File file = SPIFFS.open(LOG_FILE_PATH, "r");
    if (!file) return;

    size_t file_size = file.size();
    if (file_size <= MAX_LOG_FILE_SIZE) {
        file.close();
        return;
    }

    size_t bytes_to_skip = file_size - LOG_TRUNCATE_SIZE;
    file.seek(bytes_to_skip, SeekSet);

    while (file.available() && file.read() != '\n') {
    }

    String kept_content;
    kept_content.reserve(LOG_TRUNCATE_SIZE);

    while (file.available()) {
        kept_content += (char)file.read();
    }
    file.close();

    file = SPIFFS.open(LOG_FILE_PATH, "w");
    if (file) {
        file.print(kept_content);
        file.flush();
        file.close();
    }
}

// =============================================================================
// LOG RETRIEVAL
// =============================================================================
String read_log_tail(int max_lines) {
    if (!SPIFFS.exists(LOG_FILE_PATH)) {
        return "No log file found\n";
    }

    File file = SPIFFS.open(LOG_FILE_PATH, "r");
    if (!file) {
        return "Failed to open log file\n";
    }

    String content;
    content.reserve(LOG_BUFFER_SIZE);

    while (file.available()) {
        content += (char)file.read();
    }
    file.close();

    if (content.length() == 0) {
        return "Log file is empty\n";
    }

    int line_count = 0;
    for (int i = 0; i < content.length(); i++) {
        if (content[i] == '\n') line_count++;
    }

    if (line_count <= max_lines) {
        return content;
    }

    int lines_to_skip = line_count - max_lines;
    int skip_count = 0;
    int start_pos = 0;

    for (int i = 0; i < content.length(); i++) {
        if (content[i] == '\n') {
            skip_count++;
            if (skip_count >= lines_to_skip) {
                start_pos = i + 1;
                break;
            }
        }
    }

    return content.substring(start_pos);
}

bool delete_log_file() {
    if (!SPIFFS.exists(LOG_FILE_PATH)) {
        return false;
    }

    return SPIFFS.remove(LOG_FILE_PATH);
}
