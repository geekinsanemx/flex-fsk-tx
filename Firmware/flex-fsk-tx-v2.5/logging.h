/*
 * FLEX Paging Message Transmitter - v2.5
 * Logging System Module
 *
 * Persistent logging to Serial + SPIFFS with RAM buffering
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

void logging_init();

void logMessage(const char* message);
void logMessage(const String& message);
void logMessagef(const char* format, ...);

void append_to_log_file(const char* message);
void flush_log_buffer_to_spiffs();
void flush_log_buffer_if_due();
void trim_log_file();

String read_log_tail(int max_lines);
bool delete_log_file();

#endif // LOGGING_H
