/*
 * FLEX Paging Message Transmitter - v2.5.1
 * Binary Protocol - Packet Structure
 *
 * Defines packet structure, opcodes, and packet building/parsing functions
 */

#ifndef BINARY_PACKET_H
#define BINARY_PACKET_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include "config.h"

// =============================================================================
// PACKET CONSTANTS
// =============================================================================

#define PACKET_MAX_LEN 512
#define PACKET_HEADER_LEN 8
#define PACKET_CRC_LEN 2
#define PACKET_OVERHEAD (PACKET_HEADER_LEN + PACKET_CRC_LEN)
#define PACKET_MAX_PAYLOAD (PACKET_MAX_LEN - PACKET_OVERHEAD)

// =============================================================================
// PACKET TYPES
// =============================================================================

#define PKT_TYPE_CMD 0x01   // Command (Host → ESP32)
#define PKT_TYPE_RSP 0x02   // Response (ESP32 → Host, immediate)
#define PKT_TYPE_EVT 0x03   // Event (ESP32 → Host, async)

// =============================================================================
// OPCODES: COMMANDS (TYPE=CMD)
// =============================================================================

#define CMD_SEND_FLEX     0x01  // Send FLEX message
#define CMD_GET_STATUS    0x02  // Query device status
#define CMD_ABORT         0x03  // Abort current operation
#define CMD_SET_CONFIG    0x04  // Set configuration
#define CMD_GET_CONFIG    0x05  // Get configuration
#define CMD_PING          0x06  // Heartbeat/connectivity test
#define CMD_GET_LOGS      0x07  // Query persistent logs
#define CMD_CLEAR_LOGS    0x08  // Delete log file
#define CMD_FACTORY_RESET 0x09  // Factory reset device

// =============================================================================
// OPCODES: RESPONSES (TYPE=RSP)
// =============================================================================

#define RSP_ACK     0x01  // Command accepted
#define RSP_NACK    0x02  // Command rejected
#define RSP_STATUS  0x03  // Status response
#define RSP_CONFIG  0x04  // Config response
#define RSP_PONG    0x05  // Ping response
#define RSP_LOGS    0x06  // Log data response

// =============================================================================
// OPCODES: EVENTS (TYPE=EVT)
// =============================================================================

#define EVT_TX_QUEUED          0x01  // Message enqueued
#define EVT_TX_START           0x02  // Transmission started
#define EVT_TX_DONE            0x03  // Transmission completed
#define EVT_TX_FAILED          0x04  // Transmission failed
#define EVT_BOOT               0x05  // Device boot notification
#define EVT_ERROR              0x06  // Error notification
#define EVT_BATTERY_LOW        0x07  // Low battery alert
#define EVT_POWER_DISCONNECTED 0x08  // Power disconnect alert

// =============================================================================
// FLAGS (Control Bits)
// =============================================================================

#define FLAG_ACK_REQUIRED  (1 << 0)  // 0x01 - ACK required
#define FLAG_RETRY         (1 << 1)  // 0x02 - Retry of previous packet
#define FLAG_ERROR         (1 << 2)  // 0x04 - Error flag
#define FLAG_PRIORITY      (1 << 3)  // 0x08 - High priority
#define FLAG_FRAGMENTED    (1 << 4)  // 0x10 - Fragmented packet
#define FLAG_LAST_FRAGMENT (1 << 5)  // 0x20 - Last fragment

// =============================================================================
// STATUS CODES (RSP_ACK/RSP_NACK payload)
// =============================================================================

#define STATUS_ACCEPTED      0x00  // Command accepted
#define STATUS_REJECTED      0x01  // Command rejected
#define STATUS_QUEUE_FULL    0x02  // Message queue full
#define STATUS_INVALID_PARAM 0x03  // Invalid parameter
#define STATUS_BUSY          0x04  // Device busy
#define STATUS_ERROR         0x05  // Generic error

// =============================================================================
// RESULT CODES (EVT_TX_DONE/EVT_TX_FAILED payload)
// =============================================================================

#define RESULT_SUCCESS         0x00  // Transmission successful
#define RESULT_RADIO_ERROR     0x01  // Radio hardware error
#define RESULT_ENCODING_ERROR  0x02  // FLEX encoding error
#define RESULT_TIMEOUT         0x03  // Timeout
#define RESULT_ABORTED         0x04  // Aborted by user

// =============================================================================
// BINARY PACKET STRUCTURE
// =============================================================================

typedef struct __attribute__((packed)) {
    uint16_t len;
    uint8_t type;
    uint8_t opcode;
    uint8_t flags;
    uint8_t seq;
    uint16_t msg_id;
    uint8_t payload[PACKET_MAX_PAYLOAD];
    uint16_t crc16;
} binary_packet_t;

// =============================================================================
// PAYLOAD STRUCTURES
// =============================================================================

// CMD_SEND_FLEX payload
typedef struct __attribute__((packed)) {
    uint32_t capcode;                        // FLEX capcode (little-endian)
    float frequency;                         // Frequency in MHz (IEEE 754)
    int8_t tx_power;                         // TX power in dBm (signed)
    uint8_t mail_drop;                       // Mail drop flag (0=false, 1=true)
    uint8_t msg_len;                         // Message length (1-248)
    char message[MAX_FLEX_MESSAGE_LENGTH];   // Message text (variable)
} cmd_send_flex_payload_t;

// RSP_ACK/RSP_NACK payload
typedef struct __attribute__((packed)) {
    uint8_t status;                          // Status code
} rsp_ack_payload_t;

// RSP_STATUS payload
typedef struct __attribute__((packed)) {
    uint8_t device_state;                    // Device state (STATE_IDLE, etc.)
    uint8_t queue_count;                     // Current queue count (0-10)
    uint8_t battery_pct;                     // Battery percentage (0-100)
    uint16_t battery_mv;                     // Battery voltage (millivolts, little-endian)
    float frequency;                         // Current frequency (IEEE 754)
    int8_t power;                            // Current power (dBm, signed)
} rsp_status_payload_t;

// EVT_TX_QUEUED payload
typedef struct __attribute__((packed)) {
    uint8_t queue_pos;                       // Position in queue (1-10)
} evt_tx_queued_payload_t;

// EVT_TX_DONE payload
typedef struct __attribute__((packed)) {
    uint8_t result;                          // Result code
} evt_tx_done_payload_t;

// EVT_TX_FAILED payload
typedef struct __attribute__((packed)) {
    uint8_t error_code;                      // Error code
} evt_tx_failed_payload_t;

// =============================================================================
// FUNCTION PROTOTYPES: PACKET BUILDING
// =============================================================================

/**
 * Build CMD_SEND_FLEX packet
 *
 * @param pkt       Output packet buffer
 * @param seq       Transport sequence number
 * @param msg_id    Application message ID
 * @param capcode   FLEX capcode
 * @param frequency Frequency in MHz
 * @param tx_power  TX power in dBm
 * @param mail_drop Mail drop flag (0=false, 1=true)
 * @param message   Message text (null-terminated)
 * @param msg_len   Message length (1-248)
 * @return          Total packet length (including CRC)
 */
size_t build_cmd_send_flex(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id,
                           uint32_t capcode, float frequency, int8_t tx_power,
                           uint8_t mail_drop, const char *message, uint8_t msg_len);

/**
 * Build RSP_ACK packet
 *
 * @param pkt       Output packet buffer
 * @param seq       Transport sequence number (from received command)
 * @param msg_id    Application message ID (from received command)
 * @param status    Status code (STATUS_ACCEPTED, etc.)
 * @return          Total packet length
 */
size_t build_rsp_ack(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t status);

/**
 * Build RSP_NACK packet (same as RSP_ACK but different opcode)
 *
 * @param pkt       Output packet buffer
 * @param seq       Transport sequence number
 * @param msg_id    Application message ID
 * @param status    Status code (STATUS_REJECTED, etc.)
 * @return          Total packet length
 */
size_t build_rsp_nack(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t status);

/**
 * Build RSP_STATUS packet
 *
 * @param pkt       Output packet buffer
 * @param seq       Transport sequence number
 * @param msg_id    Application message ID
 * @param status    Status payload
 * @return          Total packet length
 */
size_t build_rsp_status(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id,
                        const rsp_status_payload_t *status);

/**
 * Build RSP_PONG packet
 *
 * @param pkt       Output packet buffer
 * @param seq       Transport sequence number
 * @param msg_id    Application message ID
 * @return          Total packet length
 */
size_t build_rsp_pong(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id);

/**
 * Build EVT_TX_QUEUED packet
 *
 * @param pkt       Output packet buffer
 * @param seq       Transport sequence number
 * @param msg_id    Application message ID
 * @param pos       Queue position (1-10)
 * @return          Total packet length
 */
size_t build_evt_tx_queued(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t pos);

/**
 * Build EVT_TX_START packet
 *
 * @param pkt       Output packet buffer
 * @param seq       Transport sequence number
 * @param msg_id    Application message ID
 * @return          Total packet length
 */
size_t build_evt_tx_start(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id);

/**
 * Build EVT_TX_DONE packet
 *
 * @param pkt       Output packet buffer
 * @param seq       Transport sequence number
 * @param msg_id    Application message ID
 * @param result    Result code (RESULT_SUCCESS, etc.)
 * @return          Total packet length
 */
size_t build_evt_tx_done(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t result);

/**
 * Build EVT_TX_FAILED packet
 *
 * @param pkt       Output packet buffer
 * @param seq       Transport sequence number
 * @param msg_id    Application message ID
 * @param error     Error code (RESULT_RADIO_ERROR, etc.)
 * @return          Total packet length
 */
size_t build_evt_tx_failed(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t error);

// =============================================================================
// FUNCTION PROTOTYPES: PACKET PARSING
// =============================================================================

/**
 * Parse binary packet from raw data
 *
 * @param data      Raw packet data (already COBS-decoded)
 * @param len       Length of data
 * @param pkt       Output packet structure
 * @return          true if parse successful, false otherwise
 *
 * This function does NOT validate CRC. Use validate_packet() separately.
 */
bool parse_packet(const uint8_t *data, size_t len, binary_packet_t *pkt);

/**
 * Validate packet (check CRC)
 *
 * @param pkt       Packet to validate
 * @return          true if CRC is valid, false otherwise
 */
bool validate_packet(const binary_packet_t *pkt);

// =============================================================================
// HELPER MACROS
// =============================================================================

// Convert network byte order (big-endian) to host byte order
#define ntohs_custom(x) ((uint16_t)((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF)))
#define htons_custom(x) ntohs_custom(x)  // Same operation

// Get payload pointer from packet
#define PACKET_PAYLOAD(pkt) ((pkt)->payload)

// Get payload length from packet
#define PACKET_PAYLOAD_LEN(pkt) ((pkt)->len - PACKET_OVERHEAD)

#endif // BINARY_PACKET_H
