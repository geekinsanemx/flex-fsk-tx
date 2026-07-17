/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * IMAP Module - multi-account IMAP polling, scheduled checks
 */

#ifndef IMAP_H
#define IMAP_H

#include <Arduino.h>

#ifdef ENABLE_IMAP

#include <vector>

// =============================================================================
// STRUCTS
// =============================================================================
struct IMAPAccount {
    uint8_t id;
    char name[32];
    char server[64];
    uint16_t port;
    bool use_ssl;
    char username[64];
    char password[64];
    uint16_t check_interval_min;
    uint64_t capcode;
    float frequency;
    bool mail_drop;
    unsigned long last_check;
    uint8_t failed_check_cycles;
    bool suspended;
};

struct IMAPConfig {
    bool enabled;
    std::vector<IMAPAccount> accounts;
    uint8_t account_count;
};

// =============================================================================
// GLOBALS
// =============================================================================
extern IMAPConfig imap_config;
extern unsigned long last_imap_check;
extern int imap_failed_cycles;

// =============================================================================
// FUNCTIONS (implemented in Batch 4)
// =============================================================================
bool save_imap_config();
bool load_imap_config();
void init_imap_scheduler();
void imap_scheduler_loop();
bool any_imap_accounts_suspended();
bool add_imap_account(const String& name, const String& server, uint16_t port, bool use_ssl,
                      const String& username, const String& password, uint16_t check_interval_min,
                      uint64_t capcode, float frequency, bool mail_drop);
bool edit_imap_account(uint8_t id, const String& name, const String& server, uint16_t port, bool use_ssl,
                       const String& username, const String& password, uint16_t check_interval_min,
                       uint64_t capcode, float frequency, bool mail_drop);
bool delete_imap_account(uint8_t id);

#endif // ENABLE_IMAP

#endif // IMAP_H
