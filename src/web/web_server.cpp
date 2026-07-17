/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Web Server Module - HTTP server, REST API, web UI
 */

#include "../web/web_server.h"
#include "../core/config.h"
#include "../version.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../core/display.h"
#include "../core/hardware.h"
#include "../core/utils.h"
#include "../protocol/flex_protocol.h"
#include "../protocol/transmission.h"
#include "../network/network.h"
#include "../network/gsm.h"
#include "../services/mqtt.h"
#include "../services/imap.h"
#include "../services/chatgpt.h"
#include "../services/grafana.h"
#include <WiFi.h>

// =============================================================================
// GLOBALS
// =============================================================================
WebServer webServer(WEB_SERVER_PORT);

// =============================================================================
// FORWARD DECLARATIONS (file-local)
// =============================================================================
static unsigned long last_auth_attempt = 0;
static int auth_failures = 0;
const int MAX_AUTH_FAILURES = 5;
const unsigned long AUTH_LOCKOUT_TIME = 300000;

String gsm_nav_tab_html(bool active) {
#ifdef ENABLE_GSM
    return "<a href='/gsm' class='tab-" + String(active ? "active" : "inactive") +
           String(gsm_config.enable_gsm ? " nav-status-enabled" : "") + "'>📶 GSM</a>";
#else
    return "";
#endif
}

String imap_nav_tab_html(bool active) {
#ifdef ENABLE_IMAP
    return "<a href='/imap' class='tab-" + String(active ? "active" : "inactive") +
           String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>";
#else
    return "";
#endif
}

String chatgpt_nav_tab_html(bool active) {
#ifdef ENABLE_CHATGPT
    return "<a href='/chatgpt' class='tab-" + String(active ? "active" : "inactive") +
           String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>";
#else
    return "";
#endif
}

void web_server_init() {
#ifdef ENABLE_GSM
    webServer.on("/gsm", handle_gsm_config);
    webServer.on("/save_gsm", HTTP_POST, handle_save_gsm);
    webServer.on("/gsm_status", handle_gsm_status);
#endif

    webServer.on("/", handle_root);
    webServer.on("/send", HTTP_POST, handle_send_message);
    webServer.on("/config", handle_configuration);
    webServer.on("/save_config", HTTP_POST, handle_save_config);
    webServer.on("/flex", handle_flex_config);
    webServer.on("/save_flex", HTTP_POST, handle_save_flex);
    webServer.on("/mqtt", handle_mqtt);
    webServer.on("/save_mqtt", HTTP_POST, handle_save_mqtt);
#ifdef ENABLE_IMAP
    webServer.on("/imap", handle_imap_config);
    webServer.on("/imap_toggle", HTTP_POST, handle_imap_toggle);
    webServer.on("/imap_add", HTTP_POST, handle_imap_add);
    webServer.on("/imap_edit/0", handle_imap_edit);
    webServer.on("/imap_edit/1", handle_imap_edit);
    webServer.on("/imap_edit/2", handle_imap_edit);
    webServer.on("/imap_edit/3", handle_imap_edit);
    webServer.on("/imap_edit/4", handle_imap_edit);
    webServer.on("/imap_delete/0", HTTP_POST, handle_imap_delete);
    webServer.on("/imap_delete/1", HTTP_POST, handle_imap_delete);
    webServer.on("/imap_delete/2", HTTP_POST, handle_imap_delete);
    webServer.on("/imap_delete/3", HTTP_POST, handle_imap_delete);
    webServer.on("/imap_delete/4", HTTP_POST, handle_imap_delete);
    webServer.on("/imap_account_data/0", handle_imap_account_data);
    webServer.on("/imap_account_data/1", handle_imap_account_data);
    webServer.on("/imap_account_data/2", handle_imap_account_data);
    webServer.on("/imap_account_data/3", handle_imap_account_data);
    webServer.on("/imap_account_data/4", handle_imap_account_data);
    webServer.on("/imap_update/0", HTTP_POST, handle_imap_update);
    webServer.on("/imap_update/1", HTTP_POST, handle_imap_update);
    webServer.on("/imap_update/2", HTTP_POST, handle_imap_update);
    webServer.on("/imap_update/3", HTTP_POST, handle_imap_update);
    webServer.on("/imap_update/4", HTTP_POST, handle_imap_update);
#endif
    webServer.on("/upload_certificate", HTTP_POST, handle_upload_certificate, handle_file_upload);
    webServer.on("/status", handle_device_status);
    webServer.on("/logs", handle_logs);
    webServer.on("/download_logs", handle_download_logs);
    webServer.on("/factory_reset", HTTP_POST, handle_web_factory_reset);
    webServer.on("/backup_settings", handle_backup_settings);
    webServer.on("/restore_settings", handle_restore_settings);
    webServer.on("/upload_restore", HTTP_POST, handle_upload_restore, handle_upload_restore);

    webServer.on("/api", HTTP_POST, handle_api_message);
    webServer.on("/api/v1/alerts", HTTP_POST, handle_grafana_webhook);
    webServer.on("/api/wifi/scan", HTTP_GET, handle_api_wifi_scan);
    webServer.on("/api/wifi/delete", HTTP_POST, handle_api_wifi_delete);
    webServer.on("/api/wifi/add", HTTP_POST, handle_api_wifi_add);
    webServer.on("/api_config", handle_api_config);
    webServer.on("/grafana", handle_grafana);
    webServer.on("/save_api", HTTP_POST, handle_save_api);
    webServer.on("/grafana_toggle", HTTP_POST, handle_grafana_toggle);

#ifdef ENABLE_CHATGPT
    webServer.on("/chatgpt", handle_chatgpt);
    webServer.on("/chatgpt/config", HTTP_POST, handle_chatgpt_config);
    webServer.on("/chatgpt/notifications", HTTP_POST, handle_chatgpt_notifications);
    webServer.on("/chatgpt/api_key", HTTP_POST, handle_chatgpt_api_key);
    webServer.on("/chatgpt/add_prompt", HTTP_POST, handle_chatgpt_add_prompt);
    webServer.on("/chatgpt/get_prompt/0", handle_chatgpt_get_prompt);
    webServer.on("/chatgpt/get_prompt/1", handle_chatgpt_get_prompt);
    webServer.on("/chatgpt/get_prompt/2", handle_chatgpt_get_prompt);
    webServer.on("/chatgpt/get_prompt/3", handle_chatgpt_get_prompt);
    webServer.on("/chatgpt/get_prompt/4", handle_chatgpt_get_prompt);
    webServer.on("/chatgpt/edit_prompt/0", HTTP_POST, handle_chatgpt_edit_prompt);
    webServer.on("/chatgpt/edit_prompt/1", HTTP_POST, handle_chatgpt_edit_prompt);
    webServer.on("/chatgpt/edit_prompt/2", HTTP_POST, handle_chatgpt_edit_prompt);
    webServer.on("/chatgpt/edit_prompt/3", HTTP_POST, handle_chatgpt_edit_prompt);
    webServer.on("/chatgpt/edit_prompt/4", HTTP_POST, handle_chatgpt_edit_prompt);
    webServer.on("/chatgpt/toggle/0", HTTP_POST, handle_chatgpt_toggle);
    webServer.on("/chatgpt/toggle/1", HTTP_POST, handle_chatgpt_toggle);
    webServer.on("/chatgpt/toggle/2", HTTP_POST, handle_chatgpt_toggle);
    webServer.on("/chatgpt/toggle/3", HTTP_POST, handle_chatgpt_toggle);
    webServer.on("/chatgpt/toggle/4", HTTP_POST, handle_chatgpt_toggle);
    webServer.on("/chatgpt/delete/0", handle_chatgpt_delete);
    webServer.on("/chatgpt/delete/1", handle_chatgpt_delete);
    webServer.on("/chatgpt/delete/2", handle_chatgpt_delete);
    webServer.on("/chatgpt/delete/3", handle_chatgpt_delete);
    webServer.on("/chatgpt/delete/4", handle_chatgpt_delete);
#endif

    webServer.begin();
    logMessage("STARTUP: HTTP server started on port " + String(settings.http_port));
}

void send_chunked_html_response(const String& title, const std::function<void()>& body_generator) {

    if (!CHECK_HEAP(1024)) {
        webServer.send(503, "text/plain", "Insufficient memory");
        return;
    }

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");


    String header = get_html_header(title);
    webServer.sendContent(header);


    body_generator();


    String footer = get_html_footer();
    webServer.sendContent(footer);
    webServer.sendContent("");
}

String get_html_header(String title) {
    String theme_bg = "#FFFFFF";
    String theme_card = "#F2F2F2";
    String theme_text = "#222222";
    String theme_accent = "#222222";
    String theme_button = "#222222";
    String theme_button_hover = "#000000";
    String theme_input = "#FFFFFF";
    String theme_border = "#E0E0E0";
    String theme_nav_active = "#222222";
    String theme_nav_inactive = "#999999";
    String theme_background = "#FFFFFF";
    String theme_nav_hover = "#F0F0F0";

    if (settings.theme == 0) {
        theme_bg = "#FFFFFF";
        theme_card = "#F8F9FA";
        theme_text = "#222222";
        theme_accent = "#333333";
        theme_button = "#333333";
        theme_button_hover = "#000000";
        theme_input = "#FFFFFF";
        theme_border = "#E0E0E0";
        theme_nav_active = "#333333";
        theme_nav_inactive = "#999999";
        theme_background = "#FFFFFF";
        theme_nav_hover = "#F0F0F0";
    } else if (settings.theme == 1) {
        theme_bg = "#121212";
        theme_card = "#1E1E1E";
        theme_text = "#E0E0E0";
        theme_accent = "#FFFFFF";
        theme_button = "#404040";
        theme_button_hover = "#555555";
        theme_input = "#2A2A2A";
        theme_border = "#404040";
        theme_nav_active = "#404040";
        theme_nav_inactive = "#888888";
        theme_background = "#2A2A2A";
        theme_nav_hover = "#3A3A3A";
    }

    return "<!DOCTYPE html><html><head><title>" + title + "</title>"
           "<meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width, initial-scale=1'>"
           "<style>"
           ":root {"
           "  --theme-bg: " + theme_bg + ";"
           "  --theme-card: " + theme_card + ";"
           "  --theme-text: " + theme_text + ";"
           "  --theme-accent: " + theme_accent + ";"
           "  --theme-button: " + theme_button + ";"
           "  --theme-button-hover: " + theme_button_hover + ";"
           "  --theme-input: " + theme_input + ";"
           "  --theme-border: " + theme_border + ";"
           "  --theme-nav-active: " + theme_nav_active + ";"
           "  --theme-nav-inactive: " + theme_nav_inactive + ";"
           "  --theme-background: " + theme_background + ";"
           "  --theme-nav-hover: " + theme_nav_hover + ";"
           "}"
           "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 20px; background-color: var(--theme-bg); color: var(--theme-text); line-height: 1.6; transition: all 0.3s ease; }"
           ".container { max-width: 800px; margin: 0 auto; background-color: var(--theme-card); padding: 30px; border-radius: 16px; box-shadow: 0 8px 32px rgba(0,0,0,0.1); border: 1px solid var(--theme-border); transition: all 0.3s ease; }"
           ".header { text-align: center; margin-bottom: 30px; }"
           ".header h1 { color: var(--theme-accent); margin: 0; font-size: 2.2em; font-weight: 300; letter-spacing: -0.5px; transition: color 0.3s ease; }"
           ".header p { color: var(--theme-text); margin: 10px 0 0 0; opacity: 0.8; transition: color 0.3s ease; }"
           ".form-group { margin-bottom: 24px; }"
           ".form-group label { display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text); font-size: 14px; transition: color 0.3s ease; }"
           ".form-group input, .form-group select, .form-group textarea, .input-std { width: 100%; padding: 14px 16px; border: 2px solid var(--theme-border); border-radius: 12px; font-size: 16px; box-sizing: border-box; background-color: var(--theme-input); color: var(--theme-text); transition: all 0.3s ease; }"
           ".form-group input:focus, .form-group select:focus, .form-group textarea:focus, .input-std:focus { outline: none; border-color: var(--theme-accent); box-shadow: 0 0 0 3px var(--theme-accent)20; transform: translateY(-1px); }"
           ".button { background-color: var(--theme-button); color: white; padding: 14px 28px; border: none; border-radius: 12px; cursor: pointer; font-size: 16px; font-weight: 500; text-decoration: none; display: inline-block; margin: 8px 6px; transition: all 0.3s ease; }"
           ".button:hover { background-color: var(--theme-button-hover); transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.15); }"
           ".button.secondary { background-color: var(--theme-nav-inactive); color: var(--theme-text); }"
           ".button.secondary:hover { background-color: var(--theme-nav-active); color: white; }"
           ".button.success { background-color: #28a745 !important; color: white !important; }"
           ".button.success:hover { background-color: #1e7e34 !important; }"
           ".button.edit { background-color: #007bff !important; color: white !important; }"
           ".button.edit:hover { background-color: #0056b3 !important; }"
           ".button.danger { background-color: #dc3545 !important; color: white !important; }"
           ".button.danger:hover { background-color: #c82333 !important; }"
           ".nav { display: flex; justify-content: center; margin-bottom: 30px; background-color: transparent; border-bottom: 2px solid var(--theme-border); }"
           ".nav a { flex: 1; max-width: 150px; padding: 14px 8px; text-decoration: none; font-weight: 500; font-size: 13px; text-align: center; border-bottom: 3px solid transparent; transition: all 0.3s ease; color: var(--theme-nav-inactive); position: relative; }"
           ".nav a.tab-active { color: var(--theme-accent); border-bottom-color: var(--theme-accent); background-color: var(--theme-input); }"
           ".nav a.tab-inactive { color: var(--theme-nav-inactive); }"
           ".nav a.tab-inactive:hover { color: var(--theme-text); background-color: var(--theme-nav-hover); }"
           ".status { padding: 16px 20px; border-radius: 12px; margin: 24px 0; font-weight: 500; border: none; }"
           ".status.success { background-color: #10B981; color: white; box-shadow: 0 4px 12px rgba(16, 185, 129, 0.3); }"
           ".status.error { background-color: #EF4444; color: white; box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3); }"
           ".char-counter { font-size: 13px; color: var(--theme-nav-inactive); margin-top: 8px; font-weight: 500; transition: color 0.3s ease; }"
           ".progress-bar { width: 100%; height: 8px; background-color: var(--theme-border); border-radius: 6px; margin-top: 8px; overflow: hidden; transition: background-color 0.3s ease; }"
           ".progress-fill { height: 100%; background: linear-gradient(90deg, var(--theme-accent), var(--theme-button)); border-radius: 6px; transition: all 0.4s ease; }"
           "h3 { color: var(--theme-accent); font-weight: 500; font-size: 1.3em; margin-bottom: 16px; transition: color 0.3s ease; }"
           "p { margin-bottom: 12px; color: var(--theme-text); transition: color 0.3s ease; }"
           ".serial-log { background-color: var(--theme-input); border: 2px solid var(--theme-border); border-radius: 12px; padding: 20px; max-height: 300px; overflow-y: auto; font-family: 'Courier New', monospace; font-size: 13px; line-height: 1.4; transition: all 0.3s ease; }"
           ".serial-log div { margin-bottom: 6px; }"
           ".serial-log .timestamp { color: var(--theme-nav-inactive); font-weight: bold; transition: color 0.3s ease; }"
           "#temp-message { position: fixed; top: 20px; right: 20px; z-index: 1000; max-width: 400px; padding: 16px 20px; border-radius: 12px; font-weight: 500; box-shadow: 0 8px 32px rgba(0,0,0,0.2); transform: translateX(calc(100% + 50px)); transition: transform 0.4s ease; }"
           "#temp-message.show { transform: translateX(0); }"
           "#temp-message.success { background-color: #10B981; color: white; }"
           "#temp-message.error { background-color: #EF4444; color: white; }"
           "#temp-message.warning { background-color: #F59E0B; color: white; }"
           "#temp-message.info { background-color: #3B82F6; color: white; }"
           "input:-webkit-autofill, input:-webkit-autofill:hover, input:-webkit-autofill:focus, input:-webkit-autofill:active { -webkit-box-shadow: 0 0 0 1000px var(--theme-input) inset !important; -webkit-text-fill-color: var(--theme-text) !important; background-color: var(--theme-input) !important; }"
           "input:-moz-autofill { background-color: var(--theme-input) !important; color: var(--theme-text) !important; }"
           "input[type='text'] { background-color: var(--theme-input) !important; }"
           "select { background-color: var(--theme-input) !important; }"
           ".toggle-switch { position: relative; width: 50px; height: 24px; border-radius: 12px; cursor: pointer; transition: background-color 0.3s; display: inline-block; }"
           ".toggle-slider { position: absolute; top: 2px; width: 20px; height: 20px; background-color: white; border-radius: 50%; transition: left 0.3s; }"
           ".nav-status-enabled { color: #28a745 !important; }"
           ".nav-status-disabled { color: #dc3545 !important; }"
           ".flex-row { display: flex; gap: 12px; }"
           ".flex-row-wrap { display: flex; gap: 15px; flex-wrap: wrap; }"
           ".flex-space-between { display: flex; justify-content: space-between; align-items: center; }"
           ".flex-center { display: flex; align-items: center; gap: 12px; }"
           ".form-section { margin: 20px 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card); }"
           ".form-row { display: flex; gap: 15px; margin-bottom: 20px; }"
           ".form-col { flex: 1; margin-bottom: 0; }"
           ".text-large { font-size: 1.1em; font-weight: 500; }"
           ".mb-20 { margin-bottom: 20px; }"
           ".mb-0 { margin-bottom: 0; }"
           ".text-success { color: #28a745; }"
           ".text-danger { color: #dc3545; }"
           ".text-warning { color: #F59E0B; }"
           ".text-info { color: #3B82F6; }"
           ".text-muted { color: var(--theme-nav-inactive); }"
           ".toggle-switch.is-active { background-color: #28a745; }"
           ".toggle-switch.is-inactive { background-color: #ccc; }"
           ".toggle-slider.is-active { left: 26px; }"
           ".toggle-slider.is-inactive { left: 2px; }"
           ".button-compact { padding: 8px 12px; font-size: 0.85em; }"
           ".button-medium { padding: 10px 20px; font-size: 14px; }"
           ".button-large { padding: 15px 30px; font-size: 16px; font-weight: 500; }"
           ".modal { display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.7); }"
           ".modal.show { display: block; }"
           ".modal-content { background-color: var(--theme-card); margin: 15% auto; padding: 25px; border-radius: 12px; width: 400px; max-width: 90%; border: 2px solid var(--theme-border); box-shadow: 0 10px 30px rgba(0,0,0,0.3); }"
           ".grid-three { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 12px; margin-bottom: 12px; font-size: 0.9em; }"
           ".grid-two { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; }"
           ".alert { padding: 15px; margin: 20px; border-radius: 8px; border: 2px solid; }"
           ".alert-danger { background-color: var(--theme-card); color: #dc3545; border-color: #dc3545; }"
           ".card { background-color: var(--theme-card); border: 1px solid var(--theme-border); border-radius: 12px; padding: 20px; margin-bottom: 15px; position: relative; }"
           ".card-header { display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 12px; }"
           ".mt-20 { margin-top: 20px; }"
           ".text-right { text-align: right; }"
           ".flex-col { display: flex; flex-direction: column; gap: 6px; }"
           ".flex-align-center { display: flex; align-items: center; gap: 6px; }"
           ".grid-span-2 { grid-column: 1 / span 2; }"
           "</style>"
           "<script>"
           "function showTempMessage(message, type, duration) {"
           "  var msgDiv = document.getElementById('temp-message');"
           "  if (!msgDiv) {"
           "    msgDiv = document.createElement('div');"
           "    msgDiv.id = 'temp-message';"
           "    document.body.appendChild(msgDiv);"
           "  }"
           "  msgDiv.textContent = message;"
           "  msgDiv.className = type + ' show';"
           "  setTimeout(function() {"
           "    msgDiv.classList.remove('show');"
           "  }, duration || 5000);"
           "}"
           "var themes = ["
           "  {bg:'#FFFFFF',card:'#F8F9FA',text:'#222222',accent:'#333333',button:'#333333',buttonHover:'#000000',input:'#FFFFFF',border:'#E0E0E0',navActive:'#333333',navInactive:'#999999'},"
           "  {bg:'#121212',card:'#1E1E1E',text:'#E0E0E0',accent:'#FFFFFF',button:'#404040',buttonHover:'#555555',input:'#2A2A2A',border:'#404040',navActive:'#404040',navInactive:'#888888'}"
           "];"
           "function applyTheme(themeIndex) {"
           "  var theme = themes[themeIndex];"
           "  var root = document.documentElement;"
           "  root.style.setProperty('--theme-bg', theme.bg);"
           "  root.style.setProperty('--theme-card', theme.card);"
           "  root.style.setProperty('--theme-text', theme.text);"
           "  root.style.setProperty('--theme-accent', theme.accent);"
           "  root.style.setProperty('--theme-button', theme.button);"
           "  root.style.setProperty('--theme-button-hover', theme.buttonHover);"
           "  root.style.setProperty('--theme-input', theme.input);"
           "  root.style.setProperty('--theme-border', theme.border);"
           "  root.style.setProperty('--theme-nav-active', theme.navActive);"
           "  root.style.setProperty('--theme-nav-inactive', theme.navInactive);"
           "}"
           "function onThemeChange() {"
           "  var themeSelect = document.getElementById('theme');"
           "  if (themeSelect) {"
           "    applyTheme(parseInt(themeSelect.value));"
           "  }"
           "}"
           "function submitFormAjax(form, successMsg, restartMsg) {"
           "  var formData = new FormData(form);"
           "  fetch(form.action, { method: 'POST', body: formData })"
           "  .then(response => response.json())"
           "  .then(data => {"
           "    if (data.success) {"
           "      if (data.restart) {"
           "        showTempMessage(restartMsg || 'Settings saved, restarting in 5 seconds...', 'info', 5000);"
           "        setTimeout(() => location.reload(), 5000);"
           "      } else {"
           "        showTempMessage(successMsg || 'Settings saved successfully!', 'success', 5000);"
           "      }"
           "    } else {"
           "      showTempMessage('Error: ' + (data.error || 'Failed to save settings'), 'error', 5000);"
           "    }"
           "  })"
           "  .catch(() => showTempMessage('Network error occurred', 'error', 5000));"
           "  return false;"
           "}"
           "function pollLogs() {"
           "  fetch('/logs')"
           "    .then(response => response.json())"
           "    .then(data => {"
           "      if (data.logs && data.logs.length > 0) {"
           "        data.logs.forEach(log => {"
           "          if (log.timestamp > lastLogTimestamp) {"
           "            lastLogTimestamp = Math.max(lastLogTimestamp, log.timestamp);"
           "          }"
           "        });"
           "      }"
           "    })"
           "    .catch(() => {});"
           "}"
           "function updateServerSettings() {"
           "  var serverInput = document.getElementById('imap_server');"
           "  var portField = document.getElementById('imap_port');"
           "  var sslField = document.getElementById('imap_use_ssl');"
           "  "
           "  if (serverInput.value === 'imap.gmail.com' || serverInput.value === 'outlook.office365.com' || serverInput.value === 'imap.mail.yahoo.com') {"
           "    portField.value = '993';"
           "    sslField.value = '1';"
           "  }"
           "}"
           "function updateToggleVisual(checkbox) {"
           "  var toggle = checkbox.parentElement;"
           "  var slider = toggle.querySelector('.toggle-slider');"
           "  if (checkbox.checked) {"
           "    toggle.classList.add('is-active');"
           "    toggle.classList.remove('is-inactive');"
           "    slider.classList.add('is-active');"
           "    slider.classList.remove('is-inactive');"
           "  } else {"
           "    toggle.classList.add('is-inactive');"
           "    toggle.classList.remove('is-active');"
           "    slider.classList.add('is-inactive');"
           "    slider.classList.remove('is-active');"
           "  }"
           "}"
           "function universalToggleSwitch(switchElement, hiddenInputId, callback) {"
           "  var slider = switchElement.querySelector('.toggle-slider');"
           "  var hiddenInput = hiddenInputId ? document.getElementById(hiddenInputId) : null;"
           "  var currentEnabled = switchElement.classList.contains('is-active');"
           "  var newEnabled = !currentEnabled;"
           "  if (newEnabled) {"
           "    switchElement.classList.add('is-active');"
           "    switchElement.classList.remove('is-inactive');"
           "    slider.classList.add('is-active');"
           "    slider.classList.remove('is-inactive');"
           "  } else {"
           "    switchElement.classList.add('is-inactive');"
           "    switchElement.classList.remove('is-active');"
           "    slider.classList.add('is-inactive');"
           "    slider.classList.remove('is-active');"
           "  }"
           "  if (hiddenInput) hiddenInput.value = newEnabled ? '1' : '0';"
           "  if (callback) callback(newEnabled);"
           "  return newEnabled;"
           "}"
           "document.addEventListener('DOMContentLoaded', function() {"
           "  var toggles = document.querySelectorAll('.toggle-switch input[type=\"checkbox\"]');"
           "  toggles.forEach(function(toggle) {"
           "    toggle.addEventListener('change', function() {"
           "      updateToggleVisual(this);"
           "    });"
           "  });"
           "});"
           "</script>"
           "</head><body><div class='container'>";
}

String get_html_footer() {
    return "<div style='margin-top:40px;padding:20px 0;border-top:1px solid var(--theme-border);text-align:right;font-size:12px;color:var(--theme-nav-inactive);'>"
           "<a href='https://github.com/geekinsanemx/flex-fsk-tx' target='_blank' style='color:var(--theme-accent);text-decoration:none;'>geekinsanemx</a>"
           " | <span style='color:var(--theme-nav-inactive);'>" + String(FIRMWARE_VERSION) + "</span>"
           "</div></div></body></html>";
}

void handle_root() {
    reset_oled_timeout();
    feed_watchdog();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("FLEX Paging Message Transmitter");

    chunk += "<div class='header'>"
            "<h1>FLEX Paging Message Transmitter</h1>"
            ""
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-active'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            + chatgpt_nav_tab_html(false) +
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            + imap_nav_tab_html(false) +
            gsm_nav_tab_html(false) +
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div id='temp-message'></div>"

            "<div class='form-section'>"
            "<form id='send-form' action='/send' method='post'>"
            "<div class='form-row'>"
            "<div class='form-group form-col'>"
            "<label for='capcode'>🎯 Capcode:</label>"
            "<input type='number' id='capcode' name='capcode' value='" + String(settings.default_capcode) + "' min='1' max='4291000000' required>"
            "</div>"
            "<div class='form-group form-col'>"
            "<label for='frequency'>📻 Frequency (MHz):</label>"
            "<input type='number' id='frequency' name='frequency' value='" + String(settings.default_frequency, 4) + "' min='400' max='1000' step='0.0001' required>"
            "</div>"
            "<div class='form-group form-col'>"
            "<label for='power'>⚡ TX Power (dBm):</label>"
            "<input type='number' id='power' name='power' value='" + String(settings.default_txpower) + "' min='0' max='20' required>"
            "</div>"
            "</div>"

            "<div class='form-group'>"
            "<label for='message'>💬 Message:</label>"
            "<textarea id='message' name='message' rows='4' maxlength='" + String(MAX_FLEX_MESSAGE_LENGTH) + "' placeholder='Enter your FLEX message here...' required oninput='updateCharCounter()'></textarea>"
            "<div class='char-counter'>Characters: <span id='char-count'>0</span>/" + String(MAX_FLEX_MESSAGE_LENGTH) + "</div>"
            "<div class='progress-bar'><div class='progress-fill' id='char-progress'></div></div>"
            "</div>"

            "<div class='flex-row'>"
            "<button type='button' onclick='submitSendForm(event, false)' class='button success'>📤 Send Message</button>"
            "<button type='button' onclick='submitSendForm(event, true)' class='button edit'>📧 Send as MailDrop</button>"
            "</div>"
            "</form>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<script>"
            "function updateCharCounter() {"
            "    const message = document.getElementById('message');"
            "    const charCount = document.getElementById('char-count');"
            "    const charProgress = document.getElementById('char-progress');"
            "    const currentLength = message.value.length;"
            "    const maxLength = " + String(MAX_FLEX_MESSAGE_LENGTH) + ";"
            "    const truncateThreshold = 245;"
            "    "
            "    if (currentLength > maxLength) {"
            "        charCount.textContent = currentLength + ' (will be truncated to ' + maxLength + ')';"
            "        charCount.style.color = '#ff4444';"
            "    } else if (currentLength > truncateThreshold) {"
            "        charCount.textContent = currentLength + ' (truncation at ' + maxLength + ')';"
            "        charCount.style.color = '#ff8800';"
            "    } else {"
            "        charCount.textContent = currentLength;"
            "        charCount.style.color = 'var(--theme-nav-inactive)';"
            "    }"
            "    "
            "    const percentage = Math.min((currentLength / maxLength) * 100, 100);"
            "    charProgress.style.width = percentage + '%';"
            "    "
            "    if (currentLength > maxLength) {"
            "        charProgress.style.backgroundColor = '#ff4444';"
            "    } else if (currentLength > truncateThreshold) {"
            "        charProgress.style.backgroundColor = '#ff8800';"
            "    } else if (currentLength > (maxLength * 0.63)) {"
            "        charProgress.style.backgroundColor = '#ffa726';"
            "    } else {"
            "        charProgress.style.backgroundColor = '#667eea';"
            "    }"
            "}"
            "updateCharCounter();"
            ""
            "let lastSendTime = 0;"
            "const DEBOUNCE_TIME = 10000;"
            ""
            "function submitSendForm(event, isMailDrop) {"
            "  event.preventDefault();"
            "  "
            "  const now = Date.now();"
            "  if (now - lastSendTime < DEBOUNCE_TIME) {"
            "    const waitTime = Math.ceil((DEBOUNCE_TIME - (now - lastSendTime))/1000);"
            "    showTempMessage('⏳ Please wait ' + waitTime + ' seconds before sending another message', 'warning', 3000);"
            "    return;"
            "  }"
            "  lastSendTime = now;"
            "  "
            "  var form = document.getElementById('send-form');"
            "  var formData = new FormData(form);"
            "  if (isMailDrop) formData.append('mail_drop', '1');"
            "  "
            "  fetch('/send', {"
            "    method: 'POST',"
            "    body: formData"
            "  })"
            "  .then(response => response.json())"
            "  .then(data => {"
            "    if (data.success) {"
            "      showTempMessage('✅ Message sent successfully!', 'success', 5000);"
            "    } else {"
            "      showTempMessage('❌ Error: ' + (data.error || 'Failed to send message'), 'error', 5000);"
            "    }"
            "  })"
            "  .catch(error => {"
            "    showTempMessage('❌ Network error occurred', 'error', 5000);"
            "  });"
            "  return false;"
            "}"
            "</script>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_send_message() {
    reset_oled_timeout();

    if (!webServer.hasArg("frequency") || !webServer.hasArg("power") ||
        !webServer.hasArg("capcode") || !webServer.hasArg("message")) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Missing required parameters\"}");
        return;
    }

    float frequency = webServer.arg("frequency").toFloat();
    int power = webServer.arg("power").toInt();
    uint64_t capcode = strtoull(webServer.arg("capcode").c_str(), NULL, 10);
    String message = webServer.arg("message");
    bool mail_drop = webServer.hasArg("mail_drop");

    if (frequency < 400.0 || frequency > 1000.0) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Frequency must be between 400.0 and 1000.0 MHz\"}");
        return;
    }

    if (power < 0 || power > 20) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"TX Power must be between 0 and 20 dBm\"}");
        return;
    }

    if (!validate_flex_capcode(capcode)) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid FLEX capcode. Valid ranges: 1-1933312, 1998849-2031614, 2101249-4291000000\"}");
        return;
    }

    bool message_was_truncated = false;
    if (message.length() > MAX_FLEX_MESSAGE_LENGTH) {
        message = truncate_message_with_ellipsis(message);
        message_was_truncated = true;
    }

    if (queue_add_message(capcode, frequency, power, mail_drop, message.c_str())) {
        String response_message;
        if (message_was_truncated) {
            if (device_state == STATE_IDLE) {
                response_message = "Message truncated to 248 chars and queued for immediate transmission";
            } else {
                response_message = "Message truncated to 248 chars and queued for transmission (position " + String(queue_count) + ")";
            }
        } else {
            if (device_state == STATE_IDLE) {
                response_message = "Message queued for immediate transmission";
            } else {
                response_message = "Message queued for transmission (position " + String(queue_count) + ")";
            }
        }

        int status_code = (device_state == STATE_IDLE) ? 200 : 202;
        webServer.send(status_code, "application/json", "{\"success\":true,\"message\":\"" + response_message + "\"}");
    } else {
        webServer.send(503, "application/json", "{\"success\":false,\"message\":\"Queue is full. Please try again later.\"}");
    }
}

bool authenticate_api_request() {
    unsigned long now = millis();


    if (auth_failures >= MAX_AUTH_FAILURES) {
        if (now - last_auth_attempt < AUTH_LOCKOUT_TIME) {
            return false;
        } else {
            auth_failures = 0;
        }
    }

    last_auth_attempt = now;

    if (!webServer.hasHeader("Authorization")) {
        auth_failures++;
        return false;
    }

    String auth_header = webServer.header("Authorization");
    if (!auth_header.startsWith("Basic ")) {
        return false;
    }

    String encoded_credentials = auth_header.substring(6);
    String decoded_credentials = base64_decode(encoded_credentials);

    int colon_index = decoded_credentials.indexOf(':');
    if (colon_index == -1) {
        return false;
    }

    String username = decoded_credentials.substring(0, colon_index);
    String password = decoded_credentials.substring(colon_index + 1);

    bool username_valid = (username.length() == strlen(settings.api_username));
    bool password_valid = (password.length() == strlen(settings.api_password));

    if (username_valid) {
        for (size_t i = 0; i < username.length(); i++) {
            if (username[i] != settings.api_username[i]) {
                username_valid = false;
            }
        }
    }

    if (password_valid) {
        for (size_t i = 0; i < password.length(); i++) {
            if (password[i] != settings.api_password[i]) {
                password_valid = false;
            }
        }
    }

    bool authenticated = username_valid && password_valid;
    if (!authenticated) {
        auth_failures++;
    } else {
        auth_failures = 0;
    }

    return authenticated;
}
