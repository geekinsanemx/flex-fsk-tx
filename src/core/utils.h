/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Utils Module - base64, HTML/JSON escaping, CRC32, IP string helpers
 */

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

// =============================================================================
// FUNCTIONS
// =============================================================================
String base64_encode_string(const String& input);
String base64_decode_string(const String& encoded_string);
String base64_decode(String input);

String htmlEscape(const String& str);
String json_escape_string(String input);

String calculate_crc32(const String& input);

String ip_array_to_string(uint8_t ip[4]);
void string_to_ip_array(const String& ip_str, uint8_t ip[4]);
void parse_ip_string(const String& ip_str, uint8_t ip[4]);

String get_cert_status(const char* saved_cert);
bool has_valid_certificate(const char* cert);

#endif // UTILS_H
