/*
 * FLEX Paging Message Transmitter - v2.5
 * Transmission Task Implementation (Core 0)
 */

#include "transmission.h"
#include "config.h"
#include "flex_protocol.h"
#include "hardware.h"
#include "logging.h"
#include "display.h"
#include "boards/boards.h"

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
TaskHandle_t transmission_task_handle = NULL;
volatile bool transmission_task_running = false;

extern uint64_t current_tx_capcode;

// =============================================================================
// FIFO INTERRUPT HANDLER
// =============================================================================
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void on_interrupt_fifo_has_space() {
    fifo_empty = true;
}

// =============================================================================
// CORE 0 TRANSMISSION TASK
// =============================================================================
void transmission_task(void* parameter) {
    logMessage("TRANSMISSION: Core 0 task started");
    transmission_task_running = true;

    while (true) {
        if (!queue_is_empty()) {
            QueuedMessage* msg = queue_get_next_message();
            if (msg == nullptr) {
                delay(10);
                continue;
            }

            logMessagef("TRANSMISSION: Processing message (capcode=%lu)", (unsigned long)msg->capcode);

            current_tx_frequency = msg->frequency;
            current_tx_power = msg->power;
            current_tx_capcode = msg->capcode;

            if (radio_set_frequency(msg->frequency) != RADIOLIB_ERR_NONE) {
                logMessage("TRANSMISSION: Failed to set frequency");
                queue_remove_message();
                continue;
            }

            if (radio_set_power(msg->power) != RADIOLIB_ERR_NONE) {
                logMessage("TRANSMISSION: Failed to set power");
                queue_remove_message();
                continue;
            }

            if (!flex_encode_and_store(msg->capcode, msg->message, msg->mail_drop)) {
                logMessage("TRANSMISSION: Encoding failed");
                queue_remove_message();
                continue;
            }

            rfamp_enable();

            device_state = STATE_TRANSMITTING;
            digitalWrite(LED_PIN, HIGH);

            display_update_requested = true;

            send_emr_if_needed();

            fifo_empty = true;
            current_tx_remaining_length = current_tx_total_length;
            radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);

            while (current_tx_remaining_length > 0) {
                if (fifo_empty) {
                    fifo_empty = false;
                    bool complete = radio.fifoAdd(tx_data_buffer, current_tx_total_length,
                                                 &current_tx_remaining_length);
                    if (complete) {
                        break;
                    }
                }
                delay(1);
            }

            if (radio_start_transmit_status == RADIOLIB_ERR_NONE) {
                logMessagef("TRANSMISSION: Success (capcode=%llu, freq=%.4f MHz, power=%.1f dBm)",
                          current_tx_capcode, current_tx_frequency, current_tx_power);
            } else {
                logMessagef("TRANSMISSION: Failed (error=%d)", radio_start_transmit_status);
            }

            radio_standby();

            device_state = STATE_IDLE;
            digitalWrite(LED_PIN, LOW);

            rfamp_disable();

            display_update_requested = true;

            queue_remove_message();
        }

        delay(10);
    }
}

// =============================================================================
// TRANSMISSION TASK INITIALIZATION
// =============================================================================
void transmission_init() {
    logMessage("TRANSMISSION: Initializing Core 0 task");

    radio_set_fifo_callback(on_interrupt_fifo_has_space);

    xTaskCreatePinnedToCore(
        transmission_task,
        "TransmissionTask",
        4096,
        NULL,
        1,
        &transmission_task_handle,
        0
    );

    logMessage("TRANSMISSION: Core 0 task created");
}

// =============================================================================
// TASK HEALTH MONITORING
// =============================================================================
void check_transmission_task_health() {
    if (transmission_task_handle != NULL) {
        eTaskState state = eTaskGetState(transmission_task_handle);

        if (state == eDeleted || state == eInvalid) {
            logMessage("TRANSMISSION: Task health check FAILED - task is dead");
            transmission_task_running = false;
        }
    } else {
        logMessage("TRANSMISSION: Task handle is NULL");
        transmission_task_running = false;
    }
}
