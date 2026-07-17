/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * WiFi Module - stored networks, AP mode, connection state
 */

#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>
#include <IPAddress.h>
#include <WiFiClientSecure.h>

// =============================================================================
// STRUCTS
// =============================================================================
struct WiFiNetwork {
    char ssid[33];
    char password[65];
    bool use_dhcp;
    uint8_t static_ip[4];
    uint8_t netmask[4];
    uint8_t gateway[4];
    uint8_t dns[4];
};

// =============================================================================
// GLOBALS
// =============================================================================
extern WiFiNetwork stored_networks[];
extern int stored_networks_count;
extern String current_connected_ssid;

extern bool wifi_connected;
extern bool ap_mode_active;
extern unsigned long wifi_connect_start;
extern int wifi_retry_count;
extern bool wifi_retry_silent;
extern bool wifi_scan_available;
extern bool wifi_auth_failed;
extern bool network_boot_complete;
extern unsigned long last_wifi_scan_ms;
extern IPAddress device_ip;
extern String ap_ssid;
extern String ap_password;
extern String mac_suffix;
extern WiFiClientSecure wifiClientSecure;

// =============================================================================
// FUNCTIONS (implemented in Batch 3)
// =============================================================================
bool wifi_ssid_scan();
void wifi_connect();
bool scan_and_connect_wifi();
void start_ap_mode();
void check_wifi_connection();
String generate_ap_ssid();
String generate_ap_password();

#endif // WIFI_H
