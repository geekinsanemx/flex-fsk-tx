/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * MQTT Module - AWS IoT style MQTT client, activity log
 */

#include "../services/mqtt.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../core/hardware.h"
#include "../protocol/transmission.h"
#include "../protocol/flex_protocol.h"
#include "../network/network.h"
#include "../network/gsm.h"
#include "../network/wifi.h"
#include "../network/ntp_time.h"

#include <WiFi.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <memory>

// =============================================================================
// GLOBALS
// =============================================================================
PubSubClient mqttClient;
uint8_t mqtt_connection_attempt = 0;
uint8_t mqtt_reboot_count = 0;
bool mqtt_initialized = false;
bool mqtt_suspended = false;
int mqtt_failed_cycles = 0;
unsigned long mqtt_next_retry_time = 0;
MQTTActivity mqtt_activity_log[10];
int mqtt_activity_count = 0;
String mqtt_deferred_status_payload = "";
String mqtt_deferred_ack_payload = "";

const int MAX_CONNECTION_FAILURES = 5;
const uint8_t MQTT_MAX_REBOOTS = 5;

static unsigned long mqtt_initialized_time = 0;
static bool mqtt_failure_notification_sent = false;
static String current_message_id = "";
static device_state_t mqtt_previous_state = STATE_IDLE;
static bool mqtt_state_active = false;
#ifdef ENABLE_GSM
static std::unique_ptr<SSLClientParameters> gsm_client_mutual_params;
#endif

// =============================================================================
// FORWARD DECLARATIONS (file-local)
// =============================================================================
static inline void restore_mqtt_state();
static void mqtt_callback(char* topic, byte* payload, unsigned int length);
static void mqtt_publish_status(const String& status);
static void sendDeliveryAck(String messageId, String status);
static void mqtt_log_activity(const char* event, const char* details, bool success, float freq = 0.0, uint64_t cap = 0);
static void mqtt_send_suspension_notification();

// =============================================================================
// FUNCTIONS
// =============================================================================
static inline void restore_mqtt_state() {
    if (mqtt_state_active) {
        change_device_state(mqtt_previous_state);
        mqtt_state_active = false;
    }
}

static void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    logMessagef("MQTT: Message received on topic '%s' (%d bytes)", topic, length);

    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        logMessagef("MQTT: JSON parse error: %s", error.c_str());
        logMessagef("MQTT: Problematic JSON: %s", message);

        String raw_msg = String(message);
        int id_start = raw_msg.indexOf("\"id\":\"");
        if (id_start != -1) {
            id_start += 6;
            int id_end = raw_msg.indexOf("\"", id_start);
            if (id_end != -1) {
                String msg_id = raw_msg.substring(id_start, id_end);
                if (msg_id.length() > 0) {
                    sendDeliveryAck(msg_id, "failed");
                }
            }
        }
        return;
    }

    String type = doc["type"] | "";
    String from = doc["from"] | "";
    String msg = doc["message"] | "";
    String subject = doc["subject"] | "";
    String id = doc["id"] | "";
    uint64_t timestamp = doc["ts"] | 0;

    bool capcode_from_msg = doc.containsKey("capcode");
    bool frequency_from_msg = doc.containsKey("frequency");
    bool power_from_msg = doc.containsKey("power");

    uint64_t capcode = capcode_from_msg ? doc["capcode"] : settings.default_capcode;
    float frequency = frequency_from_msg ? doc["frequency"] : settings.default_frequency;
    float power = power_from_msg ? doc["power"] : settings.default_txpower;
    bool mail_drop_from_msg = doc.containsKey("mail_drop");
    bool mail_drop = mail_drop_from_msg ? doc["mail_drop"] : false;

    if (msg.length() == 0) {
        logMessage("MQTT: Message rejected - missing mandatory 'message' field");
        if (id.length() > 0) {
            sendDeliveryAck(id, "failed");
        }
        return;
    }

    if (type.length() == 0 || (type != "paging" && type != "webhook" && type != "email")) {
        if (type.length() > 0) {
            logMessage("MQTT: Unknown type '" + type + "' - defaulting to 'paging'");
        }
        type = "paging";
    }

    float original_frequency = frequency;
    if (frequency >= 1000.0) {
        frequency = frequency / 1000000.0;
        logMessagef("MQTT: Converted frequency from %.0f Hz to %.6f MHz", original_frequency, frequency);
    }

    String paging_message;
    if (type == "email") {
        paging_message = from + "\n" + subject + "\n" + msg;
    } else {
        paging_message = msg;
    }

    paging_message = truncate_message_with_ellipsis(paging_message);

    current_message_id = id;

    DynamicJsonDocument debugDoc(256);
    debugDoc["type"] = type;
    debugDoc["from"] = from;
    debugDoc["subject"] = subject;
    debugDoc["message"] = msg;
    String debugJson;
    serializeJson(debugDoc, debugJson);

    String param_sources = "";
    if (!capcode_from_msg) param_sources += "capcode=default,";
    if (!frequency_from_msg) param_sources += "freq=default,";
    if (!power_from_msg) param_sources += "power=default,";
    String param_summary = "default";
    if (param_sources.length() > 0) {
        param_sources = param_sources.substring(0, param_sources.length() - 1);
        param_summary = param_sources;
    }

    logMessagef("MQTT: Processing %s message id=%s from=%s (params:%s)",
                  type.c_str(), id.length() ? id.c_str() : "none",
                  from.length() ? from.c_str() : "unknown",
                  param_summary.c_str());

    String msg_preview = msg.length() > 50 ? msg.substring(0, 50) + "..." : msg;
    String activity_details = "From: " + (from.length() > 0 ? from : String("MQTT")) +
                              " | Msg: " + msg_preview;

    bool tx_success = queue_add_message(capcode, frequency, power, mail_drop, paging_message.c_str());

    mqtt_log_activity("Message Received", activity_details.c_str(), tx_success,
                      frequency, capcode);

    if (tx_success) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "MQTT: Message queued (id=%s, from=%s, capcode=%llu)",
                 id.length() ? id.c_str() : "none", from.c_str(), capcode);
        logMessage(log_msg);
        char status_msg[128];
        snprintf(status_msg, sizeof(status_msg), "Message queued from %s", from.c_str());
        mqtt_publish_status(status_msg);
    } else {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "MQTT: Queue full, message id=%s from %s rejected",
                 id.length() ? id.c_str() : "none", from.c_str());
        logMessage(log_msg);
        mqtt_publish_status("Queue full - message rejected");
        if (id.length() > 0) {
            sendDeliveryAck(id, "failed");
        }
        current_message_id = "";
    }
}

bool mqtt_connect() {
    logMessage("MQTT: connect start");

    if (!settings.mqtt_enabled || strlen(settings.mqtt_server) == 0) {
        logMessage("MQTT: Disabled or no server configured");
        return false;
    }

    time_t now;
    time(&now);
    logMessagef("MQTT: Current timestamp: %ld", (long)now);

    if (!system_time_initialized || now < 1600000000) {
        logMessage("MQTT: System time not synchronized! SSL connection will likely fail.");
        logMessage("MQTT: Time should be initialized (RTC or NTP) before MQTT initialization");
        return false;
    } else {
        logMessagef("MQTT: System time appears synchronized: %ld (delta %ld s)",
                    (long)now, (long)(now - (time_t)(last_ntp_sync / 1000UL)));
    }

#ifdef ENABLE_GSM
    if (active_network == NETWORK_GSM_ACTIVE) {
        logMessagef("MQTT: GSM GPRS status check: %d", gsm_modem_is_gprs_connected());

        if (!gsm_modem_is_gprs_connected()) {
            logMessage("MQTT: GSM GPRS data session lost, attempting reconnection...");
            bool gprs_restored = gsm_modem_gprs_connect(
                gsm_config.apn,
                gsm_config.apn_user,
                gsm_config.apn_pass
            );
            if (!gprs_restored) {
                logMessage("MQTT: Failed to restore GPRS connection");
                return false;
            }
            logMessage("MQTT: GPRS connection restored");
        }
    }
#endif

    if (!network_is_connected()) {
        logMessage("MQTT: No network connection available");
        return false;
    }

#ifdef ENABLE_GSM
    if (active_network == NETWORK_GSM_ACTIVE && !gsm_internet_verified) {
        logMessage("MQTT: GSM internet not verified yet - deferring MQTT connect");
        return false;
    }
#endif

    logMessage("=== MQTT Connection Debug ===");
    logMessagef("MQTT Server: '%s'", settings.mqtt_server);
    logMessagef("MQTT Port: %d", settings.mqtt_port);
    logMessagef("MQTT Thing Name: '%s'", settings.mqtt_thing_name);
    logMessagef("MQTT Subscribe Topic: '%s'", settings.mqtt_subscribe_topic);
    logMessagef("MQTT Publish Topic: '%s'", settings.mqtt_publish_topic);
    logMessagef("Network Type: %s", active_network_label(active_network));

    String mqtt_ca_cert = loadCertificateFromSPIFFS(MQTT_CA_CERT_FILE);
    String mqtt_device_cert = loadCertificateFromSPIFFS(MQTT_DEVICE_CERT_FILE);
    String mqtt_device_key = loadCertificateFromSPIFFS(MQTT_DEVICE_KEY_FILE);

    if (mqtt_ca_cert.length() == 0 || mqtt_device_cert.length() == 0 || mqtt_device_key.length() == 0) {
        logMessage("MQTT: Missing certificates in SPIFFS - cannot connect to AWS IoT without proper authentication");
        logMessage("   Please upload Root CA, Device Certificate, and Private Key files in the MQTT configuration page");
        return false;
    }

    logMessage("MQTT: Configuring certificate authentication...");

    if (mqtt_ca_cert.length() < 100 || mqtt_ca_cert.indexOf("-----BEGIN CERTIFICATE-----") == -1) {
        logMessage("MQTT: Invalid CA certificate format");
        return false;
    }
    if (mqtt_device_cert.length() < 100 || mqtt_device_cert.indexOf("-----BEGIN CERTIFICATE-----") == -1) {
        logMessage("MQTT: Invalid device certificate format");
        return false;
    }
    if (mqtt_device_key.length() < 100 || mqtt_device_key.indexOf("-----BEGIN") == -1) {
        logMessage("MQTT: Invalid private key format");
        return false;
    }

    if (!mqtt_state_active) {
        mqtt_previous_state = device_state;
    }
    change_device_state(STATE_MQTT_CONNECTING);
    mqtt_state_active = true;

    bool connection_result = false;
    uint16_t socket_timeout = (active_network == NETWORK_GSM_ACTIVE) ? 15 : 5;

#ifdef ENABLE_GSM
    if (active_network == NETWORK_GSM_ACTIVE) {
        gsm_reset_ssl_client();
        SSLClientParameters params = SSLClientParameters::fromPEM(
            mqtt_device_cert.c_str(), mqtt_device_cert.length(),
            mqtt_device_key.c_str(), mqtt_device_key.length());
        gsm_client_mutual_params.reset(new SSLClientParameters(params));
        gsm_ssl_set_mutual_params(*gsm_client_mutual_params);
        gsm_ssl_set_timeout(socket_timeout * 1000UL);
        mqttClient.setClient(gsm_active_ssl_client());
        logMessage("MQTT: Using GSM SSL client");
        logMessage("MQTT: GSM TLS trust anchors loaded (Amazon Root CA 1)");
    } else
#endif
    {
        wifiClientSecure.setCACert(mqtt_ca_cert.c_str());
        wifiClientSecure.setCertificate(mqtt_device_cert.c_str());
        wifiClientSecure.setPrivateKey(mqtt_device_key.c_str());
        wifiClientSecure.setTimeout(socket_timeout);
        mqttClient.setClient(wifiClientSecure);
        logMessage("MQTT: Using WiFi TLS client");
    }

    logMessage("MQTT: SSL Configuration:");
    logMessagef("  - CA cert loaded from SPIFFS (%d chars)", mqtt_ca_cert.length());
    logMessagef("  - Client cert loaded from SPIFFS (%d chars)", mqtt_device_cert.length());
    logMessagef("  - Private key loaded from SPIFFS (%d chars)", mqtt_device_key.length());

    logMessage("MQTT: Certificates configured successfully");

    if (active_network == NETWORK_GSM_ACTIVE) {
        delay(500);
    }

    logMessage("MQTT: Setting server and callback...");
    mqttClient.setServer(settings.mqtt_server, settings.mqtt_port);
    mqttClient.setBufferSize(2048);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(socket_timeout);
    logMessagef("MQTT: socket timeout set to %u s", socket_timeout);
    logMessage("MQTT: Buffer size set to 2048 bytes");
    mqttClient.setCallback(mqtt_callback);

    logMessage("MQTT: Testing SSL connection...");
    logMessagef("MQTT: Attempting connection to %s:%d with client ID %s over %s...",
                settings.mqtt_server, settings.mqtt_port, settings.mqtt_thing_name,
                active_network_label(active_network));

    feed_watchdog();

    unsigned long mqtt_connect_start = millis();
    unsigned long connect_timeout_ms = 10000UL;

    bool connected = mqttClient.connect(settings.mqtt_thing_name, NULL, NULL, NULL, 0, 0, NULL, false);
    unsigned long mqtt_connect_time = millis() - mqtt_connect_start;

    yield();

    logMessagef("MQTT: connect returned %s in %lu ms", connected ? "true" : "false", mqtt_connect_time);
    logMessagef("MQTT: MQTT state code %d", mqttClient.state());

    if (connected) {
        logMessage("MQTT: Connection successful!");

        if (strlen(settings.mqtt_subscribe_topic) > 0) {
            logMessagef("MQTT: Subscribing to topic: '%s' with QoS 1", settings.mqtt_subscribe_topic);
            bool subscribed = mqttClient.subscribe(settings.mqtt_subscribe_topic, 1);
            logMessagef("MQTT: Subscription %s", subscribed ? "successful" : "failed");
        } else {
            logMessage("MQTT: No subscribe topic configured");
        }

        mqtt_failed_cycles = 0;
        mqtt_connection_attempt = 0;

        connection_result = true;
    } else {
        int error_code = mqttClient.state();
        logMessagef("MQTT: Connection failed with error code: %d", error_code);

        switch (error_code) {
            case -4: logMessage("MQTT Error: Connection timeout"); break;
            case -3: logMessage("MQTT Error: Connection lost"); break;
            case -2: logMessage("MQTT Error: Connect failed"); break;
            case -1: logMessage("MQTT Error: Disconnected"); break;
            case 1: logMessage("MQTT Error: Bad protocol version"); break;
            case 2: logMessage("MQTT Error: Bad client ID"); break;
            case 3: logMessage("MQTT Error: Server unavailable"); break;
            case 4: logMessage("MQTT Error: Bad username/password"); break;
            case 5: logMessage("MQTT Error: Not authorized"); break;
            default: logMessagef("MQTT Error: Unknown error code %d", error_code); break;
        }

        logMessagef("MQTT: Active transport: %s", active_network_label(active_network));
        logMessagef("MQTT: Free heap: %d bytes", ESP.getFreeHeap());

#ifdef ENABLE_GSM
        if (active_network == NETWORK_GSM_ACTIVE) {
            int ssl_error = gsm_ssl_get_write_error();
            if (ssl_error != SSLClient::SSL_OK) {
                logMessagef("MQTT: GSM TLS error code %d (%s)",
                            ssl_error, sslclient_error_label(ssl_error));
            }
            gsm_reset_ssl_client();
        } else
#endif
        {
            logMessagef("MQTT: WiFi Status: %d (should be WL_CONNECTED)", WiFi.status());
        }
    }

    restore_mqtt_state();
    return connection_result;
}

void mqtt_initialize() {
    if (!settings.mqtt_enabled || strlen(settings.mqtt_server) == 0) {
        logMessage("MQTT: Initialization skipped - disabled or not configured");
        return;
    }

    if (mqtt_initialized) {
        logMessage("MQTT: Already initialized");
        return;
    }

    if (!certificateExistsInSPIFFS(MQTT_CA_CERT_FILE) || !certificateExistsInSPIFFS(MQTT_DEVICE_CERT_FILE) || !certificateExistsInSPIFFS(MQTT_DEVICE_KEY_FILE)) {
        logMessage("MQTT: Initialization failed - missing certificates in SPIFFS");
        return;
    }

    logMessage("MQTT: Starting initialization...");

    if (!system_time_initialized) {
        logMessage("MQTT: Time sync required before MQTT initialization");
        logMessage("MQTT: Initialization deferred - waiting for time sync (RTC or NTP)");
        return;
    }

    if (mqtt_suspended) {
        mqtt_initialized = true;
        mqtt_initialized_time = millis();
        mqtt_next_retry_time = millis();
        logMessage("MQTT: Starting in suspended state (reboot limit reached)");
        return;
    }

    if (mqtt_connect()) {
        mqtt_initialized = true;
        mqtt_initialized_time = millis();
        logMessage("MQTT: Initialization successful - connected");
    } else {
        mqtt_initialized = true;
        mqtt_initialized_time = millis();
        logMessage("MQTT: Initial connection failed, will retry in mqtt_loop()");
    }
}

void mqtt_loop() {
    static unsigned long lastMqttCheck = 0;
    static unsigned long lastReconnectAttempt = 0;
    static unsigned long connectionEstablishedTime = 0;
    static unsigned long reconnectInterval = 10000;
    static bool wasConnected = false;

    if (!mqtt_initialized) {
        return;
    }

    unsigned long now = millis();

    if (mqtt_suspended) {
        if (mqtt_next_retry_time == 0 || (unsigned long)(now - mqtt_next_retry_time) < settings.mqtt_retry_interval_mins * 60000UL) {
            return;
        }

        logMessagef("MQTT: Retry interval elapsed (%lu minutes), attempting reconnection...", settings.mqtt_retry_interval_mins);
        mqtt_next_retry_time = now;
        mqtt_connection_attempt++;

        if (mqtt_connect()) {
            mqtt_failed_cycles = 0;
            mqtt_suspended = false;
            mqtt_failure_notification_sent = false;
            mqtt_next_retry_time = 0;
            mqtt_reboot_count = 0;
            save_mqtt_reboot_count();
            reconnectInterval = 10000;
            wasConnected = true;
            connectionEstablishedTime = now;
            mqtt_log_activity("Reconnected", "Successfully reconnected after suspension", true);
            logMessage("MQTT: Reconnected successfully after suspension");
        } else {
            mqtt_log_activity("Retry Failed", ("Next retry in " + String(settings.mqtt_retry_interval_mins) + " min").c_str(), false);
            logMessagef("MQTT: Retry failed, next attempt in %lu minutes", settings.mqtt_retry_interval_mins);
        }
        return;
    }

    if ((unsigned long)(now - lastMqttCheck) < 100) return;
    lastMqttCheck = now;

    bool isConnected = mqttClient.connected();

    if (isConnected && !wasConnected) {
        connectionEstablishedTime = now;
        wasConnected = true;
        mqtt_failed_cycles = 0;
        mqtt_suspended = false;
        mqtt_failure_notification_sent = false;
        mqtt_reboot_count = 0;
        save_mqtt_reboot_count();
        mqtt_log_activity("Connected", "Connection established", true);
        logMessagef("MQTT: Connection re-established at %lu ms", now);
    }
    else if (!isConnected && wasConnected) {
        unsigned long duration = now - connectionEstablishedTime;
        wasConnected = false;
        mqtt_log_activity("Disconnected", ("Connection lost after " + String(duration/1000) + "s").c_str(), false);
        logMessagef("MQTT: Connection lost after %lu ms", duration);
        lastReconnectAttempt = now;
    }

    if (!isConnected) {
        if ((unsigned long)(now - lastReconnectAttempt) >= reconnectInterval) {
            mqtt_connection_attempt++;
            logMessagef("MQTT: Attempting reconnection... (last attempt %lu ms ago)",
                          now - lastReconnectAttempt);
            lastReconnectAttempt = now;
            if (mqtt_connect()) {
                connectionEstablishedTime = now;
                wasConnected = true;
                mqtt_failed_cycles = 0;
                mqtt_suspended = false;
                mqtt_failure_notification_sent = false;
                mqtt_reboot_count = 0;
                save_mqtt_reboot_count();
                reconnectInterval = 10000;
                logMessage("MQTT: Connected successfully");
            } else {
                reconnectInterval = random(1000, 30001);

                mqtt_failed_cycles++;
                logMessagef("MQTT: Connection failed (%d/%d failures), next retry in %lu ms",
                           mqtt_failed_cycles, MAX_CONNECTION_FAILURES, reconnectInterval);

                if (mqtt_failed_cycles >= MAX_CONNECTION_FAILURES) {
                    mqtt_failed_cycles = 0;
                    mqtt_reboot_count++;
                    save_mqtt_reboot_count();

                    if (mqtt_reboot_count >= MQTT_MAX_REBOOTS) {
                        mqtt_suspended = true;
                        mqtt_next_retry_time = now;
                        mqtt_send_suspension_notification();
                        mqtt_log_activity("Suspended", ("After " + String(MQTT_MAX_REBOOTS) + " reboots, retrying every " + String(settings.mqtt_retry_interval_mins) + " min").c_str(), false);
                        logMessagef("MQTT: Reboot limit (%u) reached, suspended - retrying every %lu minutes",
                                    MQTT_MAX_REBOOTS, settings.mqtt_retry_interval_mins);
                    } else {
                        logMessagef("MQTT: %d failures, rebooting (reboot %u/%u)",
                                    MAX_CONNECTION_FAILURES, mqtt_reboot_count, MQTT_MAX_REBOOTS);
                        delay(100);
                        ESP.restart();
                    }
                }
            }
        }
    }
    else {
        if (!mqttClient.loop()) {
            logMessagef("MQTT: WARNING - loop() returned false at %lu ms", now);
        }
    }
}

static void mqtt_publish_status(const String& status) {
    if (!settings.mqtt_enabled || strlen(settings.mqtt_publish_topic) == 0) {
        return;
    }

    DynamicJsonDocument doc(256);
    doc["device"] = settings.mqtt_thing_name;
    doc["status"] = status;
    doc["timestamp"] = millis();

    String output;
    serializeJson(doc, output);

    if (transmission_guard_active()) {
        mqtt_deferred_status_payload = output;
        return;
    }

    if (mqttClient.connected()) {
        if (mqttClient.publish(settings.mqtt_publish_topic, output.c_str())) {
            logMessagef("MQTT: Status published: %s", status.c_str());
        } else {
            mqtt_deferred_status_payload = output;
        }
    } else {
        mqtt_deferred_status_payload = output;
    }
}

static void sendDeliveryAck(String messageId, String status) {
    if (!settings.mqtt_enabled || strlen(settings.mqtt_publish_topic) == 0 || messageId.length() == 0) {
        return;
    }

    DynamicJsonDocument ackDoc(256);
    ackDoc["type"] = "delivery_ack";
    ackDoc["message_id"] = messageId;
    ackDoc["status"] = status;
    ackDoc["timestamp"] = getUnixTimestamp();
    ackDoc["device"] = String(settings.mqtt_thing_name);

    String ackPayload;
    serializeJson(ackDoc, ackPayload);

    if (transmission_guard_active() || !mqttClient.connected()) {
        mqtt_deferred_ack_payload = ackPayload;
        return;
    }

    if (mqttClient.publish(settings.mqtt_publish_topic, ackPayload.c_str())) {
        logMessagef("MQTT: Delivery ACK sent for %s (%s)", messageId.c_str(), status.c_str());
    } else {
        mqtt_deferred_ack_payload = ackPayload;
    }
}

void mqtt_flush_deferred() {
    if (!settings.mqtt_enabled || !mqttClient.connected()) {
        return;
    }

    if (mqtt_deferred_ack_payload.length() > 0) {
        if (mqttClient.publish(settings.mqtt_publish_topic, mqtt_deferred_ack_payload.c_str())) {
            DynamicJsonDocument doc(256);
            deserializeJson(doc, mqtt_deferred_ack_payload);
            const char* msgId = doc["message_id"];
            const char* msgStatus = doc["status"];
            logMessagef("MQTT: Delivery ACK sent for %s (%s)", msgId, msgStatus);
            mqtt_deferred_ack_payload = "";
        } else {
            return;
        }
    }

    if (mqtt_deferred_status_payload.length() > 0) {
        if (mqttClient.publish(settings.mqtt_publish_topic, mqtt_deferred_status_payload.c_str())) {
            DynamicJsonDocument doc(256);
            deserializeJson(doc, mqtt_deferred_status_payload);
            const char* statusMsg = doc["status"];
            logMessagef("MQTT: Status published: %s", statusMsg);
            mqtt_deferred_status_payload = "";
        }
    }
}

static void mqtt_log_activity(const char* event, const char* details, bool success, float freq, uint64_t cap) {
    if (mqtt_activity_count >= 10) {
        for (int i = 0; i < 9; i++) {
            mqtt_activity_log[i] = mqtt_activity_log[i + 1];
        }
        mqtt_activity_count = 9;
    }

    MQTTActivity& activity = mqtt_activity_log[mqtt_activity_count];
    activity.timestamp = millis();
    strlcpy(activity.event, event, sizeof(activity.event));
    strlcpy(activity.details, details, sizeof(activity.details));
    activity.success = success;
    activity.frequency = freq;
    activity.capcode = cap;

    time_t now = getLocalTimestamp();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(activity.datetime, sizeof(activity.datetime), "%b %d %H:%M:%S", &timeinfo);

    mqtt_activity_count++;
}

static void mqtt_send_suspension_notification() {
    if (!settings.mqtt_notify_failures) {
        return;
    }

    if (mqtt_failure_notification_sent) {
        return;
    }

    String msg = "MQTT service suspended after " + String(MAX_CONNECTION_FAILURES) +
                 " failures. Retrying every " + String(settings.mqtt_retry_interval_mins) + " minutes.";

    if (queue_add_message(settings.default_capcode, settings.default_frequency,
                         settings.default_txpower, false, msg.c_str())) {
        mqtt_failure_notification_sent = true;
        logMessage("MQTT: Suspension notification sent to pager");
    }
}

void load_mqtt_reboot_count() {
    Preferences prefs;
    prefs.begin("mqtt_retry", true);
    mqtt_reboot_count = prefs.getUChar("reboots", 0);
    prefs.end();
}

void save_mqtt_reboot_count() {
    Preferences prefs;
    prefs.begin("mqtt_retry", false);
    prefs.putUChar("reboots", mqtt_reboot_count);
    prefs.end();
}
