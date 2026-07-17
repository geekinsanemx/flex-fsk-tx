#include "../protocol/transmission.h"

#include "../core/config.h"
#include "../../include/boards/boards.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../core/hardware.h"
#include "../core/display.h"
#include "../protocol/flex_protocol.h"

volatile device_state_t device_state = STATE_IDLE;
device_state_t previous_state = STATE_IDLE;
unsigned long state_timeout = 0;

QueuedMessage message_queue[MAX_QUEUE_SIZE];
volatile int queue_head = 0;
volatile int queue_tail = 0;
volatile int queue_count = 0;
portMUX_TYPE queue_mux = portMUX_INITIALIZER_UNLOCKED;

SX1276 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, LORA_RST_PIN, LORA_GPIO_PIN);
float tx_power = 0;
int8_t current_tx_power = 0;
float current_tx_frequency = 0;
uint64_t current_tx_capcode = 0;

uint8_t tx_data_buffer[2048] = {0};
int current_tx_total_length = 0;
int current_tx_remaining_length = 0;
volatile bool fifo_empty = false;
int16_t radio_start_transmit_status = RADIOLIB_ERR_NONE;

TaskHandle_t tx_task_handle = NULL;
volatile unsigned long core0_last_heartbeat = 0;
volatile bool display_update_requested = false;

// =============================================================================
// DEVICE STATE MACHINE
// =============================================================================
const char* state_to_string(device_state_t state) {
    switch(state) {
        case STATE_IDLE: return "IDLE";
        case STATE_WAITING_FOR_DATA: return "WAITING_FOR_DATA";
        case STATE_WAITING_FOR_MSG: return "WAITING_FOR_MSG";
        case STATE_TRANSMITTING: return "TRANSMITTING";
        case STATE_ERROR: return "ERROR";
        case STATE_WIFI_CONNECTING: return "WIFI_CONNECTING";
        case STATE_WIFI_AP_MODE: return "WIFI_AP_MODE";
        case STATE_IMAP_PROCESSING: return "IMAP_PROCESSING";
        case STATE_NTP_SYNC: return "NTP_SYNC";
        case STATE_MQTT_CONNECTING: return "MQTT_CONNECTING";
        case STATE_GSM_INITIALIZING: return "GSM_INITIALIZING";
        case STATE_GSM_CONNECTING: return "GSM_CONNECTING";
        case STATE_GSM_REGISTERING: return "GSM_REGISTERING";
        default: return "UNKNOWN";
    }
}

void change_device_state(device_state_t new_state) {
    if (!is_valid_state_transition(device_state, new_state)) {
        logMessagef("STATE: Invalid transition %s -> %s", state_to_string(device_state), state_to_string(new_state));
        return;
    }

    previous_state = device_state;
    device_state = new_state;
    logMessagef("STATE: %s -> %s", state_to_string(previous_state), state_to_string(new_state));

    if (new_state != STATE_IDLE &&
        new_state != STATE_NTP_SYNC &&
        new_state != STATE_IMAP_PROCESSING) {
        reset_oled_timeout();
    }

    display_status();
}

bool is_valid_state_transition(device_state_t from, device_state_t to) {
    switch(from) {
        case STATE_IDLE:
            return true;
        case STATE_TRANSMITTING:
            return (to == STATE_IDLE || to == STATE_ERROR);
        case STATE_WAITING_FOR_DATA:
        case STATE_WAITING_FOR_MSG:
            return (to == STATE_TRANSMITTING || to == STATE_IDLE || to == STATE_ERROR);
        case STATE_ERROR:
            return (to == STATE_IDLE);
        default:
            return true;
    }
}

// =============================================================================
// MESSAGE QUEUE
// =============================================================================
bool queue_is_empty() {
    return queue_count == 0;
}

bool queue_is_full() {
    return queue_count >= MAX_QUEUE_SIZE;
}

bool queue_add_message(uint64_t capcode, float frequency, int power, bool mail_drop, const char* message) {

    if (!validate_flex_capcode(capcode)) {
        logMessagef("QUEUE: Rejected message - invalid capcode %llu", capcode);
        return false;
    }

    portENTER_CRITICAL(&queue_mux);

    if (queue_count >= MAX_QUEUE_SIZE) {
        portEXIT_CRITICAL(&queue_mux);
        return false;
    }

    QueuedMessage* msg = &message_queue[queue_tail];
    msg->capcode = capcode;
    msg->frequency = frequency;
    msg->power = power;
    msg->mail_drop = mail_drop;

    String converted_message = convert_unicode_to_ascii(String(message));
    converted_message = truncate_message_with_ellipsis(converted_message);
    strncpy(msg->message, converted_message.c_str(), MAX_FLEX_MESSAGE_LENGTH);
    msg->message[MAX_FLEX_MESSAGE_LENGTH] = '\0';

    queue_tail = (queue_tail + 1) % MAX_QUEUE_SIZE;
    queue_count++;

    portEXIT_CRITICAL(&queue_mux);

    if (tx_task_handle != NULL) {
        xTaskNotifyGive(tx_task_handle);
    }

    return true;
}

struct QueuedMessage* queue_get_next_message() {
    portENTER_CRITICAL(&queue_mux);
    if (queue_count == 0) {
        portEXIT_CRITICAL(&queue_mux);
        return nullptr;
    }
    QueuedMessage* msg = &message_queue[queue_head];
    portEXIT_CRITICAL(&queue_mux);
    return msg;
}

void queue_remove_message() {
    portENTER_CRITICAL(&queue_mux);
    if (queue_count > 0) {
        queue_head = (queue_head + 1) % MAX_QUEUE_SIZE;
        queue_count--;
    }
    portEXIT_CRITICAL(&queue_mux);
}

void queue_process_next() {
    if (queue_is_empty() || (device_state != STATE_IDLE && device_state != STATE_IMAP_PROCESSING)) {
        return;
    }

    QueuedMessage* msg = queue_get_next_message();
    if (msg == nullptr) {
        return;
    }

    if (abs(msg->frequency - current_tx_frequency) > 0.0001) {
        int state = radio.setFrequency(apply_frequency_correction(msg->frequency));
        if (state != RADIOLIB_ERR_NONE) {
            queue_remove_message();
            return;
        }
        current_tx_frequency = msg->frequency;
    }

    if (abs(msg->power - tx_power) > 0.1) {
        int state = radio.setOutputPower(msg->power);
        if (state != RADIOLIB_ERR_NONE) {
            queue_remove_message();
            return;
        }
        tx_power = msg->power;
    }

    feed_watchdog();

    if (!flex_encode_and_store(msg->capcode, msg->message, msg->mail_drop)) {
        queue_remove_message();
        return;
    }

    current_tx_capcode = msg->capcode;
    device_state = STATE_TRANSMITTING;
    LED_ON();

    send_emr_if_needed();

    int radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);
    if (radio_start_transmit_status != RADIOLIB_ERR_NONE) {
        device_state = STATE_IDLE;
        LED_OFF();
        display_status();
    } else {
        display_status();
    }

    queue_remove_message();
}

// =============================================================================
// CORE 0 TX TASK
// =============================================================================
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void on_interrupt_fifo_has_space() {
    fifo_empty = true;
}

void transmission_task(void* parameter) {
    while (true) {
        core0_last_heartbeat = millis();

        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));

        while (true) {
            QueuedMessage* msg = queue_get_next_message();
            if (msg == nullptr) {
                break;
            }

            if (abs(msg->frequency - current_tx_frequency) > 0.0001) {
                int state = radio.setFrequency(apply_frequency_correction(msg->frequency));
                if (state != RADIOLIB_ERR_NONE) {
                    queue_remove_message();
                    continue;
                }
                current_tx_frequency = msg->frequency;
            }

            if (abs(msg->power - tx_power) > 0.1) {
                int state = radio.setOutputPower(msg->power);
                if (state != RADIOLIB_ERR_NONE) {
                    queue_remove_message();
                    continue;
                }
                tx_power = msg->power;
            }

            if (!flex_encode_and_store(msg->capcode, msg->message, msg->mail_drop)) {
                queue_remove_message();
                continue;
            }

            current_tx_capcode = msg->capcode;
            device_state = STATE_TRANSMITTING;
            LED_ON();

            display_update_requested = true;

            rfamp_enable();

            send_emr_if_needed();

            fifo_empty = true;
            current_tx_remaining_length = current_tx_total_length;
            radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);

            if (radio_start_transmit_status != RADIOLIB_ERR_NONE) {
                device_state = STATE_IDLE;
                LED_OFF();
                display_update_requested = true;
                queue_remove_message();
                continue;
            }

            display_update_requested = true;

            bool transmission_complete = false;
            while (!transmission_complete) {
                if (fifo_empty && current_tx_remaining_length > 0) {
                    fifo_empty = false;
                    transmission_complete = radio.fifoAdd(tx_data_buffer, current_tx_total_length, &current_tx_remaining_length);
                }
                delay(1);
            }

            if (radio_start_transmit_status == RADIOLIB_ERR_NONE) {
                logMessagef("FLEX: Message sent successfully (capcode=%llu, freq=%.4f MHz, power=%.1f dBm)",
                          current_tx_capcode, current_tx_frequency, tx_power);
            }

            radio.standby();

            rfamp_disable();

            device_state = STATE_IDLE;
            LED_OFF();
            display_update_requested = true;

            queue_remove_message();
        }
    }
}

void init_transmission_core() {
    xTaskCreatePinnedToCore(
        transmission_task,
        "TX_Core0",
        4096,
        NULL,
        configMAX_PRIORITIES - 1,
        &tx_task_handle,
        0
    );

    logMessage("TX: Core 0 task created - isolated transmission");
}

void check_transmission_task_health() {
    static unsigned long last_check = 0;

    if (millis() - last_check > 10000) {
        if (millis() - core0_last_heartbeat > 15000) {
            logMessage("CRITICAL: Core 0 transmission task unresponsive for 15s");
        }
        last_check = millis();
    }
}
