#include "../core/tx_lock.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static SemaphoreHandle_t flash_guard_mutex = NULL;
static SemaphoreHandle_t log_guard_mutex   = NULL;

static TickType_t guard_ticks(uint32_t timeout_ms) {
    if (timeout_ms == FLASH_GUARD_WAIT_FOREVER) {
        return portMAX_DELAY;
    }
    return pdMS_TO_TICKS(timeout_ms);
}

void tx_lock_init() {
    if (flash_guard_mutex == NULL) {
        flash_guard_mutex = xSemaphoreCreateRecursiveMutex();
    }
    if (log_guard_mutex == NULL) {
        log_guard_mutex = xSemaphoreCreateRecursiveMutex();
    }
}

bool flash_guard_take(uint32_t timeout_ms) {
    if (flash_guard_mutex == NULL) return true;
    return xSemaphoreTakeRecursive(flash_guard_mutex, guard_ticks(timeout_ms)) == pdTRUE;
}

bool flash_guard_try() {
    if (flash_guard_mutex == NULL) return true;
    return xSemaphoreTakeRecursive(flash_guard_mutex, 0) == pdTRUE;
}

void flash_guard_give() {
    if (flash_guard_mutex == NULL) return;
    xSemaphoreGiveRecursive(flash_guard_mutex);
}

bool log_guard_take(uint32_t timeout_ms) {
    if (log_guard_mutex == NULL) return true;
    return xSemaphoreTakeRecursive(log_guard_mutex, guard_ticks(timeout_ms)) == pdTRUE;
}

void log_guard_give() {
    if (log_guard_mutex == NULL) return;
    xSemaphoreGiveRecursive(log_guard_mutex);
}
