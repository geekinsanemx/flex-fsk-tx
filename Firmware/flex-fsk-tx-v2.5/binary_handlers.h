/*
 * FLEX Paging Message Transmitter - v2.5.1
 * Binary Protocol - Command Handlers
 *
 * Handlers for binary protocol commands received from host
 */

#ifndef BINARY_HANDLERS_H
#define BINARY_HANDLERS_H

#include <Arduino.h>
#include "binary_packet.h"

// =============================================================================
// COMMAND HANDLERS
// =============================================================================

/**
 * Handle CMD_SEND_FLEX command
 *
 * @param pkt    Received command packet
 */
void handle_cmd_send_flex(binary_packet_t *pkt);

/**
 * Handle CMD_GET_STATUS command
 *
 * @param pkt    Received command packet
 */
void handle_cmd_get_status(binary_packet_t *pkt);

/**
 * Handle CMD_ABORT command
 *
 * @param pkt    Received command packet
 */
void handle_cmd_abort(binary_packet_t *pkt);

/**
 * Handle CMD_PING command
 *
 * @param pkt    Received command packet
 */
void handle_cmd_ping(binary_packet_t *pkt);

/**
 * Handle CMD_SET_CONFIG command
 *
 * @param pkt    Received command packet
 */
void handle_cmd_set_config(binary_packet_t *pkt);

/**
 * Handle CMD_GET_CONFIG command
 *
 * @param pkt    Received command packet
 */
void handle_cmd_get_config(binary_packet_t *pkt);

// =============================================================================
// COMMAND DISPATCHER
// =============================================================================

/**
 * Dispatch command packet to appropriate handler
 *
 * @param pkt    Received command packet
 */
void dispatch_binary_command(binary_packet_t *pkt);

#endif // BINARY_HANDLERS_H
