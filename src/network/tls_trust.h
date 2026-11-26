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

#endif // TLS_TRUST_H
