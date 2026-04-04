/*
 * FLEX Paging Message Transmitter - v2.5
 * Utility Functions Module
 *
 * Helper functions for string conversion, validation, etc.
 */

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

// =============================================================================
// STRING CONVERSION
// =============================================================================
int str2uint64(uint64_t *out, const char *s);
float apply_frequency_correction(float base_freq);

// =============================================================================
// VALIDATION
// =============================================================================
bool validate_flex_capcode(uint64_t capcode);
bool is_reserved_pin(uint8_t pin);

// =============================================================================
// MESSAGE PROCESSING
// =============================================================================
String truncate_message_with_ellipsis(const String& message);

// =============================================================================
// TIME UTILITIES
// =============================================================================
String format_uptime(unsigned long milliseconds);

#endif // UTILS_H
