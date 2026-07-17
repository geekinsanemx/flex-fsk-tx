/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * AT Commands Module - serial AT command parser, binary data mode, FLEX message mode
 */

#ifndef AT_COMMANDS_H
#define AT_COMMANDS_H

#include <Arduino.h>

// =============================================================================
// FUNCTIONS
// =============================================================================
void at_send_ok();
void at_send_error();
void at_send_response(const char* cmd, const char* value);
void at_send_response_float(const char* cmd, float value, int decimals);
void at_send_response_int(const char* cmd, int value);
void at_reset_state();
void at_flush_serial_buffers();
bool at_parse_command(char* cmd_buffer);

void at_handle_binary_data();
void at_handle_flex_message();
void at_process_serial();

#endif // AT_COMMANDS_H
