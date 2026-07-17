/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * GSM Module - A7670SA/SIM800L modem transport (SIM7600/SIM800 TinyGSM drivers)
 */

#ifndef GSM_H
#define GSM_H

#include <Arduino.h>

#ifdef ENABLE_GSM

#include <SSLClient.h>
#include <SSLClientParameters.h>

// =============================================================================
// ENUMS
// =============================================================================
enum GsmModuleType {
    GSM_MODULE_A7670SA = 0,
    GSM_MODULE_SIM800L = 1
};

// =============================================================================
// STRUCTS
// =============================================================================
struct GSMConfig {
    bool enable_gsm;
    char apn[64];
    char apn_user[33];
    char apn_pass[33];
    char pin[9];
    uint8_t tx_pin;
    uint8_t rx_pin;
    uint8_t power_pin;
    uint32_t baudrate;
    uint32_t connection_timeout;
    bool require_cell_signal;
    uint8_t min_signal_quality;
};

// =============================================================================
// GLOBALS
// =============================================================================
extern GSMConfig gsm_config;
extern HardwareSerial SerialGSM;

extern bool gsm_connected;
extern bool gsm_internet_verified;
extern uint8_t gsm_internet_test_attempt;
extern String gsm_ip_address;
extern bool gsm_boot_modes_exhausted;

extern GsmModuleType gsm_module_type;
extern bool gsm_module_detected;
extern bool gsm_power_state;
extern bool gsm_modem_ready;
extern bool gsm_registration_complete;
extern String gsm_operator_name;
extern int gsm_signal_quality;
extern String gsm_gateway_address;
extern String gsm_dns_primary;
extern String gsm_dns_secondary;
extern String gsm_subnet_mask;

// =============================================================================
// FUNCTIONS (implemented in Batch 3)
// =============================================================================
const char* gsm_module_label(GsmModuleType module);
bool gsm_initialize();
void gsm_connect();
void gsm_disconnect();
void gsm_power_off();
void check_gsm_connection();
void gsm_update_network_info();
void gsm_reset_ssl_client();
String gsm_get_ip_address();

// Cross-module GSM module-capability queries (used by network.cpp boot arbitration)
bool gsm_module_requires_internet_verification();
size_t gsm_module_network_mode_count();
const char* gsm_current_network_mode_name();

// Cross-module GSM SSL/GPRS access (used by mqtt.cpp for GSM-transport MQTT)
bool gsm_modem_is_gprs_connected();
bool gsm_modem_gprs_connect(const char* apn, const char* user, const char* pass);
SSLClient& gsm_active_ssl_client();
void gsm_ssl_set_mutual_params(const SSLClientParameters& params);
void gsm_ssl_set_timeout(uint32_t timeout_ms);
int gsm_ssl_get_write_error();
const char* sslclient_error_label(int error);

#endif // ENABLE_GSM

#endif // GSM_H
