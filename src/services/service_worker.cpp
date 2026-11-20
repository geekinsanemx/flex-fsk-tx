/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Service Worker Module - runs blocking network jobs (IMAP, ChatGPT) off the loop task
 *
 * The loop task owns all service state (schedules, configuration, status) and
 * submits network jobs here. The worker executes them one at a time and hands
 * each finished job back through a completion queue, so state is only ever
 * modified on the loop task and a stalled TLS/IMAP/HTTP call can no longer
 * freeze the web server, MQTT or the loop watchdog.
 */

#include "../services/service_worker.h"
#include "../core/logging.h"
#include "../protocol/transmission.h"

// =============================================================================
// FILE-LOCAL STATE
// =============================================================================
static const uint32_t SERVICE_WORKER_STACK_SIZE = 10240;
static const UBaseType_t SERVICE_WORKER_PRIORITY = 1;
static const BaseType_t SERVICE_WORKER_CORE = 1;
static const UBaseType_t SERVICE_WORKER_QUEUE_LENGTH = 16;
static const uint32_t SERVICE_WORKER_TX_POLL_MS = 100;

struct ServiceJob {
    const char* label;
    ServiceWork work;
    ServiceWork done;
};

static QueueHandle_t job_queue = nullptr;
static QueueHandle_t done_queue = nullptr;
static TaskHandle_t worker_task = nullptr;
static UBaseType_t jobs_pending = 0;

// =============================================================================
// WORKER TASK
// =============================================================================
static void service_worker_task(void* parameter) {
    for (;;) {
        ServiceJob* job = nullptr;
        if (xQueueReceive(job_queue, &job, portMAX_DELAY) != pdTRUE || job == nullptr) {
            continue;
        }

        while (transmission_guard_active()) {
            vTaskDelay(pdMS_TO_TICKS(SERVICE_WORKER_TX_POLL_MS));
        }

        unsigned long start = millis();
        if (job->work) {
            job->work();
        }
        logMessagef("WORKER: %s finished in %lu ms (stack free: %u bytes)",
                    job->label, millis() - start, (unsigned)uxTaskGetStackHighWaterMark(NULL));

        xQueueSend(done_queue, &job, portMAX_DELAY);
    }
}

// =============================================================================
// PUBLIC API
// =============================================================================
bool service_worker_init() {
    if (worker_task != nullptr) {
        return true;
    }

    job_queue = xQueueCreate(SERVICE_WORKER_QUEUE_LENGTH, sizeof(ServiceJob*));
    done_queue = xQueueCreate(SERVICE_WORKER_QUEUE_LENGTH, sizeof(ServiceJob*));
    if (job_queue == nullptr || done_queue == nullptr) {
        logMessage("WORKER: Failed to create job queues");
        return false;
    }

    if (xTaskCreatePinnedToCore(service_worker_task, "svc_worker", SERVICE_WORKER_STACK_SIZE, NULL,
                                SERVICE_WORKER_PRIORITY, &worker_task, SERVICE_WORKER_CORE) != pdPASS) {
        worker_task = nullptr;
        logMessage("WORKER: Failed to create service worker task");
        return false;
    }

    logMessage("WORKER: Service worker task created");
    return true;
}

bool service_worker_submit(const char* label, ServiceWork work, ServiceWork done) {
    if (worker_task == nullptr || jobs_pending >= SERVICE_WORKER_QUEUE_LENGTH) {
        logMessagef("WORKER: Cannot queue %s - worker %s", label, worker_task == nullptr ? "not running" : "queue full");
        return false;
    }

    ServiceJob* job = new ServiceJob{label, std::move(work), std::move(done)};
    if (xQueueSend(job_queue, &job, 0) != pdTRUE) {
        delete job;
        logMessagef("WORKER: Cannot queue %s - queue full", label);
        return false;
    }

    jobs_pending++;
    return true;
}

void service_worker_process_completions() {
    if (done_queue == nullptr) {
        return;
    }

    ServiceJob* job = nullptr;
    while (xQueueReceive(done_queue, &job, 0) == pdTRUE) {
        if (job->done) {
            job->done();
        }
        delete job;
        if (jobs_pending > 0) {
            jobs_pending--;
        }
    }
}
