/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Web Server Module - REST API message and WiFi management handlers
 */

#include "../web/web_server.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../core/display.h"
#include "../network/wifi.h"
#include "../protocol/flex_protocol.h"
#include "../protocol/transmission.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// =============================================================================
// REST API MESSAGE SEND
// =============================================================================

void handle_api_message() {
    reset_oled_timeout();

    if (!settings.api_enabled) {
        webServer.send(503, "application/json", "{\"error\":\"API service is disabled\"}");
        return;
    }

    if (!authenticate_api_request()) {
        webServer.sendHeader("WWW-Authenticate", "Basic realm=\"FLEX API\"");
        webServer.send(401, "application/json", "{\"error\":\"Authentication required\"}");
        return;
    }

    if (webServer.method() != HTTP_POST) {
        webServer.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
        return;
    }

    if (!webServer.hasArg("plain")) {
        webServer.send(400, "application/json", "{\"error\":\"No JSON payload\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

    if (error) {
        webServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc["message"].is<String>()) {
        webServer.send(400, "application/json", "{\"error\":\"Missing required field: message\"}");
        return;
    }

    uint64_t capcode = settings.default_capcode;
    if (doc["capcode"].is<uint64_t>()) {
        capcode = doc["capcode"];
    } else if (doc["capcode"].is<String>()) {
        capcode = strtoull(doc["capcode"].as<String>().c_str(), nullptr, 10);
    }

    float frequency = settings.default_frequency;
    if (doc["frequency"].is<float>()) {
        frequency = doc["frequency"];
    } else if (doc["frequency"].is<double>()) {
        frequency = (float)doc["frequency"].as<double>();
    } else if (doc["frequency"].is<String>()) {
        frequency = atof(doc["frequency"].as<String>().c_str());
    }

    String message = doc["message"].as<String>();

    int power = settings.default_txpower;
    if (doc["power"].is<int>()) {
        power = doc["power"];
    } else if (doc["power"].is<String>()) {
        power = atoi(doc["power"].as<String>().c_str());
    } else if (doc["tx_power"].is<int>()) {
        power = doc["tx_power"];
    } else if (doc["tx_power"].is<String>()) {
        power = atoi(doc["tx_power"].as<String>().c_str());
    }

    bool mail_drop = false;
    if (doc["mail_drop"].is<bool>()) {
        mail_drop = doc["mail_drop"];
    } else if (doc["mail_drop"].is<String>()) {
        String mail_drop_str = doc["mail_drop"].as<String>();
        mail_drop_str.toLowerCase();
        mail_drop = (mail_drop_str == "true" || mail_drop_str == "1");
    }

    if (frequency > 1000.0) {
        frequency = frequency / 1000000.0;
    }

    if (frequency < 400.0 || frequency > 1000.0) {
        webServer.send(400, "application/json", "{\"error\":\"Frequency must be between 400.0-1000.0 MHz or 400000000-1000000000 Hz\"}");
        return;
    }

    if (power < 0 || power > 20) {
        webServer.send(400, "application/json", "{\"error\":\"TX Power must be between 0 and 20 dBm\"}");
        return;
    }

    if (message.length() == 0) {
        webServer.send(400, "application/json", "{\"error\":\"Message cannot be empty\"}");
        return;
    }

    bool message_was_truncated = false;
    if (message.length() > MAX_FLEX_MESSAGE_LENGTH) {
        message = truncate_message_with_ellipsis(message);
        message_was_truncated = true;
    }

    if (queue_add_message(capcode, frequency, power, mail_drop, message.c_str())) {
        JsonDocument response;
        response["frequency"] = frequency;
        response["power"] = power;
        response["capcode"] = capcode;
        response["text"] = message;
        response["truncated"] = message_was_truncated;

        if (device_state == STATE_IDLE) {
            response["status"] = "queued";
            if (message_was_truncated) {
                response["message"] = "Message truncated to 248 chars and queued for immediate transmission";
            } else {
                response["message"] = "Message queued for immediate transmission";
            }
        } else {
            response["status"] = "queued";
            if (message_was_truncated) {
                response["message"] = "Message truncated to 248 chars and queued for transmission";
            } else {
                response["message"] = "Message queued for transmission";
            }
            response["queue_position"] = queue_count;
        }

        String response_str;
        serializeJson(response, response_str);
        webServer.send(200, "application/json", response_str);
    } else {
        JsonDocument response;
        response["status"] = "error";
        response["message"] = "Queue is full. Please try again later.";
        response["max_queue_size"] = MAX_QUEUE_SIZE;

        String response_str;
        serializeJson(response, response_str);
        webServer.send(503, "application/json", response_str);
    }
}

// =============================================================================
// WIFI MANAGEMENT API
// =============================================================================

void handle_api_wifi_scan() {
    logMessage("API: WiFi scan requested");

    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_RUNNING) {
        DynamicJsonDocument doc(256);
        doc["success"] = false;
        doc["scanning"] = true;
        String response;
        serializeJson(doc, response);
        webServer.send(202, "application/json", response);
        logMessage("API: Scan already in progress");
        return;
    }

    if (n >= 0) {
        WiFi.scanDelete();
    }

    WiFi.scanNetworks(true, false);

    unsigned long scan_start = millis();
    while (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
        if ((unsigned long)(millis() - scan_start) > 10000) {
            logMessage("API: Scan timeout after 10s");
            DynamicJsonDocument doc(256);
            doc["success"] = false;
            doc["error"] = "Scan timeout";
            String response;
            serializeJson(doc, response);
            webServer.send(500, "application/json", response);
            return;
        }
        delay(100);
        webServer.handleClient();
    }

    n = WiFi.scanComplete();

    if (n == WIFI_SCAN_FAILED || n < 0) {
        logMessagef("API: WiFi scan failed (code: %d), reinitializing...", n);
        WiFi.disconnect(true, false);
        WiFi.mode(WIFI_STA);
        delay(200);

        n = WiFi.scanNetworks();
        if (n == WIFI_SCAN_FAILED || n < 0) {
            logMessagef("API: WiFi scan failed after retry (code: %d)", n);
            DynamicJsonDocument doc(256);
            doc["success"] = false;
            doc["error"] = "WiFi scan failed";
            String response;
            serializeJson(doc, response);
            webServer.send(500, "application/json", response);
            return;
        }
        logMessagef("API: WiFi scan recovered, found %d networks", n);
    }

    DynamicJsonDocument doc(2048);
    doc["success"] = (n >= 0);

    if (n > 0) {
        JsonArray networks = doc.createNestedArray("networks");

        for (int i = 0; i < n; i++) {
            JsonObject net = networks.createNestedObject();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["channel"] = WiFi.channel(i);
            net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted";

            bool is_stored = false;
            for (int j = 0; j < stored_networks_count; j++) {
                if (String(stored_networks[j].ssid) == WiFi.SSID(i)) {
                    is_stored = true;
                    break;
                }
            }
            net["stored"] = is_stored;
        }

        logMessagef("API: Scan found %d networks", n);
    } else {
        logMessage("API: Scan found no networks");
    }

    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
}

void handle_api_wifi_delete() {
    if (!webServer.hasArg("ssid")) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Missing SSID parameter\"}");
        return;
    }

    String ssid_to_delete = webServer.arg("ssid");
    logMessagef("API: Delete network '%s' requested", ssid_to_delete.c_str());

    int network_idx = -1;
    for (int i = 0; i < stored_networks_count; i++) {
        if (String(stored_networks[i].ssid) == ssid_to_delete) {
            network_idx = i;
            break;
        }
    }

    if (network_idx == -1) {
        webServer.send(404, "application/json", "{\"success\":false,\"message\":\"Network not found\"}");
        return;
    }

    // Shift remaining networks down
    for (int i = network_idx; i < stored_networks_count - 1; i++) {
        stored_networks[i] = stored_networks[i + 1];
    }
    stored_networks_count--;

    if (save_runtime_settings()) {
        logMessagef("API: Network '%s' deleted successfully", ssid_to_delete.c_str());
        webServer.send(200, "application/json", "{\"success\":true,\"message\":\"Network deleted\"}");
    } else {
        logMessage("API: Failed to save settings after network deletion");
        webServer.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save settings\"}");
    }
}

void handle_api_wifi_add() {
    reset_oled_timeout();

    String ssid = webServer.arg("ssid");
    String password = webServer.arg("password");
    ssid.trim();
    password.trim();

    if (ssid.length() == 0) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"SSID required\"}");
        return;
    }

    if (password.length() == 0) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Password required\"}");
        return;
    }

    int network_idx = -1;
    for (int i = 0; i < stored_networks_count; i++) {
        if (String(stored_networks[i].ssid) == ssid) {
            network_idx = i;
            break;
        }
    }

    if (network_idx == -1 && stored_networks_count < MAX_WIFI_NETWORKS) {
        network_idx = stored_networks_count;
        stored_networks_count++;
    } else if (network_idx == -1) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Maximum networks reached\"}");
        return;
    }

    strlcpy(stored_networks[network_idx].ssid, ssid.c_str(), sizeof(stored_networks[network_idx].ssid));
    strlcpy(stored_networks[network_idx].password, password.c_str(), sizeof(stored_networks[network_idx].password));
    stored_networks[network_idx].use_dhcp = (webServer.arg("use_dhcp") == "1");

    if (webServer.hasArg("static_ip")) {
        IPAddress parsed_ip;
        if (parsed_ip.fromString(webServer.arg("static_ip"))) {
            for (int i = 0; i < 4; i++) stored_networks[network_idx].static_ip[i] = parsed_ip[i];
        }
    }

    if (webServer.hasArg("netmask")) {
        IPAddress parsed_netmask;
        if (parsed_netmask.fromString(webServer.arg("netmask"))) {
            for (int i = 0; i < 4; i++) stored_networks[network_idx].netmask[i] = parsed_netmask[i];
        }
    }

    if (webServer.hasArg("gateway")) {
        IPAddress parsed_gateway;
        if (parsed_gateway.fromString(webServer.arg("gateway"))) {
            for (int i = 0; i < 4; i++) stored_networks[network_idx].gateway[i] = parsed_gateway[i];
        }
    }

    if (webServer.hasArg("dns")) {
        IPAddress parsed_dns;
        if (parsed_dns.fromString(webServer.arg("dns"))) {
            for (int i = 0; i < 4; i++) stored_networks[network_idx].dns[i] = parsed_dns[i];
        }
    }

    if (save_runtime_settings()) {
        webServer.send(200, "application/json", "{\"success\":true}");
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save\"}");
    }
}
