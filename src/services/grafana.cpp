/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Grafana Module - Alertmanager webhook integration
 */

#include "../services/grafana.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../protocol/transmission.h"
#include "../protocol/flex_protocol.h"
#include "../core/display.h"
#include "../web/web_server.h"
#include "../services/mqtt.h"
#include "../network/gsm.h"
#include <WiFi.h>
#include <ArduinoJson.h>

void handle_grafana() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("Grafana Integration");

    chunk += "<div class='header'>"
            "<h1>🚨 Grafana Integration</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive'" + String(settings.api_enabled ? " style='color:#28a745;'" : "") + ">🔗 API</a>"
            "<a href='/grafana' class='tab-active'" + String(settings.grafana_enabled ? " style='color:#28a745;'" : "") + ">🚨 Grafana</a>"
            + chatgpt_nav_tab_html(false) +
            "<a href='/mqtt' class='tab-inactive'" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " style='color:#dc3545;'" : (settings.mqtt_enabled ? " style='color:#28a745;'" : "")) + ">📡 MQTT</a>"
            + imap_nav_tab_html(false)
            + gsm_nav_tab_html(false) +
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div id='temp-message'></div>";

    chunk += "<div class='form-section'>"
            "<div class='flex-space-between mb-20'>"
            "<div class='flex-center'>"
            "<span style='text-large'>Enable Grafana</span>"
            "<div class='toggle-switch " + String(settings.grafana_enabled ? "is-active" : "is-inactive") + "' onclick='toggleGrafana()'>"
            "<div class='toggle-slider " + String(settings.grafana_enabled ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='padding:20px;'>";

    chunk += "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;margin:20px 0;border-left:5px solid #ff6b35;'>"
            "<h3 style='margin:0 0 15px 0;color:#ff6b35;'>📡 Webhook Endpoint</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Configure Grafana to send alert notifications to this device:</p>"
            "<div style='background-color:var(--theme-input);padding:15px;border-radius:8px;border:2px solid var(--theme-border);'>"
            "<code style='font-size:16px;font-weight:bold;color:var(--theme-text);word-break:break-all;'>"
            "http://" + WiFi.localIP().toString() + "/api/v1/alerts"
            "</code>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>⚙️ Grafana Alertmanager Configuration</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Add this webhook configuration to your alertmanager.yml:</p>"
            "<textarea readonly style='width:100%;height:302px;padding:15px;border:2px solid var(--theme-border);border-radius:8px;font-family:monospace;font-size:13px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:vertical;'>"
            "# alertmanager.yml\n"
            "route:\n"
            "  group_by: ['alertname']\n"
            "  group_wait: 10s\n"
            "  group_interval: 10s\n"
            "  repeat_interval: 1h\n"
            "  receiver: 'flex-pager'\n\n"
            "receivers:\n"
            "- name: 'flex-pager'\n"
            "  webhook_configs:\n"
            "  - url: 'http://" + WiFi.localIP().toString() + "/api/v1/alerts'\n"
            "    http_config:\n"
            "      basic_auth:\n"
            "        username: '" + String(settings.api_username) + "'\n"
            "        password: '" + String(settings.api_password) + "'"
            "</textarea>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>🏷️ Alert Field Mapping</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Configure alert labels and annotations to control paging behavior:</p>"

            "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:20px 0;'>"
            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;border-left:4px solid #28a745;'>"
            "<h4 style='margin:0 0 15px 0;color:#28a745;'>📋 Labels (Paging Parameters)</h4>"
            "<div style='font-family:monospace;font-size:12px;line-height:1.6;'>"
            "<div><strong>capcode</strong> → Target pager capcode</div>"
            "<div><strong>pager_capcode</strong> → Alternative capcode field</div><br>"
            "<div><strong>frequency</strong> → TX frequency (MHz/Hz)</div>"
            "<div><strong>pager_frequency</strong> → Alternative frequency field</div><br>"
            "<div><strong>mail_drop</strong> → Enable mail drop flag</div>"
            "<div><strong>pager_mail_drop</strong> → Alternative mail drop field</div><br>"
            "<div style='color:var(--theme-secondary);font-size:11px;font-style:italic;'>"
            "All fields are optional. Missing values use FLEX tab defaults.</div>"
            "</div>"
            "</div>"

            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;border-left:4px solid #007bff;'>"
            "<h4 style='margin:0 0 15px 0;color:#007bff;'>💬 Annotations (Message Content)</h4>"
            "<div style='font-family:monospace;font-size:12px;line-height:1.6;'>"
            "<div><strong>1. summary</strong> → Primary message content</div>"
            "<div><strong>2. description</strong> → Secondary content</div>"
            "<div><strong>3. message</strong> → Tertiary content</div>"
            "<div><strong>4. \"Alert triggered\"</strong> → Default fallback</div><br>"
            "<div style='color:var(--theme-secondary);font-size:11px;'>Priority order: 1 → 2 → 3 → 4</div>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>📝 Message Format</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Alert messages are automatically formatted with status and content:</p>"
            "<div style='background-color:var(--theme-input);padding:15px;border-radius:8px;border:2px solid var(--theme-border);font-family:monospace;'>"
            "<div style='color:#dc3545;'>[FIRING] AlertName: Message content</div>"
            "<div style='color:#28a745;'>[RESOLVED] AlertName: Message content</div>"
            "</div>"
            "<p style='font-size:12px;color:var(--theme-secondary);margin:10px 0;'><em>Status determined by alert 'endsAt' field: ongoing alerts show FIRING, resolved alerts show RESOLVED.</em></p>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>🧪 Test Webhook</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Test the Grafana webhook endpoint with this curl command:</p>"
            "<textarea readonly style='width:100%;height:272px;padding:15px;border:2px solid var(--theme-border);border-radius:8px;font-family:monospace;font-size:12px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:vertical;'>"
            "curl -X POST \\\n"
            "     -u " + String(settings.api_username) + ":" + String(settings.api_password) + " \\\n"
            "     -H \"Content-Type: application/json\" \\\n"
            "     http://" + WiFi.localIP().toString() + "/api/v1/alerts \\\n"
            "     -d '[{\n"
            "       \"labels\": {\n"
            "         \"alertname\": \"TestAlert\",\n"
            "         \"capcode\": \"1234567\",\n"
            "         \"frequency\": \"931937500\"\n"
            "       },\n"
            "       \"annotations\": {\n"
            "         \"summary\": \"Test notification from curl\"\n"
            "       },\n"
            "       \"endsAt\": \"0001-01-01T00:00:00Z\"\n"
            "     }]'"
            "</textarea>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>📨 Webhook Response</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Multi-alert processing response with detailed status:</p>"
            "<textarea readonly style='width:100%;height:269px;padding:15px;border:2px solid var(--theme-border);border-radius:8px;font-family:monospace;font-size:12px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:vertical;'>"
            "{\n"
            "  \"status\": \"completed\",\n"
            "  \"total_alerts\": 1,\n"
            "  \"successful\": 1,\n"
            "  \"failed\": 0,\n"
            "  \"results\": [{\n"
            "    \"alert_index\": 1,\n"
            "    \"alert_name\": \"TestAlert\",\n"
            "    \"capcode\": 1234567,\n"
            "    \"frequency\": 931.9375,\n"
            "    \"message\": \"[FIRING] TestAlert: Test notification\",\n"
            "    \"truncated\": false,\n"
            "    \"success\": true\n"
            "  }]\n"
            "}"
            "</textarea>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>🔧 Integration Notes</h3>"
            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;margin:20px 0;'>"
            "<ul style='margin:0;padding-left:20px;line-height:1.8;'>"
            "<li><strong>Queue System:</strong> Grafana alerts use the same message queue as other API calls</li>"
            "<li><strong>EMR Support:</strong> Messages benefit from Emergency Message Resynchronization</li>"
            "<li><strong>Multi-Alert:</strong> Single webhook call can process multiple alerts</li>"
            "<li><strong>Auto-Truncation:</strong> Long messages automatically truncated to 248 characters</li>"
            "<li><strong>FLEX Tab Defaults:</strong> Missing fields (capcode, frequency, mail_drop) use FLEX tab configuration</li>"
            "<li><strong>Authentication:</strong> Same HTTP Basic Auth as standard API endpoint</li>"
            "</ul>"
            "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<script>"
            "function toggleGrafana() {"
            "  const toggleSwitch = document.querySelector('.toggle-switch');"
            "  const toggleSlider = document.querySelector('.toggle-slider');"
            "  "
            "  const currentEnabled = toggleSwitch.style.backgroundColor === 'rgb(40, 167, 69)';"
            "  const newEnabled = !currentEnabled;"
            "  "
            "  if (newEnabled) {"
            "    toggleSwitch.style.backgroundColor = '#28a745';"
            "    toggleSlider.style.left = '26px';"
            "  } else {"
            "    toggleSwitch.style.backgroundColor = '#ccc';"
            "    toggleSlider.style.left = '2px';"
            "  }"
            "  "
            "  fetch('/grafana_toggle', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
            "    body: 'grafana_enabled=' + (newEnabled ? '1' : '0')"
            "  })"
            "  .then(response => response.text())"
            "  .then(data => {"
            "    console.log('Grafana toggle response:', data);"
            "  })"
            "  .catch(error => {"
            "    console.error('Error toggling Grafana:', error);"
            "  });"
            "}"
            "</script>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_grafana_toggle() {
    reset_oled_timeout();

    if (webServer.hasArg("grafana_enabled")) {
        settings.grafana_enabled = (webServer.arg("grafana_enabled") == "1");

        if (save_runtime_settings()) {
            webServer.send(200, "text/plain", "OK");
            logMessage("CONFIG: Grafana toggled - Enabled:" + String(settings.grafana_enabled ? "true" : "false"));
        } else {
            webServer.send(500, "text/plain", "Error saving configuration");
        }
    } else {
        webServer.send(400, "text/plain", "Missing parameter");
    }
}

void handle_grafana_webhook() {
    reset_oled_timeout();

    if (!settings.grafana_enabled) {
        webServer.send(503, "application/json", "{\"error\":\"Grafana webhook service is disabled\"}");
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

    if (!doc.is<JsonArray>()) {
        webServer.send(400, "application/json", "{\"error\":\"Expected JSON array of alerts\"}");
        return;
    }

    JsonArray alerts = doc.as<JsonArray>();
    int total_alerts = alerts.size();
    int successful = 0;
    int failed = 0;

    logMessage("GRAFANA: Processing " + String(total_alerts) + " alerts");

    JsonDocument response;
    response["status"] = "completed";
    response["total_alerts"] = total_alerts;

    JsonArray results = response["results"].to<JsonArray>();

    for (int i = 0; i < total_alerts; i++) {
        JsonObject alert = alerts[i];
        JsonObject result = results.createNestedObject();

        result["alert_index"] = i + 1;

        JsonObject labels = alert["labels"];
        JsonObject annotations = alert["annotations"];

        String ends_at = alert["endsAt"].as<String>();
        String status = (ends_at == "0001-01-01T00:00:00Z") ? "FIRING" : "RESOLVED";

        String alert_name = labels["alertname"].as<String>();
        if (alert_name.isEmpty()) {
            alert_name = "Unknown Alert";
        }
        result["alert_name"] = alert_name;

        uint64_t capcode = settings.default_capcode;
        if (labels["capcode"].is<uint64_t>()) {
            capcode = labels["capcode"];
        } else if (labels["capcode"].is<String>()) {
            capcode = strtoull(labels["capcode"].as<String>().c_str(), nullptr, 10);
        } else if (labels["pager_capcode"].is<uint64_t>()) {
            capcode = labels["pager_capcode"];
        } else if (labels["pager_capcode"].is<String>()) {
            capcode = strtoull(labels["pager_capcode"].as<String>().c_str(), nullptr, 10);
        }
        result["capcode"] = capcode;

        float frequency = settings.default_frequency;
        if (labels["frequency"].is<float>()) {
            frequency = labels["frequency"];
        } else if (labels["frequency"].is<String>()) {
            frequency = atof(labels["frequency"].as<String>().c_str());
        } else if (labels["pager_frequency"].is<float>()) {
            frequency = labels["pager_frequency"];
        } else if (labels["pager_frequency"].is<String>()) {
            frequency = atof(labels["pager_frequency"].as<String>().c_str());
        }

        if (frequency > 1000.0) {
            frequency = frequency / 1000000.0;
        }
        result["frequency"] = frequency;

        bool mail_drop = false;
        if (labels["mail_drop"].is<bool>()) {
            mail_drop = labels["mail_drop"];
        } else if (labels["mail_drop"].is<String>()) {
            String mail_drop_str = labels["mail_drop"].as<String>();
            mail_drop = (mail_drop_str == "true" || mail_drop_str == "1");
        } else if (labels["pager_mail_drop"].is<bool>()) {
            mail_drop = labels["pager_mail_drop"];
        } else if (labels["pager_mail_drop"].is<String>()) {
            String mail_drop_str = labels["pager_mail_drop"].as<String>();
            mail_drop = (mail_drop_str == "true" || mail_drop_str == "1");
        }

        String message_content = "";
        if (annotations["summary"].is<String>() && !annotations["summary"].as<String>().isEmpty()) {
            message_content = annotations["summary"].as<String>();
        } else if (annotations["description"].is<String>() && !annotations["description"].as<String>().isEmpty()) {
            message_content = annotations["description"].as<String>();
        } else if (annotations["message"].is<String>() && !annotations["message"].as<String>().isEmpty()) {
            message_content = annotations["message"].as<String>();
        } else {
            message_content = "Alert triggered";
        }

        String final_message = "[" + status + "] " + alert_name + ": " + message_content;

        bool message_was_truncated = false;
        if (final_message.length() > MAX_FLEX_MESSAGE_LENGTH) {
            final_message = truncate_message_with_ellipsis(final_message);
            message_was_truncated = true;
        }

        result["message"] = final_message;
        result["truncated"] = message_was_truncated;

        if (queue_add_message(capcode, frequency, settings.default_txpower, mail_drop, final_message.c_str())) {
            result["success"] = true;
            successful++;
            logMessage("GRAFANA: Alert " + String(i + 1) + " queued - " + alert_name + " (capcode=" + String(capcode) + ")");
        } else {
            result["success"] = false;
            result["error"] = "Queue is full";
            failed++;
            logMessage("GRAFANA: Alert " + String(i + 1) + " failed - Queue full");
        }
    }

    response["successful"] = successful;
    response["failed"] = failed;

    String response_str;
    serializeJson(response, response_str);

    int status_code = (failed == 0) ? 200 : 207;
    webServer.send(status_code, "application/json", response_str);

    logMessage("GRAFANA: Completed processing - " + String(successful) + " successful, " + String(failed) + " failed");
}
