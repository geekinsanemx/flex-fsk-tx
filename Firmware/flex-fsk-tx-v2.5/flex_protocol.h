/*
 * FLEX Paging Message Transmitter - v2.5
 * FLEX Protocol Module
 *
 * FLEX encoding, EMR, and message queue
 */

#ifndef FLEX_PROTOCOL_H
#define FLEX_PROTOCOL_H

#include <Arduino.h>
#include "config.h"

// =============================================================================
// MESSAGE QUEUE
// =============================================================================
struct QueuedMessage {
    uint32_t capcode;
    float frequency;
    int power;
    bool mail_drop;
    char message[MAX_FLEX_MESSAGE_LENGTH + 1];
};

// =============================================================================
// GLOBAL QUEUE STATE
// =============================================================================
extern QueuedMessage message_queue[MESSAGE_QUEUE_SIZE];
extern volatile int queue_head;
extern volatile int queue_tail;
extern volatile int queue_count;

// =============================================================================
// TRANSMISSION BUFFER
// =============================================================================
extern uint8_t tx_data_buffer[2048];
extern int current_tx_total_length;
extern int current_tx_remaining_length;

extern volatile bool fifo_empty;
extern int16_t radio_start_transmit_status;

// =============================================================================
// EMR (Emergency Message Resynchronization)
// =============================================================================
extern bool first_message_sent;
extern unsigned long last_emr_transmission;

void send_emr_if_needed();

// =============================================================================
// FLEX ENCODING
// =============================================================================
bool flex_encode_and_store(uint64_t capcode, const char *message, bool mail_drop);

// =============================================================================
// MESSAGE QUEUE FUNCTIONS
// =============================================================================
void queue_init();
bool queue_is_empty();
bool queue_is_full();
bool queue_add_message(uint32_t capcode, float frequency, int power,
                       bool mail_drop, const char* message);
QueuedMessage* queue_get_next_message();
void queue_remove_message();

#endif // FLEX_PROTOCOL_H
