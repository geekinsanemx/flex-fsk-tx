/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Transmission Module - SX1276 radio, message queue, Core 0 TX task, device state machine
 */

#ifndef TRANSMISSION_H
#define TRANSMISSION_H

#include <Arduino.h>
#include <RadioLib.h>
#include "../core/config.h"

// =============================================================================
// DEVICE STATE MACHINE
// =============================================================================
typedef enum {
    STATE_IDLE,
    STATE_WAITING_FOR_DATA,
    STATE_WAITING_FOR_MSG,
    STATE_TRANSMITTING,
    STATE_ERROR,
    STATE_WIFI_CONNECTING,
    STATE_WIFI_AP_MODE,
    STATE_IMAP_PROCESSING,
    STATE_NTP_SYNC,
    STATE_MQTT_CONNECTING,
    STATE_GSM_INITIALIZING,
    STATE_GSM_CONNECTING,
    STATE_GSM_REGISTERING
} device_state_t;

extern volatile device_state_t device_state;
extern device_state_t previous_state;
extern unsigned long state_timeout;

// =============================================================================
// MESSAGE QUEUE
// =============================================================================
struct QueuedMessage {
    uint64_t capcode;
    float frequency;
    int power;
    bool mail_drop;
    char message[MAX_FLEX_MESSAGE_LENGTH + 1];
};

extern QueuedMessage message_queue[MAX_QUEUE_SIZE];
extern volatile int queue_head;
extern volatile int queue_tail;
extern volatile int queue_count;
extern portMUX_TYPE queue_mux;

// =============================================================================
// RADIO
// =============================================================================
extern SX1276 radio;
extern float tx_power;
extern int8_t current_tx_power;
extern float current_tx_frequency;
extern uint64_t current_tx_capcode;

extern uint8_t tx_data_buffer[2048];
extern int current_tx_total_length;
extern int current_tx_remaining_length;
extern volatile bool fifo_empty;
extern int16_t radio_start_transmit_status;

// =============================================================================
// CORE 0 TX TASK
// =============================================================================
extern TaskHandle_t tx_task_handle;
extern volatile unsigned long core0_last_heartbeat;
extern volatile bool display_update_requested;

// =============================================================================
// FUNCTIONS
// =============================================================================
#define TRANSMISSION_GUARD_ACTIVE() (device_state == STATE_TRANSMITTING || device_state == STATE_WAITING_FOR_DATA || device_state == STATE_WAITING_FOR_MSG)
inline bool transmission_guard_active() {
    return TRANSMISSION_GUARD_ACTIVE();
}

const char* state_to_string(device_state_t state);
void change_device_state(device_state_t new_state);
bool is_valid_state_transition(device_state_t from, device_state_t to);

bool queue_is_empty();
bool queue_is_full();
bool queue_add_message(uint64_t capcode, float frequency, int power, bool mail_drop, const char* message);
struct QueuedMessage* queue_get_next_message();
void queue_remove_message();
void queue_process_next();

void on_interrupt_fifo_has_space();
void transmission_task(void* parameter);
void init_transmission_core();
void check_transmission_task_health();

#endif // TRANSMISSION_H
