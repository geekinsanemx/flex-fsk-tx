/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * TLS Trust Module - server certificate verification for outbound HTTPS/IMAPS
 */

#include "../network/tls_trust.h"
#include "../core/config.h"
#include <esp_heap_caps.h>

extern const uint8_t x509_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t x509_crt_bundle_end[] asm("_binary_x509_crt_bundle_end");

static const unsigned long TLS_HANDSHAKE_TIMEOUT_S = 15;

void tls_configure_client(WiFiClientSecure& client) {
    client.setCACertBundle(x509_crt_bundle_start, x509_crt_bundle_end - x509_crt_bundle_start);
    client.setHandshakeTimeout(TLS_HANDSHAKE_TIMEOUT_S);
}

bool tls_memory_available(String& error) {
    size_t free_heap = heap_caps_get_free_size(TLS_HEAP_CAPS);
    size_t largest_block = heap_caps_get_largest_free_block(TLS_HEAP_CAPS);
    if (free_heap >= TLS_MIN_FREE_HEAP && largest_block >= TLS_MIN_LARGEST_BLOCK) {
        return true;
    }
    error = "Insufficient memory for TLS (free " + String((unsigned)free_heap) + " bytes, largest block " +
            String((unsigned)largest_block) + " bytes)";
    return false;
}
