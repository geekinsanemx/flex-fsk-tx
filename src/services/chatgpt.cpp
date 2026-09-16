/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * ChatGPT Module - scheduled prompt execution via OpenAI API
 */

#include "../core/config.h"
#include "../services/chatgpt.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../protocol/transmission.h"
#include "../protocol/flex_protocol.h"
#include "../network/ntp_time.h"
#include "../core/utils.h"

#ifdef ENABLE_CHATGPT

#include <SPIFFS.h>
#include "../core/tx_lock.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// =============================================================================
// GLOBALS (extern, declared in chatgpt.h)
// =============================================================================
ChatGPTConfig chatgpt_config;
ChatGPTActivity chatgpt_activity_log[10];
int chatgpt_activity_count = 0;
unsigned long last_chatgpt_check = 0;

// =============================================================================
// FILE-LOCAL STATE
// =============================================================================
static int chatgpt_last_http_code = 0;

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================
static String chatgpt_query(String prompt, String api_key);
static bool chatgpt_execute_prompt(ChatGPTPrompt& prompt, int prompt_index);
static bool chatgpt_is_time_to_execute(ChatGPTPrompt& prompt);
static void chatgpt_log_activity(const char* prompt_name, const char* query, const char* response,
                                 bool query_success, bool transmission_success, uint64_t capcode, float frequency, int prompt_index, bool mail_drop);

// =============================================================================
// PROMPT / API-KEY HELPERS
// =============================================================================
String sanitize_chatgpt_prompt(String input) {
    String output = input;

    output.trim();

    output.replace("\r\n", "\n");
    output.replace("\r", "\n");

    while (output.indexOf("  ") >= 0) {
        output.replace("  ", " ");
    }

    while (output.indexOf("\n\n\n") >= 0) {
        output.replace("\n\n\n", "\n\n");
    }

    if (output.length() > 250) {
        output = output.substring(0, 250);
    }

    return output;
}

String chatgpt_encode_api_key(String api_key) {
    return base64_encode_string(api_key);
}

String chatgpt_decode_api_key(String encoded_key) {
    return base64_decode_string(encoded_key);
}

// Preserved verbatim from the original source; not called anywhere in that
// codebase either, but kept per the no-behavior-change porting rule.
static String chatgpt_mask_api_key(String api_key) {
    if (api_key.length() < 10) return "Invalid";

    String masked = api_key.substring(0, 3);
    masked += "***...***";
    masked += api_key.substring(api_key.length() - 3);
    return masked;
}

bool chatgpt_validate_api_key(String api_key) {
    return api_key.startsWith("sk-") && api_key.length() >= 20 && api_key.length() <= 200;
}

// =============================================================================
// CONFIG PERSISTENCE (/chatgpt_settings.json)
// =============================================================================
bool chatgpt_load_config() {
    FlashGuard fg;
    if (!fg.ok()) return false;

    File file = SPIFFS.open("/chatgpt_settings.json", "r");
    if (!file) {
        logMessage("CHATGPT: No configuration file found, using defaults");
        chatgpt_config.enabled = false;
        chatgpt_config.chatgpt_notify_failures = true;
        chatgpt_config.prompts.clear();
        chatgpt_config.prompt_count = 0;
        strncpy(chatgpt_config.api_key_b64, "", sizeof(chatgpt_config.api_key_b64) - 1);
    chatgpt_config.api_key_b64[sizeof(chatgpt_config.api_key_b64) - 1] = '\0';
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        logMessage("CHATGPT: Failed to parse configuration file");
        return false;
    }

    chatgpt_config.enabled = doc["enabled"] | false;
    chatgpt_config.chatgpt_notify_failures = doc["chatgpt_notify_failures"] | true;
    strlcpy(chatgpt_config.api_key_b64, doc["api_key_b64"] | "", sizeof(chatgpt_config.api_key_b64));

    JsonArray prompts = doc["prompts"];
    chatgpt_config.prompts.clear();
    chatgpt_config.prompt_count = 0;

    for (JsonVariant prompt_var : prompts) {
        if (chatgpt_config.prompts.size() >= MAX_CHATGPT_PROMPTS) break;

        JsonObject prompt = prompt_var.as<JsonObject>();
        ChatGPTPrompt p;

        p.id = prompt["id"] | (chatgpt_config.prompts.size() + 1);
        strlcpy(p.name, prompt["name"] | "", sizeof(p.name));
        strlcpy(p.prompt, prompt["prompt"] | "", sizeof(p.prompt));

        JsonArray days = prompt["days"];
        for (int i = 0; i < 7; i++) {
            p.days[i] = days[i].as<int>() != 0;
        }

        String time_str = prompt["time"] | "08:00";
        sscanf(time_str.c_str(), "%hhu:%hhu", &p.hour, &p.minute);

        p.capcode = prompt["capcode"] | settings.default_capcode;
        p.frequency = prompt["frequency"] | settings.default_frequency;
        p.mail_drop = prompt["mail_drop"] | false;
        p.enabled = prompt["enabled"] | true;
        p.retry_count = 0;

        chatgpt_config.prompts.push_back(p);
        chatgpt_config.prompt_count = chatgpt_config.prompts.size();
    }

    logMessage("CHATGPT: Configuration loaded, " + String(chatgpt_config.prompts.size()) + " prompts, enabled: " + String(chatgpt_config.enabled ? "yes" : "no"));
    return true;
}

bool chatgpt_save_config() {
    FlashGuard fg;
    if (!fg.ok()) return false;

    DynamicJsonDocument doc(4096);

    doc["enabled"] = chatgpt_config.enabled;
    doc["chatgpt_notify_failures"] = chatgpt_config.chatgpt_notify_failures;
    doc["api_key_b64"] = chatgpt_config.api_key_b64;

    JsonArray prompts = doc.createNestedArray("prompts");
    for (size_t i = 0; i < chatgpt_config.prompts.size(); i++) {
        ChatGPTPrompt& p = chatgpt_config.prompts[i];
        JsonObject prompt = prompts.createNestedObject();

        prompt["id"] = p.id;
        prompt["name"] = p.name;
        prompt["prompt"] = p.prompt;

        JsonArray days = prompt.createNestedArray("days");
        for (int j = 0; j < 7; j++) {
            days.add(p.days[j] ? 1 : 0);
        }

        char time_str[6];
        sprintf(time_str, "%02d:%02d", p.hour, p.minute);
        prompt["time"] = time_str;

        prompt["capcode"] = p.capcode;
        prompt["frequency"] = p.frequency;
        prompt["mail_drop"] = p.mail_drop;
        prompt["enabled"] = p.enabled;
    }

    File file = SPIFFS.open("/chatgpt_settings.json", "w");
    if (!file) {
        logMessage("CHATGPT: Failed to open configuration file for writing");
        return false;
    }

    if (serializeJsonPretty(doc, file) == 0) {
        file.flush();
        file.close();
        logMessage("CHATGPT: Failed to write configuration file");
        return false;
    }

    file.flush();
    file.close();
    logMessage("CHATGPT: Configuration saved successfully");
    return true;
}

// Preserved verbatim; dead code in the original source (never called there either).
static bool chatgpt_validate_json(String json_content) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, json_content);

    if (error) {
        return false;
    }

    if (!doc.containsKey("enabled") || !doc.containsKey("prompts")) {
        return false;
    }

    JsonArray prompts = doc["prompts"];
    if (prompts.size() > 5) {
        return false;
    }

    for (JsonVariant prompt_var : prompts) {
        JsonObject prompt = prompt_var.as<JsonObject>();
        if (!prompt.containsKey("name") || !prompt.containsKey("prompt") ||
            !prompt.containsKey("days") || !prompt.containsKey("time")) {
            return false;
        }
    }

    return true;
}

// Preserved verbatim; dead code in the original source (never called there either).
static void chatgpt_create_default_config() {
    chatgpt_config.enabled = false;
    chatgpt_config.chatgpt_notify_failures = true;
    chatgpt_config.prompts.clear();
    chatgpt_config.prompt_count = 0;
    strncpy(chatgpt_config.api_key_b64, "", sizeof(chatgpt_config.api_key_b64) - 1);
    chatgpt_config.api_key_b64[sizeof(chatgpt_config.api_key_b64) - 1] = '\0';
    logMessage("CHATGPT: Default configuration created");
}

// =============================================================================
// ACTIVITY LOG
// =============================================================================
static void chatgpt_log_activity(const char* prompt_name, const char* query, const char* response,
                                 bool query_success, bool transmission_success, uint64_t capcode, float frequency, int prompt_index, bool mail_drop) {
    if (chatgpt_activity_count >= 10) {
        for (int i = 0; i < 9; i++) {
            chatgpt_activity_log[i] = chatgpt_activity_log[i + 1];
        }
        chatgpt_activity_count = 9;
    }

    ChatGPTActivity& activity = chatgpt_activity_log[chatgpt_activity_count];
    activity.timestamp = millis();
    strlcpy(activity.prompt_name, prompt_name, sizeof(activity.prompt_name));
    strlcpy(activity.query, query, sizeof(activity.query));
    strlcpy(activity.response, response, sizeof(activity.response));
    activity.query_success = query_success;
    activity.transmission_success = transmission_success;
    activity.capcode = capcode;
    activity.frequency = frequency;
    activity.prompt_index = prompt_index;
    activity.mail_drop = mail_drop;

    time_t now = getLocalTimestamp();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(activity.datetime, sizeof(activity.datetime), "%b %d %H:%M:%S", &timeinfo);

    chatgpt_activity_count++;
}

// =============================================================================
// EXECUTION
// =============================================================================
static bool chatgpt_execute_prompt(ChatGPTPrompt& prompt, int prompt_index) {
    if (!chatgpt_config.enabled || !prompt.enabled) {
        return false;
    }

    String api_key = chatgpt_decode_api_key(String(chatgpt_config.api_key_b64));
    if (api_key.length() == 0) {
        logMessage("CHATGPT: Cannot execute prompt - no API key configured");
        chatgpt_log_activity(prompt.name, prompt.prompt, "No API key", false, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop);
        return false;
    }

    logMessage("CHATGPT: Executing prompt '" + String(prompt.name) + "' (attempt " + String(prompt.retry_count + 1) + "/3)");

    String response = chatgpt_query(String(prompt.prompt), api_key);
    bool query_success = response.length() > 0;

    if (!query_success) {
        prompt.retry_count++;
        logMessage("CHATGPT: Query failed, HTTP code: " + String(chatgpt_last_http_code) + ", attempt " + String(prompt.retry_count) + "/3");

        if (prompt.retry_count >= 3) {
            logMessage("CHATGPT: All 3 attempts failed for '" + String(prompt.name) + "'");
            chatgpt_log_activity(prompt.name, prompt.prompt, "Failed after 3 attempts", false, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop);

            if (chatgpt_config.chatgpt_notify_failures) {
                String failure_msg = "ChatGPT Failed: " + String(prompt.name) + " - All 3 attempts failed";
                bool failure_queued = queue_add_message(prompt.capcode, prompt.frequency, settings.default_txpower, prompt.mail_drop, failure_msg.c_str());
                if (failure_queued) {
                    logMessage("CHATGPT: Failure notification sent for '" + String(prompt.name) + "'");
                }
            }

            prompt.retry_count = 0;
            prompt.next_retry_time = 0;
            return false;
        } else {
            prompt.next_retry_time = millis() + 60000;
            logMessage("CHATGPT: Will retry in 1 minute at " + String(prompt.next_retry_time));
            chatgpt_log_activity(prompt.name, prompt.prompt, "Query failed", false, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop);
            return false;
        }
    }

    response = truncate_message_with_ellipsis(response);

    bool queued = queue_add_message(prompt.capcode, prompt.frequency, settings.default_txpower, prompt.mail_drop, response.c_str());

    if (queued) {
        logMessage("CHATGPT: Response queued for transmission to " + String(prompt.capcode));
        chatgpt_log_activity(prompt.name, prompt.prompt, response.c_str(), true, true, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop);
        prompt.retry_count = 0;
        prompt.next_retry_time = 0;
        return true;
    } else {
        prompt.retry_count++;
        logMessage("CHATGPT: Failed to queue response, attempt " + String(prompt.retry_count) + "/3");

        if (prompt.retry_count >= 3) {
            logMessage("CHATGPT: All 3 queue attempts failed for '" + String(prompt.name) + "'");
            chatgpt_log_activity(prompt.name, prompt.prompt, "Queue failed after 3 attempts", true, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop);

            if (chatgpt_config.chatgpt_notify_failures) {
                String failure_msg = "ChatGPT Failed: " + String(prompt.name) + " - Queue full after 3 attempts";
                queue_add_message(prompt.capcode, prompt.frequency, settings.default_txpower, prompt.mail_drop, failure_msg.c_str());
            }

            prompt.retry_count = 0;
            prompt.next_retry_time = 0;
            return false;
        } else {
            prompt.next_retry_time = millis() + 60000;
            logMessage("CHATGPT: Will retry queue in 1 minute at " + String(prompt.next_retry_time));
            chatgpt_log_activity(prompt.name, prompt.prompt, response.c_str(), true, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop);
            return false;
        }
    }
}

static String chatgpt_query(String prompt, String api_key) {
    if (WiFi.status() != WL_CONNECTED) {
        logMessage("CHATGPT: No WiFi connection for API query");
        return "";
    }

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();

    logMessage("CHATGPT: WiFi status: " + String(WiFi.status()) + ", RSSI: " + String(WiFi.RSSI()) + " dBm");
    logMessage("CHATGPT: Free heap: " + String(ESP.getFreeHeap()) + " bytes");

    bool connected = http.begin(client, "https://api.openai.com/v1/chat/completions");
    if (!connected) {
        logMessage("CHATGPT: Failed to connect to OpenAI API endpoint");
        http.end();
        return "";
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + api_key);
    http.setTimeout(30000);

    String json_request = "{"
        "\"model\":\"gpt-3.5-turbo\","
        "\"messages\":["
            "{\"role\":\"system\",\"content\":\"You are an assistant that must always reply with plain text only, no emojis, no hashtags, no special symbols. IMPORTANT: Every response must be 248 characters or fewer.\"},"
            "{\"role\":\"user\",\"content\":\"" + json_escape_string(prompt) + "\"}"
        "],"
        "\"max_tokens\":100,"
        "\"temperature\":0.7"
        "}";

    logMessage("CHATGPT: Sending query to OpenAI API");
    logMessage("CHATGPT: Query payload: " + json_request);
    int httpCode = http.POST(json_request);
    chatgpt_last_http_code = httpCode;

    String response = "";
    if (httpCode == 200) {
        String payload = http.getString();

        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            if (doc["choices"].size() > 0) {
                response = doc["choices"][0]["message"]["content"].as<String>();
                response.trim();

                String token_info = "";
                if (doc["usage"]["total_tokens"]) {
                    int total_tokens = doc["usage"]["total_tokens"];
                    int completion_tokens = doc["usage"]["completion_tokens"];
                    token_info = ", " + String(completion_tokens) + "/" + String(total_tokens) + " tokens";
                 }

                 logMessage("CHATGPT: Received response (" + String(response.length()) + " chars" + token_info + "): " + response);
            } else {
                logMessage("CHATGPT: API response contains no choices");
            }
        } else {
            logMessage("CHATGPT: Failed to parse API response JSON");
            String payload_preview = payload.length() > 200 ? payload.substring(0, 200) + "..." : payload;
            logMessage("CHATGPT: JSON payload preview: " + payload_preview);
        }
    } else {
        logMessage("CHATGPT: API request failed with HTTP code " + String(httpCode));
        if (httpCode == -1) {
            logMessage("CHATGPT: HTTP -1 indicates: connection failed, DNS lookup failed, or SSL handshake failed");
            logMessage("CHATGPT: WiFi status: " + String(WiFi.status()) + ", connected: " + String(WiFi.isConnected()));
        } else if (httpCode > 0) {
            String error_response = http.getString();
            String error_preview = error_response.length() > 200 ? error_response.substring(0, 200) + "..." : error_response;
            logMessage("CHATGPT: Error response: " + error_preview);
        } else {
            logMessage("CHATGPT: Negative HTTP code indicates internal HTTPClient error");
        }
    }

    http.end();
    return response;
}

// =============================================================================
// SCHEDULING
// =============================================================================
static bool chatgpt_is_time_to_execute(ChatGPTPrompt& prompt) {
    if (!system_time_initialized || !chatgpt_config.enabled || !prompt.enabled) {
        return false;
    }

    if (prompt.retry_count > 0 && prompt.next_retry_time > 0) {
        return millis() >= prompt.next_retry_time;
    }

    if (prompt.retry_count > 0) {
        return false;
    }

    time_t local_time = getLocalTimestamp();
    struct tm* timeinfo = localtime(&local_time);

    int current_day = timeinfo->tm_wday;

    if (!prompt.days[current_day]) {
        return false;
    }

    int current_hour = timeinfo->tm_hour;
    int current_minute = timeinfo->tm_min;

    return (current_hour == prompt.hour && current_minute == prompt.minute);
}

String chatgpt_format_next_execution(ChatGPTPrompt& prompt) {
    if (!prompt.enabled) {
        return "Disabled";
    }

    if (!system_time_initialized) {
        return "Time not synchronized";
    }

    time_t local_time = getLocalTimestamp();
    struct tm* timeinfo = localtime(&local_time);

    int current_day = timeinfo->tm_wday;

    if (prompt.days[current_day]) {
        int current_hour = timeinfo->tm_hour;
        int current_minute = timeinfo->tm_min;
        int current_time_mins = current_hour * 60 + current_minute;
        int prompt_time_mins = prompt.hour * 60 + prompt.minute;

        if (prompt_time_mins > current_time_mins) {
            char time_str[6];
            sprintf(time_str, "%02d:%02d", prompt.hour, prompt.minute);
            return "Today " + String(time_str);
        }
    }

    String days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int i = 1; i <= 7; i++) {
        int check_day = (current_day + i) % 7;
        if (prompt.days[check_day]) {
            char time_str[6];
            sprintf(time_str, "%02d:%02d", prompt.hour, prompt.minute);
            return days[check_day] + " " + String(time_str);
        }
    }

    return "Never";
}

void chatgpt_check_schedules() {
    if (!chatgpt_config.enabled || chatgpt_config.prompts.size() == 0) {
        return;
    }

    for (size_t i = 0; i < chatgpt_config.prompts.size(); i++) {
        ChatGPTPrompt& prompt = chatgpt_config.prompts[i];
        if (chatgpt_is_time_to_execute(prompt)) {
            logMessage("CHATGPT: Scheduled execution triggered for '" + String(prompt.name) + "'");
            chatgpt_execute_prompt(prompt, i + 1);

            delay(1000);
        }
    }
}

#endif // ENABLE_CHATGPT
