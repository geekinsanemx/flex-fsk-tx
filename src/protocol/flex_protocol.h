/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * FLEX Protocol Module - tinyflex encoding, EMR framing, capcode validation, message sanitization
 */

#ifndef FLEX_PROTOCOL_H
#define FLEX_PROTOCOL_H

#include <Arduino.h>

// =============================================================================
// FUNCTIONS
// =============================================================================
float apply_frequency_correction(float base_freq);
bool validate_flex_capcode(uint64_t capcode);
String truncate_message_with_ellipsis(String message);
String convert_unicode_to_ascii(String message);

bool flex_encode_and_store(uint64_t capcode, const char *message, bool mail_drop);
void send_emr_if_needed();

#endif // FLEX_PROTOCOL_H
