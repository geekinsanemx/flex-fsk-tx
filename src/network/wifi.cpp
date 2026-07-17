/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * WiFi Module - stored networks, AP mode, connection state
 */

#include "../network/wifi.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../core/display.h"
#include "../core/hardware.h"
#include "../protocol/transmission.h"
#include "../network/network.h"
#include "../network/gsm.h"

#include <WiFi.h>

// =============================================================================
// GLOBALS
// =============================================================================
WiFiNetwork stored_networks[MAX_WIFI_NETWORKS];
int stored_networks_count = 0;
String current_connected_ssid = "";

bool wifi_connected = false;
bool ap_mode_active = false;
unsigned long wifi_connect_start = 0;
int wifi_retry_count = 0;
bool wifi_retry_silent = false;
bool wifi_scan_available = false;
bool wifi_auth_failed = false;
bool network_boot_complete = false;
unsigned long last_wifi_scan_ms = 0;
IPAddress device_ip;
String ap_ssid = "";
String ap_password = "";
String mac_suffix = "";
WiFiClientSecure wifiClientSecure;

// =============================================================================
// FUNCTIONS
// =============================================================================
String generate_ap_ssid() {
    WiFi.mode(WIFI_STA);
    delay(100);

    uint8_t mac[6];
    WiFi.macAddress(mac);

    uint32_t unique_id = (mac[3] << 16) | (mac[4] << 8) | mac[5];

    String suffix = String(unique_id & 0xFFFF, HEX);
    suffix.toUpperCase();
    while (suffix.length() < 4) {
        suffix = "0" + suffix;
    }

    String macStr = "SYSTEM: Device MAC: ";
    for (int i = 0; i < 6; i++) {
        if (i > 0) macStr += ":";
        if (mac[i] < 16) macStr += "0";
        macStr += String(mac[i], HEX);
    }
    logMessage(macStr);
    logMessage("SYSTEM: Generated AP SSID: FLEX_" + suffix);

    return "FLEX_" + suffix;
}

String generate_ap_password() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    char password[9];
    sprintf(password, "%02X%02X%02X%02X",
            mac[2], mac[3], mac[4], mac[5]);

    return String(password);
}

bool wifi_ssid_scan() {
    logMessage("WiFi: Scanning for stored networks...");

    int n = WiFi.scanNetworks();

    if (n == WIFI_SCAN_FAILED || n < 0) {
        logMessagef("WiFi: Scan failed (code: %d), reinitializing...", n);
        WiFi.disconnect(true, false);
        WiFi.mode(WIFI_STA);
        delay(200);

        n = WiFi.scanNetworks();
        if (n == WIFI_SCAN_FAILED || n < 0) {
            logMessagef("WiFi: Scan failed after retry (code: %d)", n);
            wifi_scan_available = false;
            WiFi.scanDelete();
            last_wifi_scan_ms = millis();
            return false;
        }
        logMessagef("WiFi: Scan recovered, found %d networks", n);
    }

    wifi_scan_available = false;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < stored_networks_count; j++) {
            if (WiFi.SSID(i) == String(stored_networks[j].ssid)) {
                wifi_scan_available = true;
                logMessagef("WiFi: Stored network '%s' found (RSSI: %d dBm)",
                           stored_networks[j].ssid, WiFi.RSSI(i));
                break;
            }
        }
        if (wifi_scan_available) break;
    }

    if (!wifi_scan_available) {
        logMessagef("WiFi: No stored networks found (%d networks scanned)", n);
    }

    WiFi.scanDelete();
    last_wifi_scan_ms = millis();
    return wifi_scan_available;
}

bool scan_and_connect_wifi() {
    if (stored_networks_count == 0) {
        logMessage("WIFI: No stored networks configured");
        return false;
    }

    logMessagef("WIFI: Scanning for networks (have %d stored)", stored_networks_count);

    int n = WiFi.scanNetworks();

    if (n == 0) {
        logMessage("WIFI: No networks found in scan");
        return false;
    }

    logMessagef("WIFI: Scan complete, found %d network(s)", n);

    for (int j = 0; j < n; j++) {
        logMessagef("  [%d] %s (RSSI: %d dBm, Ch: %d, Enc: %s)",
                    j + 1,
                    WiFi.SSID(j).c_str(),
                    WiFi.RSSI(j),
                    WiFi.channel(j),
                    (WiFi.encryptionType(j) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted");
    }

    for (int i = 0; i < stored_networks_count; i++) {
        String stored_ssid = String(stored_networks[i].ssid);

        bool found = false;
        int rssi = 0;

        for (int j = 0; j < n; j++) {
            if (WiFi.SSID(j) == stored_ssid) {
                found = true;
                rssi = WiFi.RSSI(j);
                break;
            }
        }

        if (!found) {
            logMessagef("WIFI: Stored network '%s' not in range", stored_ssid.c_str());
            continue;
        }

        logMessagef("WIFI: Attempting connection to '%s' (RSSI: %d dBm, Priority: %d)",
                    stored_ssid.c_str(), rssi, i + 1);

        WiFi.disconnect();
        WiFi.mode(WIFI_STA);
        delay(100);

        if (!stored_networks[i].use_dhcp) {
            logMessage("WIFI: Configuring static IP");

            IPAddress ip(stored_networks[i].static_ip[0],
                         stored_networks[i].static_ip[1],
                         stored_networks[i].static_ip[2],
                         stored_networks[i].static_ip[3]);

            IPAddress netmask(stored_networks[i].netmask[0],
                              stored_networks[i].netmask[1],
                              stored_networks[i].netmask[2],
                              stored_networks[i].netmask[3]);

            IPAddress gateway(stored_networks[i].gateway[0],
                              stored_networks[i].gateway[1],
                              stored_networks[i].gateway[2],
                              stored_networks[i].gateway[3]);

            IPAddress dns(stored_networks[i].dns[0],
                          stored_networks[i].dns[1],
                          stored_networks[i].dns[2],
                          stored_networks[i].dns[3]);

            if (!WiFi.config(ip, gateway, netmask, dns)) {
                logMessage("WIFI: Static IP configuration failed, skipping");
                continue;
            }

            logMessagef("WIFI: Static IP: %s, Gateway: %s", ip.toString().c_str(), gateway.toString().c_str());
        } else {
            logMessage("WIFI: Using DHCP");
        }

        WiFi.begin(stored_networks[i].ssid, stored_networks[i].password);

        unsigned long connect_start = millis();
        int dots = 0;

        while (WiFi.status() != WL_CONNECTED && (millis() - connect_start) < 15000) {
            delay(500);
            dots++;
            if (dots % 2 == 0) {
                logMessage("WIFI: Connecting...");
            }
        }

        if (WiFi.status() == WL_CONNECTED) {
            current_connected_ssid = stored_ssid;
            logMessage("WIFI: ========================================");
            logMessagef("WIFI: \xE2\x9C\x93 CONNECTED to '%s'", stored_ssid.c_str());
            logMessagef("WIFI: IP Address: %s", WiFi.localIP().toString().c_str());
            logMessagef("WIFI: Subnet Mask: %s", WiFi.subnetMask().toString().c_str());
            logMessagef("WIFI: Gateway: %s", WiFi.gatewayIP().toString().c_str());
            logMessagef("WIFI: DNS: %s", WiFi.dnsIP().toString().c_str());
            logMessagef("WIFI: RSSI: %d dBm", WiFi.RSSI());
            logMessage("WIFI: ========================================");
            return true;
        } else {
            logMessagef("WIFI: \xE2\x9C\x97 Failed to connect to '%s' (timeout)", stored_ssid.c_str());
        }
    }

    logMessage("WIFI: No stored networks could be reached");
    return false;
}

void wifi_connect() {
    if (stored_networks_count == 0) {
        start_ap_mode();
        network_connect_pending = true;
        return;
    }

    device_state = STATE_WIFI_CONNECTING;
    display_status();

    WiFi.mode(WIFI_STA);

    WiFi.disconnect(true, true);
    delay(50);

    if (mac_suffix == "") {
        delay(100);
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char suffix[5];
        sprintf(suffix, "%02X%02X", mac[4], mac[5]);
        mac_suffix = String(suffix);
    }

    scan_and_connect_wifi();
    wifi_connect_start = millis();
    wifi_retry_count = 0;
    wifi_retry_silent = false;
}

void start_ap_mode() {
    device_state = STATE_WIFI_AP_MODE;
    ap_mode_active = true;

    if (ap_ssid == "") {
        ap_ssid = generate_ap_ssid();
        ap_password = generate_ap_password();
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());

    device_ip = WiFi.softAPIP();
    display_status();

    logMessage("WIFI: AP mode started");
    logMessage("WIFI: AP SSID: " + String(ap_ssid));
    logMessage("WIFI: AP password: " + ap_password + " (MAC-based)");
    logMessage("WIFI: AP IP address: " + device_ip.toString());
}

void check_wifi_connection() {
    if (device_state == STATE_WIFI_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            wifi_connected = true;
            device_ip = WiFi.localIP();
            device_state = STATE_IDLE;

            logMessage("WIFI: Connected successfully");
            logMessage("WIFI: IP address: " + device_ip.toString());

            if (boot_phase == BOOT_NETWORK_PENDING) {
                boot_phase = BOOT_NETWORK_READY;
                logMessage("BOOT: Phase -> BOOT_NETWORK_READY");
            }

#ifdef ENABLE_GSM
            if (gsm_connected) {
                logMessage("WIFI: Switching from GSM to WiFi");
                network_update_active_state();
                gsm_disconnect();
                gsm_power_off();
            } else {
                network_update_active_state();
            }
#else
            network_update_active_state();
#endif

            display_status();

            wifi_retry_silent = false;
        } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
            wifi_retry_count++;
            if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
#ifdef ENABLE_GSM
                if (gsm_config.enable_gsm && (gsm_connected || active_network == NETWORK_GSM_ACTIVE)) {
                    logMessage("WIFI: Authentication failed (GSM already active)");
                } else {
                    logMessage("WIFI: Authentication failed, enabling GSM fallback");
                }
#else
                logMessage("WIFI: Authentication failed");
#endif
                device_state = STATE_IDLE;
                wifi_retry_silent = false;
            } else {
                if (!wifi_retry_silent) {
                    logMessagef("WIFI: Retry attempt %d", wifi_retry_count);
                }
                scan_and_connect_wifi();
                wifi_connect_start = millis();
            }
        } else if ((unsigned long)(millis() - wifi_connect_start) > WIFI_CONNECT_TIMEOUT) {
            wifi_retry_count++;
            if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
#ifdef ENABLE_GSM
                if (gsm_config.enable_gsm && (gsm_connected || active_network == NETWORK_GSM_ACTIVE)) {
                    logMessage("WIFI: Connection timeout (GSM already active)");
                } else {
                    logMessage("WIFI: Connection timeout, enabling GSM fallback");
                }
#else
                logMessage("WIFI: Connection timeout");
#endif
                device_state = STATE_IDLE;
                wifi_retry_silent = false;
            } else {
                if (!wifi_retry_silent) {
                    logMessagef("WIFI: Timeout retry attempt %d", wifi_retry_count);
                }
                scan_and_connect_wifi();
                wifi_connect_start = millis();
            }
        }
    }
}
