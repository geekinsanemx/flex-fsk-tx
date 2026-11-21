/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * TLS Trust Module - server certificate verification for outbound HTTPS/IMAPS
 */

#ifndef TLS_TRUST_H
#define TLS_TRUST_H

#include <WiFiClientSecure.h>

// =============================================================================
// FUNCTIONS
// =============================================================================

// Verifies server certificates against the ESP-IDF Mozilla root CA bundle and bounds the TLS handshake time.
void tls_configure_client(WiFiClientSecure& client);

// Checks that enough heap is free for a TLS handshake with certificate verification; fills `error` when not.
bool tls_memory_available(String& error);

#endif // TLS_TRUST_H
