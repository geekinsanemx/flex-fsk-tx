/*
 * FLEX Paging Message Transmitter - v2.5
 * Transmission Task Module (Core 0)
 *
 * Dedicated Core 0 task for isolated RF transmission timing
 */

#ifndef TRANSMISSION_H
#define TRANSMISSION_H

#include <Arduino.h>

// =============================================================================
// CORE 0 TASK
// =============================================================================
void transmission_init();
void transmission_task(void* parameter);

// =============================================================================
// FIFO INTERRUPT HANDLER
// =============================================================================
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void on_interrupt_fifo_has_space();

// =============================================================================
// TASK HEALTH MONITORING
// =============================================================================
void check_transmission_task_health();

extern TaskHandle_t transmission_task_handle;
extern volatile bool transmission_task_running;

#endif // TRANSMISSION_H
