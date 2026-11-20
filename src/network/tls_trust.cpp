/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * TLS Trust Module - server certificate verification for outbound HTTPS/IMAPS
 */

#include "../network/tls_trust.h"

extern const uint8_t x509_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t x509_crt_bundle_end[] asm("_binary_x509_crt_bundle_end");

static const unsigned long TLS_HANDSHAKE_TIMEOUT_S = 15;

void tls_configure_client(WiFiClientSecure& client) {
    client.setCACertBundle(x509_crt_bundle_start, x509_crt_bundle_end - x509_crt_bundle_start);
    client.setHandshakeTimeout(TLS_HANDSHAKE_TIMEOUT_S);
}
