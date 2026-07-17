/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Network Module - WiFi/GSM failover arbitration
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

// =============================================================================
// ENUMS
// =============================================================================
enum ActiveNetwork {
    NETWORK_NONE = 0,
    NETWORK_WIFI_ACTIVE = 1,
    NETWORK_GSM_ACTIVE = 2
};

enum NetworkMode {
    NETWORK_MODE_AUTO = 0,
    NETWORK_MODE_WIFI = 1,
    NETWORK_MODE_GSM = 2,
    NETWORK_MODE_AP = 3
};

// =============================================================================
// GLOBALS
// =============================================================================
extern ActiveNetwork active_network;
extern NetworkMode network_mode;
extern bool network_connect_pending;
extern bool network_available_cached;
extern bool imap_suspended_on_gsm;
extern bool chatgpt_suspended_on_gsm;

// =============================================================================
// FUNCTIONS (implemented in Batch 3)
// =============================================================================
const char* active_network_label(ActiveNetwork network);
bool network_can_mutate();
void network_update_active_state();
void network_boot();
void network_reconnect();
bool network_is_connected();
void network_mode_switch(NetworkMode new_mode);

#endif // NETWORK_H
