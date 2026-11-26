/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Service Worker Module - runs blocking network jobs (IMAP, ChatGPT) off the loop task
 */

#ifndef SERVICE_WORKER_H
#define SERVICE_WORKER_H

#include <Arduino.h>
#include <functional>

using ServiceWork = std::function<void()>;

// =============================================================================
// FUNCTIONS
// =============================================================================

// Creates the worker task (Core 1) and its job/completion queues.
bool service_worker_init();

// Queues `work` for the worker task; `done` runs later on the loop task via service_worker_process_completions().
bool service_worker_submit(const char* label, ServiceWork work, ServiceWork done);

// Runs the completion callbacks of finished jobs. Must be called from the loop task.
void service_worker_process_completions();

#endif // SERVICE_WORKER_H
