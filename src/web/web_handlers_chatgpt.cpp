/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Web Server Module - ChatGPT scheduler page and prompt CRUD handlers
 */

#include "../web/web_server.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../core/display.h"
#include "../core/hardware.h"
#include "../network/wifi.h"
#include "../network/gsm.h"
#include "../services/mqtt.h"
#include "../services/imap.h"
#include "../services/chatgpt.h"
#include "../protocol/flex_protocol.h"
#include <WiFi.h>
#include <ArduinoJson.h>

#ifdef ENABLE_CHATGPT

void handle_chatgpt() {
    reset_oled_timeout();
    feed_watchdog();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    webServer.sendContent(get_html_header("ChatGPT Scheduler"));

    String chunk = "<div class='header'>"
                   "<h1>ChatGPT Scheduler</h1>"
                   "</div>";
    webServer.sendContent(chunk);

    chunk = "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            + chatgpt_nav_tab_html(true) +
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            + imap_nav_tab_html(false) +
            gsm_nav_tab_html(false) +
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";
    webServer.sendContent(chunk);

    chunk = "<div id='temp-message'></div>";
    webServer.sendContent(chunk);

    webServer.sendContent(chunk);
    feed_watchdog();

    chunk = "<div class='form-section'>"
            "<div class='flex-space-between mb-20'>"
            "<div class='flex-center'>"
            "<span style='text-large'>Enable ChatGPT</span>"
            "<div class='toggle-switch " + String(chatgpt_config.enabled ? "is-active" : "is-inactive") + "' onclick='toggleChatGPT()'>"
            "<div class='toggle-slider " + String(chatgpt_config.enabled ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "</div>";
    bool hasApiKey = strlen(chatgpt_config.api_key_b64) > 0;
    chunk += "<div class='flex-center'>"
             "<button type='button' onclick='showApiKeyModal()' class='button edit button-compact'>"
             "🔑 OpenAI API Key" + String(hasApiKey ? " <span class='text-success'>✓</span>" : "") + "</button>"
             "</div>"
             "</div>";
    chunk += "<div class='flex-center'>"
             "<span style='text-large'>Failure notifications</span>"
             "<div class='toggle-switch " + String(chatgpt_config.chatgpt_notify_failures ? "is-active" : "is-inactive") + "' onclick='toggleChatGPTNotifications()'>"
             "<div class='toggle-slider " + String(chatgpt_config.chatgpt_notify_failures ? "is-active" : "is-inactive") + "'></div>"
             "</div>"
             "</div>"
             "</div>";
    webServer.sendContent(chunk);
    feed_watchdog();

    chunk = "<div id='apiKeyModal' style='display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.7);'>"
            "<div style='background-color: var(--theme-card); margin: 15% auto; padding: 25px; border-radius: 12px; width: 400px; max-width: 90%; border: 2px solid var(--theme-border); box-shadow: 0 10px 30px rgba(0,0,0,0.3);'>"
            "<h3 style='margin-top: 0; color: var(--theme-text); text-align: center;'>🔑 Enter ChatGPT API Key</h3>"
            "<input type='password' id='modalApiKey' placeholder='sk-...' style='width: 100%; padding: 12px; margin: 15px 0; border: 2px solid var(--theme-border); border-radius: 8px; background-color: var(--theme-input); color: var(--theme-text); font-size: 14px; box-sizing: border-box;'>"
            "<div style='text-align: right; margin-top: 20px;'>"
            "<button onclick='closeApiKeyModal()' class='button danger button-medium' style='margin-right: 10px;'>Cancel</button>"
            "<button onclick='saveApiKey()' class='button success button-medium'>Save</button>"
            "</div>"
            "</div>"
            "</div>";
    webServer.sendContent(chunk);
    feed_watchdog();

    chunk = "<div class='form-section'>"
            "<h3>📅 Scheduled Prompts (" + String(chatgpt_config.prompts.size()) + "/" + String(MAX_CHATGPT_PROMPTS) + ")</h3>";
    if (chatgpt_config.prompts.size() == 0) {
        chunk += "<div style='text-align: center; padding: 40px; color: #666;'>"
                 "<p>No prompts configured yet.</p>"
                 "<p>Click 'Add New Prompt' to get started.</p>"
                 "</div>";
    }
    webServer.sendContent(chunk);
    feed_watchdog();

    if (chatgpt_config.prompts.size() > 0) {
        for (size_t i = 0; i < chatgpt_config.prompts.size(); i++) {
            ChatGPTPrompt& prompt = chatgpt_config.prompts[i];

            chunk = "<div class='form-section' style='margin: 15px 0; position: relative; border: 2px solid var(--theme-border); border-radius: 8px; padding: 15px; background-color: var(--theme-card);'>"
                    "<div style='display: flex; justify-content: space-between; align-items: flex-start;'>"
                    "<div style='flex: 1; padding-right: 15px;'>";

            String days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            String schedule_days = "";
            bool any_day = false;
            for (int d = 0; d < 7; d++) {
                if (prompt.days[d]) {
                    if (any_day) schedule_days += ", ";
                    schedule_days += days[d];
                    any_day = true;
                }
            }
            if (!any_day) schedule_days = "Never";

            char time_str[6];
            sprintf(time_str, "%02d:%02d", prompt.hour, prompt.minute);

            chunk += "<div style='display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 12px; margin-bottom: 12px; font-size: 0.9em;'>"
                     "<div style='display: flex; align-items: center; gap: 6px;'>"
                     "<span class='" + String(prompt.enabled ? "text-success" : "text-danger") + "'>" + String(prompt.enabled ? "✅" : "❌") + "</span><strong>Status:</strong> <span class='" + String(prompt.enabled ? "text-success" : "text-danger") + "'>" + String(prompt.enabled ? "Enabled" : "Disabled") + "</span>"
                     "</div>"
                     "<div></div>"
                     "<div style='display: flex; align-items: center; gap: 6px;'>"
                     "<span>" + String(prompt.mail_drop ? "📧" : "📟") + "</span><strong>Capcode:</strong> " + String(prompt.capcode) +
                     "</div>"
                     "<div style='display: flex; align-items: center; gap: 6px; grid-column: 1 / span 2;'>"
                     "<span>📅</span><strong>Schedule:</strong> " + schedule_days + " at " + String(time_str) +
                     "</div>"
                     "<div style='display: flex; align-items: center; gap: 6px;'>"
                     "<span>📡</span><strong>Frequency:</strong> " + String(prompt.frequency, 4) + " MHz" +
                     "</div>"
                     "</div>";

            chunk += "<div style='background: var(--theme-input); padding: 10px; border-radius: 6px; margin-bottom: 24px;'>"
                     "<p style='margin: 0; font-style: italic; color: var(--theme-text); font-size: 0.95em;'>\"" + String(prompt.prompt) + "\"</p>"
                     "</div>";

            String next_execution_info = "🕒 <strong>Next execution:</strong> " + chatgpt_format_next_execution(prompt);

            chunk += "</div>"
                     "<div style='display: flex; flex-direction: column; gap: 6px;'>"
                     "<button onclick='editPrompt(" + String(i) + ")' class='button edit button-compact'>✏️ Edit</button>"
                     "<button onclick='deletePrompt(" + String(i) + ")' class='button danger button-compact'>🗑️ Delete</button>"
                     "</div>"
                     "</div>"
                     "<div style='position: absolute; bottom: 8px; left: 12px; font-size: 0.85em; color: var(--theme-nav-inactive);'>" + next_execution_info + "</div>"
                     "<div style='position: absolute; bottom: 8px; right: 12px; font-size: 0.85em; color: var(--theme-nav-inactive); font-weight: bold;'>#" + String(i + 1) + "</div>"
                     "</div>";
            webServer.sendContent(chunk);
            feed_watchdog();
        }
    }

    chunk = "";
    if (chatgpt_config.prompts.size() < MAX_CHATGPT_PROMPTS) {
        chunk = "<button onclick='addNewPrompt()' class='button success' style='width: 100%; padding: 12px; margin-top: 20px;'>➕ Add New Prompt</button>";
    }
    chunk += "<div id='new-prompt-form' style='display: none; margin-top: 20px; padding: 20px; border: 2px solid var(--theme-border); border-radius: 12px; background-color: var(--theme-input);'>"
             "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 18px;'>"
             "<h4 id='form-title'>📝 New Prompt Configuration</h4>"
             "<div id='prompt-enable-toggle' style='display: none; flex-direction: row; align-items: center; gap: 8px;'>"
             "<div class='toggle-switch is-active' onclick='toggleFormPromptEnabled()'>"
             "<div class='toggle-slider is-active' id='form-toggle-slider'></div>"
             "</div>"
             "<label id='enable-label' style='cursor: pointer; color: var(--theme-text); font-weight: 500;'>Enable</label>"
             "</div>"
             "</div>"
             "<div class='form-group'>"
             "<label for='new-prompt-text'>Prompt Text (max 250 chars):</label>"
             "<textarea id='new-prompt-text' maxlength='250' rows='3' style='width: 100%; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text);'></textarea>"
             "<div id='prompt-char-count' style='text-align: right; font-size: 12px; color: #666; margin-top: 4px;'>0/250</div>"
             "</div>"
             "<div class='form-group'>"
             "<label>Days of Week:</label>"
             "<div style='display: grid; grid-template-columns: repeat(7, 1fr); gap: 8px; margin: 8px 12px 0 12px;'>"
             "<label style='display: flex; align-items: center; justify-content: center; gap: 4px; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-input); color: var(--theme-text); cursor: pointer; text-align: center; font-size: 0.9em;'><input type='checkbox' id='day-sun' value='1' style='margin: 0;'> Sun</label>"
             "<label style='display: flex; align-items: center; justify-content: center; gap: 4px; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-input); color: var(--theme-text); cursor: pointer; text-align: center; font-size: 0.9em;'><input type='checkbox' id='day-mon' value='2' style='margin: 0;'> Mon</label>"
             "<label style='display: flex; align-items: center; justify-content: center; gap: 4px; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-input); color: var(--theme-text); cursor: pointer; text-align: center; font-size: 0.9em;'><input type='checkbox' id='day-tue' value='4' style='margin: 0;'> Tue</label>"
             "<label style='display: flex; align-items: center; justify-content: center; gap: 4px; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-input); color: var(--theme-text); cursor: pointer; text-align: center; font-size: 0.9em;'><input type='checkbox' id='day-wed' value='8' style='margin: 0;'> Wed</label>"
             "<label style='display: flex; align-items: center; justify-content: center; gap: 4px; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-input); color: var(--theme-text); cursor: pointer; text-align: center; font-size: 0.9em;'><input type='checkbox' id='day-thu' value='16' style='margin: 0;'> Thu</label>"
             "<label style='display: flex; align-items: center; justify-content: center; gap: 4px; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-input); color: var(--theme-text); cursor: pointer; text-align: center; font-size: 0.9em;'><input type='checkbox' id='day-fri' value='32' style='margin: 0;'> Fri</label>"
             "<label style='display: flex; align-items: center; justify-content: center; gap: 4px; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-input); color: var(--theme-text); cursor: pointer; text-align: center; font-size: 0.9em;'><input type='checkbox' id='day-sat' value='64' style='margin: 0;'> Sat</label>"
             "</div>"
             "</div>"
             "<div style='display: flex; gap: 15px;'>"
             "<div class='form-group' style='flex: 1;'>"
             "<label for='new-prompt-time'>Time (HH:MM):</label>"
             "<input type='time' id='new-prompt-time' style='width: 100%; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px;'>"
             "</div>"
             "<div class='form-group' style='flex: 1;'>"
             "<label for='new-prompt-capcode'>Capcode:</label>"
             "<input type='number' id='new-prompt-capcode' min='1' max='4291000000' value='" + String(settings.default_capcode) + "' style='width: 100%; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px;'>"
             "</div>"
             "<div class='form-group' style='flex: 1;'>"
             "<label for='new-prompt-frequency'>Frequency (MHz):</label>"
             "<input type='number' id='new-prompt-frequency' step='0.0001' min='400' max='1000' value='" + String(settings.default_frequency, 4) + "' style='width: 100%; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px;'>"
             "</div>"
             "<div class='form-group' style='flex: 0 0 auto; min-width: 90px;'>"
             "<label for='new-prompt-maildrop' style='display: block; margin-bottom: 8px;'>Mail Drop:</label>"
             "<select id='new-prompt-maildrop' style='width: 100%; padding: 8px; border: 1px solid var(--theme-border); border-radius: 6px; background-color: var(--theme-background); color: var(--theme-text);'><option value='0'>No</option><option value='1'>Yes</option></select>"
             "</div>"
             "</div>"
             "<div style='display: flex; gap: 10px; margin-top: 20px;'>"
             "<button id='save-button' onclick='saveNewPrompt()' class='button success button-medium'>✅ Save Prompt</button>"
             "<button onclick='cancelNewPrompt()' class='button danger button-medium'>❌ Cancel</button>"
             "</div>"
             "</div>";
    chunk += "</div>";
    webServer.sendContent(chunk);
    feed_watchdog();

    chunk = "<div class='form-section'>"
            "<h3>📊 Recent Activity</h3>";
    if (chatgpt_activity_count == 0) {
        chunk += "<div style='text-align: center; padding: 20px; color: #666;'>"
                 "<p>No ChatGPT executions yet. Activities will appear here after scheduled prompts run.</p>"
                 "</div>";
    }
    webServer.sendContent(chunk);
    feed_watchdog();

    if (chatgpt_activity_count > 0) {
        for (int i = chatgpt_activity_count - 1; i >= 0; i--) {
            ChatGPTActivity& activity = chatgpt_activity_log[i];
            String queryStatus = activity.query_success ? "✅" : "❌";
            String transmissionStatus = activity.transmission_success ? "✅" : "❌";

            chunk = "<div style='border: 1px solid var(--theme-border); border-radius: 8px; padding: 12px; margin-bottom: 10px; background-color: var(--theme-input);'>"
                    "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;'>"
                    "<div style='font-weight: bold; color: var(--theme-text);'>" + queryStatus + " Query " + transmissionStatus + " Transmission</div>"
                    "<div style='font-size: 0.9em; color: var(--theme-nav-inactive);'>#" + String(activity.prompt_index) + " | " + String(activity.mail_drop ? "📧" : "📟") + " " + String(activity.capcode) + " | 📡 " + String(activity.frequency, 4) + " MHz | " + String(activity.datetime) + "</div>"
                    "</div>"
                    "<div style='color: var(--theme-text);'>" + String(activity.response).substring(0, 80) + (strlen(activity.response) > 80 ? "..." : "") + "</div>"
                    "</div>";
            webServer.sendContent(chunk);
            feed_watchdog();
        }
    }

    chunk = "</div>";
    webServer.sendContent(chunk);
    feed_watchdog();

    chunk = "<script>"
            "function addNewPrompt() {"
            "  document.getElementById('new-prompt-form').style.display = 'block';"
            "  document.querySelector('button[onclick=\\\"addNewPrompt()\\\"]').style.display = 'none';"
            "  document.getElementById('prompt-enable-toggle').style.display = 'none';"
            "  document.getElementById('new-prompt-text').focus();"
            "}"
            ""
            "function cancelNewPrompt() {"
            "  document.getElementById('new-prompt-form').style.display = 'none';"
            "  document.querySelector('button[onclick=\\\"addNewPrompt()\\\"]').style.display = 'block';"
            "  document.getElementById('form-title').textContent = '📝 New Prompt Configuration';"
            "  document.getElementById('prompt-enable-toggle').style.display = 'none';"
            "  document.getElementById('save-button').textContent = '✅ Save Prompt';"
            "  editingIndex = -1;"
            "  clearNewPromptForm();"
            "}"
            ""
            "function clearNewPromptForm() {"
            "  document.getElementById('new-prompt-text').value = '';"
            "  document.getElementById('new-prompt-time').value = '';"
            "  document.getElementById('new-prompt-capcode').value = " + String(settings.default_capcode) + ";"
            "  document.getElementById('new-prompt-frequency').value = " + String(settings.default_frequency, 4) + ";"
            "  document.getElementById('new-prompt-maildrop').value = '0';"
            ""
            "  document.querySelectorAll('#new-prompt-form input[type=\\\"checkbox\\\"][id^=\\\"day-\\\"]').forEach(cb => cb.checked = false);"
            "  updateCharCount();"
            "}"
            "function updateCharCount() {"
            "  const text = document.getElementById('new-prompt-text').value;"
            "  document.getElementById('prompt-char-count').textContent = text.length + '/250';"
            "}"
            "function saveNewPrompt() {"
            "  const promptText = document.getElementById('new-prompt-text').value.trim();"
            "  const time = document.getElementById('new-prompt-time').value;"
            "  const capcode = document.getElementById('new-prompt-capcode').value;"
            "  const frequency = document.getElementById('new-prompt-frequency').value;"
            "  const mailDrop = document.getElementById('new-prompt-maildrop').value === '1';"
            "  const formToggle = document.getElementById('form-toggle-slider');"
            "  const enabled = editingIndex >= 0 ? (formToggle && formToggle.style.left === '26px') : true;"
            "  "
            "  if (!promptText) { alert('Please enter prompt text'); return; }"
            "  if (!time) { alert('Please select a time'); return; }"
            "  if (!capcode || capcode < 1) { alert('Please enter a valid capcode'); return; }"
            "  if (!frequency || frequency < 400 || frequency > 1000) { alert('Please enter frequency between 400-1000 MHz'); return; }"
            "  "
            "  let dayMask = 0;"
            "  document.querySelectorAll('#new-prompt-form input[type=\\\"checkbox\\\"][id^=\\\"day-\\\"]:checked').forEach(cb => {"
            "    dayMask |= parseInt(cb.value);"
            "  });"
            "  if (dayMask === 0) { alert('Please select at least one day'); return; }"
            "  "
            "  const data = {"
            "    prompt: promptText,"
            "    days: dayMask,"
            "    time: time,"
            "    capcode: parseInt(capcode),"
            "    frequency: parseFloat(frequency),"
            "    mail_drop: mailDrop,"
            "    enabled: enabled"
            "  };"
            "  "
            "  const url = editingIndex >= 0 ? '/chatgpt/edit_prompt/' + editingIndex : '/chatgpt/add_prompt';"
            "  const successMsg = editingIndex >= 0 ? 'Prompt updated successfully!' : 'Prompt added successfully!';"
            "  "
            "  fetch(url, {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/json' },"
            "    body: JSON.stringify(data)"
            "  })"
            "  .then(response => response.json())"
            "  .then(result => {"
            "    if (result.success) {"
            "      showTempMessage(successMsg, 'success', 5000);"
            "      setTimeout(() => location.reload(), 1000);"
            "    } else {"
            "      showTempMessage('Error: ' + (result.error || 'Failed to add prompt'), 'error', 5000);"
            "    }"
            "  })"
            "  .catch(error => {"
            "    showTempMessage('Network error occurred', 'error', 5000);"
            "  });"
            "}";
    webServer.sendContent(chunk);
    feed_watchdog();

    chunk = ""
            "let editingIndex = -1;"
            ""
            "function editPrompt(index) {"
            "  fetch('/chatgpt/get_prompt/' + index)"
            "  .then(response => response.json())"
            "  .then(data => {"
            "    if (data.success) {"
            "      editingIndex = index;"
            "      fillEditForm(data.prompt);"
            "      document.getElementById('new-prompt-form').style.display = 'block';"
            "      document.querySelector('button[onclick=\\\"addNewPrompt()\\\"]').style.display = 'none';"
            "      document.getElementById('form-title').textContent = '✏️ Edit Prompt #' + (index + 1);"
            "      document.getElementById('prompt-enable-toggle').style.display = 'flex';"
            "      document.getElementById('enable-label').textContent = 'Enable';"
            "      document.getElementById('save-button').textContent = '💾 Update Prompt';"
            "      document.getElementById('new-prompt-text').focus();"
            "    } else {"
            "      showTempMessage('Error loading prompt: ' + data.error, 'error', 5000);"
            "    }"
            "  })"
            "  .catch(error => {"
            "    showTempMessage('Network error occurred', 'error', 5000);"
            "  });"
            "}"
            ""
            "function fillEditForm(prompt) {"
            "  document.getElementById('new-prompt-text').value = prompt.prompt;"
            "  document.getElementById('new-prompt-time').value = prompt.time;"
            "  document.getElementById('new-prompt-capcode').value = prompt.capcode;"
            "  document.getElementById('new-prompt-frequency').value = prompt.frequency.toFixed(4);"
            "  document.getElementById('new-prompt-maildrop').value = prompt.mail_drop ? '1' : '0';"
            "  const formToggle = document.getElementById('form-toggle-slider');"
            "  const formToggleSwitch = formToggle ? formToggle.parentElement : null;"
            "  if (formToggle && formToggleSwitch) {"
            "    formToggle.style.left = prompt.enabled ? '26px' : '2px';"
            "    formToggleSwitch.style.backgroundColor = prompt.enabled ? '#28a745' : '#ccc';"
            "  }"
            "  document.querySelectorAll('#new-prompt-form input[type=\\\"checkbox\\\"][id^=\\\"day-\\\"]').forEach(cb => cb.checked = false);"
            "  "
            "  const dayIds = ['day-sun', 'day-mon', 'day-tue', 'day-wed', 'day-thu', 'day-fri', 'day-sat'];"
            "  for (let i = 0; i < prompt.days.length; i++) {"
            "    if (prompt.days[i]) {"
            "      document.getElementById(dayIds[i]).checked = true;"
            "    }"
            "  }"
            "  updateCharCount();"
            "}"
            ""
            "function toggleFormPromptEnabled() {"
            "  const slider = document.getElementById('form-toggle-slider');"
            "  const toggle = slider.parentElement;"
            "  const currentlyEnabled = slider.style.left === '26px';"
            "  const newState = !currentlyEnabled;"
            "  "
            "  slider.style.left = newState ? '26px' : '2px';"
            "  toggle.style.backgroundColor = newState ? '#28a745' : '#ccc';"
            "}"
            ""
            "function deletePrompt(index) {"
            "  if (confirm('Delete this prompt?')) {"
            "    window.location = '/chatgpt/delete/' + index;"
            "  }"
            "}"
            ""
            "document.addEventListener('DOMContentLoaded', function() {"
            "  const textArea = document.getElementById('new-prompt-text');"
            "  if (textArea) {"
            "    textArea.addEventListener('input', updateCharCount);"
            "    updateCharCount();"
            "  }"
            "});";
    webServer.sendContent(chunk);
    feed_watchdog();

    chunk = ""
            ""
            "function showApiKeyModal() {"
            "  document.getElementById('apiKeyModal').style.display = 'block';"
            "  document.getElementById('modalApiKey').focus();"
            "  document.getElementById('modalApiKey').placeholder = 'Enter new API key (existing key will be replaced)';"
            "}"
            ""
            "function closeApiKeyModal() {"
            "  document.getElementById('apiKeyModal').style.display = 'none';"
            "  document.getElementById('modalApiKey').value = '';"
            "}"
            ""
            "function saveApiKey() {"
            "  const apiKey = document.getElementById('modalApiKey').value.trim();"
            "  "
            "  if (apiKey === '') {"
            "    alert('Please enter an API key.');"
            "    return;"
            "  }"
            "  "
            "  if (!apiKey.startsWith('sk-') || apiKey.length < 20) {"
            "    alert('Invalid API key format.\\\\n\\\\nPlease enter a valid OpenAI API key (starts with \\\"sk-\\\" and at least 20 characters).');"
            "    return;"
            "  }"
            "  "
            "  fetch('/chatgpt/config', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
            "    body: 'api_key=' + encodeURIComponent(apiKey)"
            "  })"
            "  .then(response => {"
            "    if (response.ok) {"
            "      const button = document.querySelector('button[onclick=\\\"showApiKeyModal()\\\"]');"
            "      button.innerHTML = '🔑 OpenAI API Key <span style=\\\"color: #28a745;\\\">✓</span>';"
            "      closeApiKeyModal();"
            "      showTempMessage('API key saved successfully!', 'success', 3000);"
            "    } else {"
            "      alert('Failed to save API key');"
            "    }"
            "  })"
            "  .catch(error => {"
            "    alert('Network error occurred');"
            "  });"
            "}";
    webServer.sendContent(chunk);
    feed_watchdog();

    chunk = ""
            "function toggleChatGPT() {"
            "  const toggleSwitches = document.querySelectorAll('.toggle-switch');"
            "  const toggleSwitch = toggleSwitches[0];"
            "  const toggleSlider = toggleSwitch.querySelector('.toggle-slider');"
            "  const hasApiKey = document.querySelector('button[onclick=\\\"showApiKeyModal()\\\"]').innerHTML.includes('✓');"
            "  "
            "  const isCurrentlyEnabled = toggleSlider.style.left === '26px';"
            "  "
            "  if (!isCurrentlyEnabled && !hasApiKey) {"
            "    alert('Please add an API key first before enabling ChatGPT.');"
            "    return;"
            "  }"
            "  "
            "  const newState = !isCurrentlyEnabled;"
            "  "
            "  fetch('/chatgpt/config', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
            "    body: newState ? 'enabled=1' : ''"
            "  })"
            "  .then(response => {"
            "    if (response.ok) {"
            "      toggleSlider.style.left = newState ? '26px' : '2px';"
            "      toggleSwitch.style.backgroundColor = newState ? '#28a745' : '#ccc';"
            "      showTempMessage('ChatGPT ' + (newState ? 'enabled' : 'disabled') + ' successfully!', 'success', 3000);"
            "    } else {"
            "      alert('Failed to update setting');"
            "    }"
            "  })"
            "  .catch(error => {"
            "    alert('Network error occurred');"
            "  });"
            "}"
            ""
            "function toggleChatGPTNotifications() {"
            "  const toggleSwitches = document.querySelectorAll('.toggle-switch');"
            "  const toggleSwitch = toggleSwitches[1];"
            "  const toggleSlider = toggleSwitch.querySelector('.toggle-slider');"
            "  "
            "  const isCurrentlyEnabled = toggleSlider.style.left === '26px';"
            "  const newState = !isCurrentlyEnabled;"
            "  "
            "  fetch('/chatgpt/notifications', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
            "    body: newState ? 'chatgpt_notify_failures=1' : ''"
            "  })"
            "  .then(response => {"
            "    if (response.ok) {"
            "      toggleSlider.style.left = newState ? '26px' : '2px';"
            "      toggleSwitch.style.backgroundColor = newState ? '#28a745' : '#ccc';"
            "      showTempMessage('Failure notifications ' + (newState ? 'enabled' : 'disabled') + ' successfully!', 'success', 3000);"
            "    } else {"
            "      alert('Failed to update setting');"
            "    }"
            "  })"
            "  .catch(error => {"
            "    alert('Network error occurred');"
            "  });"
            "}"
            ""
            "window.onclick = function(event) {"
            "  const modal = document.getElementById('apiKeyModal');"
            "  if (event.target == modal) {"
            "    closeApiKeyModal();"
            "  }"
            "}"
            "</script>";
    webServer.sendContent(chunk);
    feed_watchdog();

    webServer.sendContent(get_html_footer());
    feed_watchdog();
    webServer.sendContent("");
}
void handle_chatgpt_config() {
    reset_oled_timeout();

    bool enable_requested = webServer.hasArg("enabled");
    String api_key = webServer.arg("api_key");

    if (api_key.length() > 0) {
        if (!api_key.startsWith("sk-") || api_key.length() < 20 || api_key.length() > 200) {
            webServer.sendHeader("Location", "/chatgpt?msg=Invalid+API+key+format");
            webServer.send(302, "text/plain", "");
            return;
        }

        String encoded_key = chatgpt_encode_api_key(api_key);
        strlcpy(chatgpt_config.api_key_b64, encoded_key.c_str(), sizeof(chatgpt_config.api_key_b64));
        logMessage("CHATGPT: API key updated");
    }

    if (enable_requested && strlen(chatgpt_config.api_key_b64) == 0) {
        webServer.sendHeader("Location", "/chatgpt?msg=Cannot+enable+without+API+key");
        webServer.send(302, "text/plain", "");
        return;
    }

    chatgpt_config.enabled = enable_requested;

    if (chatgpt_save_config()) {
        String msg = "Configuration+updated+-+enabled:+" + String(chatgpt_config.enabled ? "yes" : "no");
        if (api_key.length() > 0) {
            msg += ",+API+key+updated";
        }
        logMessage("CHATGPT: Configuration updated - enabled: " + String(chatgpt_config.enabled ? "yes" : "no") +
                  (api_key.length() > 0 ? ", API key updated" : ""));
        webServer.sendHeader("Location", "/chatgpt?msg=" + msg);
    } else {
        webServer.sendHeader("Location", "/chatgpt?msg=Config+save+failed");
    }
    webServer.send(302, "text/plain", "");
}
void handle_chatgpt_notifications() {
    reset_oled_timeout();

    bool notify_requested = webServer.hasArg("chatgpt_notify_failures");
    chatgpt_config.chatgpt_notify_failures = notify_requested;

    if (chatgpt_save_config()) {
        logMessage("CHATGPT: Failure notifications " + String(chatgpt_config.chatgpt_notify_failures ? "enabled" : "disabled"));
        webServer.send(200, "text/plain", "OK");
    } else {
        webServer.send(500, "text/plain", "Failed to save configuration");
    }
}
void handle_chatgpt_api_key() {
    reset_oled_timeout();

    if (webServer.hasArg("api_key")) {
        String api_key = webServer.arg("api_key");

        if (api_key.length() == 0) {
            if (strlen(chatgpt_config.api_key_b64) > 0) {
                webServer.send(200, "application/json", "{\"success\":true,\"message\":\"API key unchanged\"}");
            } else {
                webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Please enter API key\"}");
            }
            return;
        } else if (chatgpt_validate_api_key(api_key)) {
            if (strlen(chatgpt_config.api_key_b64) > 0) {
                String current_key = chatgpt_decode_api_key(String(chatgpt_config.api_key_b64));
                if (api_key.equals(current_key)) {
                    webServer.send(200, "application/json", "{\"success\":true,\"message\":\"API key unchanged\"}");
                    return;
                }
            }

            String encoded_key = chatgpt_encode_api_key(api_key);
            strlcpy(chatgpt_config.api_key_b64, encoded_key.c_str(), sizeof(chatgpt_config.api_key_b64));

            if (chatgpt_save_config()) {
                logMessage("CHATGPT: API key updated successfully");
                webServer.send(200, "application/json", "{\"success\":true,\"message\":\"API key updated successfully\"}");
            } else {
                webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save API key\"}");
            }
            return;
        } else {
            webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid API key format\"}");
            return;
        }
    } else {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"No API key provided\"}");
        return;
    }
}
void handle_chatgpt_add_prompt() {
    reset_oled_timeout();

    if (chatgpt_config.prompts.size() >= MAX_CHATGPT_PROMPTS) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Maximum " + String(MAX_CHATGPT_PROMPTS) + " prompts allowed\"}");
        return;
    }

    String body = webServer.arg("plain");
    if (body.length() == 0) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"No JSON data provided\"}");
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON format\"}");
        return;
    }

    if (!doc.containsKey("prompt") || !doc.containsKey("days") || !doc.containsKey("time") ||
        !doc.containsKey("capcode") || !doc.containsKey("frequency")) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Missing required fields\"}");
        return;
    }

    String promptText = doc["prompt"];
    uint8_t days = doc["days"];
    String timeStr = doc["time"];
    uint64_t capcode = doc["capcode"];
    float frequency = doc["frequency"];
    bool mailDrop = doc["mail_drop"] | false;
    bool enabled = doc["enabled"] | true;

    if (promptText.length() == 0 || promptText.length() > 250) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Prompt text must be 1-250 characters\"}");
        return;
    }

    if (days == 0 || days > 127) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid day selection\"}");
        return;
    }

    if (timeStr.length() != 5 || timeStr.indexOf(':') != 2) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid time format (use HH:MM)\"}");
        return;
    }

    if (!validate_flex_capcode(capcode)) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid FLEX capcode. Valid ranges: 1-1933312, 1998849-2031614, 2101249-4291000000\"}");
        return;
    }

    if (frequency < 400.0 || frequency > 1000.0) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Frequency must be 400-1000 MHz\"}");
        return;
    }

    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid time values\"}");
        return;
    }

    ChatGPTPrompt newPrompt;
    newPrompt.id = chatgpt_config.prompts.size() + 1;
    strlcpy(newPrompt.name, ("Prompt " + String(newPrompt.id)).c_str(), sizeof(newPrompt.name));
    String sanitized = sanitize_chatgpt_prompt(promptText);
    strlcpy(newPrompt.prompt, sanitized.c_str(), sizeof(newPrompt.prompt));

    for (int i = 0; i < 7; i++) {
        newPrompt.days[i] = (days & (1 << i)) != 0;
    }

    newPrompt.hour = hour;
    newPrompt.minute = minute;
    newPrompt.capcode = capcode;
    newPrompt.frequency = frequency;
    newPrompt.mail_drop = mailDrop;
    newPrompt.enabled = true;
    newPrompt.retry_count = 0;

    chatgpt_config.prompts.push_back(newPrompt);
    chatgpt_config.prompt_count = chatgpt_config.prompts.size();

    if (chatgpt_save_config()) {
        logMessage("CHATGPT: New prompt added successfully (total: " + String(chatgpt_config.prompts.size()) + ")");
        webServer.send(200, "application/json", "{\"success\":true,\"message\":\"Prompt added successfully\"}");
    } else {
        chatgpt_config.prompts.pop_back();
        chatgpt_config.prompt_count = chatgpt_config.prompts.size();
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save prompt configuration\"}");
    }
}
void handle_chatgpt_get_prompt() {
    reset_oled_timeout();

    String path = webServer.uri();
    int index = path.substring(path.lastIndexOf('/') + 1).toInt();

    if (index < 0 || index >= (int)chatgpt_config.prompts.size()) {
        webServer.send(404, "application/json", "{\"success\":false,\"error\":\"Prompt not found\"}");
        return;
    }

    ChatGPTPrompt& prompt = chatgpt_config.prompts[index];

    DynamicJsonDocument doc(1024);
    doc["success"] = true;

    JsonObject promptObj = doc.createNestedObject("prompt");
    promptObj["prompt"] = String(prompt.prompt);

    char time_str[6];
    sprintf(time_str, "%02d:%02d", prompt.hour, prompt.minute);
    promptObj["time"] = String(time_str);

    promptObj["capcode"] = prompt.capcode;
    promptObj["frequency"] = prompt.frequency;
    promptObj["mail_drop"] = prompt.mail_drop;
    promptObj["enabled"] = prompt.enabled;

    JsonArray days = promptObj.createNestedArray("days");
    for (int i = 0; i < 7; i++) {
        days.add(prompt.days[i]);
    }

    String json;
    serializeJson(doc, json);
    webServer.send(200, "application/json", json);
}
void handle_chatgpt_edit_prompt() {
    reset_oled_timeout();

    String path = webServer.uri();
    int index = path.substring(path.lastIndexOf('/') + 1).toInt();

    if (index < 0 || index >= (int)chatgpt_config.prompts.size()) {
        webServer.send(404, "application/json", "{\"success\":false,\"error\":\"Prompt not found\"}");
        return;
    }

    String body = webServer.arg("plain");
    if (body.length() == 0) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"No JSON data provided\"}");
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON format\"}");
        return;
    }

    if (!doc.containsKey("prompt") || !doc.containsKey("days") || !doc.containsKey("time") ||
        !doc.containsKey("capcode") || !doc.containsKey("frequency")) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Missing required fields\"}");
        return;
    }

    String promptText = doc["prompt"];
    uint8_t days = doc["days"];
    String timeStr = doc["time"];
    uint64_t capcode = doc["capcode"];
    float frequency = doc["frequency"];
    bool mailDrop = doc["mail_drop"] | false;
    bool enabled = doc["enabled"] | true;

    if (promptText.length() == 0 || promptText.length() > 250) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Prompt text must be 1-250 characters\"}");
        return;
    }

    if (days == 0 || days > 127) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid day selection\"}");
        return;
    }

    if (timeStr.length() != 5 || timeStr.indexOf(':') != 2) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid time format (use HH:MM)\"}");
        return;
    }

    if (!validate_flex_capcode(capcode)) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid FLEX capcode. Valid ranges: 1-1933312, 1998849-2031614, 2101249-4291000000\"}");
        return;
    }

    if (frequency < 400.0 || frequency > 1000.0) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Frequency must be 400-1000 MHz\"}");
        return;
    }

    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid time values\"}");
        return;
    }

    ChatGPTPrompt& editPrompt = chatgpt_config.prompts[index];
    String sanitized = sanitize_chatgpt_prompt(promptText);
    strlcpy(editPrompt.prompt, sanitized.c_str(), sizeof(editPrompt.prompt));

    for (int i = 0; i < 7; i++) {
        editPrompt.days[i] = (days & (1 << i)) != 0;
    }

    editPrompt.hour = hour;
    editPrompt.minute = minute;
    editPrompt.capcode = capcode;
    editPrompt.frequency = frequency;
    editPrompt.mail_drop = mailDrop;
    editPrompt.enabled = enabled;

    if (chatgpt_save_config()) {
        logMessage("CHATGPT: Prompt " + String(index + 1) + " updated successfully");
        webServer.send(200, "application/json", "{\"success\":true,\"message\":\"Prompt updated successfully\"}");
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save prompt configuration\"}");
    }
}
void handle_chatgpt_toggle() {
    reset_oled_timeout();

    String uri = webServer.uri();
    int index = uri.substring(uri.lastIndexOf('/') + 1).toInt();

    if (index < 0 || index >= (int)chatgpt_config.prompts.size()) {
        webServer.send(404, "application/json", "{\"success\":false,\"error\":\"Prompt not found\"}");
        return;
    }

    ChatGPTPrompt& prompt = chatgpt_config.prompts[index];
    prompt.enabled = !prompt.enabled;

    if (chatgpt_save_config()) {
        String status = prompt.enabled ? "enabled" : "disabled";
        logMessage("CHATGPT: Prompt " + String(index + 1) + " " + status);
        webServer.send(200, "application/json", "{\"success\":true,\"enabled\":" + String(prompt.enabled ? "true" : "false") + "}");
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save configuration\"}");
    }
}
void handle_chatgpt_delete() {
    reset_oled_timeout();

    String uri = webServer.uri();
    int index = uri.substring(uri.lastIndexOf('/') + 1).toInt();

    if (index >= 0 && index < (int)chatgpt_config.prompts.size()) {
        chatgpt_config.prompts.erase(chatgpt_config.prompts.begin() + index);

        for (size_t i = index; i < chatgpt_config.prompts.size(); i++) {
            chatgpt_config.prompts[i].id = i + 1;
        }
        chatgpt_config.prompt_count = chatgpt_config.prompts.size();

        if (chatgpt_save_config()) {
            logMessage("CHATGPT: Prompt " + String(index + 1) + " deleted");
            webServer.sendHeader("Location", "/chatgpt?msg=Prompt+deleted");
        } else {
            webServer.sendHeader("Location", "/chatgpt?msg=Delete+failed");
        }
    } else {
        webServer.sendHeader("Location", "/chatgpt?msg=Invalid+prompt+index");
    }
    webServer.send(302, "text/plain", "");
}

#endif // ENABLE_CHATGPT
