/*
 * FLEX Paging Message Transmitter - v2.5.1
 * Binary Protocol - Event Senders
 *
 * Functions for sending async events to host
 */

#ifndef BINARY_EVENTS_H
#define BINARY_EVENTS_H

#include <Arduino.h>
#include <stdint.h>

// =============================================================================
// EVENT SENDER STATE
// =============================================================================

extern bool binary_protocol_active;
extern uint8_t binary_event_seq;

// =============================================================================
// GENERIC EVENT SENDERS
// =============================================================================

/**
 * Send binary event (no payload)
 *
 * @param opcode    Event opcode (EVT_TX_START, etc.)
 * @param msg_id    Application message ID
 */
void send_binary_event(uint8_t opcode, uint16_t msg_id);

/**
 * Send binary event with payload
 *
 * @param opcode    Event opcode
 * @param msg_id    Application message ID
 * @param payload   Payload data
 * @param len       Payload length
 */
void send_binary_event_with_payload(uint8_t opcode, uint16_t msg_id,
                                    const uint8_t *payload, size_t len);

// =============================================================================
// SPECIFIC EVENT SENDERS
// =============================================================================

/**
 * Send EVT_TX_QUEUED event
 *
 * @param msg_id    Application message ID
 * @param pos       Queue position (1-10)
 */
void send_evt_tx_queued(uint16_t msg_id, uint8_t pos);

/**
 * Send EVT_TX_START event
 *
 * @param msg_id    Application message ID
 */
void send_evt_tx_start(uint16_t msg_id);

/**
 * Send EVT_TX_DONE event
 *
 * @param msg_id    Application message ID
 * @param result    Result code (RESULT_SUCCESS, etc.)
 */
void send_evt_tx_done(uint16_t msg_id, uint8_t result);

/**
 * Send EVT_TX_FAILED event
 *
 * @param msg_id    Application message ID
 * @param error     Error code (RESULT_RADIO_ERROR, etc.)
 */
void send_evt_tx_failed(uint16_t msg_id, uint8_t error);

/**
 * Send EVT_BOOT event
 */
void send_evt_boot();

/**
 * Send EVT_BATTERY_LOW event
 *
 * @param battery_pct    Battery percentage
 */
void send_evt_battery_low(uint8_t battery_pct);

/**
 * Send EVT_POWER_DISCONNECTED event
 */
void send_evt_power_disconnected();

// =============================================================================
// RESPONSE SENDERS
// =============================================================================

/**
 * Send RSP_ACK response
 *
 * @param seq       Transport sequence (from command)
 * @param msg_id    Application message ID
 * @param status    Status code (STATUS_ACCEPTED, etc.)
 */
void send_binary_response_ack(uint8_t seq, uint16_t msg_id, uint8_t status);

/**
 * Send RSP_NACK response
 *
 * @param seq       Transport sequence (from command)
 * @param msg_id    Application message ID
 * @param status    Status code (STATUS_REJECTED, etc.)
 */
void send_binary_response_nack(uint8_t seq, uint16_t msg_id, uint8_t status);

/**
 * Send RSP_PONG response
 *
 * @param seq       Transport sequence (from command)
 * @param msg_id    Application message ID
 */
void send_binary_response_pong(uint8_t seq, uint16_t msg_id);

/**
 * Send RSP_STATUS response
 *
 * @param seq               Transport sequence
 * @param msg_id            Application message ID
 * @param device_state      Device state
 * @param queue_count       Queue count
 * @param battery_pct       Battery percentage
 * @param battery_mv        Battery voltage (mV)
 * @param frequency         Current frequency
 * @param power             Current power (dBm)
 */
void send_binary_response_status(uint8_t seq, uint16_t msg_id,
                                 uint8_t device_state, uint8_t queue_count,
                                 uint8_t battery_pct, uint16_t battery_mv,
                                 float frequency, int8_t power);

#endif // BINARY_EVENTS_H
