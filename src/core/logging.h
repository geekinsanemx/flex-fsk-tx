/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Logging Module - Serial/SPIFFS log ring buffer, syslog forwarding
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

// =============================================================================
// SYSLOG
// =============================================================================
uint8_t detectSeverity(const String& message);
String formatSyslogMessage(const String& message, uint8_t severity);
void sendSyslog(const String& message);

// =============================================================================
// LOGGING
// =============================================================================
void logMessage(const char* message);
void logMessage(const String& message);
void logMessagef(const char* format, ...);

// =============================================================================
// SPIFFS LOG FILE
// =============================================================================
void flush_log_buffer_to_spiffs();
void flush_log_buffer_if_due();
void trim_log_file();
void append_to_log_file(const char* message);
String read_log_tail(int max_lines);

#endif // LOGGING_H
