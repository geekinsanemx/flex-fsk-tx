/*
 * FLEX Paging Message Transmitter - v2.5
 * FLEX Protocol Implementation
 */

#include "flex_protocol.h"
#include "config.h"
#include "logging.h"
#include "hardware.h"
#include "tinyflex/tinyflex.h"

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
QueuedMessage message_queue[MESSAGE_QUEUE_SIZE];
volatile int queue_head = 0;
volatile int queue_tail = 0;
volatile int queue_count = 0;

uint8_t tx_data_buffer[2048] = {0};
int current_tx_total_length = 0;
int current_tx_remaining_length = 0;

volatile bool fifo_empty = false;
int16_t radio_start_transmit_status = RADIOLIB_ERR_NONE;

bool first_message_sent = false;
unsigned long last_emr_transmission = 0;

// Thread safety
portMUX_TYPE queue_mutex = portMUX_INITIALIZER_UNLOCKED;

// =============================================================================
// QUEUE INITIALIZATION
// =============================================================================
void queue_init() {
    portENTER_CRITICAL(&queue_mutex);
    queue_head = 0;
    queue_tail = 0;
    queue_count = 0;
    portEXIT_CRITICAL(&queue_mutex);

    logMessage("QUEUE: Initialized (10 messages capacity)");
}

// =============================================================================
// QUEUE MANAGEMENT
// =============================================================================
bool queue_is_empty() {
    portENTER_CRITICAL(&queue_mutex);
    bool empty = (queue_count == 0);
    portEXIT_CRITICAL(&queue_mutex);
    return empty;
}

bool queue_is_full() {
    portENTER_CRITICAL(&queue_mutex);
    bool full = (queue_count >= MESSAGE_QUEUE_SIZE);
    portEXIT_CRITICAL(&queue_mutex);
    return full;
}

bool queue_add_message_with_id(uint16_t msg_id, uint32_t capcode,
                               float frequency, int power,
                               bool mail_drop, const char* message) {
    if (queue_is_full()) {
        logMessage("QUEUE: Full, cannot add message");
        return false;
    }

    portENTER_CRITICAL(&queue_mutex);

    message_queue[queue_tail].msg_id = msg_id;
    message_queue[queue_tail].capcode = capcode;
    message_queue[queue_tail].frequency = frequency;
    message_queue[queue_tail].power = power;
    message_queue[queue_tail].mail_drop = mail_drop;
    strlcpy(message_queue[queue_tail].message, message,
            sizeof(message_queue[queue_tail].message));

    queue_tail = (queue_tail + 1) % MESSAGE_QUEUE_SIZE;
    queue_count++;

    portEXIT_CRITICAL(&queue_mutex);

    logMessagef("QUEUE: Added message (msg_id=0x%04X, count=%d, capcode=%lu)",
                msg_id, queue_count, (unsigned long)capcode);
    return true;
}

bool queue_add_message(uint32_t capcode, float frequency, int power,
                       bool mail_drop, const char* message) {
    // Wrapper for AT mode compatibility - generate msg_id from 0x8000 range
    static uint16_t at_msg_id = 0x8000;
    uint16_t msg_id = at_msg_id++;
    if (at_msg_id == 0) at_msg_id = 0x8000;  // Rollover to 0x8000

    return queue_add_message_with_id(msg_id, capcode, frequency, power,
                                     mail_drop, message);
}

QueuedMessage* queue_get_next_message() {
    if (queue_is_empty()) {
        return nullptr;
    }

    return &message_queue[queue_head];
}

void queue_remove_message() {
    if (queue_is_empty()) {
        return;
    }

    portENTER_CRITICAL(&queue_mutex);
    queue_head = (queue_head + 1) % MESSAGE_QUEUE_SIZE;
    queue_count--;
    portEXIT_CRITICAL(&queue_mutex);

    logMessagef("QUEUE: Removed message (count=%d)", queue_count);
}

// =============================================================================
// FLEX ENCODING
// =============================================================================
bool flex_encode_and_store(uint64_t capcode, const char *message, bool mail_drop) {
    uint8_t flex_buffer[FLEX_BUFFER_SIZE];
    struct tf_message_config config = {0};
    config.mail_drop = mail_drop ? 1 : 0;

    int error = 0;
    size_t encoded_size = tf_encode_flex_message_ex(message, capcode, flex_buffer,
                                                   sizeof(flex_buffer), &error, &config);

    if (error < 0 || encoded_size == 0 || encoded_size > sizeof(tx_data_buffer)) {
        logMessagef("FLEX: Encoding failed (error=%d, size=%d)", error, encoded_size);
        return false;
    }

    memcpy(tx_data_buffer, flex_buffer, encoded_size);
    current_tx_total_length = encoded_size;
    current_tx_remaining_length = encoded_size;

    logMessagef("FLEX: Encoded successfully (%d bytes)", encoded_size);
    return true;
}

// =============================================================================
// EMR (Emergency Message Resynchronization)
// =============================================================================
void send_emr_if_needed() {
    bool need_emr = !first_message_sent ||
                    ((millis() - last_emr_transmission) >= EMR_TIMEOUT_MS);

    if (need_emr) {
        logMessage("EMR: Sending synchronization burst");

        uint8_t emr_pattern[] = {EMR_PATTERN[0], EMR_PATTERN[1],
                                EMR_PATTERN[2], EMR_PATTERN[3]};
        radio.startTransmit(emr_pattern, sizeof(emr_pattern));

        unsigned long emr_start = millis();
        while (radio.getPacketLength() > 0 &&
               ((unsigned long)(millis() - emr_start) < 2000)) {
            delay(1);
        }
        delay(100);

        last_emr_transmission = millis();
        first_message_sent = true;

        logMessage("EMR: Synchronization burst sent");
    }
}
