/*
 * FLEX Paging Message Transmitter - v2.5.1
 * COBS Implementation
 */

#include "cobs.h"

// =============================================================================
// COBS ENCODER
// =============================================================================

size_t cobs_encode(const uint8_t *input, size_t length, uint8_t *output) {
    if (input == nullptr || output == nullptr) {
        return 0;
    }

    const uint8_t *src = input;
    uint8_t *dst = output;
    uint8_t *code_ptr = dst++;  // Reserve first byte for code
    uint8_t code = 0x01;

    for (size_t i = 0; i < length; i++) {
        if (*src == 0x00) {
            // Found zero byte
            *code_ptr = code;
            code_ptr = dst++;
            code = 0x01;
        } else {
            // Non-zero byte
            *dst++ = *src;
            code++;

            if (code == 0xFF) {
                // Maximum code value reached (254 non-zero bytes)
                *code_ptr = code;
                code_ptr = dst++;
                code = 0x01;
            }
        }
        src++;
    }

    // Write final code
    *code_ptr = code;

    // Add frame delimiter
    *dst++ = 0x00;

    return dst - output;
}

// =============================================================================
// COBS DECODER
// =============================================================================

size_t cobs_decode(const uint8_t *input, size_t length, uint8_t *output) {
    if (input == nullptr || output == nullptr || length == 0) {
        return 0;
    }

    // Check frame delimiter
    if (input[length - 1] != 0x00) {
        return 0;  // Invalid frame
    }

    const uint8_t *src = input;
    uint8_t *dst = output;
    size_t remaining = length - 1;  // Exclude delimiter

    while (remaining > 0) {
        uint8_t code = *src++;
        remaining--;

        if (code == 0x00) {
            // Invalid code (should never happen in valid COBS)
            return 0;
        }

        // Copy non-zero bytes
        for (uint8_t i = 1; i < code && remaining > 0; i++) {
            *dst++ = *src++;
            remaining--;
        }

        // Add zero byte if code < 0xFF (unless at end of data)
        if (code < 0xFF && remaining > 0) {
            *dst++ = 0x00;
        }
    }

    return dst - output;
}
