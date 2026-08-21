/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * TX Lock Module - Cross-core serialization guards for RF-critical sections
 *
 * On the ESP32 every spi_flash_* operation (SPIFFS read/write, NVS commit)
 * calls spi_flash_disable_interrupts_caches_and_other_cpu(), which disables the
 * instruction cache and parks the opposite CPU in a spin loop. The Core 0
 * transmission task cannot run during that window regardless of its priority,
 * and the SX1276 FIFO refill deadline has no slack, so any flash access from
 * Core 1 during an active transmission corrupts the FLEX bitstream.
 *
 * flash_guard is held by the Core 0 transmission task for the whole RF-active
 * window. Every flash accessor on Core 1 must acquire it first.
 * log_guard protects the shared log ring buffer written from both cores.
 *
 * Lock order is always flash_guard -> log_guard. Never the reverse.
 */

#ifndef TX_LOCK_H
#define TX_LOCK_H

#include <Arduino.h>

#define FLASH_GUARD_WAIT_FOREVER 0xFFFFFFFFUL
#define FLASH_GUARD_TIMEOUT_MS   10000
#define LOG_GUARD_TIMEOUT_MS     100

void tx_lock_init();

bool flash_guard_take(uint32_t timeout_ms);
bool flash_guard_try();
void flash_guard_give();

bool log_guard_take(uint32_t timeout_ms);
void log_guard_give();

// Scope guard for flash accessors with multiple exit paths. The guard is
// recursive, so nesting these across call layers is safe.
class FlashGuard {
public:
    explicit FlashGuard(uint32_t timeout_ms = FLASH_GUARD_TIMEOUT_MS) {
        held = flash_guard_take(timeout_ms);
    }
    ~FlashGuard() {
        if (held) flash_guard_give();
    }
    bool ok() const { return held; }

    FlashGuard(const FlashGuard&) = delete;
    FlashGuard& operator=(const FlashGuard&) = delete;

private:
    bool held;
};

#endif // TX_LOCK_H
