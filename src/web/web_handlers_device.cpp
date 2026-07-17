/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Web Server Module - Device status, logs, backup/restore, certificate upload handlers
 */

#include "../web/web_server.h"
#include "../core/config.h"
#include "../version.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../core/display.h"
#include "../core/hardware.h"
#include "../core/utils.h"
#include "../network/wifi.h"
#include "../network/gsm.h"
#include "../services/mqtt.h"
#include "../services/imap.h"
#include "../protocol/transmission.h"
#include "../network/ntp_time.h"
#include "../../include/boards/boards.h"
#include <WiFi.h>
#include <SPIFFS.h>

// =============================================================================
// GLOBALS (file-local)
// =============================================================================
static String uploaded_cert_data = "";

// =============================================================================
// DEVICE STATUS PAGE
// =============================================================================

void handle_device_status() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("Device Status");

    chunk += "<div class='header'>"
            "<h1>📊 Device Status</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            + chatgpt_nav_tab_html(false) +
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            + imap_nav_tab_html(false)
            + gsm_nav_tab_html(false) +
            "<a href='/status' class='tab-active'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card); margin-bottom: 20px;'>";
    chunk += "<h2 style='margin-top:0; margin-bottom:25px; border-bottom: 2px solid var(--theme-border); padding-bottom:10px;'>📊 System Status</h2>";

    chunk += "<div style='display:flex;gap:20px;flex-wrap:wrap;'>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📡 Device Information</h3>";
    unsigned long uptimeSeconds = millis() / 1000;
    unsigned long days = uptimeSeconds / 86400;
    unsigned long hours = (uptimeSeconds % 86400) / 3600;
    unsigned long mins = (uptimeSeconds % 3600) / 60;
    String uptime = "";
    if (days > 0) uptime += String(days) + " days, ";
    if (hours > 0 || days > 0) uptime += String(hours) + " hours, ";
    uptime += String(mins) + " mins";
    chunk += "<p><strong>Uptime:</strong> " + uptime + "</p>";
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint8_t heapPercent = (freeHeap * 100) / totalHeap;
    chunk += "<p><strong>Free Heap:</strong> " + String(freeHeap) + " bytes (" + String(heapPercent) + "%)</p>";
    String theme_name = "";
    switch(settings.theme) {
        case 0: theme_name = "🌞 Minimal White"; break;
        case 1: theme_name = "🌙 Carbon Black"; break;
        default: theme_name = "Unknown"; break;
    }
    chunk += "<p><strong>Theme:</strong> " + theme_name + "</p>";
    String wifi_status = "Enabled";
    chunk += "<p><strong>WiFi Status:</strong> " + wifi_status + "</p>";
    chunk += "<p><strong>Chip Model:</strong> " + String(ESP.getChipModel()) + "</p>";
    chunk += "<p><strong>CPU Frequency:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>";
    chunk += "</div>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📻 FLEX Configuration</h3>";
    chunk += "<p><strong>Banner:</strong> " + String(settings.banner_message) + "</p>";
    chunk += "<p><strong>Frequency:</strong> " + String(current_tx_frequency, 4) + " MHz</p>";
    chunk += "<p><strong>TX Power:</strong> " + String(tx_power, 1) + " dBm</p>";
    chunk += "<p><strong>Default Capcode:</strong> " + String(settings.default_capcode) + "</p>";
    chunk += "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='display:flex;gap:20px;flex-wrap:wrap;margin-top:20px;'>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📶 Network Information</h3>";

    if (wifi_connected) {
        chunk += "<p><strong>WiFi SSID:</strong> " + WiFi.SSID() + "</p>";
        chunk += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
        chunk += "<p><strong>Subnet Mask:</strong> " + WiFi.subnetMask().toString() + "</p>";
        chunk += "<p><strong>Gateway:</strong> " + WiFi.gatewayIP().toString() + "</p>";
        chunk += "<p><strong>DNS Server:</strong> " + WiFi.dnsIP().toString() + "</p>";
        chunk += "<p><strong>MAC Address:</strong> " + WiFi.macAddress() + "</p>";
        chunk += "<p><strong>RSSI:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
        chunk += "<p><strong>DHCP:</strong> DHCP</p>";
    } else if (ap_mode_active) {
        chunk += "<p><strong>Mode:</strong> Access Point (Configuration Mode)</p>";
        chunk += "<p><strong>AP SSID:</strong> " + ap_ssid + "</p>";
        chunk += "<p><strong>AP IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
        chunk += "<p><strong>AP MAC:</strong> " + WiFi.softAPmacAddress() + "</p>";
        chunk += "<p><strong>Connected Clients:</strong> " + String(WiFi.softAPgetStationNum()) + "</p>";
    } else {
        chunk += "<p><strong>Status:</strong> WiFi Disabled</p>";
        chunk += "<p><strong>MAC Address:</strong> " + WiFi.macAddress() + "</p>";
    }
    chunk += "</div>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>🔋 Battery Status</h3>";

    uint16_t battery_voltage_mv_status;
    int battery_percentage_status;
    getBatteryInfo(&battery_voltage_mv_status, &battery_percentage_status);

    int adc_raw = analogRead(BATTERY_ADC_PIN);
    float battery_voltage_status = battery_voltage_mv_status / 1000.0;
    bool is_connected = (battery_voltage_status > 4.17);
    bool is_actively_charging = (battery_voltage_status > 4.20);

    if (battery_present) {
        chunk += "<p><strong>Battery Present:</strong> ✅ Yes</p>";
        chunk += "<p><strong>Voltage:</strong> " + String(battery_voltage_status, 3) + "V (" + String(battery_voltage_mv_status) + " mV)</p>";
        chunk += "<p><strong>Percentage:</strong> " + String(battery_percentage_status) + "%</p>";
        chunk += "<p><strong>ADC Raw Value:</strong> " + String(adc_raw) + " (0-4095)</p>";

        String power_status = is_connected ? "<span style='color:#28a745;'>🔌 Connected</span>" : "<span style='color:#ffc107;'>🔋 On Battery</span>";
        chunk += "<p><strong>Power Status:</strong> " + power_status + "</p>";

        String charging_status = is_actively_charging ? "<span style='color:#17a2b8;'>⚡ Yes</span>" : "<span style='color:#6c757d;'>○ No</span>";
        chunk += "<p><strong>Charging:</strong> " + charging_status + "</p>";

        chunk += "<p><strong>Check Interval:</strong> 60 seconds</p>";

        if (settings.enable_low_battery_alert) {
            chunk += "<p><strong>Low Battery Alert:</strong> ✅ Enabled (≤10%)</p>";
        } else {
            chunk += "<p><strong>Low Battery Alert:</strong> ❌ Disabled</p>";
        }

        if (settings.enable_power_disconnect_alert) {
            chunk += "<p><strong>Power Disconnect Alert:</strong> ✅ Enabled</p>";
        } else {
            chunk += "<p><strong>Power Disconnect Alert:</strong> ❌ Disabled</p>";
        }
    } else {
        chunk += "<p><strong>Battery Present:</strong> ❌ No</p>";
        chunk += "<p><strong>Voltage:</strong> " + String(battery_voltage_status, 3) + "V (below 2.5V threshold)</p>";
        chunk += "<p><strong>ADC Raw Value:</strong> " + String(adc_raw) + " (0-4095)</p>";
    }

    chunk += "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='display:flex;gap:20px;flex-wrap:wrap;margin-top:20px;'>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>🕐 Time Synchronization</h3>";

    time_t now;
    time(&now);
    time_t local_time = getLocalTimestamp();
    chunk += "<p><strong>Current Time:</strong> " + String(ctime(&local_time)) + "</p>";
    chunk += "<p><strong>Unix Timestamp:</strong> " + String((long)now) + "</p>";
    chunk += "<p><strong>NTP Synchronized:</strong> " + String(ntp_synced ? "✅ Yes" : "❌ No") + "</p>";

    if (last_ntp_sync > 0) {
        unsigned long time_since_sync = (millis() - last_ntp_sync) / 1000;
        unsigned long minutes_since = time_since_sync / 60;
        unsigned long hours_since = minutes_since / 60;

        String last_sync_str;
        if (hours_since > 0) {
            last_sync_str = String(hours_since) + "h " + String(minutes_since % 60) + "m ago";
        } else {
            last_sync_str = String(minutes_since) + "m " + String(time_since_sync % 60) + "s ago";
        }
        chunk += "<p><strong>Last NTP Sync:</strong> " + last_sync_str + "</p>";

        unsigned long next_sync = NTP_SYNC_INTERVAL_MS - (millis() - last_ntp_sync);
        if (next_sync > NTP_SYNC_INTERVAL_MS) next_sync = 0;
        unsigned long next_minutes = next_sync / 60000;
        chunk += "<p><strong>Next Sync In:</strong> " + String(next_minutes) + " minutes</p>";
    } else {
        chunk += "<p><strong>Last NTP Sync:</strong> Never</p>";
        chunk += "<p><strong>Next Sync In:</strong> On WiFi connection</p>";
    }

    if (now > 1600000000) {
        chunk += "<p><strong>Time Quality:</strong> ✅ Good (SSL ready)</p>";
    } else {
        chunk += "<p><strong>Time Quality:</strong> ❌ Poor (SSL may fail)</p>";
    }

    chunk += "</div>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>🖥️ Remote Logging</h3>";

    if (!settings.rsyslog_enabled) {
        chunk += "<p><strong>Status:</strong> ❌ Disabled</p>";
    } else if (strlen(settings.rsyslog_server) == 0) {
        chunk += "<p><strong>Status:</strong> ⚠️ Not Configured</p>";
    } else {
        chunk += "<p><strong>Status:</strong> ✅ Enabled</p>";
        chunk += "<p><strong>Server:</strong> " + String(settings.rsyslog_server) + ":" + String(settings.rsyslog_port) + "</p>";
        chunk += "<p><strong>Protocol:</strong> " + String(settings.rsyslog_use_tcp ? "TCP" : "UDP") + "</p>";
        String severity_name;
        switch (settings.rsyslog_min_severity) {
            case 0: severity_name = "Emergency"; break;
            case 1: severity_name = "Alert"; break;
            case 2: severity_name = "Critical"; break;
            case 3: severity_name = "Error"; break;
            case 4: severity_name = "Warning"; break;
            case 5: severity_name = "Notice"; break;
            case 6: severity_name = "Informational"; break;
            case 7: severity_name = "Debug"; break;
            default: severity_name = "Unknown"; break;
        }
        chunk += "<p><strong>Min Severity:</strong> " + severity_name + " (" + String(settings.rsyslog_min_severity) + ")</p>";
        chunk += "<p><strong>Hostname:</strong> " + String(settings.banner_message) + "</p>";
        chunk += "<p><strong>Facility:</strong> local0 (16)</p>";
    }
    chunk += "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='display:flex;gap:20px;flex-wrap:wrap;margin-top:20px;'>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📡 MQTT Status</h3>";

    if (!settings.mqtt_enabled) {
        chunk += "<p><strong>Status:</strong> ❌ Disabled</p>";
    } else if (strlen(settings.mqtt_server) == 0) {
        chunk += "<p><strong>Status:</strong> ⚠️ Not Configured</p>";
    } else {
        String status;
        if (mqtt_suspended) {
            status = "<span style='color:#dc3545;'>🚫 Suspended</span>";
        } else if (mqttClient.connected()) {
            status = "<span style='color:#28a745;'>✅ Connected</span>";
        } else {
            status = "<span style='color:#dc3545;'>❌ Disconnected</span>";
        }
        chunk += "<p><strong>Status:</strong> " + status + "</p>";
        chunk += "<p><strong>Server:</strong> " + String(settings.mqtt_server) + ":" + String(settings.mqtt_port) + "</p>";
        chunk += "<p><strong>Thing Name:</strong> " + String(settings.mqtt_thing_name) + "</p>";
        chunk += "<p><strong>Subscribe Topic:</strong> " + String(settings.mqtt_subscribe_topic) + "</p>";
        chunk += "<p><strong>Publish Topic:</strong> " + String(settings.mqtt_publish_topic) + "</p>";
        chunk += "<p><strong>Initialized:</strong> " + String(mqtt_initialized ? "✅ Yes" : "❌ No") + "</p>";

        bool has_certs = (certificateExistsInSPIFFS(MQTT_CA_CERT_FILE) && certificateExistsInSPIFFS(MQTT_DEVICE_CERT_FILE) && certificateExistsInSPIFFS(MQTT_DEVICE_KEY_FILE));
        chunk += "<p><strong>SSL Certificates:</strong> " + String(has_certs ? "✅ Configured" : "⚠️ Using Insecure Connection") + "</p>";
    }
    chunk += "</div>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📧 IMAP Status</h3>";

#ifdef ENABLE_IMAP
    if (!imap_config.enabled) {
        chunk += "<p><strong>Status:</strong> ❌ IMAP System Disabled</p>";
    } else if (imap_config.account_count == 0) {
        chunk += "<p><strong>Status:</strong> ⚠️ No Accounts Configured</p>";
    } else {
        String suspended_accounts = "";
        int suspended_count = 0;

        for (size_t i = 0; i < imap_config.accounts.size(); i++) {
            if (imap_config.accounts[i].suspended) {
                if (suspended_count > 0) suspended_accounts += ", ";
                suspended_accounts += String(imap_config.accounts[i].name);
                suspended_count++;
            }
        }

        String status;
        if (suspended_count > 0) {
            status = "<span style='color:#dc3545;'>⚠️ " + String(imap_config.account_count - suspended_count) + "/" + String(imap_config.account_count) + " Active</span>";
        } else {
            status = "<span style='color:#28a745;'>✅ Active</span>";
        }
        chunk += "<p><strong>Status:</strong> " + status + "</p>";
        chunk += "<p><strong>Accounts:</strong> " + String(imap_config.account_count) + "</p>";

        if (suspended_count > 0) {
            chunk += "<div style='background-color:#fff3cd;border:1px solid #ffeaa7;padding:10px;border-radius:5px;margin:10px 0;'>";
            chunk += "<strong style='color:#856404;'>🚫 Suspended Accounts:</strong> " + suspended_accounts;
            chunk += "</div>";
        }
        chunk += "<p><strong>Alert Mode:</strong> 📢 All Unread Emails</p>";

        if (queue_count > 0) {
            chunk += "<p><strong>Queue:</strong> " + String(queue_count) + " message(s) pending</p>";
        }

        if (last_imap_check > 0) {
            unsigned long time_since_check = (millis() - last_imap_check) / 1000;
            unsigned long minutes_since = time_since_check / 60;
            chunk += "<p><strong>Last Global Check:</strong> " + String(minutes_since) + " minutes ago</p>";
        }

        chunk += "<div style='margin-top:15px;'>";
        chunk += "<h4 style='margin:10px 0 8px 0;color:var(--theme-text);'>Account Details:</h4>";

        webServer.sendContent(chunk);
        chunk = "";

        for (size_t i = 0; i < imap_config.accounts.size(); i++) {
            const IMAPAccount& account = imap_config.accounts[i];
            String ssl_icon = account.use_ssl ? "🔒" : "🔓";
            String account_status;
            String status_color;

            if (account.suspended) {
                account_status = "🚫 Suspended";
                status_color = "#dc3545";
            } else {
                account_status = "✅ Active";
                status_color = "#28a745";
            }

            chunk += "<div style='background-color:var(--theme-card);border:1px solid var(--theme-border);border-radius:6px;padding:12px;margin:5px 0;'>";
            chunk += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
            chunk += "<div>";
            chunk += "<strong>" + ssl_icon + " " + String(account.name) + "</strong>";
            chunk += "<div style='font-size:0.9em;color:var(--theme-secondary);margin-top:2px;'>";

            if (account.last_check > 0) {
                unsigned long account_time_since = (millis() - account.last_check) / 1000;
                unsigned long account_minutes = account_time_since / 60;
                chunk += "Last: " + String(account_minutes) + "m ago";
            } else {
                chunk += "Last: Never";
            }

            if (account.suspended && account.failed_check_cycles > 0) {
                chunk += " | Failures: " + String(account.failed_check_cycles);
            }

            chunk += "</div>";
            chunk += "</div>";
            chunk += "<span style='color:" + status_color + ";font-weight:500;'>" + account_status + "</span>";
            chunk += "</div>";
            chunk += "</div>";
            webServer.sendContent(chunk);
            chunk = "";
        }
        chunk += "</div>";
    }
#else
    chunk += "<p><strong>Status:</strong> Not compiled in this firmware</p>";
#endif // ENABLE_IMAP
    chunk += "</div>";
    chunk += "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card); margin-bottom: 20px;'>";
    chunk += "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;'>";
    chunk += "<h3 style='margin: 0;'>📡 Recent Serial Messages</h3>";
    chunk += "<div style='display: flex; align-items: center; gap: 8px; flex-wrap: wrap;'>";
    chunk += "<div class='toggle-switch' id='live-logs-toggle' onclick='toggleLiveLogs()' style='background-color: #ccc;'>";
    chunk += "<div class='toggle-slider' style='left: 2px;'></div>";
    chunk += "</div>";
    chunk += "<span style='font-size: 0.9em; color: var(--theme-text);'>Live Logs</span>";
    chunk += "<label style='display:flex;align-items:center;gap:5px;'>";
    chunk += "<span style='font-size:0.9em;'>Interval:</span>";
    chunk += "<input type='number' id='refresh-interval' value='5' min='1' max='60' style='width:50px;padding:4px 6px;border:1px solid var(--theme-border);border-radius:4px;background-color:var(--theme-input);color:var(--theme-text);' onblur='updateRefreshInterval()'>";
    chunk += "<span style='font-size:0.9em;'>s</span>";
    chunk += "</label>";
    chunk += "<label style='display:flex;align-items:center;gap:5px;'>";
    chunk += "<span style='font-size:0.9em;'>Lines:</span>";
    chunk += "<input type='number' id='lines-count' value='100' min='10' max='500' style='width:60px;padding:4px 6px;border:1px solid var(--theme-border);border-radius:4px;background-color:var(--theme-input);color:var(--theme-text);' onblur='updateLinesToShow()'>";
    chunk += "</label>";
    chunk += "<a href='/download_logs' download='serial.log'><button style='padding:6px 10px;font-size:0.8em;background-color:#007bff;color:white;border-radius:4px;border:none;cursor:pointer;'>📥 Download</button></a>";
    chunk += "</div>";
    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    String logs = read_log_tail(100);

    if (logs.length() == 0) {
        chunk += "<p>No log entries found.</p>";
    } else {
        chunk += "<style>.serial-log{max-height:500px;overflow-y:auto;}</style>";
        chunk += "<div class='serial-log'>";
        webServer.sendContent(chunk);
        chunk = "";

        int lineStart = 0;
        int lineCount = 0;

        for (int i = 0; i < logs.length(); i++) {
            if (logs[i] == '\n') {
                String line = logs.substring(lineStart, i);
                lineStart = i + 1;

                int spacePos = line.indexOf(' ', 11);
                if (spacePos > 0) {
                    String timestamp = line.substring(0, spacePos);
                    String message = line.substring(spacePos + 1);

                    chunk += "<div>";
                    chunk += "<span class='timestamp'>" + timestamp + "</span> ";
                    chunk += message;
                    chunk += "</div>";
                } else {
                    chunk += "<div>" + line + "</div>";
                }

                lineCount++;
                if (lineCount % 10 == 0) {
                    webServer.sendContent(chunk);
                    chunk = "";
                }
            }
        }

        if (chunk.length() > 0) {
            webServer.sendContent(chunk);
            chunk = "";
        }
        chunk += "</div>";
    }
    chunk += "</div>";
    chunk += "<script>document.addEventListener('DOMContentLoaded', function() {"
             "const container = document.querySelector('.serial-log');"
             "if (container) container.scrollTop = container.scrollHeight;"
             "});</script>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>";
    chunk += "<h3 style='margin-top:0;'>⚙️ Device Management</h3>";
    chunk += "<p>Backup, restore, or reset device configuration:</p>";
    chunk += "<div style='display:flex;gap:8px;flex-wrap:wrap;'>";

    chunk += "<form action='/backup_settings' method='get' style='margin:0;'>";
    chunk += "<button type='submit' style='padding:6px 10px;font-size:0.8em;background-color:#28a745;color:white;border-radius:4px;border:none;cursor:pointer;'>💾 Backup</button>";
    chunk += "</form>";

    chunk += "<form action='/restore_settings' method='get' style='margin:0;'>";
    chunk += "<button type='submit' style='padding:6px 10px;font-size:0.8em;background-color:#fd7e14;color:white;border-radius:4px;border:none;cursor:pointer;'>📁 Restore</button>";
    chunk += "</form>";

    chunk += "<form action='/factory_reset' method='post' onsubmit='return confirm(\"Are you sure you want to reset to factory defaults? This will clear all configuration and restart the device.\")' style='margin:0;'>";
    chunk += "<button type='submit' style='padding:6px 10px;font-size:0.8em;background-color:#dc3545;color:white;border-radius:4px;border:none;cursor:pointer;'>🔄 Reset</button>";
    chunk += "</form>";

    chunk += "</div>";
    chunk += "<div style='margin-top:10px;font-size:0.9em;color:#666;'>";
    chunk += "<strong>Backup:</strong> Download all settings to JSON file<br>";
    chunk += "<strong>Restore:</strong> Upload and apply settings from backup file<br>";
    chunk += "<strong>Factory Reset:</strong> Clear all settings and return to defaults";
    chunk += "</div>";
    chunk += "</div>";
    chunk += "<script>"
           "let liveLogsEnabled = false;"
           "let liveLogsInterval = null;"
           "let refreshInterval = 5000;"
           "let linesToShow = 100;"
           "function pollLogs() {"
           "  fetch('/logs?lines=' + linesToShow)"
           "    .then(response => response.json())"
           "    .then(data => {"
           "      const container = document.querySelector('.serial-log');"
           "      if (!container) return;"
           "      container.innerHTML = '';"
           "      if (data.logs && data.logs.length > 0) {"
           "        data.logs.forEach(log => {"
           "          const logDiv = document.createElement('div');"
           "          logDiv.innerHTML = \"<span class='timestamp'>\" + log.timestamp + \"</span> \" + log.message;"
           "          container.appendChild(logDiv);"
           "        });"
           "      } else {"
           "        container.innerHTML = '<p>No log entries found.</p>';"
           "      }"
           "    })"
           "    .catch(() => {"
           "      container.innerHTML = '<div style=\"color:red;\">Failed to load logs</div>';"
           "    });"
           "}"
           "function loadLogs(lines) {"
           "  const container = document.querySelector('.serial-log');"
           "  if (!container) return;"
           "  container.innerHTML = '<div style=\"text-align:center;padding:20px;\">Loading...</div>';"
           "  fetch('/logs?lines=' + lines)"
           "    .then(response => response.json())"
           "    .then(data => {"
           "      container.innerHTML = '';"
           "      if (data.logs && data.logs.length > 0) {"
           "        data.logs.forEach(log => {"
           "          const logDiv = document.createElement('div');"
           "          logDiv.innerHTML = \"<span class='timestamp'>\" + log.timestamp + \"</span> \" + log.message;"
           "          container.appendChild(logDiv);"
           "        });"
           "      } else {"
           "        container.innerHTML = '<p>No log entries found.</p>';"
           "      }"
           "    })"
           "    .catch(() => {"
           "      container.innerHTML = '<div style=\"color:red;\">Failed to load logs</div>';"
           "    });"
           "}"
           "function updateRefreshInterval() {"
           "  const input = document.getElementById('refresh-interval');"
           "  const seconds = parseInt(input.value);"
           "  if (seconds >= 1 && seconds <= 60) {"
           "    refreshInterval = seconds * 1000;"
           "    if (liveLogsEnabled) {"
           "      clearInterval(liveLogsInterval);"
           "      liveLogsInterval = setInterval(pollLogs, refreshInterval);"
           "    }"
           "  } else {"
           "    input.value = refreshInterval / 1000;"
           "  }"
           "}"
           "function updateLinesToShow() {"
           "  const input = document.getElementById('lines-count');"
           "  const lines = parseInt(input.value);"
           "  if (lines >= 10 && lines <= 500) {"
           "    linesToShow = lines;"
           "    pollLogs();"
           "  } else {"
           "    input.value = linesToShow;"
           "  }"
           "}"
           "function toggleLiveLogs() {"
           "  liveLogsEnabled = !liveLogsEnabled;"
           "  const toggle = document.getElementById('live-logs-toggle');"
           "  const slider = toggle.querySelector('.toggle-slider');"
           "  if (liveLogsEnabled) {"
           "    toggle.style.backgroundColor = '#28a745';"
           "    slider.style.left = '26px';"
           "    liveLogsInterval = setInterval(pollLogs, refreshInterval);"
           "    pollLogs();"
           "  } else {"
           "    toggle.style.backgroundColor = '#ccc';"
           "    slider.style.left = '2px';"
           "    if (liveLogsInterval) {"
           "      clearInterval(liveLogsInterval);"
           "      liveLogsInterval = null;"
           "    }"
           "    loadLogs(linesToShow);"
           "  }"
           "}"
           "</script>";

    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

// =============================================================================
// SERIAL LOG VIEWER
// =============================================================================

void handle_logs() {
    reset_oled_timeout();

    int numLines = 20;
    if (webServer.hasArg("lines")) {
        numLines = webServer.arg("lines").toInt();
        if (numLines <= 0) numLines = 20;
    }

    String logs = read_log_tail(numLines);
    String response = "{\"logs\":[";

    int lineStart = 0;
    int lineCount = 0;

    for (int i = 0; i < logs.length(); i++) {
        if (logs[i] == '\n') {
            String line = logs.substring(lineStart, i);
            lineStart = i + 1;

            if (lineCount > 0) response += ",";

            String timestamp = "";
            String message = line;

            if (line.length() > 20) {
                timestamp = line.substring(0, 19);
                message = line.substring(20);
            }

            response += "{";
            response += "\"timestamp\":\"" + timestamp + "\",";
            response += "\"message\":\"" + message + "\"";
            response += "}";

            lineCount++;
        }
    }

    response += "]}";

    webServer.send(200, "application/json", response);
}

void handle_download_logs() {
    reset_oled_timeout();

    if (!SPIFFS.exists("/serial.log")) {
        webServer.send(404, "text/plain", "Log file not found");
        return;
    }

    File file = SPIFFS.open("/serial.log", "r");
    if (!file) {
        webServer.send(500, "text/plain", "Failed to open log file");
        return;
    }

    webServer.sendHeader("Content-Type", "text/plain");
    webServer.sendHeader("Content-Disposition", "attachment; filename=\"serial.log\"");
    webServer.sendHeader("Content-Length", String(file.size()));

    webServer.setContentLength(file.size());
    webServer.send(200, "text/plain", "");

    uint8_t buffer[512];
    while (file.available()) {
        size_t len = file.readBytes((char*)buffer, 512);
        webServer.client().write(buffer, len);
    }

    file.close();
}

// =============================================================================
// FACTORY RESET
// =============================================================================

void handle_web_factory_reset() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("Factory Reset");
    chunk += "<div class='header'>"
            "<h1>🔄 Factory Reset</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            + chatgpt_nav_tab_html(false) +
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            + imap_nav_tab_html(false)
            + gsm_nav_tab_html(false) +
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div class='status success'>✅ Factory reset completed! Device will restart in 3 seconds...</div>";
    chunk += "<script>setTimeout(function() { window.location.href = '/'; }, 3000);</script>";
    chunk += get_html_footer();

    webServer.sendContent(chunk);
    webServer.sendContent("");

    delay(3000);
    perform_factory_reset();
}

// =============================================================================
// BACKUP / RESTORE
// =============================================================================

void handle_backup_settings() {
    reset_oled_timeout();

    String json_backup = export_user_backup();

    char filename[80];
    sprintf(filename, "flex-settings-backup-%s-%lu.json", FIRMWARE_VERSION, millis());

    webServer.sendHeader("Content-Disposition", "attachment; filename=\"" + String(filename) + "\"");
    webServer.sendHeader("Content-Type", "application/json");
    webServer.send(200, "application/json", json_backup);

    logMessage("BACKUP: Settings exported to JSON file");
}

void handle_restore_settings() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("Restore Settings");
    chunk += "<div class='header'>"
            "<h1>📁 Restore Settings</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            + chatgpt_nav_tab_html(false) +
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            + imap_nav_tab_html(false)
            + gsm_nav_tab_html(false) +
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div class='container'>";
    chunk += "<div class='status warning'>⚠️ <strong>Warning:</strong> Restoring settings will completely override your current configuration. This action cannot be undone.</div>";

    chunk += "<h3>📁 Upload Backup File</h3>";
    chunk += "<p>Select a JSON backup file previously exported from this device:</p>";

    chunk += "<form id='restore-form' enctype='multipart/form-data' method='post' action='/upload_restore'>";
    chunk += "<div style='margin-bottom:20px;'>";
    chunk += "<input type='file' name='backup' accept='.json' required style='padding:10px;border:2px solid #ddd;border-radius:5px;'>";
    chunk += "</div>";
    chunk += "<button type='submit' class='button' style='background-color:#dc3545;' onclick='return confirm(\"Are you sure you want to restore settings? This will override ALL current configuration and restart the device.\")'>🔄 Restore Settings</button>";
    chunk += "</form>";

    chunk += "<div id='status-message' style='margin-top:20px;'></div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<script>";
    chunk += "document.getElementById('restore-form').addEventListener('submit', function(e) {";
    chunk += "  e.preventDefault();";
    chunk += "  var formData = new FormData(this);";
    chunk += "  var statusDiv = document.getElementById('status-message');";
    chunk += "  statusDiv.innerHTML = '<div class=\"status info\">⏳ Uploading and validating backup file...</div>';";
    chunk += "  fetch('/upload_restore', { method: 'POST', body: formData })";
    chunk += "    .then(response => {";
    chunk += "      if (response.ok) {";
    chunk += "        return response.json();";
    chunk += "      } else {";
    chunk += "        throw new Error('HTTP ' + response.status);";
    chunk += "      }";
    chunk += "    })";
    chunk += "    .then(data => {";
    chunk += "      if (data.success) {";
    chunk += "        statusDiv.innerHTML = '<div class=\"status success\">✅ ' + data.message + '</div>';";
    chunk += "        if (data.restart) {";
    chunk += "          statusDiv.innerHTML += '<br><div class=\"status info\">⏳ Device is restarting...</div>';";
    chunk += "          setTimeout(() => { window.location.href = '/'; }, 5000);";
    chunk += "        }";
    chunk += "      } else {";
    chunk += "        statusDiv.innerHTML = '<div class=\"status error\">❌ ' + data.message + '</div>';";
    chunk += "      }";
    chunk += "    })";
    chunk += "    .catch(error => {";
    chunk += "      if (error.message.includes('Failed to fetch')) {";
    chunk += "        statusDiv.innerHTML = '<div class=\"status success\">✅ Settings restored successfully. Device is restarting...</div>';";
    chunk += "        setTimeout(() => { window.location.href = '/'; }, 5000);";
    chunk += "      } else {";
    chunk += "        statusDiv.innerHTML = '<div class=\"status error\">❌ Upload failed: ' + error.message + '</div>';";
    chunk += "      }";
    chunk += "    });";
    chunk += "});";
    chunk += "</script>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "</div>";
    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_upload_restore() {
    reset_oled_timeout();

    HTTPUpload& upload = webServer.upload();
    static String restore_content = "";

    if (upload.status == UPLOAD_FILE_START) {
        restore_content = "";
        logMessage("RESTORE: Starting backup file upload");

    } else if (upload.status == UPLOAD_FILE_WRITE) {
        restore_content += String((char*)upload.buf).substring(0, upload.currentSize);

    } else if (upload.status == UPLOAD_FILE_END) {
        logMessage("RESTORE: Backup file upload completed, validating...");

        String error_msg = "";
        bool success = import_user_backup(restore_content, error_msg);

        if (success) {
            if (save_runtime_settings()) {
                logMessage("RESTORE: Settings restored successfully from backup");
                webServer.send(200, "application/json",
                    "{\"success\":true,\"message\":\"Settings restored successfully. Device will restart in 5 seconds.\",\"restart\":true}");

                for (int i = 0; i < 10; i++) {
                    webServer.handleClient();
                    delay(100);
                }

                ESP.restart();
            } else {
                webServer.send(500, "application/json",
                    "{\"success\":false,\"message\":\"Failed to save restored settings\"}");
            }
        } else {
            logMessage("RESTORE: Backup validation failed - " + error_msg);
            webServer.send(400, "application/json",
                "{\"success\":false,\"message\":\"" + error_msg + "\"}");
        }

        restore_content = "";

    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        restore_content = "";
        webServer.send(400, "application/json",
            "{\"success\":false,\"message\":\"File upload was aborted\"}");
    }
}

// =============================================================================
// CERTIFICATE UPLOAD
// =============================================================================

void handle_file_upload() {
    HTTPUpload& upload = webServer.upload();

    if (upload.status == UPLOAD_FILE_START) {
        uploaded_cert_data = "";
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        uploaded_cert_data += String((const char*)upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
    }
}

void handle_upload_certificate() {
    reset_oled_timeout();

    String cert_data = uploaded_cert_data;
    cert_data.trim();

    if (cert_data.length() == 0) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"No certificate data received\"}");
        return;
    }

    String cert_type = "";
    if (webServer.hasArg("cert_type")) {
        cert_type = webServer.arg("cert_type");
    } else {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Certificate type not specified\"}");
        return;
    }

    if (!has_valid_certificate(cert_data.c_str())) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid certificate format\"}");
        return;
    }

    if (cert_data.length() > 2048) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Certificate too large (max 2KB)\"}");
        return;
    }

    String filename = getCertificateFilename(cert_type);
    if (filename == "") {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid certificate type: " + cert_type + "\"}");
        return;
    }

    if (!saveCertificateToSPIFFS(filename.c_str(), cert_data)) {
        webServer.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save certificate to SPIFFS\"}");
        return;
    }

    String cert_name = cert_type;
    cert_name.replace("_", " ");
    String protocol = (cert_type.startsWith("mqtt_") || cert_type == "root_ca" || cert_type == "device_cert" || cert_type == "device_key") ? "MQTT" : "HTTPS";
    logMessage(protocol + ": " + cert_name + " certificate uploaded and saved to SPIFFS");
    webServer.send(200, "application/json", "{\"success\":true,\"message\":\"" + cert_name + " certificate saved successfully\"}");

    uploaded_cert_data = "";
}
