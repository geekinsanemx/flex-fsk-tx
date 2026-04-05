/*
 * FLEX Paging Message Transmitter - v2.5.1
 * AT Command Protocol Module + Binary Protocol Processing
 *
 * AT command parser and handler + Binary protocol detection
 */

#ifndef AT_COMMANDS_H
#define AT_COMMANDS_H

#include <Arduino.h>
#include "config.h"

// =============================================================================
// AT PROTOCOL STATE
// =============================================================================
extern char at_buffer[AT_BUFFER_SIZE];
extern int at_buffer_pos;
extern bool at_command_ready;

extern int expected_data_length;
extern unsigned long data_receive_timeout;

extern uint64_t flex_capcode;
extern char flex_message_buffer[MAX_FLEX_MESSAGE_LENGTH + 1];
extern int flex_message_pos;
extern unsigned long flex_message_timeout;
extern bool flex_mail_drop;

extern bool console_loop_enable;

extern uint64_t current_tx_capcode;

// =============================================================================
// AT COMMAND FUNCTIONS
// =============================================================================
void at_init();
void at_process_serial();
void at_reset_state();
void at_flush_serial_buffers();

bool at_parse_command(char* cmd_buffer);
void at_handle_binary_data();
void at_handle_flex_message();

void at_send_ok();
void at_send_error();
void at_send_response(const char* cmd, const char* value);
void at_send_response_float(const char* cmd, float value, int decimals);
void at_send_response_int(const char* cmd, int value);

// =============================================================================
// TRANSMISSION GUARD
// =============================================================================
bool transmission_guard_active();

// =============================================================================
// BINARY PROTOCOL PROCESSING
// =============================================================================
extern size_t binary_frame_pos;
void process_binary_frame();
void handle_binary_packet(uint8_t *cobs_data, size_t len);

#endif // AT_COMMANDS_H
