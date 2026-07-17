/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Web Server Module - Settings/configuration page handlers
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
#include "../network/network.h"
#include "../network/ntp_time.h"
#include "../services/mqtt.h"
#include "../services/imap.h"
#include "../protocol/transmission.h"
#include "../../include/boards/boards.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// =============================================================================
// FORWARD DECLARATIONS (file-local helpers)
// =============================================================================
static String getReservedPinsJson();
static bool is_using_default_api_password();

// =============================================================================
// DEVICE CONFIGURATION PAGE
// =============================================================================

void handle_configuration() {
    reset_oled_timeout();

    if (ESP.getFreeHeap() < 9216) {
        webServer.send(503, "text/plain", "Insufficient memory");
        return;
    }

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    webServer.sendContent(get_html_header("Configuration"));

    webServer.sendContent("<div class='header'>"
                         "<h1>⚙️ Device Configuration</h1>"
                         "</div>");

    String nav_content = "<div class='nav'>"
                        "<a href='/' class='tab-inactive'>📡 Message</a>"
                        "<a href='/config' class='tab-active'>⚙️ Config</a>"
                        "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
                        "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
                        "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
                        + chatgpt_nav_tab_html(false) +
                        "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
                        + imap_nav_tab_html(false)
                        + gsm_nav_tab_html(false) +
                        "<a href='/status' class='tab-inactive'>📊 Status</a>"
                        "</div>";
    webServer.sendContent(nav_content);

    webServer.sendContent("<form action='/save_config' method='post' onsubmit='return submitFormAjax(this, \"Settings saved successfully!\", \"Settings saved, restarting in 5 seconds...\")'>"
                         "<div style='display: flex; flex-direction: column; gap: 20px; margin: 20px 0;'>");

    String interface_section = "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
                              "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🎨 Interface Settings</h4>"
                              "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 10px;'>"
                              "<div>"
                              "<label for='banner_message' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Device Banner (16 chars max):</label>"
                              "<input type='text' id='banner_message' name='banner_message' value='" + htmlEscape(String(settings.banner_message)) + "' maxlength='16' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                              "</div>"
                              "<div>"
                              "<label for='theme' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>UI Theme:</label>"
                              "<select id='theme' name='theme' onchange='onThemeChange()' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                              "<option value='0'" + (settings.theme == 0 ? " selected" : "") + ">🌞 Minimal White</option>"
                              "<option value='1'" + (settings.theme == 1 ? " selected" : "") + ">🌙 Carbon Black</option>"
                              "</select>"
                              "</div>"
                              "</div>"
                              "</div>";
    webServer.sendContent(interface_section);

    String timezone_section = "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
                             "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🕐 Timezone Settings</h4>"
                             "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 10px;'>"
                             "<div>"
                             "<label for='ntp_server' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>NTP Server:</label>"
                             "<input type='text' id='ntp_server' name='ntp_server' value='" + htmlEscape(String(settings.ntp_server)) + "' maxlength='63' placeholder='pool.ntp.org' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                             "</div>"
                             "<div>"
                             "<label for='timezone_offset' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Local Timezone:</label>"
                             "<select id='timezone_offset' name='timezone_offset' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                             "<option value='-12.0'" + String(settings.timezone_offset_hours == -12.0 ? " selected" : "") + ">UTC-12:00 (Baker Island)</option>"
                             "<option value='-11.0'" + String(settings.timezone_offset_hours == -11.0 ? " selected" : "") + ">UTC-11:00 (American Samoa)</option>"
                             "<option value='-10.0'" + String(settings.timezone_offset_hours == -10.0 ? " selected" : "") + ">UTC-10:00 (Hawaii)</option>"
                             "<option value='-9.0'" + String(settings.timezone_offset_hours == -9.0 ? " selected" : "") + ">UTC-09:00 (Alaska)</option>"
                             "<option value='-8.0'" + String(settings.timezone_offset_hours == -8.0 ? " selected" : "") + ">UTC-08:00 (Pacific Time)</option>"
                             "<option value='-7.0'" + String(settings.timezone_offset_hours == -7.0 ? " selected" : "") + ">UTC-07:00 (Mountain Time)</option>"
                             "<option value='-6.0'" + String(settings.timezone_offset_hours == -6.0 ? " selected" : "") + ">UTC-06:00 (Central Time)</option>"
                             "<option value='-5.0'" + String(settings.timezone_offset_hours == -5.0 ? " selected" : "") + ">UTC-05:00 (Eastern Time)</option>"
                             "<option value='-4.0'" + String(settings.timezone_offset_hours == -4.0 ? " selected" : "") + ">UTC-04:00 (Atlantic Time)</option>"
                             "<option value='-3.5'" + String(settings.timezone_offset_hours == -3.5 ? " selected" : "") + ">UTC-03:30 (Newfoundland)</option>"
                             "<option value='-3.0'" + String(settings.timezone_offset_hours == -3.0 ? " selected" : "") + ">UTC-03:00 (Brazil, Argentina)</option>"
                             "<option value='-2.0'" + String(settings.timezone_offset_hours == -2.0 ? " selected" : "") + ">UTC-02:00 (Mid-Atlantic)</option>"
                             "<option value='-1.0'" + String(settings.timezone_offset_hours == -1.0 ? " selected" : "") + ">UTC-01:00 (Azores)</option>"
                             "<option value='0.0'" + String(settings.timezone_offset_hours == 0.0 ? " selected" : "") + ">UTC+00:00 (London, Dublin)</option>"
                             "<option value='1.0'" + String(settings.timezone_offset_hours == 1.0 ? " selected" : "") + ">UTC+01:00 (Paris, Berlin)</option>"
                             "<option value='2.0'" + String(settings.timezone_offset_hours == 2.0 ? " selected" : "") + ">UTC+02:00 (Athens, Cairo)</option>"
                             "<option value='3.0'" + String(settings.timezone_offset_hours == 3.0 ? " selected" : "") + ">UTC+03:00 (Moscow, Istanbul)</option>"
                             "<option value='3.5'" + String(settings.timezone_offset_hours == 3.5 ? " selected" : "") + ">UTC+03:30 (Tehran)</option>"
                             "<option value='4.0'" + String(settings.timezone_offset_hours == 4.0 ? " selected" : "") + ">UTC+04:00 (Dubai)</option>"
                             "<option value='4.5'" + String(settings.timezone_offset_hours == 4.5 ? " selected" : "") + ">UTC+04:30 (Kabul)</option>"
                             "<option value='5.0'" + String(settings.timezone_offset_hours == 5.0 ? " selected" : "") + ">UTC+05:00 (Karachi)</option>"
                             "<option value='5.5'" + String(settings.timezone_offset_hours == 5.5 ? " selected" : "") + ">UTC+05:30 (India)</option>"
                             "<option value='6.0'" + String(settings.timezone_offset_hours == 6.0 ? " selected" : "") + ">UTC+06:00 (Dhaka)</option>"
                             "<option value='6.5'" + String(settings.timezone_offset_hours == 6.5 ? " selected" : "") + ">UTC+06:30 (Myanmar)</option>"
                             "<option value='7.0'" + String(settings.timezone_offset_hours == 7.0 ? " selected" : "") + ">UTC+07:00 (Bangkok, Jakarta)</option>"
                             "<option value='8.0'" + String(settings.timezone_offset_hours == 8.0 ? " selected" : "") + ">UTC+08:00 (Singapore, Beijing)</option>"
                             "<option value='9.0'" + String(settings.timezone_offset_hours == 9.0 ? " selected" : "") + ">UTC+09:00 (Tokyo, Seoul)</option>"
                             "<option value='9.5'" + String(settings.timezone_offset_hours == 9.5 ? " selected" : "") + ">UTC+09:30 (Adelaide)</option>"
                             "<option value='10.0'" + String(settings.timezone_offset_hours == 10.0 ? " selected" : "") + ">UTC+10:00 (Sydney, Melbourne)</option>"
                             "<option value='11.0'" + String(settings.timezone_offset_hours == 11.0 ? " selected" : "") + ">UTC+11:00 (Solomon Islands)</option>"
                             "<option value='12.0'" + String(settings.timezone_offset_hours == 12.0 ? " selected" : "") + ">UTC+12:00 (Fiji, New Zealand)</option>"
                             "<option value='13.0'" + String(settings.timezone_offset_hours == 13.0 ? " selected" : "") + ">UTC+13:00 (Tonga)</option>"
                             "<option value='14.0'" + String(settings.timezone_offset_hours == 14.0 ? " selected" : "") + ">UTC+14:00 (Line Islands)</option>"
                             "</select>"
                             "</div>"
                             "</div>"
                             "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 15px;'>"
                             "<div style='padding: 12px; border: 2px solid var(--theme-border); border-radius: 8px; background-color: var(--theme-input); text-align: center;'>"
                             "<div style='font-size: 13px; color: var(--theme-secondary); margin-bottom: 6px;'>🌍 Hardware Clock (UTC)</div>"
                             "<div id='utc_time' style='font-size: 22px; font-weight: bold; color: var(--theme-text);'>--:--:--</div>"
                             "<div id='utc_date' style='font-size: 13px; color: var(--theme-secondary); margin-top: 4px;'>----</div>"
                             "</div>"
                             "<div style='padding: 12px; border: 2px solid var(--theme-accent); border-radius: 8px; background-color: var(--theme-input); text-align: center;'>"
                             "<div style='font-size: 13px; color: var(--theme-secondary); margin-bottom: 6px;'>📍 Local Time <span id='local_offset' style='font-size: 11px; color: var(--theme-accent);'>(UTC+00:00)</span></div>"
                             "<div id='local_time' style='font-size: 22px; font-weight: bold; color: var(--theme-text);'>--:--:--</div>"
                             "<div id='local_date' style='font-size: 13px; color: var(--theme-secondary); margin-top: 4px;'>----</div>"
                             "</div>"
                             "</div>"
                             "<script>"
                             "let deviceUTCOffset = 0;"
                             "function initClocks() {"
                             "  const deviceUTC = " + String(getUnixTimestamp()) + ";"
                             "  const clientUTC = Math.floor(Date.now() / 1000);"
                             "  deviceUTCOffset = deviceUTC - clientUTC;"
                             "  updateClocks();"
                             "  setInterval(updateClocks, 1000);"
                             "}"
                             "function updateClocks() {"
                             "  const now = Math.floor(Date.now() / 1000) + deviceUTCOffset;"
                             "  const tzOffset = parseFloat(document.getElementById('timezone_offset').value);"
                             "  const localTime = now + (tzOffset * 3600);"
                             "  const utcDate = new Date(now * 1000);"
                             "  document.getElementById('utc_time').textContent = utcDate.toISOString().substr(11, 8);"
                             "  document.getElementById('utc_date').textContent = utcDate.toISOString().substr(0, 10);"
                             "  const localDate = new Date(localTime * 1000);"
                             "  document.getElementById('local_time').textContent = localDate.toISOString().substr(11, 8);"
                             "  document.getElementById('local_date').textContent = localDate.toISOString().substr(0, 10);"
                             "  const offsetHours = Math.floor(Math.abs(tzOffset));"
                             "  const offsetMins = (Math.abs(tzOffset) % 1) * 60;"
                             "  const offsetStr = (tzOffset >= 0 ? '+' : '-') + String(offsetHours).padStart(2, '0') + ':' + String(offsetMins).padStart(2, '0');"
                             "  document.getElementById('local_offset').textContent = '(UTC' + offsetStr + ')';"
                             "}"
                             "document.getElementById('timezone_offset').addEventListener('change', updateClocks);"
                             "initClocks();"
                             "</script>";
#ifdef ENABLE_RTC
    timezone_section += "<div style='margin-top:15px;padding:15px;border:1px dashed var(--theme-border);border-radius:8px;background-color:var(--theme-input);color:var(--theme-text);'>";
    timezone_section += "<strong>RTC Module:</strong> ";
    timezone_section += String(rtc_available ? "✅ Detected" : "⚠️ Not detected");
    timezone_section += "<p style='margin:8px 0 0 0;font-size:0.9em;color:var(--theme-secondary);'>Hardware RTC presence is determined by the DS3231 wiring on the I²C bus. When detected, it seeds the system clock at boot and is refreshed automatically after WiFi (NTP) or GSM modem sync.</p>";
    timezone_section += "</div>";
#else
    timezone_section += "<div style='margin-top:15px;padding:15px;border:1px dashed var(--theme-border);border-radius:8px;background-color:var(--theme-input);color:var(--theme-text);'>"
                        "<strong>RTC Module:</strong> 🚫 Disabled at build time"
                        "<p style='margin:8px 0 0 0;font-size:0.9em;color:var(--theme-secondary);'>Recompile firmware with RTC support enabled to use an external DS3231 hardware clock.</p>"
                        "</div>";
#endif
    timezone_section += "</div>";
    webServer.sendContent(timezone_section);

    String static_ip_script = "<script>"
                             "var storedNetworks = [";
    for (int i = 0; i < stored_networks_count; i++) {
        if (i > 0) static_ip_script += ",";
        static_ip_script += "{"
                           "ssid:'" + String(stored_networks[i].ssid) + "',"
                           "password:'" + String(stored_networks[i].password) + "',"
                           "use_dhcp:" + String(stored_networks[i].use_dhcp ? "true" : "false") + ","
                           "static_ip:'" + String(stored_networks[i].static_ip[0]) + "." +
                                           String(stored_networks[i].static_ip[1]) + "." +
                                           String(stored_networks[i].static_ip[2]) + "." +
                                           String(stored_networks[i].static_ip[3]) + "',"
                           "netmask:'" + String(stored_networks[i].netmask[0]) + "." +
                                         String(stored_networks[i].netmask[1]) + "." +
                                         String(stored_networks[i].netmask[2]) + "." +
                                         String(stored_networks[i].netmask[3]) + "',"
                           "gateway:'" + String(stored_networks[i].gateway[0]) + "." +
                                         String(stored_networks[i].gateway[1]) + "." +
                                         String(stored_networks[i].gateway[2]) + "." +
                                         String(stored_networks[i].gateway[3]) + "',"
                           "dns:'" + String(stored_networks[i].dns[0]) + "." +
                                     String(stored_networks[i].dns[1]) + "." +
                                     String(stored_networks[i].dns[2]) + "." +
                                     String(stored_networks[i].dns[3]) + "'"
                           "}";
    }
    static_ip_script += "];"
                       "var currentConnectedSSID = '" + (wifi_connected ? current_connected_ssid : "") + "';"
                       "var currentWiFiIP = '" + (wifi_connected ? WiFi.localIP().toString() : "0.0.0.0") + "';"
                       "var currentWiFiNetmask = '" + (wifi_connected ? WiFi.subnetMask().toString() : "0.0.0.0") + "';"
                       "var currentWiFiGateway = '" + (wifi_connected ? WiFi.gatewayIP().toString() : "0.0.0.0") + "';"
                       "var currentWiFiDNS = '" + (wifi_connected ? WiFi.dnsIP().toString() : "0.0.0.0") + "';"
                       "function onSSIDChange() {"
                       "  var select = document.getElementById('wifi_ssid');"
                       "  var value = select.value;"
                       "  var customInput = document.getElementById('custom_ssid_input');"
                       "  var deleteBtn = document.getElementById('delete_network_btn');"
                       "  var addBtn = document.getElementById('add_network_btn');"
                       "  "
                       "  if (value === '__SCAN__') {"
                       "    deleteBtn.disabled = true;"
                       "    deleteBtn.style.opacity = '0.5';"
                       "    deleteBtn.style.cursor = 'not-allowed';"
                       "    addBtn.disabled = true;"
                       "    addBtn.style.opacity = '0.5';"
                       "    addBtn.style.cursor = 'not-allowed';"
                       "    scanWiFi();"
                       "    return;"
                       "  }"
                       "  "
                       "  if (value === '__CUSTOM__') {"
                       "    select.style.display = 'none';"
                       "    customInput.style.display = 'flex';"
                       "    customInput.focus();"
                       "    document.getElementById('wifi_password').value = '';"
                       "    document.getElementById('use_dhcp').value = '1';"
                       "    deleteBtn.disabled = true;"
                       "    deleteBtn.style.opacity = '0.5';"
                       "    deleteBtn.style.cursor = 'not-allowed';"
                       "    addBtn.disabled = false;"
                       "    addBtn.style.opacity = '1';"
                       "    addBtn.style.cursor = 'pointer';"
                       "    toggleStaticIP();"
                       "    return;"
                       "  }"
                       "  "
                       "  customInput.style.display = 'none';"
                       "  select.style.display = 'flex';"
                       "  "
                       "  var ssid = value;"
                       "  var network = null;"
                       "  "
                       "  for (var i = 0; i < storedNetworks.length; i++) {"
                       "    if (storedNetworks[i].ssid === ssid) {"
                       "      network = storedNetworks[i];"
                       "      break;"
                       "    }"
                       "  }"
                       "  "
                       "  if (network) {"
                       "    document.getElementById('wifi_password').value = network.password;"
                       "    document.getElementById('use_dhcp').value = network.use_dhcp ? '1' : '0';"
                       "    "
                       "    var isConnectedToThis = (ssid === currentConnectedSSID);"
                       "    "
                       "    if (isConnectedToThis) {"
                       "      document.getElementById('static_ip').value = currentWiFiIP;"
                       "      document.getElementById('netmask').value = currentWiFiNetmask;"
                       "      document.getElementById('gateway').value = currentWiFiGateway;"
                       "      document.getElementById('dns').value = currentWiFiDNS;"
                       "    } else if (!network.use_dhcp) {"
                       "      document.getElementById('static_ip').value = network.static_ip || '';"
                       "      document.getElementById('netmask').value = network.netmask || '';"
                       "      document.getElementById('gateway').value = network.gateway || '';"
                       "      document.getElementById('dns').value = network.dns || '';"
                       "    }"
                       "    "
                       "    deleteBtn.disabled = false;"
                       "    deleteBtn.style.opacity = '1';"
                       "    deleteBtn.style.cursor = 'pointer';"
                       "    addBtn.disabled = true;"
                       "    addBtn.style.opacity = '0.5';"
                       "    addBtn.style.cursor = 'not-allowed';"
                       "  } else {"
                       "    document.getElementById('wifi_password').value = '';"
                       "    document.getElementById('use_dhcp').value = '1';"
                       "    deleteBtn.disabled = true;"
                       "    deleteBtn.style.opacity = '0.5';"
                       "    deleteBtn.style.cursor = 'not-allowed';"
                       "    "
                       "    if (ssid && ssid !== '') {"
                       "      addBtn.disabled = false;"
                       "      addBtn.style.opacity = '1';"
                       "      addBtn.style.cursor = 'pointer';"
                       "    } else {"
                       "      addBtn.disabled = true;"
                       "      addBtn.style.opacity = '0.5';"
                       "      addBtn.style.cursor = 'not-allowed';"
                       "    }"
                       "  }"
                       "  toggleStaticIP();"
                       "}"
                       "function deleteNetwork() {"
                       "  var value = document.getElementById('wifi_ssid').value;"
                       "  var ssid = value.trim();"
                       "  if (!ssid) return;"
                       "  "
                       "  if (confirm('Delete network \"' + ssid + '\"?')) {"
                       "    fetch('/api/wifi/delete?ssid=' + encodeURIComponent(ssid), {method: 'POST'})"
                       "      .then(function(response) { return response.json(); })"
                       "      .then(function(data) {"
                       "        if (data.success) {"
                       "          for (var i = 0; i < storedNetworks.length; i++) {"
                       "            if (storedNetworks[i].ssid === ssid) {"
                       "              storedNetworks.splice(i, 1);"
                       "              break;"
                       "            }"
                       "          }"
                       "          var select = document.getElementById('wifi_ssid');"
                       "          for (var i = 0; i < select.options.length; i++) {"
                       "            if (select.options[i].value === ssid) {"
                       "              select.removeChild(select.options[i]);"
                       "              break;"
                       "            }"
                       "          }"
                       "          select.value = '';"
                       "          onSSIDChange();"
                       "          alert('Network deleted successfully');"
                       "        } else {"
                       "          alert('Failed to delete network');"
                       "        }"
                       "      })"
                       "      .catch(function(err) {"
                       "        alert('Error deleting network');"
                       "      });"
                       "  }"
                       "}"
                       "function addNetwork() {"
                       "  var select = document.getElementById('wifi_ssid');"
                       "  var customInput = document.getElementById('custom_ssid_input');"
                       "  var ssid = (customInput.style.display === 'flex') ? customInput.value.trim() : select.value.trim();"
                       "  var password = document.getElementById('wifi_password').value.trim();"
                       "  var useDhcp = document.getElementById('use_dhcp').value;"
                       "  "
                       "  if (!ssid || ssid === '__SCAN__' || ssid === '__CUSTOM__') {"
                       "    alert('Please select or enter a valid network');"
                       "    return;"
                       "  }"
                       "  "
                       "  if (password.length === 0) {"
                       "    alert('Please enter a password');"
                       "    return;"
                       "  }"
                       "  "
                       "  if (useDhcp === '0') {"
                       "    var staticIp = document.getElementById('static_ip').value.trim();"
                       "    var netmask = document.getElementById('netmask').value.trim();"
                       "    var gateway = document.getElementById('gateway').value.trim();"
                       "    var dns = document.getElementById('dns').value.trim();"
                       "    "
                       "    if (!staticIp || !netmask || !gateway || !dns) {"
                       "      alert('Please fill all IP configuration fields for static IP');"
                       "      return;"
                       "    }"
                       "    "
                       "    var ipPattern = /^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}$/;"
                       "    if (!ipPattern.test(staticIp) || !ipPattern.test(netmask) || !ipPattern.test(gateway) || !ipPattern.test(dns)) {"
                       "      alert('Invalid IP address format');"
                       "      return;"
                       "    }"
                       "  }"
                       "  "
                       "  if (confirm('Add network \"' + ssid + '\"?')) {"
                       "    var params = new URLSearchParams();"
                       "    params.append('ssid', ssid);"
                       "    params.append('password', password);"
                       "    params.append('use_dhcp', useDhcp);"
                       "    "
                       "    if (useDhcp === '0') {"
                       "      params.append('static_ip', document.getElementById('static_ip').value);"
                       "      params.append('netmask', document.getElementById('netmask').value);"
                       "      params.append('gateway', document.getElementById('gateway').value);"
                       "      params.append('dns', document.getElementById('dns').value);"
                       "    }"
                       "    "
                       "    fetch('/api/wifi/add', {"
                       "      method: 'POST',"
                       "      headers: {'Content-Type': 'application/x-www-form-urlencoded'},"
                       "      body: params.toString()"
                       "    })"
                       "    .then(function(response) { return response.json(); })"
                       "    .then(function(data) {"
                       "      if (data.success) {"
                       "        showTempMessage('Network added successfully!', 'success', 3000);"
                       "        "
                       "        var select = document.getElementById('wifi_ssid');"
                       "        var newOption = document.createElement('option');"
                       "        newOption.value = ssid;"
                       "        newOption.text = ssid;"
                       "        "
                       "        var insertIndex = select.options.length - 2;"
                       "        select.add(newOption, insertIndex);"
                       "        "
                       "        var networkData = {"
                       "          ssid: ssid,"
                       "          password: password,"
                       "          use_dhcp: useDhcp === '1',"
                       "          static_ip: (useDhcp === '0') ? document.getElementById('static_ip').value : '',"
                       "          netmask: (useDhcp === '0') ? document.getElementById('netmask').value : '',"
                       "          gateway: (useDhcp === '0') ? document.getElementById('gateway').value : '',"
                       "          dns: (useDhcp === '0') ? document.getElementById('dns').value : ''"
                       "        };"
                       "        storedNetworks.push(networkData);"
                       "        "
                       "        select.value = ssid;"
                       "        onSSIDChange();"
                       "      } else {"
                       "        showTempMessage('Error: ' + (data.error || 'Failed'), 'error', 5000);"
                       "      }"
                       "    })"
                       "    .catch(function() {"
                       "      showTempMessage('Network error', 'error', 5000);"
                       "    });"
                       "  }"
                       "}"
                       "window.addEventListener('DOMContentLoaded', function() {"
                       "  onSSIDChange();"
                       "  "
                       "  var customInput = document.getElementById('custom_ssid_input');"
                       "  var select = document.getElementById('wifi_ssid');"
                       "  "
                       "  customInput.addEventListener('blur', function(e) {"
                       "    if (!customInput.value.trim()) {"
                       "      customInput.style.display = 'none';"
                       "      select.style.display = 'flex';"
                       "      select.value = '';"
                       "    }"
                       "  });"
                       "  "
                       "  customInput.addEventListener('keydown', function(e) {"
                       "    if (e.key === 'Escape') {"
                       "      customInput.style.display = 'none';"
                       "      select.style.display = 'flex';"
                       "      select.value = '';"
                       "      customInput.value = '';"
                       "    }"
                       "  });"
                       "});"
                       "function scanWiFi() {"
                             "  var select = document.getElementById('wifi_ssid');"
                             "  var scanOption = null;"
                             "  "
                             "  for (var i = 0; i < select.options.length; i++) {"
                             "    if (select.options[i].value === '__SCAN__') {"
                             "      scanOption = select.options[i];"
                             "      break;"
                             "    }"
                             "  }"
                             "  "
                             "  if (scanOption) {"
                             "    scanOption.text = '⏳ Scanning...';"
                             "    scanOption.disabled = true;"
                             "  }"
                             "  "
                             "  fetch('/api/wifi/scan')"
                             "    .then(function(response) { return response.json(); })"
                             "    .then(function(data) {"
                             "      if (data.success) {"
                             "        for (var i = select.options.length - 1; i >= 0; i--) {"
                             "          var opt = select.options[i];"
                             "          var isSpecial = (opt.value === '' || opt.value === '__SCAN__' || opt.value === '__CUSTOM__');"
                             "          var isStored = storedNetworks.some(function(n) { return n.ssid === opt.value; });"
                             "          if (!isSpecial && !isStored) {"
                             "            select.removeChild(opt);"
                             "          }"
                             "        }"
                             "        "
                             "        var insertBeforeIndex = -1;"
                             "        for (var i = 0; i < select.options.length; i++) {"
                             "          if (select.options[i].value === '__SCAN__') {"
                             "            insertBeforeIndex = i;"
                             "            break;"
                             "          }"
                             "        }"
                             "        "
                             "        if (insertBeforeIndex > 0 && storedNetworks.length > 0 && data.networks.length > 0) {"
                             "          var separatorOpt = document.createElement('option');"
                             "          separatorOpt.value = '';"
                             "          separatorOpt.text = '[--- Scanned Networks ---]';"
                             "          separatorOpt.disabled = true;"
                             "          select.insertBefore(separatorOpt, select.options[insertBeforeIndex]);"
                             "          insertBeforeIndex++;"
                             "        }"
                             "        "
                             "        data.networks.forEach(function(net) {"
                             "          var isStored = false;"
                             "          for (var j = 0; j < storedNetworks.length; j++) {"
                             "            if (storedNetworks[j].ssid === net.ssid) {"
                             "              isStored = true;"
                             "              break;"
                             "            }"
                             "          }"
                             "          "
                             "          if (!isStored && insertBeforeIndex >= 0) {"
                             "            var opt = document.createElement('option');"
                             "            opt.value = net.ssid;"
                             "            opt.text = net.ssid;"
                             "            select.insertBefore(opt, select.options[insertBeforeIndex]);"
                             "            insertBeforeIndex++;"
                             "          }"
                             "        });"
                             "      }"
                             "      "
                             "      if (scanOption) {"
                             "        scanOption.text = '🔍 Scan again...';"
                             "        scanOption.disabled = false;"
                             "      }"
                             "      select.value = '';"
                             "      onSSIDChange();"
                             "    })"
                             "    .catch(function(err) {"
                             "      if (scanOption) {"
                             "        scanOption.text = '🔍 Scan for networks...';"
                             "        scanOption.disabled = false;"
                             "      }"
                             "      select.value = '';"
                             "      onSSIDChange();"
                             "      alert('Error scanning networks');"
                             "    });"
                             "}"
                             "function toggleStaticIP() {"
                             "  var useDhcp = document.getElementById('use_dhcp').value === '1';"
                             "  var select = document.getElementById('wifi_ssid');"
                             "  var selectedValue = select ? select.value : '';"
                             "  var selectedSSID = selectedValue;"
                             "  var isConnectedNetwork = (selectedSSID === currentConnectedSSID && selectedSSID !== '');"
                             "  "
                             "  var shouldShowFields = !useDhcp || (useDhcp && isConnectedNetwork);"
                             "  "
                             "  var row1 = document.getElementById('ip_settings_row1');"
                             "  var row2 = document.getElementById('ip_settings_row2');"
                             "  "
                             "  if (row1) row1.style.display = shouldShowFields ? 'grid' : 'none';"
                             "  if (row2) row2.style.display = shouldShowFields ? 'grid' : 'none';"
                             "  "
                             "  if (shouldShowFields) {"
                             "    var staticFields = ['static_ip', 'netmask', 'gateway', 'dns'];"
                             "    for (var i = 0; i < staticFields.length; i++) {"
                             "      var field = document.getElementById(staticFields[i]);"
                             "      if (field) {"
                             "        field.disabled = useDhcp;"
                             "        if (useDhcp) {"
                             "          field.style.backgroundColor = '#f5f5f5';"
                             "          field.style.color = '#999';"
                             "        } else {"
                             "          field.style.backgroundColor = 'var(--theme-input)';"
                             "          field.style.color = 'var(--theme-text)';"
                             "        }"
                             "      }"
                             "    }"
                             "  }"
                             "}"
                             "function togglePasswordVisibility() {"
                             "  var passwordField = document.getElementById('wifi_password');"
                             "  if (passwordField) {"
                             "    if (passwordField.type === 'password') {"
                             "      passwordField.type = 'text';"
                             "    } else {"
                             "      passwordField.type = 'password';"
                             "    }"
                             "  }"
                             "}"
                             "window.onload = function() { toggleStaticIP(); validateConfigPower(); };"
                             "function validateConfigPower() {"
                             "  var p = document.getElementById('tx_power');"
                             "  if (!p) return;"
                             "  var val = parseInt(p.value);"
                             "  if (val < 0 || val > 20) {"
                             "    p.style.borderColor = '#e74c3c';"
                             "    p.style.backgroundColor = '#fdf2f2';"
                             "  } else {"
                             "    p.style.borderColor = 'var(--theme-border)';"
                             "    p.style.backgroundColor = 'var(--theme-input)';"
                             "  }"
                             "}"
                             "var txPowerEl = document.getElementById('tx_power');"
                             "if (txPowerEl) {"
                             "  txPowerEl.addEventListener('input', validateConfigPower);"
                             "  txPowerEl.addEventListener('keyup', validateConfigPower);"
                             "}"
                             "var configForm = document.getElementById('configForm');"
                             "if (configForm) {"
                             "  configForm.addEventListener('submit', function(e) {"
                             "    var customInput = document.getElementById('custom_ssid_input');"
                             "    var select = document.getElementById('wifi_ssid');"
                             "    "
                             "    if (customInput && customInput.style.display !== 'none' && customInput.value.trim()) {"
                             "      var hiddenInput = document.createElement('input');"
                             "      hiddenInput.type = 'hidden';"
                             "      hiddenInput.name = 'wifi_ssid';"
                             "      hiddenInput.value = customInput.value.trim();"
                             "      configForm.appendChild(hiddenInput);"
                             "      select.disabled = true;"
                             "    }"
                             "    "
                             "    var txPowerEl = document.getElementById('tx_power');"
                             "    if (txPowerEl) {"
                             "      var p = parseInt(txPowerEl.value);"
                             "      if (p < 0 || p > 20) {"
                             "        e.preventDefault();"
                             "        alert('TX Power must be between 0 and 20 dBm');"
                             "        return false;"
                             "      }"
                             "    }"
                             "  });"
                             "}"
                             "</script>";
    webServer.sendContent(static_ip_script);

    String network_section_part1;
    network_section_part1 = "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>";
    network_section_part1 += "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🌐 Network Settings</h4>";
    network_section_part1 += "<div style='margin-bottom: 15px;'>";
    network_section_part1 += "<label for='wifi_ssid' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>SSID:</label>";
    network_section_part1 += "<div style='display: flex; gap: 10px;'>";
    network_section_part1 += "<select id='wifi_ssid' name='wifi_ssid' onchange='onSSIDChange()' style='flex: 1; padding:12px 16px; border:2px solid var(--theme-border); border-radius:8px; font-size:16px; box-sizing:border-box; background-color:var(--theme-input); color:var(--theme-text); transition:all 0.3s ease;'>";
    network_section_part1 += "<option value=''>-- Select Network --</option>";

    for (int i = 0; i < stored_networks_count; i++) {
        String ssid = htmlEscape(String(stored_networks[i].ssid));
        String selected = (wifi_connected && current_connected_ssid == String(stored_networks[i].ssid)) ? " selected" : "";
        network_section_part1 += "<option value='" + ssid + "'" + selected + ">" + ssid + "</option>";
    }

    network_section_part1 += "<option value='__SCAN__'>🔍 Scan for networks...</option>";
    network_section_part1 += "<option value='__CUSTOM__'>✏️ Other/Custom SSID...</option>";
    network_section_part1 += "</select>";

    network_section_part1 += "<input type='text' id='custom_ssid_input' placeholder='Enter SSID...' maxlength='32' style='display:none; flex: 1; padding:12px 16px; border:2px solid var(--theme-border); border-radius:8px; font-size:16px; box-sizing:border-box; background-color:var(--theme-input); color:var(--theme-text); transition:all 0.3s ease;'>";

    network_section_part1 += "<button type='button' id='add_network_btn' onclick='addNetwork()' class='button edit' style='white-space:nowrap;' disabled>➕ Add</button>";
    network_section_part1 += "<button type='button' id='delete_network_btn' onclick='deleteNetwork()' class='button danger' style='white-space:nowrap;' disabled>🗑️ Delete</button>";
    network_section_part1 += "</div>";
    network_section_part1 += "</div>";
    network_section_part1 += "<div id='network_settings_container' style='display: grid; grid-template-columns: 1fr 1fr; gap: 10px;'>";
    network_section_part1 += "<div style='margin-bottom: 15px;'>";
    network_section_part1 += "<label for='use_dhcp' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Use DHCP:</label>";
    network_section_part1 += "<select id='use_dhcp' name='use_dhcp' onchange='toggleStaticIP()' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>";
    network_section_part1 += "<option value='1' selected>Yes (Automatic IP)</option>";
    network_section_part1 += "<option value='0'>No (Static IP)</option>";
    network_section_part1 += "</select>";
    network_section_part1 += "</div>";
    network_section_part1 += "<div style='margin-bottom: 15px;'>";
    network_section_part1 += "<label for='wifi_password' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Password:</label>";
    network_section_part1 += "<div style='position: relative;'>";
    network_section_part1 += "<input type='password' id='wifi_password' name='wifi_password' value='' maxlength='64' style='width:100%;padding:12px 40px 12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>";
    network_section_part1 += "<button type='button' onclick='togglePasswordVisibility()' style='position:absolute;right:8px;top:50%;transform:translateY(-50%);background:none;border:none;cursor:pointer;font-size:20px;padding:4px 8px;color:var(--theme-text);opacity:0.6;transition:opacity 0.2s;' onmouseover='this.style.opacity=\"1\"' onmouseout='this.style.opacity=\"0.6\"' title='Show/Hide Password'>👁️</button>";
    network_section_part1 += "</div>";
    network_section_part1 += "</div>";
    webServer.sendContent(network_section_part1);

    String network_section_part2 = "<div id='ip_settings_row1' style='grid-column: span 2; display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 15px;'>"
                             "<div>"
                             "<label for='static_ip' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>IP Address:</label>"
                             "<input type='text' id='static_ip' name='static_ip' value='" +
                             (wifi_connected ? WiFi.localIP().toString() : String("192.168.1.100")) +
                             "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='192.168.1.100' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                             "</div>"
                             "<div>"
                             "<label for='netmask' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Netmask:</label>"
                             "<input type='text' id='netmask' name='netmask' value='" +
                             (wifi_connected ? WiFi.subnetMask().toString() : String("255.255.255.0")) +
                             "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='255.255.255.0' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                             "</div>"
                             "</div>";
    webServer.sendContent(network_section_part2);

    String network_section_part3 = "<div id='ip_settings_row2' style='grid-column: span 2; display: grid; grid-template-columns: 1fr 1fr; gap: 10px;'>"
                             "<div>"
                             "<label for='gateway' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Gateway:</label>"
                             "<input type='text' id='gateway' name='gateway' value='" +
                             (wifi_connected ? WiFi.gatewayIP().toString() : String("192.168.1.1")) +
                             "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='192.168.1.1' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                             "</div>"
                             "<div>"
                             "<label for='dns' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>DNS Server:</label>"
                             "<input type='text' id='dns' name='dns' value='" +
                             (wifi_connected ? WiFi.dnsIP().toString() : String("8.8.8.8")) +
                             "' pattern='\\d+\\.\\d+\\.\\d+\\.\\d+' placeholder='8.8.8.8' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                             "</div>"
                             "</div>"
                             "</div>"
                             "</div>";
    webServer.sendContent(network_section_part3);

    String rsyslog_section_part1 = "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
                                  "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;'>"
                                  "<h4 style='margin: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🖥️ Remote Logging (Rsyslog)</h4>"
                                  "<div class='toggle-switch " + String(settings.rsyslog_enabled ? "is-active" : "is-inactive") + "' onclick='toggleRsyslog()'>"
                                  "<div class='toggle-slider " + String(settings.rsyslog_enabled ? "is-active" : "is-inactive") + "'></div>"
                                  "</div>"
                                  "</div>"
                                  "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 15px;'>"
                                  "<div>"
                                  "<label for='rsyslog_server' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Server:</label>"
                                  "<input type='text' id='rsyslog_server' name='rsyslog_server' value='" + htmlEscape(String(settings.rsyslog_server)) + "' maxlength='50' placeholder='192.168.1.100 or syslog.example.com' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                                  "</div>"
                                  "<div>"
                                  "<label for='rsyslog_port' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Port:</label>"
                                  "<input type='number' id='rsyslog_port' name='rsyslog_port' value='" + String(settings.rsyslog_port) + "' min='1' max='65535' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                                  "</div>"
                                  "</div>";
    webServer.sendContent(rsyslog_section_part1);

    String rsyslog_section_part2 = "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 10px;'>"
                                  "<div>"
                                  "<label for='rsyslog_min_severity' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Minimum Severity:</label>"
                                  "<select id='rsyslog_min_severity' name='rsyslog_min_severity' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                                  "<option value='0'" + String(settings.rsyslog_min_severity == 0 ? " selected" : "") + ">Emergency (0)</option>"
                                  "<option value='1'" + String(settings.rsyslog_min_severity == 1 ? " selected" : "") + ">Alert (1)</option>"
                                  "<option value='2'" + String(settings.rsyslog_min_severity == 2 ? " selected" : "") + ">Critical (2)</option>"
                                  "<option value='3'" + String(settings.rsyslog_min_severity == 3 ? " selected" : "") + ">Error (3)</option>"
                                  "<option value='4'" + String(settings.rsyslog_min_severity == 4 ? " selected" : "") + ">Warning (4)</option>"
                                  "<option value='5'" + String(settings.rsyslog_min_severity == 5 ? " selected" : "") + ">Notice (5)</option>"
                                  "<option value='6'" + String(settings.rsyslog_min_severity == 6 ? " selected" : "") + ">Informational (6)</option>"
                                  "<option value='7'" + String(settings.rsyslog_min_severity == 7 ? " selected" : "") + ">Debug (7)</option>"
                                  "</select>"
                                  "<small style='display: block; margin-top: 5px; color: #666;'>Only forward messages at or below this severity level</small>"
                                  "</div>"
                                  "<div>"
                                  "<label for='rsyslog_use_tcp' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Protocol:</label>"
                                  "<select id='rsyslog_use_tcp' name='rsyslog_use_tcp' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
                                  "<option value='0'" + String(!settings.rsyslog_use_tcp ? " selected" : "") + ">UDP (recommended)</option>"
                                  "<option value='1'" + String(settings.rsyslog_use_tcp ? " selected" : "") + ">TCP</option>"
                                  "</select>"
                                  "</div>"
                                  "</div>"
                                  "</div>";
    webServer.sendContent(rsyslog_section_part2);

    String rsyslog_hidden = "<input type='hidden' id='rsyslog_enabled' name='rsyslog_enabled' value='" + String(settings.rsyslog_enabled ? "1" : "0") + "'>";
    webServer.sendContent(rsyslog_hidden);

    String alerts_section = "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
                           "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🚨 System Alerts</h4>"
                           "<div style='display: flex; align-items: center; gap: 12px;'>"
                           "<div class='toggle-switch " + String(settings.enable_low_battery_alert ? "is-active" : "is-inactive") + "' onclick='toggleLowBatteryAlert()'>"
                           "<div class='toggle-slider " + String(settings.enable_low_battery_alert ? "is-active" : "is-inactive") + "'></div>"
                           "</div>"
                           "<span style='font-weight: 500; color: var(--theme-text);'>Low Battery Alert (10% threshold)</span>"
                           "</div>"
                           "<div style='display: flex; align-items: center; gap: 12px; margin-top: 15px;'>"
                           "<div class='toggle-switch " + String(settings.enable_power_disconnect_alert ? "is-active" : "is-inactive") + "' onclick='togglePowerDisconnectAlert()'>"
                           "<div class='toggle-slider " + String(settings.enable_power_disconnect_alert ? "is-active" : "is-inactive") + "'></div>"
                           "</div>"
                           "<span style='font-weight: 500; color: var(--theme-text);'>Power Disconnect Alert (discharging)</span>"
                           "</div>"
                           "</div>";
    webServer.sendContent(alerts_section);

    webServer.sendContent("</div>");

    String hidden_inputs = "<input type='hidden' id='low_battery_alert' name='low_battery_alert' value='" + String(settings.enable_low_battery_alert ? "on" : "off") + "'>"
                          "<input type='hidden' id='power_disconnect_alert' name='power_disconnect_alert' value='" + String(settings.enable_power_disconnect_alert ? "on" : "off") + "'>";
    webServer.sendContent(hidden_inputs);

    webServer.sendContent("<div style='margin-top:30px;text-align:center;'>"
                         "<button type='submit' class='button button-large success'>💾 Save Configuration</button>"
                         "</div>"
                         "</form>");

    String scripts = "<script>"
                    "function toggleRsyslog() {"
                    "  var toggles = document.querySelectorAll('.toggle-switch');"
                    "  var toggle = toggles[0];"
                    "  var slider = toggle.querySelector('.toggle-slider');"
                    "  var input = document.getElementById('rsyslog_enabled');"
                    "  var isEnabled = input.value === '1';"
                    "  input.value = isEnabled ? '0' : '1';"
                    "  toggle.style.backgroundColor = isEnabled ? '#ccc' : '#28a745';"
                    "  slider.style.left = isEnabled ? '2px' : '26px';"
                    "}"
                    "function toggleLowBatteryAlert() {"
                    "  var toggles = document.querySelectorAll('.toggle-switch');"
                    "  var toggle = toggles[1];"
                    "  var slider = toggle.querySelector('.toggle-slider');"
                    "  var input = document.getElementById('low_battery_alert');"
                    "  var isEnabled = input.value === 'on';"
                    "  input.value = isEnabled ? 'off' : 'on';"
                    "  toggle.style.backgroundColor = isEnabled ? '#ccc' : '#28a745';"
                    "  slider.style.left = isEnabled ? '2px' : '26px';"
                    "}"
                    "function togglePowerDisconnectAlert() {"
                    "  var toggles = document.querySelectorAll('.toggle-switch');"
                    "  var toggle = toggles[toggles.length - 1];"
                    "  var slider = toggle.querySelector('.toggle-slider');"
                    "  var input = document.getElementById('power_disconnect_alert');"
                    "  var isEnabled = input.value === 'on';"
                    "  input.value = isEnabled ? 'off' : 'on';"
                    "  toggle.style.backgroundColor = isEnabled ? '#ccc' : '#28a745';"
                    "  slider.style.left = isEnabled ? '2px' : '26px';"
                    "}"
                    "</script>";
    webServer.sendContent(scripts);

    webServer.sendContent(get_html_footer());
    webServer.sendContent("");
}

void handle_save_config() {
    reset_oled_timeout();

    bool need_restart = true;
    CoreConfig old_core_config = core_config;
    DeviceSettings old_settings = settings;

    if (webServer.hasArg("wifi_ssid") && webServer.hasArg("wifi_password")) {
        String ssid = webServer.arg("wifi_ssid");
        String password = webServer.arg("wifi_password");
        ssid.trim();
        password.trim();

        if (ssid.length() > 0) {
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
            }

            if (network_idx >= 0) {
                strlcpy(stored_networks[network_idx].ssid, ssid.c_str(), sizeof(stored_networks[network_idx].ssid));
                strlcpy(stored_networks[network_idx].password, password.c_str(), sizeof(stored_networks[network_idx].password));

                stored_networks[network_idx].use_dhcp = (webServer.arg("use_dhcp") == "1");

                if (webServer.hasArg("static_ip")) {
                    String ip = webServer.arg("static_ip");
                    IPAddress parsed_ip;
                    if (parsed_ip.fromString(ip)) {
                        stored_networks[network_idx].static_ip[0] = parsed_ip[0];
                        stored_networks[network_idx].static_ip[1] = parsed_ip[1];
                        stored_networks[network_idx].static_ip[2] = parsed_ip[2];
                        stored_networks[network_idx].static_ip[3] = parsed_ip[3];
                    }
                }

                if (webServer.hasArg("netmask")) {
                    String netmask = webServer.arg("netmask");
                    IPAddress parsed_netmask;
                    if (parsed_netmask.fromString(netmask)) {
                        stored_networks[network_idx].netmask[0] = parsed_netmask[0];
                        stored_networks[network_idx].netmask[1] = parsed_netmask[1];
                        stored_networks[network_idx].netmask[2] = parsed_netmask[2];
                        stored_networks[network_idx].netmask[3] = parsed_netmask[3];
                    }
                }

                if (webServer.hasArg("gateway")) {
                    String gateway = webServer.arg("gateway");
                    IPAddress parsed_gateway;
                    if (parsed_gateway.fromString(gateway)) {
                        stored_networks[network_idx].gateway[0] = parsed_gateway[0];
                        stored_networks[network_idx].gateway[1] = parsed_gateway[1];
                        stored_networks[network_idx].gateway[2] = parsed_gateway[2];
                        stored_networks[network_idx].gateway[3] = parsed_gateway[3];
                    }
                }

                if (webServer.hasArg("dns")) {
                    String dns = webServer.arg("dns");
                    IPAddress parsed_dns;
                    if (parsed_dns.fromString(dns)) {
                        stored_networks[network_idx].dns[0] = parsed_dns[0];
                        stored_networks[network_idx].dns[1] = parsed_dns[1];
                        stored_networks[network_idx].dns[2] = parsed_dns[2];
                        stored_networks[network_idx].dns[3] = parsed_dns[3];
                    }
                }
            }
        }
    }



    if (webServer.hasArg("mqtt_enabled")) {
        settings.mqtt_enabled = (webServer.arg("mqtt_enabled") == "1");
    }

    if (webServer.hasArg("mqtt_server")) {
        String server = webServer.arg("mqtt_server");
        server.trim();
        strncpy(settings.mqtt_server, server.c_str(), sizeof(settings.mqtt_server) - 1);
        settings.mqtt_server[sizeof(settings.mqtt_server) - 1] = '\0';
    }

    if (webServer.hasArg("mqtt_port")) {
        settings.mqtt_port = webServer.arg("mqtt_port").toInt();
    }

    if (webServer.hasArg("mqtt_thing_name")) {
        String thing_name = webServer.arg("mqtt_thing_name");
        thing_name.trim();
        strncpy(settings.mqtt_thing_name, thing_name.c_str(), sizeof(settings.mqtt_thing_name) - 1);
        settings.mqtt_thing_name[sizeof(settings.mqtt_thing_name) - 1] = '\0';
    }

    if (webServer.hasArg("mqtt_subscribe_topic")) {
        String topic = webServer.arg("mqtt_subscribe_topic");
        topic.trim();
        strncpy(settings.mqtt_subscribe_topic, topic.c_str(), sizeof(settings.mqtt_subscribe_topic) - 1);
        settings.mqtt_subscribe_topic[sizeof(settings.mqtt_subscribe_topic) - 1] = '\0';
    }

    if (webServer.hasArg("mqtt_publish_topic")) {
        String topic = webServer.arg("mqtt_publish_topic");
        topic.trim();
        strncpy(settings.mqtt_publish_topic, topic.c_str(), sizeof(settings.mqtt_publish_topic) - 1);
        settings.mqtt_publish_topic[sizeof(settings.mqtt_publish_topic) - 1] = '\0';
    }

    if (webServer.hasArg("mqtt_boot_delay")) {
        long delay_seconds = webServer.arg("mqtt_boot_delay").toInt();
        if (delay_seconds < 0) delay_seconds = 0;
        if (delay_seconds > 600) delay_seconds = 600;
        settings.mqtt_boot_delay_ms = (uint32_t)delay_seconds * 1000UL;
    }

    if (webServer.hasArg("mqtt_notify_failures")) {
        settings.mqtt_notify_failures = (webServer.arg("mqtt_notify_failures") == "1");
    }

    if (webServer.hasArg("mqtt_retry_interval")) {
        long retry_mins = webServer.arg("mqtt_retry_interval").toInt();
        if (retry_mins < 5) retry_mins = 5;
        if (retry_mins > 1440) retry_mins = 1440;
        settings.mqtt_retry_interval_mins = (uint32_t)retry_mins;
    }

    if (webServer.hasArg("rsyslog_enabled")) {
        settings.rsyslog_enabled = (webServer.arg("rsyslog_enabled") == "1");
    }

    if (webServer.hasArg("rsyslog_server")) {
        String server = webServer.arg("rsyslog_server");
        server.trim();
        strncpy(settings.rsyslog_server, server.c_str(), sizeof(settings.rsyslog_server) - 1);
        settings.rsyslog_server[sizeof(settings.rsyslog_server) - 1] = '\0';
    }

    if (webServer.hasArg("rsyslog_port")) {
        uint16_t port = webServer.arg("rsyslog_port").toInt();
        if (port > 0 && port <= 65535) {
            settings.rsyslog_port = port;
        }
    }

    if (webServer.hasArg("rsyslog_use_tcp")) {
        settings.rsyslog_use_tcp = (webServer.arg("rsyslog_use_tcp") == "1");
    }

    if (webServer.hasArg("rsyslog_min_severity")) {
        uint8_t severity = webServer.arg("rsyslog_min_severity").toInt();
        if (severity <= 7) {
            settings.rsyslog_min_severity = severity;
        }
    }


    if (webServer.hasArg("tx_power")) {
        float power = webServer.arg("tx_power").toFloat();
        if (power >= 0.0 && power <= 20.0) {
            settings.default_txpower = power;
            tx_power = settings.default_txpower;
            radio.setOutputPower(tx_power);
        }
    }

    if (webServer.hasArg("banner_message")) {
        String banner = webServer.arg("banner_message");
        banner.trim();
        if (banner.length() == 0) banner = DEFAULT_BANNER;
        strncpy(settings.banner_message, banner.c_str(), sizeof(settings.banner_message) - 1);
        settings.banner_message[sizeof(settings.banner_message) - 1] = '\0';
    }

    if (webServer.hasArg("theme")) {
        settings.theme = webServer.arg("theme").toInt();
    }

    if (webServer.hasArg("low_battery_alert")) {
        settings.enable_low_battery_alert = (webServer.arg("low_battery_alert") == "on");
    } else {
        settings.enable_low_battery_alert = false;
    }

    if (webServer.hasArg("power_disconnect_alert")) {
        settings.enable_power_disconnect_alert = (webServer.arg("power_disconnect_alert") == "on");
    } else {
        settings.enable_power_disconnect_alert = false;
    }

    if (webServer.hasArg("timezone_offset")) {
        float timezone_offset = webServer.arg("timezone_offset").toFloat();
        if (timezone_offset >= -12.0 && timezone_offset <= 14.0) {
            settings.timezone_offset_hours = timezone_offset;
        }
    }

    if (webServer.hasArg("ntp_server")) {
        String server = webServer.arg("ntp_server");
        server.trim();
        if (server.length() > 0) {
            strncpy(settings.ntp_server, server.c_str(), sizeof(settings.ntp_server) - 1);
            settings.ntp_server[sizeof(settings.ntp_server) - 1] = '\0';
        }
    }

    // WiFi changes no longer require restart (handled by stored_networks array)

    if (save_runtime_settings()) {
        display_status();

        if (need_restart) {
            webServer.send(200, "application/json", "{\"success\":true,\"restart\":true}");
            delay(5000);
            ESP.restart();
        } else {
            webServer.send(200, "application/json", "{\"success\":true,\"restart\":false}");
        }
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save configuration\"}");
    }
}

// =============================================================================
// FLEX DEFAULTS PAGE
// =============================================================================

static String getReservedPinsJson() {
    String json = "[0,";
    json += String(LORA_CS_PIN) + ",";
    json += String(LORA_IRQ_PIN) + ",";
    json += String(LORA_RST_PIN) + ",";
    json += String(LORA_GPIO_PIN) + ",";
    json += String(LORA_SCK_PIN) + ",";
    json += String(LORA_MOSI_PIN) + ",";
    json += String(LORA_MISO_PIN) + ",";
    json += String(OLED_SDA_PIN) + ",";
    json += String(OLED_SCL_PIN) + ",";
    if (OLED_RST_PIN != -1) json += String(OLED_RST_PIN) + ",";
    json += String(LED_PIN) + ",";
    json += String(BATTERY_ADC_PIN);
    if (VEXT_PIN != -1) json += "," + String(VEXT_PIN);
    json += "]";
    return json;
}

void handle_flex_config() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("FLEX Configuration");

    chunk += "<div class='header'>"
            "<h1>📻 FLEX Configuration</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-active'>📻 FLEX</a>"
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

    chunk += "<div class='form-section'>"
            "<h3>📻 Default FLEX Settings</h3>"

            "<form action='/save_flex' method='post' onsubmit='return submitFormAjax(this, \"FLEX settings saved successfully!\", \"FLEX settings saved, restarting in 5 seconds...\")'>"

            "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px; margin: 20px 0;'>"

            "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>📡 Frequency</h4>"
            "<label for='default_frequency' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Default Frequency (MHz):</label>"
            "<input type='number' id='default_frequency' name='default_frequency' step='0.0001' value='" + String(settings.default_frequency, 4) + "' min='400' max='1000' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<small style='color: var(--theme-secondary); display: block; margin-top: 5px;'>Range: 400.0000 - 1000.0000 MHz</small>"
            "</div>"

            "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>⚡ TX Power</h4>"
            "<label for='tx_power' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Default TX Power (dBm):</label>"
            "<input type='number' id='tx_power' name='tx_power' value='" + String((int)settings.default_txpower) + "' min='2' max='20' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<small style='color: var(--theme-secondary); display: block; margin-top: 5px;'>Range: 2 - 20 dBm (hardware minimum)</small>"
            "</div>"

            "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🎯 Capcode</h4>"
            "<label for='default_capcode' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Default Capcode:</label>"
            "<input type='number' id='default_capcode' name='default_capcode' value='" + String(settings.default_capcode) + "' min='1' max='4291000000' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<small style='color: var(--theme-secondary); display: block; margin-top: 5px;'>Valid ranges:<br>1-1933312, 1998849-2031614, 2101249-4291000000</small>"
            "</div>"

            "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🔧 PPM Correction</h4>"
            "<label for='frequency_correction_ppm' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>PPM Correction:</label>"
            "<input type='number' id='frequency_correction_ppm' name='frequency_correction_ppm' step='0.01' value='" + String(settings.frequency_correction_ppm, 2) + "' min='-50' max='50' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<small style='color: var(--theme-secondary); display: block; margin-top: 5px;'>Range: -50.0 to +50.0 ppm</small>"
            "</div>"

            "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card); grid-column: 1 / -1;'>"
            "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;'>"
            "<h4 style='margin: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>📡 External RF Amplifier</h4>"
            "<div id='toggle_rf_enable' class='toggle-switch " + String(settings.enable_rf_amplifier ? "is-active" : "is-inactive") + "' onclick='toggleRFAmplifier()'>"
            "<div class='toggle-slider " + String(settings.enable_rf_amplifier ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "</div>"
            "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 20px; margin-bottom: 16px;'>"
            "<div>"
            "<label for='rf_amplifier_power_pin' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Power Pin (GPIO):</label>"
            "<input type='number' id='rf_amplifier_power_pin' name='rf_amplifier_power_pin' value='" + String((settings.rf_amplifier_power_pin == 0) ? RFAMP_PWR_PIN : settings.rf_amplifier_power_pin) + "' min='0' max='39' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div>"
            "<label for='rf_amplifier_delay_ms' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Stabilization Delay (ms):</label>"
            "<input type='number' id='rf_amplifier_delay_ms' name='rf_amplifier_delay_ms' value='" + String(settings.rf_amplifier_delay_ms) + "' min='20' max='5000' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            "<div id='rf_amp_polarity_section' style='margin-top: 4px; margin-bottom: 16px;'>"
            "<label style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Amplifier Control Logic:</label>"
            "<div style='display: grid; grid-template-columns: auto 1fr; gap: 24px; align-items: center;'>"
            "<div style='display: flex; align-items: center; gap: 12px;'>"
            "<span style='font-size: 14px; color: var(--theme-text); min-width: 85px; text-align: right;'>Active-Low</span>"
            "<div id='toggle_rf_polarity' class='toggle-switch " + String(settings.rf_amplifier_active_high ? "is-active" : "is-inactive") + "' onclick='toggleRFAmpPolarity()'>"
            "<div class='toggle-slider " + String(settings.rf_amplifier_active_high ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "<span style='font-size: 14px; color: var(--theme-text); min-width: 85px;'>Active-High</span>"
            "</div>"
            "<div style='font-size: 12px; padding: 10px; background: var(--theme-card); border-radius: 4px; border-left: 3px solid #28a745;'>"
            "<div id='rf_amp_low_desc' style='margin-bottom: 6px; font-weight: " + String(!settings.rf_amplifier_active_high ? "600" : "normal") + "; color: " + String(!settings.rf_amplifier_active_high ? "#28a745" : "var(--theme-text)") + ";'>"
            "Active-Low: Amplifier ON with GPIO LOW<br>"
            "<span style='font-size: 11px; font-style: italic;'>(direct P-MOSFET)</span>"
            "</div>"
            "<div id='rf_amp_high_desc' style='font-weight: " + String(settings.rf_amplifier_active_high ? "600" : "normal") + "; color: " + String(settings.rf_amplifier_active_high ? "#28a745" : "var(--theme-text)") + ";'>"
            "Active-High: Amplifier ON with GPIO HIGH<br>"
            "<span style='font-size: 11px; font-style: italic;'>(2N2222 driver)</span>"
            "</div>"
            "</div>"
            "</div>"
            "</div>"
            "<input type='hidden' id='enable_rf_amplifier' name='enable_rf_amplifier' value='" + String(settings.enable_rf_amplifier ? "1" : "0") + "'>"
            "<input type='hidden' id='rf_amplifier_active_high' name='rf_amplifier_active_high' value='" + String(settings.rf_amplifier_active_high ? "1" : "0") + "'>"
            "</div>"

            "</div>"

            "<div style='margin-top:30px;text-align:center;'>"
            "<button type='submit' class='button button-large success'>💾 Save FLEX Configuration</button>"
            "</div>"
            "</form>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<script>"
            "var RESERVED_PINS = " + getReservedPinsJson() + ";"
            "function validateFlexPower() {"
            "  var p = document.getElementById('tx_power');"
            "  if (!p) return;"
            "  var val = parseInt(p.value);"
            "  if (val < 0 || val > 20) {"
            "    p.style.borderColor = '#e74c3c';"
            "    p.style.backgroundColor = '#fdf2f2';"
            "  } else {"
            "    p.style.borderColor = 'var(--theme-border)';"
            "    p.style.backgroundColor = 'var(--theme-input)';"
            "  }"
            "}"
            "function updateRFAmpFieldsState(enabled) {"
            "  var powerPinInput = document.getElementById('rf_amplifier_power_pin');"
            "  var delayInput = document.getElementById('rf_amplifier_delay_ms');"
            "  var polarityToggle = document.getElementById('toggle_rf_polarity');"
            "  var polaritySection = document.getElementById('rf_amp_polarity_section');"
            "  if (enabled) {"
            "    powerPinInput.disabled = false;"
            "    powerPinInput.style.opacity = '1';"
            "    powerPinInput.style.cursor = 'text';"
            "    delayInput.disabled = false;"
            "    delayInput.style.opacity = '1';"
            "    delayInput.style.cursor = 'text';"
            "    polarityToggle.style.pointerEvents = 'auto';"
            "    polaritySection.style.opacity = '1';"
            "  } else {"
            "    powerPinInput.disabled = true;"
            "    powerPinInput.style.opacity = '0.5';"
            "    powerPinInput.style.cursor = 'not-allowed';"
            "    delayInput.disabled = true;"
            "    delayInput.style.opacity = '0.5';"
            "    delayInput.style.cursor = 'not-allowed';"
            "    polarityToggle.style.pointerEvents = 'none';"
            "    polaritySection.style.opacity = '0.5';"
            "  }"
            "}"
            "function toggleRFAmplifier() {"
            "  var toggle = document.getElementById('toggle_rf_enable');"
            "  var slider = toggle.querySelector('.toggle-slider');"
            "  var hiddenInput = document.getElementById('enable_rf_amplifier');"
            "  var currentEnabled = hiddenInput.value === '1';"
            "  var newEnabled = !currentEnabled;"
            "  if (newEnabled) {"
            "    toggle.style.backgroundColor = '#28a745';"
            "    slider.style.left = '26px';"
            "    hiddenInput.value = '1';"
            "  } else {"
            "    toggle.style.backgroundColor = '#ccc';"
            "    slider.style.left = '2px';"
            "    hiddenInput.value = '0';"
            "  }"
            "  updateRFAmpFieldsState(newEnabled);"
            "}"
            "function toggleRFAmpPolarity() {"
            "  var toggle = document.getElementById('toggle_rf_polarity');"
            "  var slider = toggle.querySelector('.toggle-slider');"
            "  var hiddenInput = document.getElementById('rf_amplifier_active_high');"
            "  var lowDesc = document.getElementById('rf_amp_low_desc');"
            "  var highDesc = document.getElementById('rf_amp_high_desc');"
            "  var normalColor = getComputedStyle(document.body).getPropertyValue('--theme-text') || '#000';"
            "  var currentActive = hiddenInput.value === '1';"
            "  var newActive = !currentActive;"
            "  if (newActive) {"
            "    toggle.style.backgroundColor = '#28a745';"
            "    slider.style.left = '26px';"
            "    hiddenInput.value = '1';"
            "    lowDesc.style.fontWeight = 'normal';"
            "    lowDesc.style.color = normalColor;"
            "    highDesc.style.fontWeight = '600';"
            "    highDesc.style.color = '#28a745';"
            "  } else {"
            "    toggle.style.backgroundColor = '#ccc';"
            "    slider.style.left = '2px';"
            "    hiddenInput.value = '0';"
            "    lowDesc.style.fontWeight = '600';"
            "    lowDesc.style.color = '#28a745';"
            "    highDesc.style.fontWeight = 'normal';"
            "    highDesc.style.color = normalColor;"
            "  }"
            "}"
            "function validateRFAmpPin() {"
            "  var pinInput = document.getElementById('rf_amplifier_power_pin');"
            "  if (!pinInput) return true;"
            "  var pin = parseInt(pinInput.value);"
            "  var errorMsg = document.getElementById('rf_amp_pin_error');"
            "  if (RESERVED_PINS.includes(pin)) {"
            "    pinInput.style.borderColor = '#e74c3c';"
            "    pinInput.style.backgroundColor = '#fdf2f2';"
            "    if (!errorMsg) {"
            "      var msg = document.createElement('small');"
            "      msg.id = 'rf_amp_pin_error';"
            "      msg.style.color = '#e74c3c';"
            "      msg.style.display = 'block';"
            "      msg.style.marginTop = '5px';"
            "      msg.textContent = '\u26A0\uFE0F GPIO ' + pin + ' is reserved (in use by LoRa/OLED/Battery)';"
            "      pinInput.parentElement.appendChild(msg);"
            "    } else {"
            "      errorMsg.textContent = '\u26A0\uFE0F GPIO ' + pin + ' is reserved (in use by LoRa/OLED/Battery)';"
            "    }"
            "    return false;"
            "  } else {"
            "    pinInput.style.borderColor = 'var(--theme-border)';"
            "    pinInput.style.backgroundColor = 'var(--theme-input)';"
            "    if (errorMsg) {"
            "      errorMsg.remove();"
            "    }"
            "    return true;"
            "  }"
            "}"
            "window.onload = function() {"
            "  validateFlexPower();"
            "  var rfAmpEnabled = document.getElementById('enable_rf_amplifier').value === '1';"
            "  updateRFAmpFieldsState(rfAmpEnabled);"
            "  validateRFAmpPin();"
            "};"
            "var txPowerEl = document.getElementById('tx_power');"
            "if (txPowerEl) {"
            "  txPowerEl.addEventListener('input', validateFlexPower);"
            "  txPowerEl.addEventListener('keyup', validateFlexPower);"
            "}"
            "var rfPinEl = document.getElementById('rf_amplifier_power_pin');"
            "if (rfPinEl) {"
            "  rfPinEl.addEventListener('input', validateRFAmpPin);"
            "  rfPinEl.addEventListener('change', validateRFAmpPin);"
            "}"
            "</script>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_save_flex() {
    reset_oled_timeout();

    bool need_restart = false;
    CoreConfig old_core_config = core_config;
    DeviceSettings old_settings = settings;

    if (webServer.hasArg("default_frequency")) {
        float frequency = webServer.arg("default_frequency").toFloat();
        if (frequency >= 400.0 && frequency <= 1000.0) {
            settings.default_frequency = frequency;
        }
    }

    if (webServer.hasArg("tx_power")) {
        int power = webServer.arg("tx_power").toInt();
        if (power >= 2 && power <= 20) {
            settings.default_txpower = (int8_t)power;
        }
    }

    if (webServer.hasArg("default_capcode")) {
        settings.default_capcode = strtoull(webServer.arg("default_capcode").c_str(), NULL, 10);
    }

    if (webServer.hasArg("frequency_correction_ppm")) {
        float ppm = webServer.arg("frequency_correction_ppm").toFloat();
        if (ppm >= -50.0 && ppm <= 50.0) {
            settings.frequency_correction_ppm = ppm;
        }
    }

    if (webServer.hasArg("enable_rf_amplifier")) {
        settings.enable_rf_amplifier = (webServer.arg("enable_rf_amplifier") == "1");
    } else {
        settings.enable_rf_amplifier = false;
    }

    if (webServer.hasArg("rf_amplifier_power_pin")) {
        int rfamp_pwr = webServer.arg("rf_amplifier_power_pin").toInt();

        bool is_reserved = (rfamp_pwr == 0 || rfamp_pwr == LORA_CS_PIN || rfamp_pwr == LORA_IRQ_PIN ||
                           rfamp_pwr == LORA_RST_PIN || rfamp_pwr == LORA_GPIO_PIN ||
                           rfamp_pwr == LORA_SCK_PIN || rfamp_pwr == LORA_MOSI_PIN ||
                           rfamp_pwr == LORA_MISO_PIN || rfamp_pwr == OLED_SDA_PIN ||
                           rfamp_pwr == OLED_SCL_PIN || rfamp_pwr == LED_PIN ||
                           rfamp_pwr == BATTERY_ADC_PIN ||
                           (OLED_RST_PIN != -1 && rfamp_pwr == OLED_RST_PIN) ||
                           (VEXT_PIN != -1 && rfamp_pwr == VEXT_PIN));

        if (!is_reserved) {
            settings.rf_amplifier_power_pin = rfamp_pwr;
        }
    }

    if (webServer.hasArg("rf_amplifier_delay_ms")) {
        uint16_t delay_ms = webServer.arg("rf_amplifier_delay_ms").toInt();
        if (delay_ms >= 20 && delay_ms <= 5000) {
            settings.rf_amplifier_delay_ms = delay_ms;
        }
    }

    if (webServer.hasArg("rf_amplifier_active_high")) {
        settings.rf_amplifier_active_high = (webServer.arg("rf_amplifier_active_high") == "1");
    } else {
        settings.rf_amplifier_active_high = true;
    }

    need_restart = false;

    if (save_runtime_settings()) {
        current_tx_frequency = settings.default_frequency;
        tx_power = settings.default_txpower;

        display_status();

        if (need_restart) {
            webServer.send(200, "application/json", "{\"success\":true,\"message\":\"FLEX settings saved successfully. Device will restart in 3 seconds to apply changes.\",\"restart\":true}");
            delay(3000);
            ESP.restart();
        } else {
            webServer.send(200, "application/json", "{\"success\":true,\"message\":\"FLEX settings saved successfully. Device will restart in 3 seconds to apply changes.\",\"restart\":true}");
            delay(3000);
            ESP.restart();
        }
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save FLEX settings\"}");
    }
}

// =============================================================================
// MQTT CONFIGURATION PAGE
// =============================================================================

void handle_mqtt() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html", "");

    String chunk = get_html_header("MQTT Configuration");

    chunk += "<div class='header'>"
            "<h1>📡 MQTT Configuration</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            + chatgpt_nav_tab_html(false) +
            "<a href='/mqtt' class='tab-active" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            + imap_nav_tab_html(false)
            + gsm_nav_tab_html(false) +
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    if (mqtt_suspended) {
        chunk += "<div style='background-color:var(--theme-card);color:#dc3545;padding:15px;margin:20px;border-radius:8px;border:2px solid #dc3545;'>";
        chunk += "<h3 style='margin-top:0;color:#dc3545;'>⚠️ MQTT SUSPENDED</h3>";
        chunk += "<p>MQTT service has been suspended due to " + String(MAX_CONNECTION_FAILURES) + " consecutive failures.</p>";
        chunk += "<p>MQTT will automatically resume after device reboot or when the underlying issue is resolved.</p>";
        chunk += "</div>";
    }

    chunk += "<div id='temp-message'></div>";

    chunk += "<div class='form-section'>"
            "<div class='flex-space-between mb-20'>"
            "<div class='flex-center'>"
            "<span style='text-large'>Enable MQTT</span>"
            "<div class='toggle-switch " + String(settings.mqtt_enabled ? "is-active" : "is-inactive") + "' onclick='toggleMQTT()'>"
            "<div class='toggle-slider " + String(settings.mqtt_enabled ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "</div>"
            "</div>"
            "<div class='flex-space-between mb-20'>"
            "<div class='flex-center'>"
            "<span style='text-large'>Failure notifications</span>"
            "<div class='toggle-switch " + String(settings.mqtt_notify_failures ? "is-active" : "is-inactive") + "' onclick='toggleMQTTNotifications()'>"
            "<div class='toggle-slider " + String(settings.mqtt_notify_failures ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<form action='/save_mqtt' method='post' onsubmit='return submitFormAjax(this, \"MQTT settings saved successfully!\", \"MQTT settings saved, restarting in 5 seconds...\")'>"

            "<input type='hidden' id='mqtt_enabled' name='mqtt_enabled' value='" + String(settings.mqtt_enabled ? "1" : "0") + "'>"
            "<input type='hidden' id='mqtt_notify_failures' name='mqtt_notify_failures' value='" + String(settings.mqtt_notify_failures ? "1" : "0") + "'>"

            "<div class='form-section' style='margin: 20px 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🌐 AWS IoT Core Configuration</h4>"
            "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 20px;'>"
            "<div>"
            "<label for='mqtt_thing_name' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Thing Name:</label>"
            "<input type='text' id='mqtt_thing_name' name='mqtt_thing_name' value='" + String(settings.mqtt_thing_name) + "' maxlength='31' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div>"
            "<label for='mqtt_port' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Port:</label>"
            "<input type='number' id='mqtt_port' name='mqtt_port' value='" + String(settings.mqtt_port) + "' min='1' max='65535' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 20px;'>"
            "<div>"
            "<label for='mqtt_boot_delay' style='display:block;margin-bottom:8px;font-weight:500;color:var(--theme-text);'>Boot Delay (seconds):</label>"
            "<input type='number' id='mqtt_boot_delay' name='mqtt_boot_delay' value='" + String(settings.mqtt_boot_delay_ms / 1000UL) + "' min='0' max='600' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div>"
            "<label for='mqtt_retry_interval' style='display:block;margin-bottom:8px;font-weight:500;color:var(--theme-text);'>Retry Time (Minutes):</label>"
            "<input type='number' id='mqtt_retry_interval' name='mqtt_retry_interval' value='" + String(settings.mqtt_retry_interval_mins) + "' min='5' max='1440' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            "<div style='margin-bottom: 20px;'>"
            "<label for='mqtt_server' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>MQTT Server (AWS IoT Endpoint):</label>"
            "<input type='text' id='mqtt_server' name='mqtt_server' value='" + String(settings.mqtt_server) + "' maxlength='127' placeholder='your-endpoint-ats.iot.region.amazonaws.com' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: 20px;'>"
            "<div>"
            "<label for='mqtt_subscribe_topic' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Subscribe Topic:</label>"
            "<input type='text' id='mqtt_subscribe_topic' name='mqtt_subscribe_topic' value='" + String(settings.mqtt_subscribe_topic) + "' maxlength='63' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div>"
            "<label for='mqtt_publish_topic' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Publish Topic:</label>"
            "<input type='text' id='mqtt_publish_topic' name='mqtt_publish_topic' value='" + String(settings.mqtt_publish_topic) + "' maxlength='63' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div class='form-section' style='margin: 20px 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🔐 AWS IoT Certificates</h4>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Upload your certificate files from AWS IoT Core. Status indicators show current certificate validity.</p>"

            "<div style='display:flex;gap:20px;justify-content:center;margin:20px 0;'>"

            "<div style='text-align:center;flex:1;'>"
            "<div style='font-weight:bold;margin-bottom:8px;color:var(--theme-text);'>Root CA</div>"
            "<div id='root_ca_status' style='margin-bottom:10px;font-size:14px;'>"
            + getCertificateStatusFromSPIFFS(MQTT_CA_CERT_FILE) +
            "</div>"
            "<input type='file' id='root_ca_file' name='root_ca_file' accept='.pem,.crt,.cer' onchange='uploadCertificateAuto(\"root_ca\")' style='margin:0 auto 5px auto;display:block;font-size:12px;'>"
            "</div>"

            "<div style='text-align:center;flex:1;'>"
            "<div style='font-weight:bold;margin-bottom:8px;color:var(--theme-text);'>Device Certificate</div>"
            "<div id='device_cert_status' style='margin-bottom:10px;font-size:14px;'>"
            + getCertificateStatusFromSPIFFS(MQTT_DEVICE_CERT_FILE) +
            "</div>"
            "<input type='file' id='device_cert_file' name='device_cert_file' accept='.pem,.crt,.cer' onchange='uploadCertificateAuto(\"device_cert\")' style='margin:0 auto 5px auto;display:block;font-size:12px;'>"
            "</div>"

            "<div style='text-align:center;flex:1;'>"
            "<div style='font-weight:bold;margin-bottom:8px;color:var(--theme-text);'>Private Key</div>"
            "<div id='device_key_status' style='margin-bottom:10px;font-size:14px;'>"
            + getCertificateStatusFromSPIFFS(MQTT_DEVICE_KEY_FILE) +
            "</div>"
            "<input type='file' id='device_key_file' name='device_key_file' accept='.pem,.key' onchange='uploadCertificateAuto(\"device_key\")' style='margin:0 auto 5px auto;display:block;font-size:12px;'>"
            "</div>"

            "</div>"
            "<div id='cert-upload-status' style='text-align:center;margin:10px 0;font-size:12px;'></div>"
            "</div>"

            "<div style='margin-top:30px;text-align:center;'>"
            "<button type='submit' class='button button-large success'>💾 Save MQTT Configuration</button>"
            "</div>"
            "</form>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<script>"
            "function toggleMQTT() {"
            "  const toggleSwitch = document.querySelector('.toggle-switch');"
            "  const toggleSlider = document.querySelector('.toggle-slider');"
            "  const hiddenInput = document.getElementById('mqtt_enabled');"
            "  "
            "  const currentEnabled = hiddenInput.value === '1';"
            "  const newEnabled = !currentEnabled;"
            "  "
            "  if (newEnabled) {"
            "    toggleSwitch.style.backgroundColor = '#28a745';"
            "    toggleSlider.style.left = '26px';"
            "    hiddenInput.value = '1';"
            "  } else {"
            "    toggleSwitch.style.backgroundColor = '#ccc';"
            "    toggleSlider.style.left = '2px';"
            "    hiddenInput.value = '0';"
            "  }"
            "}"
            "function toggleMQTTNotifications() {"
            "  const toggleSwitches = document.querySelectorAll('.toggle-switch');"
            "  const toggleSliders = document.querySelectorAll('.toggle-slider');"
            "  const hiddenInput = document.getElementById('mqtt_notify_failures');"
            "  const toggleSwitch = toggleSwitches[1];"
            "  const toggleSlider = toggleSliders[1];"
            "  "
            "  const currentEnabled = hiddenInput.value === '1';"
            "  const newEnabled = !currentEnabled;"
            "  "
            "  if (newEnabled) {"
            "    toggleSwitch.style.backgroundColor = '#28a745';"
            "    toggleSlider.style.left = '26px';"
            "    hiddenInput.value = '1';"
            "  } else {"
            "    toggleSwitch.style.backgroundColor = '#ccc';"
            "    toggleSlider.style.left = '2px';"
            "    hiddenInput.value = '0';"
            "  }"
            "}"
            "function uploadCertificateAuto(certType) {"
            "  console.log('uploadCertificateAuto called with:', certType);"
            "  const fileInput = document.getElementById(certType + '_file');"
            "  const file = fileInput.files[0];"
            "  const statusDiv = document.getElementById('cert-upload-status');"
            "  console.log('File selected:', file ? file.name : 'none');"
            "  "
            "  if (!file) {"
            "    statusDiv.innerHTML = '<span style=\"color:red;\">❌ No file selected</span>';"
            "    return;"
            "  }"
            "  "
            "  statusDiv.innerHTML = '<span style=\"color:blue;\">🔄 Processing ' + file.name + '...</span>';"
            "  "
            "  if (file.size > 4096) {"
            "    statusDiv.innerHTML = '<span style=\"color:red;\">❌ Certificate file too large (max 4KB)</span>';"
            "    return;"
            "  }"
            "  "
            "  const formData = new FormData();"
            "  formData.append('certificate', file);"
            "  formData.append('cert_type', certType);"
            "  "
            "  statusDiv.innerHTML = '<span style=\"color:blue;\">⏳ Uploading ' + certType.replace('_', ' ') + '...</span>';"
            "  "
            "  fetch('/upload_certificate', {"
            "    method: 'POST',"
            "    body: formData"
            "  })"
            "  .then(response => response.json())"
            "  .then(data => {"
            "    if (data.success) {"
            "      statusDiv.innerHTML = '<span style=\"color:green;\">✅ ' + certType.replace('_', ' ') + ' saved successfully</span>';"
            "      const certStatusDiv = document.getElementById(certType + '_status');"
            "      if (certStatusDiv) {"
            "        if (certType === 'root_ca') certStatusDiv.innerHTML = 'Root CA ✅';"
            "        else if (certType === 'device_cert') certStatusDiv.innerHTML = 'Device Cert ✅';"
            "        else if (certType === 'device_key') certStatusDiv.innerHTML = 'Private Key ✅';"
            "      }"
            "    } else {"
            "      statusDiv.innerHTML = '<span style=\"color:red;\">❌ ' + data.message + '</span>';"
            "    }"
            "  })"
            "  .catch(error => {"
            "    statusDiv.innerHTML = '<span style=\"color:red;\">❌ Upload failed: ' + error.message + '</span>';"
            "  });"
            "}"
            "</script>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk = "<div style='margin-top: 30px; padding: 24px; background-color: var(--theme-card); border-radius: 12px; border: 1px solid var(--theme-border); box-shadow: 0 2px 8px rgba(0,0,0,0.1);'>"
            "<h3 style='margin: 0 0 20px 0; font-size: 1.3em; color: var(--theme-text);'>📊 Recent Activity</h3>";

    if (mqtt_activity_count == 0) {
        chunk += "<div style='text-align: center; padding: 20px; color: var(--theme-nav-inactive);'>"
                 "<p>No MQTT activity yet. Events will appear here after MQTT processes messages.</p>"
                 "</div>";
    } else {
        for (int i = mqtt_activity_count - 1; i >= 0; i--) {
            MQTTActivity& activity = mqtt_activity_log[i];
            String statusIcon = activity.success ? "✅" : "❌";

            bool hasTransmission = (String(activity.event) == "Message Received" || String(activity.event) == "Suspended");

            String metadata = "";
            if (hasTransmission) {
                metadata = statusIcon;
            }
            if (activity.capcode > 0) {
                if (metadata.length() > 0) metadata += " | ";
                metadata += "📟 " + String(activity.capcode);
            }
            if (activity.frequency > 0.0) {
                if (metadata.length() > 0) metadata += " | ";
                metadata += "📡 " + String(activity.frequency, 4) + " MHz";
            }
            if (metadata.length() > 0) metadata += " | ";
            metadata += String(activity.datetime);

            chunk += "<div style='border: 1px solid var(--theme-border); border-radius: 8px; padding: 12px; margin-bottom: 10px; background-color: var(--theme-input);'>"
                     "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;'>"
                     "<div style='font-weight: bold; color: var(--theme-text);'>" + statusIcon + " " + String(activity.event) + "</div>"
                     "<div style='font-size: 0.9em; color: var(--theme-nav-inactive);'>" + metadata + "</div>"
                     "</div>"
                     "<div style='color: var(--theme-text);'>" + String(activity.details) + "</div>"
                     "</div>";
        }
    }

    chunk += "</div>";
    webServer.sendContent(chunk);

    chunk = get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_save_mqtt() {
    reset_oled_timeout();


    if (webServer.hasArg("mqtt_enabled")) {
        settings.mqtt_enabled = (webServer.arg("mqtt_enabled") == "1");
    }

    if (webServer.hasArg("mqtt_server")) {
        String server = webServer.arg("mqtt_server");
        server.trim();
        strncpy(settings.mqtt_server, server.c_str(), sizeof(settings.mqtt_server) - 1);
        settings.mqtt_server[sizeof(settings.mqtt_server) - 1] = '\0';
    }

    if (webServer.hasArg("mqtt_port")) {
        settings.mqtt_port = webServer.arg("mqtt_port").toInt();
    }

    if (webServer.hasArg("mqtt_thing_name")) {
        String thing_name = webServer.arg("mqtt_thing_name");
        thing_name.trim();
        strncpy(settings.mqtt_thing_name, thing_name.c_str(), sizeof(settings.mqtt_thing_name) - 1);
        settings.mqtt_thing_name[sizeof(settings.mqtt_thing_name) - 1] = '\0';
    }

    if (webServer.hasArg("mqtt_subscribe_topic")) {
        String topic = webServer.arg("mqtt_subscribe_topic");
        topic.trim();
        strncpy(settings.mqtt_subscribe_topic, topic.c_str(), sizeof(settings.mqtt_subscribe_topic) - 1);
        settings.mqtt_subscribe_topic[sizeof(settings.mqtt_subscribe_topic) - 1] = '\0';
    }

    if (webServer.hasArg("mqtt_publish_topic")) {
        String topic = webServer.arg("mqtt_publish_topic");
        topic.trim();
        strncpy(settings.mqtt_publish_topic, topic.c_str(), sizeof(settings.mqtt_publish_topic) - 1);
        settings.mqtt_publish_topic[sizeof(settings.mqtt_publish_topic) - 1] = '\0';
    }

    if (webServer.hasArg("ntp_server")) {
        String server = webServer.arg("ntp_server");
        server.trim();
        if (server.length() > 0) {
            strncpy(settings.ntp_server, server.c_str(), sizeof(settings.ntp_server) - 1);
            settings.ntp_server[sizeof(settings.ntp_server) - 1] = '\0';
        }
    }

    mqtt_suspended = false;
    mqtt_failed_cycles = 0;
    logMessage("MQTT: Suspension flags reset due to configuration change");

    if (save_runtime_settings()) {
        display_status();

        webServer.send(200, "application/json", "{\"success\":true,\"restart\":true,\"message\":\"MQTT configuration and certificates saved successfully. Device will restart in 5 seconds.\"}");
        delay(5000);
        ESP.restart();
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save MQTT configuration\"}");
    }
}

// =============================================================================
// IMAP CONFIGURATION PAGE
// =============================================================================

#ifdef ENABLE_IMAP
void handle_imap_config() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html", "");

    String html = get_html_header("IMAP Accounts");

    html += "<div class='header'>"
            "<h1>📧 IMAP Accounts</h1>"
            "</div>";

    html += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            + chatgpt_nav_tab_html(false) +
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            "<a href='/imap' class='tab-active" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
            + gsm_nav_tab_html(false) +
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(html);
    html = "";

    if (any_imap_accounts_suspended()) {
        html += "<div style='background-color:var(--theme-card);color:#dc3545;padding:15px;margin:20px;border-radius:8px;border:2px solid #dc3545;'>";
        html += "<h3 style='margin-top:0;color:#dc3545;'>⚠️ ACCOUNT SUSPENSION</h3>";
        html += "<p>Some IMAP accounts have been suspended due to " + String(MAX_CONNECTION_FAILURES) + " consecutive check cycle failures.</p>";
        html += "<p>Suspended accounts will automatically resume when their configuration is updated or credentials are fixed.</p>";
        html += "</div>";
    }

    html += "<div id='temp-message'></div>";

    html += "<div class='form-section'>"
            "<div class='flex-space-between mb-20'>"
            "<div class='flex-center'>"
            "<span style='text-large'>Enable IMAP</span>"
            "<div class='toggle-switch " + String(imap_config.enabled ? "is-active" : "is-inactive") + "' onclick='toggleIMAPEnabled()'>"
            "<div class='toggle-slider " + String(imap_config.enabled ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "</div>"
            "</div>"
            "</div>";

    html += "<div class='form-section'>"
            "<h3>📧 IMAP Accounts (" + String(imap_config.accounts.size()) + "/" + String(IMAP_MAX_ACCOUNTS) + ")</h3>";

    webServer.sendContent(html);
    html = "";

    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        IMAPAccount& account = imap_config.accounts[i];

        html += "<div style='background-color: var(--theme-card); border: 1px solid var(--theme-border); border-radius: 12px; padding: 20px; margin-bottom: 15px; position: relative;'>"
                "<div style='display: flex; justify-content: space-between; align-items: flex-start;'>"

                "<div style='flex: 1;'>"
                "<div style='display: flex; align-items: center; gap: 10px; margin-bottom: 10px;'>"
                "<h4 style='margin: 0; color: var(--theme-text); font-size: 1.1em;'>" + String(account.name) + "</h4>";


        html += "</div>"

                "<div style='display: grid; grid-template-columns: 1fr auto 1fr; gap: 12px; margin-bottom: 24px; font-size: 0.9em;'>"
                "<div style='display: flex; align-items: center; gap: 6px;'>"
                "<span>🔗</span><strong>Server:</strong> " + String(account.server) + ":" + String(account.port) +
                "</div>"
                "<div style='display: flex; align-items: center; gap: 6px;'>"
                "<span>⏰</span><strong>Interval:</strong> " +
                (account.check_interval_min >= 60 ?
                 String(account.check_interval_min / 60) + "h" + (account.check_interval_min % 60 > 0 ? String(account.check_interval_min % 60) + "m" : "") :
                 String(account.check_interval_min) + " min") +
                "</div>"
                "<div style='display: flex; align-items: center; gap: 6px;'>"
                "<span>" + String(account.mail_drop ? "📧" : "📟") + "</span><strong>Capcode:</strong> " + String(account.capcode) +
                "</div>"
                "<div style='display: flex; align-items: center; gap: 6px;'>"
                "<span>👤</span><strong>Username:</strong> " + String(account.username) +
                "</div>"
                "<div style='display: flex; align-items: center; gap: 6px;'>"
                "<span>🔒</span><strong>SSL:</strong> " + String(account.use_ssl ? "Enabled" : "Disabled") +
                "</div>"
                "<div style='display: flex; align-items: center; gap: 6px;'>"
                "<span>📡</span><strong>Freq:</strong> " + String(account.frequency, 4) + " MHz" +
                "</div>"
                "</div>"
                "</div>"

                "<div style='display: flex; flex-direction: column; gap: 8px; margin-left: 15px;'>"
                "<button onclick='toggleEditAccount(" + String(i) + ")' style='padding: 6px 12px; background-color: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 0.9em;'>✏️ Edit</button>"
                "<button onclick='deleteAccount(" + String(i) + ")' style='padding: 6px 12px; background-color: #dc3545; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 0.9em;'>🗑️ Delete</button>"
                "</div>"
                "</div>"


                "<div id='edit-form-" + String(i) + "' style='display: none; margin-top: 20px; padding: 20px; border: 2px solid var(--theme-border); border-radius: 12px; background-color: var(--theme-input);'>"
                "<h4 style='margin: 0 0 18px 0; color: var(--theme-text); text-align: center; font-size: 1.1em;'>✏️ Edit IMAP Account</h4>"

                "<div style='margin-bottom: 16px;'>"
                "<h4 style='color: var(--theme-accent); margin: 0 0 10px 0; font-size: 0.95em;'>🔑 Authentication</h4>"
                "<div style='display: grid; grid-template-columns: 1fr 1fr 70px; gap: 15px; align-items: end;'>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Username/Email:</label><input type='text' id='edit_username_" + String(i) + "' maxlength='63' value='" + String(account.username) + "' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Password:</label><input type='password' id='edit_password_" + String(i) + "' maxlength='63' value='*****' placeholder='New password (empty = keep current)' onfocus='if(this.value===\"*****\") this.value=\"\"; this.placeholder=\"\";' onblur='if(!this.value) {this.value=\"*****\"; this.placeholder=\"New password (empty = keep current)\";}' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Check (min):</label><input type='number' id='edit_check_interval_" + String(i) + "' min='5' max='1440' value='" + String(account.check_interval_min) + "' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "</div>"
                "</div>"

                "<div style='margin-bottom: 16px;'>"
                "<h4 style='color: var(--theme-accent); margin: 0 0 10px 0; font-size: 0.95em;'>🔗 Connection Settings</h4>"
                "<div style='display: grid; grid-template-columns: 1fr 70px 100px; gap: 15px; align-items: end;'>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>IMAP Server:</label><input type='text' id='edit_server_" + String(i) + "' maxlength='63' list='imap_servers' value='" + String(account.server) + "' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Port:</label><input type='number' id='edit_port_" + String(i) + "' min='1' max='65535' value='" + String(account.port) + "' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>SSL/TLS:</label><select id='edit_use_ssl_" + String(i) + "' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'><option value='1'" + String(account.use_ssl ? " selected" : "") + ">Enabled</option><option value='0'" + String(!account.use_ssl ? " selected" : "") + ">Disabled</option></select></div>"
                "</div>"
                "<datalist id='imap_servers'><option value='imap.gmail.com'>Gmail</option><option value='outlook.office365.com'>Outlook/Hotmail</option><option value='imap.mail.yahoo.com'>Yahoo</option></datalist>"
                "</div>"

                "<div style='margin-bottom: 16px;'>"
                "<h4 style='color: var(--theme-accent); margin: 0 0 10px 0; font-size: 0.95em;'>📻 FLEX Settings</h4>"
                "<div style='display: grid; grid-template-columns: 100px 120px 80px; justify-content: space-between; align-items: end;'>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Capcode:</label><input type='number' id='edit_capcode_" + String(i) + "' min='1' max='4291000000' value='" + String(account.capcode) + "' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Frequency (MHz):</label><input type='number' id='edit_frequency_" + String(i) + "' step='0.0001' min='400' max='1000' value='" + String(account.frequency, 4) + "' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Mail Drop:</label><select id='edit_mail_drop_" + String(i) + "' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'><option value='0'" + String(!account.mail_drop ? " selected" : "") + ">No</option><option value='1'" + String(account.mail_drop ? " selected" : "") + ">Yes</option></select></div>"
                "</div>"
                "</div>"

                "<div style='display: flex; gap: 10px; margin-top: 20px;'>"
                "<button onclick='saveEditAccount(" + String(i) + ")' style='background-color: #28a745; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 0.9em;'>✅ Save Changes</button>"
                "<button onclick='cancelEditAccount(" + String(i) + ")' style='background-color: #6c757d; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 0.9em;'>❌ Cancel</button>"
                "</div>"
                "</div>"

                "</div>";

        webServer.sendContent(html);
        html = "";
    }

    if (imap_config.accounts.size() < IMAP_MAX_ACCOUNTS) {
        html += "<div style='text-align: center; margin: 20px 0;'>"
                "<button onclick='toggleAddAccount()' style='padding: 12px 24px; background-color: #28a745; color: white; border: none; border-radius: 8px; cursor: pointer; font-size: 1em; font-weight: 500;'>➕ Add IMAP Account</button>"
                "</div>";

        html += "<div id='add-account-form' style='display: none; margin: 20px 0; padding: 20px; border: 2px solid var(--theme-border); border-radius: 12px; background-color: var(--theme-input);'>"
                "<h4 style='margin: 0 0 18px 0; color: var(--theme-text); text-align: center; font-size: 1.1em;'>➕ Add IMAP Account</h4>"

                "<div style='margin-bottom: 16px;'>"
                "<h4 style='color: var(--theme-accent); margin: 0 0 10px 0; font-size: 0.95em;'>🔑 Authentication</h4>"
                "<div style='display: grid; grid-template-columns: 1fr 1fr 70px; gap: 15px; align-items: end;'>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Username/Email:</label><input type='text' id='add_username' maxlength='63' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Password:</label><input type='password' id='add_password' maxlength='63' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Check (min):</label><input type='number' id='add_check_interval' value='5' min='5' max='1440' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "</div>"
                "</div>"

                "<div style='margin-bottom: 16px;'>"
                "<h4 style='color: var(--theme-accent); margin: 0 0 10px 0; font-size: 0.95em;'>🔗 Connection Settings</h4>"
                "<div style='display: grid; grid-template-columns: 1fr 70px 100px; gap: 15px; align-items: end;'>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>IMAP Server:</label><input type='text' id='add_server' maxlength='63' list='imap_servers_add' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Port:</label><input type='number' id='add_port' value='993' min='1' max='65535' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>SSL/TLS:</label><select id='add_use_ssl' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'><option value='1'>Enabled</option><option value='0'>Disabled</option></select></div>"
                "</div>"
                "<datalist id='imap_servers_add'><option value='imap.gmail.com'>Gmail</option><option value='outlook.office365.com'>Outlook/Hotmail</option><option value='imap.mail.yahoo.com'>Yahoo</option></datalist>"
                "</div>"

                "<div style='margin-bottom: 16px;'>"
                "<h4 style='color: var(--theme-accent); margin: 0 0 10px 0; font-size: 0.95em;'>📻 FLEX Settings</h4>"
                "<div style='display: grid; grid-template-columns: 100px 120px 80px; justify-content: space-between; align-items: end;'>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Capcode:</label><input type='number' id='add_capcode' value='" + String(settings.default_capcode) + "' min='1' max='4291000000' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Frequency (MHz):</label><input type='number' id='add_frequency' value='" + String(settings.default_frequency, 4) + "' step='0.0001' min='400' max='1000' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'></div>"
                "<div><label style='display: block; margin-bottom: 5px; font-weight: 500; font-size: 0.9em;'>Mail Drop:</label><select id='add_mail_drop' style='width: 100%; padding: 8px 10px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text); font-size: 0.9em; box-sizing: border-box;'><option value='0'>No</option><option value='1' selected>Yes</option></select></div>"
                "</div>"
                "</div>"

                "<div style='display: flex; gap: 10px; margin-top: 20px;'>"
                "<button onclick='saveAddAccount()' style='background-color: #28a745; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 0.9em;'>✅ Add Account</button>"
                "<button onclick='cancelAddAccount()' style='background-color: #6c757d; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 0.9em;'>❌ Cancel</button>"
                "</div>"
                "</div>";

        webServer.sendContent(html);
        html = "";
    } else {
        html += "<div style='text-align: center; margin: 20px 0; color: var(--theme-secondary);'>"
                "<em>Maximum " + String(IMAP_MAX_ACCOUNTS) + " accounts reached</em>"
                "</div>";
    }

    html += "</div>";

    webServer.sendContent(html);
    html = "";



    html += "<div class='form-section'>"
            "<div style='background-color: var(--theme-card); padding: 20px; border-radius: 12px; border: 1px solid var(--theme-border);'>"
            "<h4 style='margin-top: 0; color: var(--theme-accent);'>ℹ️ IMAP Features</h4>"
            "<ul style='margin: 0; padding-left: 20px; line-height: 1.8;'>"
            "<li><strong>Jittered Scheduling:</strong> Accounts check at staggered intervals (Account 1: +0min, Account 2: +1min, etc.)</li>"
            "<li><strong>Per-Account FLEX Settings:</strong> Each account can transmit to a specific frequency and capcode</li>"
            "<li><strong>Individual Processing:</strong> Each account processes up to 10 emails per check cycle</li>"
            "<li><strong>Smart Reconnection:</strong> Automatic reconnection with failure tracking per account</li>"
            "<li><strong>Minimum Check Interval:</strong> " + String(IMAP_MIN_CHECK_INTERVAL) + " minutes to prevent excessive server load</li>"
            "<li><strong>Maximum Accounts:</strong> Up to " + String(IMAP_MAX_ACCOUNTS) + " accounts supported simultaneously</li>"
            "</ul>"
            "</div>"
            "</div>";

    webServer.sendContent(html);
    html = "";

    html += "<script>"
            "function toggleIMAPEnabled() {"
            "  fetch('/imap_toggle', {method: 'POST'})"
            "  .then(response => response.json())"
            "  .then(data => {"
            "    if(data.success) {"
            "      showTempMessage('IMAP toggled successfully');"
            "      setTimeout(() => location.reload(), 1000);"
            "    } else {"
            "      showTempMessage('Failed to toggle IMAP', true);"
            "    }"
            "  });"
            "}"
            ""
            "function toggleAddAccount() {"
            "  const form = document.getElementById('add-account-form');"
            "  if (form.style.display === 'none' || form.style.display === '') {"
            "    form.style.display = 'block';"
            "  } else {"
            "    form.style.display = 'none';"
            "  }"
            "}"
            ""
            "function cancelAddAccount() {"
            "  document.getElementById('add-account-form').style.display = 'none';"
            "  document.getElementById('add_server').value = '';"
            "  document.getElementById('add_port').value = '993';"
            "  document.getElementById('add_use_ssl').value = '1';"
            "  document.getElementById('add_username').value = '';"
            "  document.getElementById('add_password').value = '';"
            "  document.getElementById('add_check_interval').value = '5';"
            "  document.getElementById('add_capcode').value = '" + String(settings.default_capcode) + "';"
            "  document.getElementById('add_frequency').value = '" + String(settings.default_frequency, 4) + "';"
            "  document.getElementById('add_mail_drop').value = '1';"
            "}"

            "function saveAddAccount() {"
            "  const data = {"
            "    name: document.getElementById('add_username').value,"
            "    server: document.getElementById('add_server').value,"
            "    port: parseInt(document.getElementById('add_port').value),"
            "    use_ssl: document.getElementById('add_use_ssl').value === '1',"
            "    username: document.getElementById('add_username').value,"
            "    password: document.getElementById('add_password').value,"
            "    check_interval_min: parseInt(document.getElementById('add_check_interval').value),"
            "    capcode: parseInt(document.getElementById('add_capcode').value),"
            "    frequency: parseFloat(document.getElementById('add_frequency').value),"
            "    mail_drop: document.getElementById('add_mail_drop').value === '1'"
            "  };"
            "  "
            "  fetch('/imap_add', {"
            "    method: 'POST',"
            "    headers: {'Content-Type': 'application/json'},"
            "    body: JSON.stringify(data)"
            "  })"
            "  .then(response => response.json())"
            "  .then(data => {"
            "    if(data.success) {"
            "      showTempMessage('Account added successfully');"
            "      cancelAddAccount();"
            "      setTimeout(() => location.reload(), 1000);"
            "    } else {"
            "      showTempMessage('Failed to add account: ' + data.error, true);"
            "    }"
            "  });"
            "}"
            ""
            "function toggleEditAccount(index) {"
            "  const form = document.getElementById('edit-form-' + index);"
            "  if (form.style.display === 'none' || form.style.display === '') {"
            "    const allForms = document.querySelectorAll('[id^=\"edit-form-\"]');"
            "    allForms.forEach(f => f.style.display = 'none');"
            "    form.style.display = 'block';"
            "  } else {"
            "    form.style.display = 'none';"
            "  }"
            "}"

            "function deleteAccount(index) {"
            "  if(confirm('Are you sure you want to delete this IMAP account?')) {"
            "    fetch('/imap_delete/' + index, {method: 'POST'})"
            "    .then(response => response.json())"
            "    .then(data => {"
            "      if(data.success) {"
            "        showTempMessage('Account deleted successfully');"
            "        setTimeout(() => location.reload(), 1000);"
            "      } else {"
            "        showTempMessage('Failed to delete account', true);"
            "      }"
            "    });"
            "  }"
            "}"
            ""
            "function cancelEditAccount(index) {"
            "  document.getElementById('edit-form-' + index).style.display = 'none';"
            "}"
            ""
            "function saveEditAccount(index) {"
            "  const passwordField = document.getElementById('edit_password_' + index);"
            "  const passwordValue = passwordField.value === '*****' ? '' : passwordField.value;"
            "  const data = {"
            "    name: document.getElementById('edit_username_' + index).value,"
            "    server: document.getElementById('edit_server_' + index).value,"
            "    port: parseInt(document.getElementById('edit_port_' + index).value),"
            "    use_ssl: document.getElementById('edit_use_ssl_' + index).value === '1',"
            "    username: document.getElementById('edit_username_' + index).value,"
            "    password: passwordValue,"
            "    check_interval_min: parseInt(document.getElementById('edit_check_interval_' + index).value),"
            "    capcode: parseInt(document.getElementById('edit_capcode_' + index).value),"
            "    frequency: parseFloat(document.getElementById('edit_frequency_' + index).value),"
            "    mail_drop: document.getElementById('edit_mail_drop_' + index).value === '1'"
            "  };"
            "  "
            "  fetch('/imap_update/' + index, {"
            "    method: 'POST',"
            "    headers: {'Content-Type': 'application/json'},"
            "    body: JSON.stringify(data)"
            "  })"
            "  .then(response => response.json())"
            "  .then(data => {"
            "    if(data.success) {"
            "      showTempMessage('Account updated successfully');"
            "      cancelEditAccount(index);"
            "      setTimeout(() => location.reload(), 1000);"
            "    } else {"
            "      showTempMessage('Failed to update account: ' + data.error, true);"
            "    }"
            "  });"
            "}"
            ""
            "function showTempMessage(message, isError = false) {"
            "  const div = document.getElementById('temp-message');"
            "  div.innerHTML = '<div style=\"background-color: ' + (isError ? '#dc3545' : '#28a745') + '; color: white; padding: 10px; margin: 10px 0; border-radius: 4px; text-align: center;\">' + message + '</div>';"
            "  setTimeout(() => div.innerHTML = '', 3000);"
            "}"
            "</script>";

    webServer.sendContent(html);
    html = "";

    html += get_html_footer();
    webServer.sendContent(html);
    webServer.sendContent("");
}

void handle_imap_toggle() {
    reset_oled_timeout();

    imap_config.enabled = !imap_config.enabled;

    imap_failed_cycles = 0;
    logMessage("IMAP: Suspension flags reset due to configuration change");

    if (save_imap_config()) {
        logMessagef("IMAP: Toggled to %s", imap_config.enabled ? "enabled" : "disabled");
        webServer.send(200, "application/json", "{\"success\":true}");
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save IMAP configuration\"}");
    }
}

void handle_imap_add() {
    reset_oled_timeout();

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

    if (error) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    String name = doc["name"] | "";
    String server = doc["server"] | "";
    uint16_t port = doc["port"] | 993;
    bool use_ssl = doc["use_ssl"] | true;
    String username = doc["username"] | "";
    String password = doc["password"] | "";
    uint16_t check_interval_min = doc["check_interval_min"] | IMAP_MIN_CHECK_INTERVAL;
    uint64_t capcode = doc["capcode"] | settings.default_capcode;
    float frequency = doc["frequency"] | settings.default_frequency;
    bool mail_drop = doc["mail_drop"] | false;

    if (add_imap_account(name, server, port, use_ssl, username, password,
                         check_interval_min, capcode, frequency, mail_drop)) {
            imap_failed_cycles = 0;
        logMessage("IMAP: Suspension flags reset due to configuration change");

        if (save_imap_config()) {
            webServer.send(200, "application/json", "{\"success\":true}");
        } else {
            delete_imap_account(imap_config.accounts.size());
            webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save configuration\"}");
        }
    } else {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Failed to add account - check parameters\"}");
    }
}

void handle_imap_edit() {
    reset_oled_timeout();

    String uri = webServer.uri();
    int index = uri.substring(uri.lastIndexOf('/') + 1).toInt();

    if (index < 0 || index >= (int)imap_config.accounts.size()) {
        webServer.send(404, "text/html", "Account not found");
        return;
    }

    webServer.sendHeader("Location", "/imap");
    webServer.send(302, "text/plain", "");
}

void handle_imap_delete() {
    reset_oled_timeout();

    String uri = webServer.uri();
    int index = uri.substring(uri.lastIndexOf('/') + 1).toInt();

    if (index < 0 || index >= (int)imap_config.accounts.size()) {
        webServer.send(404, "application/json", "{\"success\":false,\"error\":\"Account not found\"}");
        return;
    }

    uint8_t account_id = imap_config.accounts[index].id;

    if (delete_imap_account(account_id)) {
            imap_failed_cycles = 0;
        logMessage("IMAP: Suspension flags reset due to configuration change");

        if (save_imap_config()) {
            webServer.send(200, "application/json", "{\"success\":true}");
        } else {
            webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save configuration\"}");
        }
    } else {
        webServer.send(404, "application/json", "{\"success\":false,\"error\":\"Account not found\"}");
    }
}

void handle_imap_account_data() {
    reset_oled_timeout();

    String uri = webServer.uri();
    int index = uri.substring(uri.lastIndexOf('/') + 1).toInt();

    if (index < 0 || index >= (int)imap_config.accounts.size()) {
        webServer.send(404, "application/json", "{\"success\":false,\"error\":\"Account not found\"}");
        return;
    }

    IMAPAccount &account = imap_config.accounts[index];

    String json = "{";
    json += "\"name\":\"" + String(account.name) + "\",";
    json += "\"server\":\"" + String(account.server) + "\",";
    json += "\"port\":" + String(account.port) + ",";
    json += "\"use_ssl\":" + String(account.use_ssl ? "true" : "false") + ",";
    json += "\"username\":\"" + String(account.username) + "\",";
    json += "\"password\":\"" + String(account.password) + "\",";
    json += "\"check_interval_min\":" + String(account.check_interval_min) + ",";
    json += "\"capcode\":" + String(account.capcode) + ",";
    json += "\"frequency\":" + String(account.frequency, 4) + ",";
    json += "\"mail_drop\":" + String(account.mail_drop ? "true" : "false");
    json += "}";

    webServer.send(200, "application/json", json);
}

void handle_imap_update() {
    reset_oled_timeout();

    String uri = webServer.uri();
    int index = uri.substring(uri.lastIndexOf('/') + 1).toInt();

    if (index < 0 || index >= (int)imap_config.accounts.size()) {
        webServer.send(404, "application/json", "{\"success\":false,\"error\":\"Account not found\"}");
        return;
    }

    String body = webServer.arg("plain");
    DynamicJsonDocument doc(1024);

    if (deserializeJson(doc, body)) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    IMAPAccount &account = imap_config.accounts[index];

    if (doc.containsKey("name")) strncpy(account.name, doc["name"], sizeof(account.name) - 1);
    if (doc.containsKey("server")) strncpy(account.server, doc["server"], sizeof(account.server) - 1);
    if (doc.containsKey("port")) account.port = doc["port"];
    if (doc.containsKey("use_ssl")) account.use_ssl = doc["use_ssl"];
    if (doc.containsKey("username")) strncpy(account.username, doc["username"], sizeof(account.username) - 1);
    if (doc.containsKey("password") && strlen(doc["password"]) > 0) strncpy(account.password, doc["password"], sizeof(account.password) - 1);
    if (doc.containsKey("check_interval_min")) account.check_interval_min = doc["check_interval_min"];
    if (doc.containsKey("capcode")) account.capcode = doc["capcode"];
    if (doc.containsKey("frequency")) account.frequency = doc["frequency"];
    if (doc.containsKey("mail_drop")) account.mail_drop = doc["mail_drop"];

    account.failed_check_cycles = 0;
    account.suspended = false;
    logMessagef("IMAP: Account '%s' suspension reset due to configuration change", account.name);

    if (save_imap_config()) {
        webServer.send(200, "application/json", "{\"success\":true}");
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save configuration\"}");
    }
}
#endif // ENABLE_IMAP

// =============================================================================
// REST API CONFIGURATION PAGE
// =============================================================================

static bool is_using_default_api_password() {
    String encoded_current = base64_encode_string(String(settings.api_password));
    return (encoded_current == "cGFzc3cwcmQ=");
}

void handle_api_config() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("API Configuration");

    chunk += "<div class='header'>"
            "<h1>🔗 API Configuration</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-active'" + String(settings.api_enabled ? " style='color:#28a745;'" : "") + ">🔗 API</a>"
            "<a href='/grafana' class='tab-inactive'" + String(settings.grafana_enabled ? " style='color:#28a745;'" : "") + ">🚨 Grafana</a>"
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
            "<span style='text-large'>Enable API</span>"
            "<div class='toggle-switch " + String(settings.api_enabled ? "is-active" : "is-inactive") + "' onclick='toggleAPI()'>"
            "<div class='toggle-slider " + String(settings.api_enabled ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<form action='/save_api' method='post' onsubmit='return submitFormAjax(this, \"API settings saved successfully!\", \"API settings saved, restarting in 5 seconds...\")'>"

            "<input type='hidden' id='api_enabled' name='api_enabled' value='" + String(settings.api_enabled ? "1" : "0") + "'>"

            "<div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0;'>"

            "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🔌 HTTP Port Settings</h4>"
            "<label for='http_port' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>HTTP Port:</label>"
            "<input type='number' id='http_port' name='http_port' value='" + String(settings.http_port) + "' min='1' max='65535' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<small style='color: var(--theme-secondary); display: block; margin-top: 5px;'>Port for HTTP API access (default: 80)</small>"
            "</div>"

            "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>🔐 API Authentication</h4>"
            "<div style='margin-bottom: 16px;'>"
            "<label for='api_username' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>API Username:</label>"
            "<input type='text' id='api_username' name='api_username' value='" + htmlEscape(String(settings.api_username)) + "' maxlength='32' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div>"
            "<label for='api_password' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>API Password:</label>"
            "<input type='password' id='api_password' name='api_password' value='" + htmlEscape(String(settings.api_password)) + "' maxlength='64' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<small style='color: var(--theme-secondary); display: block; margin-top: 5px;'>HTTP Basic Auth credentials for all API endpoints</small>"
            "</div>"

            "</div>";

    if (is_using_default_api_password()) {
        chunk += "<div style='margin-top:20px;padding:15px;background-color:#fff3cd;border:2px solid #ffc107;border-radius:8px;color:#856404;text-align:center;'>"
                "<strong>⚠️ SECURITY WARNING:</strong> API is using the default password.<br>We recommend changing it for security."
                "</div>";
    }

    chunk += "<div style='margin-top:30px;text-align:center;'>"
            "<button type='submit' class='button' style='padding: 15px 30px; background-color: #28a745; color: white; border: none; border-radius: 8px; cursor: pointer; font-size: 16px; font-weight: 500; transition: background-color 0.3s;'>💾 Save API Configuration</button>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='padding:20px;'>"

            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;margin:20px 0;border-left:5px solid #007bff;'>"
            "<h3 style='margin:0 0 15px 0;color:#007bff;'>🌐 REST API Endpoint</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Send FLEX messages via REST API with HTTP Basic Authentication:</p>"
            "<div style='background-color:var(--theme-input);padding:15px;border-radius:8px;border:2px solid var(--theme-border);'>"
            "<code style='font-size:16px;font-weight:bold;color:var(--theme-text);word-break:break-all;'>"
            "POST http://" + WiFi.localIP().toString() + "/api"
            "</code>"
            "</div>"
            "</div>"

            "<h3>📡 JSON Payload Format</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Required and optional fields for FLEX message transmission (supports both numeric and string values):</p>"
            "<textarea readonly style='width:100%;height:204px;padding:15px;border:2px solid var(--theme-border);border-radius:8px;font-family:monospace;font-size:13px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:vertical;'>"
            "{\n"
            "  \"message\": \"Hello World\",\n"
            "  \"capcode\": 1234567,\n"
            "  \"frequency\": 931.9375,\n"
            "  \"power\": 10,\n"
            "  \"mail_drop\": false\n"
            "}\n\n"
            "Note: All optional fields support both numeric and string formats\n"
            "Missing fields use FLEX configuration defaults"
            "</textarea>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>🔧 curl Command Example</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Complete curl example with HTTP Basic Auth:</p>"
            "<textarea readonly style='width:100%;height:161px;padding:15px;border:2px solid var(--theme-border);border-radius:8px;font-family:monospace;font-size:12px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:vertical;'>"
            "curl -X POST \\\n"
            "     -H \"Content-Type: application/json\" \\\n"
            "     -u " + String(settings.api_username) + ":" + String(settings.api_password) + " \\\n"
            "     http://" + WiFi.localIP().toString() + "/api \\\n"
            "     -d '{\n"
            "       \"message\": \"Hello World\",\n"
            "       \"capcode\": 1234567\n"
            "     }'"
            "</textarea>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>📨 Response Format</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>API returns JSON response with transmission status:</p>"
            "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:20px 0;'>"
            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;border-left:4px solid #28a745;'>"
            "<h4 style='margin:0 0 15px 0;color:#28a745;'>✅ Success Response (HTTP 200)</h4>"
            "<textarea readonly style='width:100%;height:183px;padding:10px;border:1px solid var(--theme-border);border-radius:6px;font-family:monospace;font-size:11px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:none;'>"
            "{\n"
            "  \"status\": \"queued\",\n"
            "  \"message\": \"Message queued\",\n"
            "  \"frequency\": 931.9375,\n"
            "  \"power\": 10,\n"
            "  \"capcode\": 1234567,\n"
            "  \"text\": \"Hello World\",\n"
            "  \"truncated\": false,\n"
            "  \"queue_position\": 1\n"
            "}"
            "</textarea>"
            "</div>"
            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;border-left:4px solid #dc3545;'>"
            "<h4 style='margin:0 0 15px 0;color:#dc3545;'>❌ Error Response (HTTP 4xx/5xx)</h4>"
            "<textarea readonly style='width:100%;height:164px;padding:10px;border:1px solid var(--theme-border);border-radius:6px;font-family:monospace;font-size:11px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:none;'>"
            "{\n"
            "  \"status\": \"error\",\n"
            "  \"message\": \"Invalid JSON\"\n"
            "}\n\n"
            "Common HTTP Status Codes:\n"
            "400: Bad Request\n"
            "401: Authentication Required\n"
            "503: Queue Full"
            "</textarea>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>🔧 API Features</h3>"
            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;margin:20px 0;'>"
            "<ul style='margin:0;padding-left:20px;line-height:1.8;'>"
            "<li><strong>Queue System:</strong> Messages are queued for sequential transmission</li>"
            "<li><strong>Auto-Truncation:</strong> Messages longer than 248 characters are automatically truncated</li>"
            "<li><strong>Dual Type Support:</strong> All fields accept both numeric and string values</li>"
            "<li><strong>Frequency Conversion:</strong> Supports both MHz (931.9375) and Hz (931937500) formats</li>"
            "<li><strong>Default Values:</strong> Missing optional fields use FLEX configuration defaults</li>"
            "<li><strong>EMR Support:</strong> Messages benefit from Emergency Message Resynchronization</li>"
            "<li><strong>HTTP Basic Auth:</strong> Same credentials for all API endpoints</li>"
            "</ul>"
            "</div>"

            "</div>"
            "</form>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<script>"
            "function toggleAPI() {"
            "  const toggleSwitch = document.querySelector('.toggle-switch');"
            "  const toggleSlider = document.querySelector('.toggle-slider');"
            "  const hiddenInput = document.getElementById('api_enabled');"
            "  "
            "  const currentEnabled = hiddenInput.value === '1';"
            "  const newEnabled = !currentEnabled;"
            "  "
            "  if (newEnabled) {"
            "    toggleSwitch.style.backgroundColor = '#28a745';"
            "    toggleSlider.style.left = '26px';"
            "    hiddenInput.value = '1';"
            "  } else {"
            "    toggleSwitch.style.backgroundColor = '#ccc';"
            "    toggleSlider.style.left = '2px';"
            "    hiddenInput.value = '0';"
            "  }"
            "}"
            "</script>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_save_api() {
    reset_oled_timeout();

    uint16_t http_port = webServer.arg("http_port").toInt();

    if (http_port < 1 || http_port > 65535) {
        webServer.send(400, "text/plain", "Invalid port range (must be 1-65535)");
        return;
    }

    if (webServer.hasArg("api_enabled")) {
        settings.api_enabled = (webServer.arg("api_enabled") == "1");
    }

    settings.http_port = http_port;

    if (webServer.hasArg("api_username")) {
        String username = webServer.arg("api_username");
        username.trim();

        if (username.length() > sizeof(settings.api_username) - 1) {
            webServer.send(400, "text/plain", "Username too long (max 31 chars)");
            return;
        }
        if (username.length() == 0) {
            webServer.send(400, "text/plain", "Username cannot be empty");
            return;
        }

        strncpy(settings.api_username, username.c_str(), sizeof(settings.api_username) - 1);
        settings.api_username[sizeof(settings.api_username) - 1] = '\0';
    }

    if (webServer.hasArg("api_password")) {
        String password = webServer.arg("api_password");
        password.trim();

        if (password.length() > sizeof(settings.api_password) - 1) {
            webServer.send(400, "text/plain", "Password too long (max 63 chars)");
            return;
        }
        if (password.length() < 4) {
            webServer.send(400, "text/plain", "Password too short (min 4 chars)");
            return;
        }

        strncpy(settings.api_password, password.c_str(), sizeof(settings.api_password) - 1);
        settings.api_password[sizeof(settings.api_password) - 1] = '\0';
    }

    if (save_runtime_settings()) {
        webServer.send(200, "application/json", "{\"success\":true,\"message\":\"API settings saved successfully!\"}");
        logMessage("CONFIG: API settings saved - HTTP:" + String(http_port));

        delay(1000);
        ESP.restart();
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save configuration\"}");
    }
}

// =============================================================================
// GSM CONFIGURATION PAGE
// =============================================================================

#ifdef ENABLE_GSM

void handle_gsm_config() {
    reset_oled_timeout();

    int actual_power_pin = (gsm_config.power_pin == 0) ? GSM_PWR_PIN : gsm_config.power_pin;
    int actual_rx_pin = (gsm_config.rx_pin == 0) ? GSM_RX_PIN : gsm_config.rx_pin;
    int actual_tx_pin = (gsm_config.tx_pin == 0) ? GSM_TX_PIN : gsm_config.tx_pin;
    unsigned long actual_baudrate = (gsm_config.baudrate == 0) ? 115200 : gsm_config.baudrate;
    int actual_timeout_seconds = (gsm_config.connection_timeout == 0) ? 30 : (gsm_config.connection_timeout / 1000);

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("GSM Configuration");

    chunk += "<div class='header'>"
            "<h1>📡 GSM Configuration</h1>"
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
            + gsm_nav_tab_html(true) +
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    String toggle_background = gsm_config.enable_gsm ? "#28a745" : "#ccc";

    chunk += "<div class='form-section'>"
            "<div class='flex-space-between mb-20'>"
            "<div class='flex-center'>"
            "<span style='text-large'>Enable GSM</span>"
            "<div class='toggle-switch' id='gsm-toggle' onclick='toggleGSM(event)' style='background-color: " +
            toggle_background + ";'>"
            "<div class='toggle-slider' style='left: " +
            String(gsm_config.enable_gsm ? "26px" : "2px") + ";'></div>"
            "</div>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<form action='/save_gsm' method='post' onsubmit='return submitFormAjax(this, \"GSM settings saved successfully!\", \"GSM settings saved, restarting now...\")'>"

            "<input type='hidden' id='gsm_enabled' name='gsm_enabled' value='" + String(gsm_config.enable_gsm ? "1" : "0") + "'>"

            "<div class='form-section' style='margin: 20px 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>📡 Network Settings</h4>"

            "<div style='margin-bottom: 20px;'>"
            "<label for='apn' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>APN (Access Point Name):</label>"
            "<input type='text' id='apn' name='apn' value='" + String(gsm_config.apn) + "' maxlength='63' placeholder='internet' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"

            "<div style='display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 20px;'>"
            "<div>"
            "<label for='apn_user' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>APN Username (optional):</label>"
            "<input type='text' id='apn_user' name='apn_user' value='" + String(gsm_config.apn_user) + "' maxlength='32' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div>"
            "<label for='apn_pass' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>APN Password (optional):</label>"
            "<input type='password' id='apn_pass' name='apn_pass' value='" + String(gsm_config.apn_pass) + "' maxlength='32' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div>"
            "<label for='pin' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>SIM PIN (optional):</label>"
            "<input type='text' id='pin' name='pin' value='" + String(gsm_config.pin) + "' maxlength='8' placeholder='1234' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div class='form-section' style='margin: 20px 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<h4 style='margin-top: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>⚡ Hardware Configuration</h4>"

            "<div style='display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 20px; margin-bottom: 20px;'>"
            "<div>"
            "<label for='power_pin' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Power Pin (GPIO):</label>"
            "<input type='number' id='power_pin' name='power_pin' value='" + String(actual_power_pin) + "' min='0' max='39' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:5px 0 0 0;'>Default: GPIO " + String(GSM_PWR_PIN) + "</p>"
            "</div>"
            "<div>"
            "<label for='rx_pin' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>RX Pin - GSM Module (GPIO):</label>"
            "<input type='number' id='rx_pin' name='rx_pin' value='" + String(actual_rx_pin) + "' min='0' max='39' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:5px 0 0 0;'>GSM RX → GPIO " + String(GSM_RX_PIN) + "</p>"
            "</div>"
            "<div>"
            "<label for='tx_pin' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>TX Pin - GSM Module (GPIO):</label>"
            "<input type='number' id='tx_pin' name='tx_pin' value='" + String(actual_tx_pin) + "' min='0' max='39' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:5px 0 0 0;'>GSM TX → GPIO " + String(GSM_TX_PIN) + "</p>"
            "</div>"
            "</div>"

            "<div style='display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 20px;'>"
            "<div>"
            "<label for='baudrate' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Baudrate (bps):</label>"
            "<input type='number' id='baudrate' name='baudrate' value='" + String(actual_baudrate) + "' min='9600' max='921600' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:5px 0 0 0;'>Default: 115200</p>"
            "</div>"
            "<div>"
            "<label for='connection_timeout' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Connection Timeout (seconds):</label>"
            "<input type='number' id='connection_timeout' name='connection_timeout' value='" + String(actual_timeout_seconds) + "' min='10' max='300' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div>"
            "<label for='min_signal_quality' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Min Signal Quality (0-31):</label>"
            "<input type='number' id='min_signal_quality' name='min_signal_quality' value='" + String(gsm_config.min_signal_quality) + "' min='0' max='31' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='margin: 20px; text-align: center;'>"
            "<button type='submit' class='button' style='padding: 15px 30px; background-color: #28a745; color: white; border: none; border-radius: 8px; cursor: pointer; font-size: 16px; font-weight: 500; transition: background-color 0.3s;'>💾 Save Configuration</button>"
            "</div>"
            "</form>";

    chunk += "<script>"
            "function updateToggleVisual(state){var toggle=document.getElementById('gsm-toggle');if(!toggle)return;var slider=toggle.querySelector('.toggle-slider');if(slider){slider.style.left=state?'26px':'2px';}toggle.style.backgroundColor=state?'#28a745':'#ccc';}"
            "function toggleGSM(event){if(event){event.preventDefault();}var input=document.getElementById('gsm_enabled');if(!input)return;var currentState=input.value==='1';var newState=!currentState;input.value=newState?'1':'0';updateToggleVisual(newState);}"
            "</script>";

    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_save_gsm() {
    reset_oled_timeout();

    if (webServer.hasArg("gsm_enabled")) {
        gsm_config.enable_gsm = (webServer.arg("gsm_enabled") == "1");
    }

    if (webServer.hasArg("apn")) {
        String apn = webServer.arg("apn");
        apn.trim();
        strncpy(gsm_config.apn, apn.c_str(), sizeof(gsm_config.apn) - 1);
        gsm_config.apn[sizeof(gsm_config.apn) - 1] = '\0';
    }

    if (webServer.hasArg("apn_user")) {
        String user = webServer.arg("apn_user");
        user.trim();
        strncpy(gsm_config.apn_user, user.c_str(), sizeof(gsm_config.apn_user) - 1);
        gsm_config.apn_user[sizeof(gsm_config.apn_user) - 1] = '\0';
    }

    if (webServer.hasArg("apn_pass")) {
        String pass = webServer.arg("apn_pass");
        pass.trim();
        strncpy(gsm_config.apn_pass, pass.c_str(), sizeof(gsm_config.apn_pass) - 1);
        gsm_config.apn_pass[sizeof(gsm_config.apn_pass) - 1] = '\0';
    }

    if (webServer.hasArg("pin")) {
        String pin = webServer.arg("pin");
        pin.trim();
        strncpy(gsm_config.pin, pin.c_str(), sizeof(gsm_config.pin) - 1);
        gsm_config.pin[sizeof(gsm_config.pin) - 1] = '\0';
    }

    if (webServer.hasArg("rx_pin")) {
        int rx = webServer.arg("rx_pin").toInt();
        gsm_config.rx_pin = (rx == 0) ? GSM_RX_PIN : rx;
    }

    if (webServer.hasArg("tx_pin")) {
        int tx = webServer.arg("tx_pin").toInt();
        gsm_config.tx_pin = (tx == 0) ? GSM_TX_PIN : tx;
    }

    if (webServer.hasArg("power_pin")) {
        int pwr = webServer.arg("power_pin").toInt();
        gsm_config.power_pin = (pwr == 0) ? GSM_PWR_PIN : pwr;
    }

    if (webServer.hasArg("baudrate")) {
        unsigned long baud = webServer.arg("baudrate").toInt();
        gsm_config.baudrate = (baud == 0) ? 115200 : baud;
    }

    if (webServer.hasArg("connection_timeout")) {
        int timeout_sec = webServer.arg("connection_timeout").toInt();
        gsm_config.connection_timeout = (timeout_sec == 0) ? 30000 : (timeout_sec * 1000);
    }

    if (webServer.hasArg("min_signal_quality")) {
        gsm_config.min_signal_quality = webServer.arg("min_signal_quality").toInt();
    }

    if (webServer.hasArg("require_cell_signal")) {
        String require_arg = webServer.arg("require_cell_signal");
        require_arg.toLowerCase();
        gsm_config.require_cell_signal = (require_arg == "on" || require_arg == "1" || require_arg == "true");
    }

    if (save_runtime_settings()) {
        logMessage("GSM: Configuration saved successfully");
        webServer.send(200, "application/json", "{\"success\":true,\"restart\":true}");
        delay(500);
        ESP.restart();
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save GSM configuration\"}");
    }
}

void handle_gsm_status() {
    reset_oled_timeout();
    network_update_active_state();

    if (!transmission_guard_active() && gsm_config.enable_gsm && gsm_connected) {
        gsm_update_network_info();
    }

    bool network_connected_flag = transmission_guard_active()
        ? network_available_cached
        : (wifi_connected || gsm_connected);

    StaticJsonDocument<512> doc;
    doc["enabled"] = gsm_config.enable_gsm;
    doc["power"] = gsm_power_state;
    doc["modem_ready"] = gsm_modem_ready;
    doc["connected"] = gsm_connected;
    doc["registration"] = gsm_registration_complete;
    doc["active"] = active_network_label(active_network);
    doc["network_available"] = network_connected_flag;
    const char* module_display = gsm_module_detected
        ? gsm_module_label(gsm_module_type)
        : "Unknown";
    doc["module"] = module_display;
    doc["module_detected"] = gsm_module_detected;

    if (gsm_connected) {
        doc["operator"] = gsm_operator_name;
        doc["signal"] = gsm_signal_quality;
        doc["ip"] = gsm_ip_address;
        doc["gateway"] = gsm_gateway_address;
        doc["dns_primary"] = gsm_dns_primary;
        doc["dns_secondary"] = gsm_dns_secondary;
        if (gsm_subnet_mask.length() > 0) {
            doc["subnet"] = gsm_subnet_mask;
        }
    }

    String payload;
    serializeJson(doc, payload);
    webServer.send(200, "application/json", payload);
}

#endif // ENABLE_GSM
