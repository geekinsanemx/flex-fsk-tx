/*
 * FLEX Paging Message Transmitter - v2.5.1
 * COBS (Consistent Overhead Byte Stuffing) Implementation
 *
 * Reference: Stuart Cheshire and Mary Baker,
 *           "Consistent Overhead Byte Stuffing"
 *
 * COBS encoding eliminates 0x00 bytes from data stream,
 * allowing unambiguous frame delimiting with 0x00.
 */

#ifndef COBS_H
#define COBS_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

// =============================================================================
// COBS ENCODING/DECODING
// =============================================================================

/**
 * Encode data using COBS algorithm
 *
 * @param input      Input data buffer
 * @param length     Length of input data
 * @param output     Output buffer (must be length + length/254 + 2 bytes)
 * @return           Length of encoded data (including 0x00 delimiter)
 *
 * Output format: [COBS_ENCODED_DATA] 0x00
 *
 * Overhead: ~0.4% (1 byte per 254 bytes worst case)
 *
 * Example:
 *   Input:  [0x11, 0x22, 0x00, 0x33]
 *   Output: [0x03, 0x11, 0x22, 0x02, 0x33, 0x00]
 */
size_t cobs_encode(const uint8_t *input, size_t length, uint8_t *output);

/**
 * Decode COBS-encoded frame
 *
 * @param input      COBS-encoded data (including 0x00 delimiter)
 * @param length     Length of encoded data
 * @param output     Output buffer (must be >= length bytes)
 * @return           Length of decoded data (0 if decode failed)
 *
 * Input format: [COBS_ENCODED_DATA] 0x00
 *
 * Returns 0 if:
 * - Input is empty
 * - Last byte is not 0x00
 * - Decoding error detected
 *
 * Example:
 *   Input:  [0x03, 0x11, 0x22, 0x02, 0x33, 0x00]
 *   Output: [0x11, 0x22, 0x00, 0x33]
 */
size_t cobs_decode(const uint8_t *input, size_t length, uint8_t *output);

#endif // COBS_H
