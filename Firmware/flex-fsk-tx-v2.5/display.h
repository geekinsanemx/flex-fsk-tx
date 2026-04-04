/*
 * FLEX Paging Message Transmitter - v2.5
 * Display Management Module
 *
 * OLED display content and visualization logic
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// =============================================================================
// DEVICE STATE ENUMERATION
// =============================================================================
typedef enum {
    STATE_IDLE,
    STATE_WAITING_FOR_DATA,
    STATE_WAITING_FOR_MSG,
    STATE_TRANSMITTING,
    STATE_ERROR
} device_state_t;

// =============================================================================
// GLOBAL STATE
// =============================================================================
extern volatile device_state_t device_state;
extern float current_tx_frequency;
extern float current_tx_power;

// =============================================================================
// DISPLAY FUNCTIONS
// =============================================================================
void display_status();
void display_factory_reset();
void display_update_if_needed();

// =============================================================================
// DISPLAY UPDATE REQUEST (For Core 0)
// =============================================================================
extern volatile bool display_update_requested;

#endif // DISPLAY_H
