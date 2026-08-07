/*
 * pocsag.h - Single-header POCSAG encoder library for ESP32
 *
 * Extracted from rpitx POCSAG implementation (MIT License)
 * Adapted for ESP32 microcontrollers
 *
 * POCSAG Protocol Specifications:
 * - Modulation: 2-FSK
 * - Baud Rates: 512, 1200, 2400 bps
 * - Frequency Deviation: ±4.5 kHz
 * - Error Correction: CRC-10 + even parity
 * - Frame Structure: 16-word batches with SYNC words
 * - Character Encoding: 7-bit ASCII (alphanumeric) or BCD (numeric)
 *
 * Original Source: https://github.com/F5OEO/rpitx/blob/master/src/pocsag/pocsag.cpp
 * License: MIT
 */

#ifndef POCSAG_H
#define POCSAG_H

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// POCSAG PROTOCOL CONSTANTS
// =============================================================================

#define POCSAG_SYNC         0x7CD215D8  // Synchronization codeword
#define POCSAG_IDLE         0x7A89C197  // Idle codeword
#define POCSAG_BATCH_SIZE   16          // Words per batch (excluding SYNC)
#define POCSAG_FRAME_SIZE   2           // Words per frame
#define POCSAG_CRC_BITS     10          // CRC-10 error correction
#define POCSAG_CRC_GENERATOR 0b11101101001  // CRC polynomial

#define POCSAG_PREAMBLE_LENGTH 576      // Preamble bits (576 bits = 72 bytes)
#define POCSAG_TEXT_BITS_PER_WORD 20    // Message data bits per word
#define POCSAG_TEXT_BITS_PER_CHAR 7     // ASCII character width

#define POCSAG_MAX_MESSAGE_CODEWORDS 256  // Maximum codewords per message
#define POCSAG_BUFFER_SIZE (POCSAG_MAX_MESSAGE_CODEWORDS * 4)  // 4 bytes per codeword

// POCSAG baud rates
#define POCSAG_BAUD_512     512
#define POCSAG_BAUD_1200    1200
#define POCSAG_BAUD_2400    2400

// Message flags
#define POCSAG_FLAG_ADDRESS 0x000000  // Address word flag (bit 20 = 0)
#define POCSAG_FLAG_MESSAGE 0x100000  // Message word flag (bit 20 = 1)

// Function bits (message type)
#define POCSAG_FUNCTION_TONE         0  // Tone only (no message data)
#define POCSAG_FUNCTION_NUMERIC      1  // Numeric message (BCD encoding)
#define POCSAG_FUNCTION_ALPHANUMERIC 3  // Text message (ASCII)

// POCSAG numeric BCD encoding table
#define POCSAG_BCD_BITS_PER_CHAR 4      // 4 bits per BCD character

// =============================================================================
// POCSAG ERROR CORRECTION FUNCTIONS
// =============================================================================

/**
 * @brief Calculate CRC-10 checksum for POCSAG message
 *
 * Uses polynomial division with generator 0b11101101001
 *
 * @param inputMsg 21-bit message to calculate CRC for
 * @return 10-bit CRC checksum
 */
static inline uint32_t pocsag_crc(uint32_t inputMsg) {
    uint32_t denominator = POCSAG_CRC_GENERATOR << 20;
    uint32_t msg = inputMsg << POCSAG_CRC_BITS;

    for (int column = 0; column <= 20; column++) {
        int msgBit = (msg >> (30 - column)) & 1;
        if (msgBit != 0) {
            msg ^= denominator;
        }
        denominator >>= 1;
    }

    return msg & 0x3FF;  // Return 10-bit CRC
}

/**
 * @brief Calculate even parity bit for 31-bit word
 *
 * @param x Input word (31 bits)
 * @return Parity bit (0 or 1)
 */
static inline uint32_t pocsag_parity(uint32_t x) {
    uint32_t p = 0;
    for (int i = 0; i < 32; i++) {
        p ^= (x & 1);
        x >>= 1;
    }
    return p;
}

/**
 * @brief Encode 21-bit message into 32-bit POCSAG codeword
 *
 * Format: [21-bit message][10-bit CRC][1-bit parity]
 *
 * @param msg 21-bit message to encode
 * @return 32-bit encoded codeword with CRC and parity
 */
static inline uint32_t pocsag_encode_codeword(uint32_t msg) {
    uint32_t fullCRC = (msg << POCSAG_CRC_BITS) | pocsag_crc(msg);
    uint32_t parity = pocsag_parity(fullCRC);
    return (fullCRC << 1) | parity;
}

// =============================================================================
// POCSAG ADDRESS FUNCTIONS
// =============================================================================

/**
 * @brief Calculate address offset (padding) for batch alignment
 *
 * POCSAG addresses must align to specific frame positions within a batch
 * based on (address & 7) * 2 (since FRAME_SIZE = 2 words)
 *
 * @param address Full POCSAG address
 * @return Number of idle codewords needed for alignment (0, 2, 4, 6, 8, 10, 12, or 14)
 */
static inline int pocsag_address_offset(uint32_t address) {
    return (address & 0x7) * POCSAG_FRAME_SIZE;
}

// =============================================================================
// POCSAG MESSAGE ENCODING FUNCTIONS
// =============================================================================

/**
 * @brief Encode ASCII text message into POCSAG alphanumeric codewords
 *
 * Supports both LSB-first and MSB-first bit ordering.
 * LSB-first: Bits encoded in reverse order (matches rpitx implementation)
 * MSB-first: Bits encoded in normal order (for some commercial pagers)
 *
 * Encoding: 7-bit ASCII characters packed into 20-bit message words
 * Each codeword contains: [1 message flag][20-bit data][11-bit CRC+parity]
 *
 * @param initial_offset Starting word position in current batch
 * @param str ASCII text to encode (null-terminated)
 * @param msb_first If true, use MSB-first bit order; if false, use LSB-first
 * @param out Output array for encoded codewords
 * @return Number of codewords generated (including interleaved SYNC words)
 */
static inline uint32_t pocsag_encode_ascii(uint32_t initial_offset, const char *str, bool msb_first, uint32_t *out) {
    uint32_t num_words_written = 0;
    uint32_t current_word = 0;
    uint32_t current_num_bits = 0;
    uint32_t word_position = initial_offset;

    while (*str != 0) {
        unsigned char c = *str;
        str++;

        for (int i = 0; i < POCSAG_TEXT_BITS_PER_CHAR; i++) {
            current_word <<= 1;
            if (msb_first) {
                current_word |= (c >> (6 - i)) & 1;
            } else {
                current_word |= (c >> i) & 1;
            }
            current_num_bits++;

            if (current_num_bits == POCSAG_TEXT_BITS_PER_WORD) {
                // Encode complete word with MESSAGE flag
                *out = pocsag_encode_codeword(current_word | POCSAG_FLAG_MESSAGE);
                out++;
                current_word = 0;
                current_num_bits = 0;
                num_words_written++;

                word_position++;
                if (word_position == POCSAG_BATCH_SIZE) {
                    // Batch full - insert SYNC and start new batch
                    *out = POCSAG_SYNC;
                    out++;
                    num_words_written++;
                    word_position = 0;
                }
            }
        }
    }

    // Write remainder of message (pad with zeros)
    if (current_num_bits > 0) {
        current_word <<= (POCSAG_TEXT_BITS_PER_WORD - current_num_bits);
        *out = pocsag_encode_codeword(current_word | POCSAG_FLAG_MESSAGE);
        out++;
        num_words_written++;

        word_position++;
        if (word_position == POCSAG_BATCH_SIZE) {
            *out = POCSAG_SYNC;
            out++;
            num_words_written++;
            word_position = 0;
        }
    }

    return num_words_written;
}

/**
 * @brief Convert character to POCSAG BCD code
 *
 * BCD encoding (4 bits per character):
 * 0-9: 0x0-0x9, Space: 0xA, U: 0xB, -: 0xC, [: 0xD, ]: 0xE, Reserved: 0xF
 *
 * @param c Character to encode
 * @return 4-bit BCD code (0xF for invalid characters)
 */
static inline uint8_t pocsag_char_to_bcd(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    switch (c) {
        case ' ': return 0xA;
        case 'U': case 'u': return 0xB;
        case '-': return 0xC;
        case '[': case '(': return 0xD;
        case ']': case ')': return 0xE;
        default: return 0xF;
    }
}

/**
 * @brief Encode numeric message into POCSAG BCD codewords
 *
 * BCD encoding: 4-bit characters packed into 20-bit message words
 * Each codeword contains: [1 message flag][20-bit data][11-bit CRC+parity]
 *
 * @param initial_offset Starting word position in current batch
 * @param str Numeric string to encode (null-terminated)
 * @param out Output array for encoded codewords
 * @return Number of codewords generated (including interleaved SYNC words)
 */
static inline uint32_t pocsag_encode_numeric(uint32_t initial_offset, const char *str, uint32_t *out) {
    uint32_t num_words_written = 0;
    uint32_t current_word = 0;
    uint32_t current_num_bits = 0;
    uint32_t word_position = initial_offset;

    while (*str != 0) {
        uint8_t bcd = pocsag_char_to_bcd(*str);
        str++;

        for (int i = 0; i < POCSAG_BCD_BITS_PER_CHAR; i++) {
            current_word <<= 1;
            current_word |= (bcd >> i) & 1;
            current_num_bits++;

            if (current_num_bits == POCSAG_TEXT_BITS_PER_WORD) {
                *out = pocsag_encode_codeword(current_word | POCSAG_FLAG_MESSAGE);
                out++;
                current_word = 0;
                current_num_bits = 0;
                num_words_written++;

                word_position++;
                if (word_position == POCSAG_BATCH_SIZE) {
                    *out = POCSAG_SYNC;
                    out++;
                    num_words_written++;
                    word_position = 0;
                }
            }
        }
    }

    if (current_num_bits > 0) {
        // Pad remaining nibbles with 0xC (space/reserved character) using LSB-first encoding
        // This prevents trailing "0000" or "3333" digits from appearing on numeric pagers
        int remaining_nibbles = (POCSAG_TEXT_BITS_PER_WORD - current_num_bits) / POCSAG_BCD_BITS_PER_CHAR;
        uint8_t padding_bcd = 0xC;  // Reserved/space character
        for (int pad = 0; pad < remaining_nibbles; pad++) {
            // Encode padding nibble LSB-first (same as digit encoding above)
            for (int i = 0; i < POCSAG_BCD_BITS_PER_CHAR; i++) {
                current_word <<= 1;
                current_word |= (padding_bcd >> i) & 1;
            }
        }
        *out = pocsag_encode_codeword(current_word | POCSAG_FLAG_MESSAGE);
        out++;
        num_words_written++;

        word_position++;
        if (word_position == POCSAG_BATCH_SIZE) {
            *out = POCSAG_SYNC;
            out++;
            num_words_written++;
            word_position = 0;
        }
    }

    return num_words_written;
}

// =============================================================================
// POCSAG TRANSMISSION ENCODING
// =============================================================================

/**
 * @brief Encode complete POCSAG transmission
 *
 * Transmission structure (per rpitx implementation):
 * 1. Preamble: 576 bits of 0xAA pattern (only on first repeat, repeatIndex==0)
 * 2. SYNC word
 * 3. Padding IDLE words (for address alignment)
 * 4. Address codeword
 * 5. Message codewords (with SYNC interleaved every 16 words)
 * 6. IDLE word (end of message marker)
 * 7. Padding IDLE words (to fill batch boundary)
 *
 * @param repeatIndex Repeat number (0 = first, includes preamble)
 * @param address POCSAG address (full 21-bit address)
 * @param function Function bits (0-3, typically 3 for alphanumeric)
 * @param message Message text (ASCII)
 * @param msb_first If true, use MSB-first bit order; if false, use LSB-first
 * @param out Output buffer for encoded transmission
 * @return Total number of 32-bit words in transmission
 */
static inline size_t pocsag_encode_transmission(int repeatIndex, uint32_t address,
                                                 uint32_t function, const char *message,
                                                 bool msb_first, uint32_t *out) {
    uint32_t *start = out;

    // Preamble (only on first transmission, repeatIndex == 0)
    if (repeatIndex == 0) {
        for (int i = 0; i < POCSAG_PREAMBLE_LENGTH / 32; i++) {
            *out = 0xAAAAAAAA;  // Alternating 1010... pattern
            out++;
        }
    }

    uint32_t *batch_start = out;

    // SYNC word (start of first batch)
    *out = POCSAG_SYNC;
    out++;

    // Padding IDLE words for address alignment
    int prefix_length = pocsag_address_offset(address);
    for (int i = 0; i < prefix_length; i++) {
        *out = POCSAG_IDLE;
        out++;
    }

    // Address codeword
    // Format: [18-bit address (addr >> 3)][2-bit function][1-bit address flag (0)]
    uint32_t address_word = ((address >> 3) << 2) | (function & 0x3);
    *out = pocsag_encode_codeword(address_word | POCSAG_FLAG_ADDRESS);
    out++;

    // Encode message based on function type
    uint32_t msg_words = 0;
    if (function == POCSAG_FUNCTION_TONE) {
        // TONE: no message data, just address
    } else if (function == POCSAG_FUNCTION_NUMERIC) {
        // NUMERIC: BCD encoding
        msg_words = pocsag_encode_numeric(prefix_length + 1, message, out);
        out += msg_words;
    } else {
        // ALPHANUMERIC: ASCII encoding
        msg_words = pocsag_encode_ascii(prefix_length + 1, message, msb_first, out);
        out += msg_words;
    }

    // IDLE word (end of message marker)
    *out = POCSAG_IDLE;
    out++;

    // Calculate padding to reach batch boundary
    // Total words = SYNC + prefix + address + message + IDLE
    size_t written = out - batch_start;
    size_t padding = (POCSAG_BATCH_SIZE + 1) - (written % (POCSAG_BATCH_SIZE + 1));

    // Pad with IDLE words
    for (size_t i = 0; i < padding; i++) {
        *out = POCSAG_IDLE;
        out++;
    }

    return out - start;  // Return total words
}

// =============================================================================
// HIGH-LEVEL ENCODING API
// =============================================================================

/**
 * @brief Encode POCSAG message (alphanumeric, single transmission)
 *
 * Simple wrapper for common use case: single alphanumeric message
 *
 * @param capcode POCSAG capcode (address)
 * @param message ASCII message text
 * @param msb_first If true, use MSB-first bit order; if false, use LSB-first
 * @param out Output buffer (must be at least POCSAG_BUFFER_SIZE)
 * @return Number of 32-bit words encoded, or 0 on error
 */
static inline size_t pocsag_encode_message(uint32_t capcode, const char *message, bool msb_first, uint32_t *out) {
    if (!message || !out) return 0;
    return pocsag_encode_transmission(0, capcode, POCSAG_FUNCTION_ALPHANUMERIC, message, msb_first, out);
}

// =============================================================================
// BYTE CONVERSION UTILITIES (for transmission)
// =============================================================================

/**
 * @brief Convert 32-bit words to byte array (MSB first)
 *
 * POCSAG transmits MSB first, so we need to convert 32-bit words
 * to big-endian byte order for transmission
 *
 * @param words Input array of 32-bit words
 * @param word_count Number of words
 * @param bytes Output byte array (must be at least word_count * 4)
 * @return Number of bytes written
 */
static inline size_t pocsag_words_to_bytes(const uint32_t *words, size_t word_count, uint8_t *bytes) {
    for (size_t i = 0; i < word_count; i++) {
        uint32_t word = words[i];
        bytes[i * 4 + 0] = (word >> 24) & 0xFF;  // MSB
        bytes[i * 4 + 1] = (word >> 16) & 0xFF;
        bytes[i * 4 + 2] = (word >> 8) & 0xFF;
        bytes[i * 4 + 3] = word & 0xFF;          // LSB
    }
    return word_count * 4;
}

/**
 * @brief Generate POCSAG preamble pattern
 *
 * Preamble: 576 bits (72 bytes) of alternating 10101010... pattern
 * This allows receivers to synchronize before the SYNC word
 *
 * @param bytes Output byte array (must be at least 72 bytes)
 * @return Number of bytes written (always 72)
 */
static inline size_t pocsag_generate_preamble(uint8_t *bytes) {
    for (int i = 0; i < 72; i++) {
        bytes[i] = 0xAA;  // 10101010 pattern
    }
    return 72;
}

#ifdef __cplusplus
}
#endif

#endif // POCSAG_H
