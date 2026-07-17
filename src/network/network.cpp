/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Network Module - WiFi/GSM failover arbitration
 */

#include "../network/network.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../core/display.h"
#include "../core/hardware.h"
#include "../protocol/transmission.h"
#include "../network/wifi.h"
#include "../network/gsm.h"
#include "../services/mqtt.h"
#include "../network/ntp_time.h"

#include <WiFi.h>

// =============================================================================
// GLOBALS
// =============================================================================
ActiveNetwork active_network = NETWORK_NONE;
NetworkMode network_mode = NETWORK_MODE_AUTO;
bool network_connect_pending = false;
bool network_available_cached = false;
bool imap_suspended_on_gsm = false;
bool chatgpt_suspended_on_gsm = false;

// =============================================================================
// FUNCTIONS
// =============================================================================
const char* active_network_label(ActiveNetwork network) {
    switch (network) {
        case NETWORK_WIFI_ACTIVE: return "WiFi";
        case NETWORK_GSM_ACTIVE:  return "GSM";
        default:                  return "None";
    }
}

bool network_can_mutate() {
    return !transmission_guard_active();
}

static void on_network_changed(ActiveNetwork previous, ActiveNetwork current) {
    if (previous == current) {
        return;
    }

    logMessagef("NETWORK: Active transport changed %s -> %s",
                active_network_label(previous), active_network_label(current));

    if (mqtt_initialized && !mqtt_suspended) {
        mqttClient.disconnect();
        delay(100);
        logMessage("MQTT: Client disconnected during transport switch");
    }

    if (mqtt_initialized) {
        mqtt_initialized = false;
        mqtt_suspended = false;
        mqtt_failed_cycles = 0;
        mqtt_reboot_count = 0;
        save_mqtt_reboot_count();
    }

#ifdef ENABLE_GSM
    if (previous == NETWORK_GSM_ACTIVE) {
        gsm_reset_ssl_client();
        logMessage("NETWORK: GSM SSL client cleaned up");
    } else
#endif
    if (previous == NETWORK_WIFI_ACTIVE) {
        wifiClientSecure.stop();
        logMessage("NETWORK: WiFi SSL client cleaned up");
    }

    ntp_sync_in_progress = false;

    if (current == NETWORK_GSM_ACTIVE) {
        imap_suspended_on_gsm = true;
        chatgpt_suspended_on_gsm = true;
        logMessage("NETWORK: IMAP/ChatGPT suspended on GSM");
    } else if (current == NETWORK_WIFI_ACTIVE) {
        imap_suspended_on_gsm = false;
        chatgpt_suspended_on_gsm = false;
        logMessage("NETWORK: IMAP/ChatGPT re-enabled on WiFi");
    } else {
        imap_suspended_on_gsm = false;
        chatgpt_suspended_on_gsm = false;
    }
}

void network_update_active_state() {
    ActiveNetwork new_network = NETWORK_NONE;
    if (wifi_connected) {
        new_network = NETWORK_WIFI_ACTIVE;
    }
#ifdef ENABLE_GSM
    else if (gsm_connected) {
        new_network = NETWORK_GSM_ACTIVE;
    }
#endif

    ActiveNetwork previous = active_network;
    active_network = new_network;
    on_network_changed(previous, active_network);
}

bool network_is_connected() {
    bool wifi_ready = (WiFi.status() == WL_CONNECTED);
#ifdef ENABLE_GSM
    bool gsm_ready = (gsm_connected && gsm_modem_is_gprs_connected());
    return wifi_ready || gsm_ready;
#else
    return wifi_ready;
#endif
}

#ifdef ENABLE_GSM
static bool network_attempt_gsm_boot() {
    if (!gsm_config.enable_gsm) {
        return false;
    }

    logMessage("NETWORK: Attempting GSM connection");

    gsm_boot_modes_exhausted = false;
    gsm_connect();

    unsigned long gsm_boot_start = millis();
    const bool require_internet = gsm_module_requires_internet_verification();
    size_t mode_budget = require_internet ? gsm_module_network_mode_count() : 1;
    unsigned long actual_timeout = (gsm_config.connection_timeout == 0) ? 30000 : gsm_config.connection_timeout;
    unsigned long connection_budget = (unsigned long)actual_timeout * (unsigned long)mode_budget;
    unsigned long internet_budget = mode_budget * 15000UL;
    unsigned long gsm_boot_timeout = connection_budget + internet_budget;
    if (gsm_boot_timeout < 60000UL) {
        gsm_boot_timeout = 60000UL;
    }

    while (((require_internet && !gsm_internet_verified) ||
            (!require_internet && !gsm_connected)) &&
           (millis() - gsm_boot_start < gsm_boot_timeout) &&
           !gsm_boot_modes_exhausted) {
        delay(500);
        check_gsm_connection();

        bool connection_active = (device_state == STATE_GSM_CONNECTING ||
                                  device_state == STATE_GSM_REGISTERING);
        if (!connection_active && !gsm_connected && !gsm_internet_verified) {
            break;
        }
    }

    if ((require_internet && gsm_internet_verified) ||
        (!require_internet && gsm_connected)) {
        logMessagef("NETWORK: Boot completed with GSM using %s network",
                    gsm_current_network_mode_name());
        return true;
    } else if (gsm_boot_modes_exhausted) {
        logMessage("NETWORK: GSM modes exhausted");
        return false;
    } else {
        logMessage("NETWORK: GSM connection failed");
        return false;
    }
}
#endif // ENABLE_GSM

void network_boot() {
    logMessage("NETWORK: Boot sequence starting");

    if (stored_networks_count == 0) {
        logMessage("NETWORK: No WiFi networks configured - entering AP mode");
        start_ap_mode();
        network_boot_complete = true;
        return;
    }

    wifi_ssid_scan();
    bool wifi_available = wifi_scan_available;
    bool wifi_success = false;

    if (wifi_available) {
        logMessage("NETWORK: WiFi available, attempting connection");
        wifi_connect();

        unsigned long wifi_boot_start = millis();
        const unsigned long wifi_boot_timeout = 30000;

        while (!wifi_connected && (millis() - wifi_boot_start < wifi_boot_timeout)) {
            delay(500);
            check_wifi_connection();

            if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
                wifi_auth_failed = true;
                logMessage("NETWORK: WiFi authentication failed after 3 attempts");
                break;
            }
        }

        if (wifi_connected) {
            logMessage("NETWORK: Boot completed with WiFi");
            wifi_success = true;
        } else {
            logMessage("NETWORK: WiFi connection failed");
        }
    } else {
        logMessage("NETWORK: WiFi not available");
    }

    if (wifi_success) {
        boot_phase = BOOT_NETWORK_READY;
        network_boot_complete = true;
        network_update_active_state();
        return;
    }

#ifdef ENABLE_GSM
    if (network_attempt_gsm_boot()) {
        boot_phase = BOOT_NETWORK_READY;
        network_boot_complete = true;
        return;
    }
#endif

    logMessage("NETWORK: No connectivity available - entering AP mode");
    start_ap_mode();
    network_boot_complete = true;
}

void network_reconnect() {
    if (!network_boot_complete) {
        return;
    }

    if (ap_mode_active && network_mode != NETWORK_MODE_AUTO) {
        return;
    }

    if (!network_can_mutate()) {
        network_connect_pending = true;
        return;
    }

    if (network_mode == NETWORK_MODE_WIFI) {
        if (wifi_connected || stored_networks_count == 0) {
            return;
        }
        bool connection_in_progress = (device_state == STATE_WIFI_CONNECTING);
        if (connection_in_progress) {
            return;
        }
        unsigned long scan_interval_ms = 60000UL;
        if ((millis() - last_wifi_scan_ms) < scan_interval_ms) {
            return;
        }
        wifi_ssid_scan();
        if (wifi_scan_available && !wifi_connected) {
            logMessage("NETWORK: WiFi mode - attempting connection");
            wifi_retry_count = 0;
            wifi_connect();
        }
        return;
    }

#ifdef ENABLE_GSM
    if (network_mode == NETWORK_MODE_GSM) {
        if (gsm_connected || !gsm_config.enable_gsm) {
            return;
        }
        bool connection_in_progress = (device_state == STATE_GSM_CONNECTING ||
                                       device_state == STATE_GSM_REGISTERING);
        if (connection_in_progress) {
            return;
        }
        static unsigned long last_gsm_retry = 0;
        unsigned long retry_interval_ms = 300000UL;
        if ((millis() - last_gsm_retry) < retry_interval_ms) {
            return;
        }
        last_gsm_retry = millis();
        logMessage("NETWORK: GSM mode - attempting connection");
        gsm_connect();
        return;
    }
#endif

    if (network_mode == NETWORK_MODE_AP) {
        return;
    }

    if (stored_networks_count == 0 || wifi_connected) {
        return;
    }

    bool connection_in_progress = (device_state == STATE_WIFI_CONNECTING ||
                                   device_state == STATE_GSM_CONNECTING ||
                                   device_state == STATE_GSM_REGISTERING);

    if (connection_in_progress) {
        return;
    }

#ifdef ENABLE_GSM
    if (gsm_connected || wifi_connected) {
        unsigned long scan_interval_ms = (gsm_connected) ? 300000UL : 60000UL;
        if ((millis() - last_wifi_scan_ms) < scan_interval_ms) {
            return;
        }
    }
#else
    if (wifi_connected) {
        unsigned long scan_interval_ms = 60000UL;
        if ((millis() - last_wifi_scan_ms) < scan_interval_ms) {
            return;
        }
    }
#endif

    wifi_ssid_scan();

    if (wifi_scan_available) {
        if (!wifi_connected) {
            logMessage("NETWORK: WiFi available, attempting connection");
            wifi_retry_count = 0;
            wifi_connect();
            wifi_retry_silent = true;

            unsigned long reconnect_start = millis();
            const unsigned long reconnect_timeout = 15000;

            while (!wifi_connected && (millis() - reconnect_start < reconnect_timeout)) {
                delay(500);
                check_wifi_connection();

                if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
                    logMessage("NETWORK: WiFi reconnection failed after 3 attempts");
                    break;
                }
            }

#ifdef ENABLE_GSM
            if (wifi_connected && gsm_connected) {
                logMessage("NETWORK: Switching from GSM to WiFi");
                network_update_active_state();
                gsm_disconnect();
                gsm_power_off();
            } else {
                network_update_active_state();
            }
#else
            network_update_active_state();
#endif

            wifi_retry_silent = false;
        }
    } else {
#ifdef ENABLE_GSM
        if (!wifi_connected && gsm_config.enable_gsm && !gsm_connected) {
            logMessage("NETWORK: WiFi unavailable, attempting GSM connection");
            gsm_connect();
        }
#endif
        network_update_active_state();
    }
}

void network_mode_switch(NetworkMode new_mode) {
    if (new_mode == network_mode) {
        return;
    }

    if (!network_can_mutate()) {
        network_connect_pending = true;
        return;
    }

    logMessagef("NETWORK: Mode switch requested: %s -> %s",
                (network_mode == NETWORK_MODE_AUTO) ? "AUTO" :
                (network_mode == NETWORK_MODE_WIFI) ? "WIFI" :
                (network_mode == NETWORK_MODE_GSM) ? "GSM" : "AP",
                (new_mode == NETWORK_MODE_AUTO) ? "AUTO" :
                (new_mode == NETWORK_MODE_WIFI) ? "WIFI" :
                (new_mode == NETWORK_MODE_GSM) ? "GSM" : "AP");

    network_mode = new_mode;

    if (new_mode == NETWORK_MODE_AUTO) {
        logMessage("NETWORK: Returning to auto mode");
        return;
    }

    if (new_mode == NETWORK_MODE_WIFI) {
#ifdef ENABLE_GSM
        if (gsm_connected) {
            gsm_disconnect();
            gsm_power_off();
        }
#endif
        if (ap_mode_active) {
            WiFi.softAPdisconnect(true);
            ap_mode_active = false;
        }
        wifi_retry_count = 0;
        wifi_connect();
    }
#ifdef ENABLE_GSM
    else if (new_mode == NETWORK_MODE_GSM) {
        if (wifi_connected) {
            WiFi.disconnect(true);
            wifi_connected = false;
        }
        if (ap_mode_active) {
            WiFi.softAPdisconnect(true);
            ap_mode_active = false;
        }
        gsm_connect();
    }
#endif
    else if (new_mode == NETWORK_MODE_AP) {
        if (wifi_connected) {
            WiFi.disconnect(true);
            wifi_connected = false;
        }
#ifdef ENABLE_GSM
        if (gsm_connected) {
            gsm_disconnect();
            gsm_power_off();
        }
#endif
        start_ap_mode();
    }

    network_update_active_state();
    display_status();
}
