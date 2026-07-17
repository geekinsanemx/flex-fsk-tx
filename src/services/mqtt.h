/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * MQTT Module - AWS IoT style MQTT client, activity log
 */

#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>

// =============================================================================
// STRUCTS
// =============================================================================
struct MQTTActivity {
    unsigned long timestamp;
    char event[51];
    char details[151];
    char datetime[20];
    float frequency;
    uint64_t capcode;
    bool success;
};

// =============================================================================
// GLOBALS
// =============================================================================
extern PubSubClient mqttClient;
extern uint8_t mqtt_connection_attempt;
extern uint8_t mqtt_reboot_count;
extern bool mqtt_initialized;
extern bool mqtt_suspended;
extern int mqtt_failed_cycles;
extern unsigned long mqtt_next_retry_time;
extern MQTTActivity mqtt_activity_log[10];
extern int mqtt_activity_count;
extern String mqtt_deferred_status_payload;
extern String mqtt_deferred_ack_payload;

extern const int MAX_CONNECTION_FAILURES;
extern const uint8_t MQTT_MAX_REBOOTS;

// =============================================================================
// FUNCTIONS (implemented in Batch 4)
// =============================================================================
bool mqtt_connect();
void mqtt_initialize();
void mqtt_loop();
void save_mqtt_reboot_count();
void load_mqtt_reboot_count();
void mqtt_flush_deferred();

#endif // MQTT_H
