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

// Decides whether a secondary TLS job (IMAP/ChatGPT) may start now: MQTT has priority while it reconnects,
// then enough heap must be free for the handshake. Fills `error` with the reason when the job must wait.
bool service_worker_tls_allowed(String& error);

// Inside a completion callback: true when the TLS heap dropped below TLS_LOW_MEMORY_FAILURE_BYTES during the job,
// so a failure of that job is attributed to memory exhaustion rather than to the remote server or account.
bool service_worker_job_ran_out_of_memory();

// Inside a completion callback: lowest free TLS heap measured while the job ran (SIZE_MAX when unknown).
size_t service_worker_job_lowest_free();

#endif // SERVICE_WORKER_H
