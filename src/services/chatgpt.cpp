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
#include <algorithm>
#include <esp_heap_caps.h>

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
static bool chatgpt_last_model_unavailable = false;
static std::vector<String> chatgpt_model_cache;

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================
static String chatgpt_query(const String& prompt, const String& api_key, const String& model);
static bool chatgpt_execute_prompt(ChatGPTPrompt& prompt, int prompt_index);
static bool chatgpt_is_time_to_execute(ChatGPTPrompt& prompt);
static void chatgpt_log_activity(const char* prompt_name, const char* query, const char* response,
                                 bool query_success, bool transmission_success, uint64_t capcode, float frequency, int prompt_index, bool mail_drop,
                                 const char* model);

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

bool chatgpt_validate_model_name(const String& model) {
    if (model.length() == 0 || model.length() >= CHATGPT_MODEL_MAX_LEN) {
        return false;
    }
    for (size_t i = 0; i < model.length(); i++) {
        char c = model[i];
        if (!isAlphaNumeric(c) && c != '-' && c != '.' && c != '_' && c != ':') {
            return false;
        }
    }
    return true;
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
        chatgpt_config.default_model[0] = '\0';
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
    strlcpy(chatgpt_config.default_model, doc["default_model"] | "", sizeof(chatgpt_config.default_model));

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
        strlcpy(p.model, prompt["model"] | "", sizeof(p.model));

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
    doc["default_model"] = chatgpt_config.default_model;

    JsonArray prompts = doc.createNestedArray("prompts");
    for (size_t i = 0; i < chatgpt_config.prompts.size(); i++) {
        ChatGPTPrompt& p = chatgpt_config.prompts[i];
        JsonObject prompt = prompts.createNestedObject();

        prompt["id"] = p.id;
        prompt["name"] = p.name;
        prompt["prompt"] = p.prompt;
        prompt["model"] = p.model;

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
    chatgpt_config.default_model[0] = '\0';
    logMessage("CHATGPT: Default configuration created");
}

// =============================================================================
// ACTIVITY LOG
// =============================================================================
static void chatgpt_log_activity(const char* prompt_name, const char* query, const char* response,
                                 bool query_success, bool transmission_success, uint64_t capcode, float frequency, int prompt_index, bool mail_drop,
                                 const char* model) {
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
    strlcpy(activity.model, model, sizeof(activity.model));
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
static String chatgpt_resolve_model(const String& override_model) {
    return override_model.length() > 0 ? override_model : String(chatgpt_config.default_model);
}

static String chatgpt_query_with_fallback(const String& text, const String& api_key, const String& override_model,
                                          String& model, const String& label) {
    String response = chatgpt_query(text, api_key, model);

    if (response.length() == 0 && chatgpt_last_model_unavailable && override_model.length() > 0 &&
        strlen(chatgpt_config.default_model) > 0 && model != chatgpt_config.default_model) {
        logMessage("CHATGPT: Model '" + model + "' unavailable for '" + label + "', falling back to default model '" + String(chatgpt_config.default_model) + "'");
        model = chatgpt_config.default_model;
        response = chatgpt_query(text, api_key, model);
    }

    return response;
}

static bool chatgpt_execute_prompt(ChatGPTPrompt& prompt, int prompt_index) {
    if (!chatgpt_config.enabled || !prompt.enabled) {
        return false;
    }

    String model = chatgpt_resolve_model(String(prompt.model));

    String api_key = chatgpt_decode_api_key(String(chatgpt_config.api_key_b64));
    if (api_key.length() == 0) {
        logMessage("CHATGPT: Cannot execute prompt - no API key configured");
        chatgpt_log_activity(prompt.name, prompt.prompt, "No API key", false, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop, model.c_str());
        return false;
    }

    if (model.length() == 0) {
        logMessage("CHATGPT: Cannot execute prompt '" + String(prompt.name) + "' - no default model selected and no model override set");
        chatgpt_log_activity(prompt.name, prompt.prompt, "No model configured", false, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop, "");
        return false;
    }

    logMessage("CHATGPT: Executing prompt '" + String(prompt.name) + "' with model '" + model + "' (attempt " + String(prompt.retry_count + 1) + "/3)");

    String response = chatgpt_query_with_fallback(String(prompt.prompt), api_key, String(prompt.model), model, String(prompt.name));

    bool query_success = response.length() > 0;

    if (!query_success && chatgpt_last_model_unavailable) {
        logMessage("CHATGPT: Model '" + model + "' unavailable, skipping retries for '" + String(prompt.name) + "'");
        String reason = "Model unavailable: " + model;
        chatgpt_log_activity(prompt.name, prompt.prompt, reason.c_str(), false, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop, model.c_str());

        if (chatgpt_config.chatgpt_notify_failures) {
            String failure_msg = "ChatGPT Failed: " + String(prompt.name) + " - Model " + model + " unavailable";
            if (queue_add_message(prompt.capcode, prompt.frequency, settings.default_txpower, prompt.mail_drop, failure_msg.c_str())) {
                logMessage("CHATGPT: Failure notification sent for '" + String(prompt.name) + "'");
            }
        }

        prompt.retry_count = 0;
        prompt.next_retry_time = 0;
        return false;
    }

    if (!query_success) {
        prompt.retry_count++;
        logMessage("CHATGPT: Query failed, HTTP code: " + String(chatgpt_last_http_code) + ", attempt " + String(prompt.retry_count) + "/3");

        if (prompt.retry_count >= 3) {
            logMessage("CHATGPT: All 3 attempts failed for '" + String(prompt.name) + "'");
            chatgpt_log_activity(prompt.name, prompt.prompt, "Failed after 3 attempts", false, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop, model.c_str());

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
            chatgpt_log_activity(prompt.name, prompt.prompt, "Query failed", false, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop, model.c_str());
            return false;
        }
    }

    response = truncate_message_with_ellipsis(response);

    bool queued = queue_add_message(prompt.capcode, prompt.frequency, settings.default_txpower, prompt.mail_drop, response.c_str());

    if (queued) {
        logMessage("CHATGPT: Response queued for transmission to " + String(prompt.capcode));
        chatgpt_log_activity(prompt.name, prompt.prompt, response.c_str(), true, true, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop, model.c_str());
        prompt.retry_count = 0;
        prompt.next_retry_time = 0;
        return true;
    } else {
        prompt.retry_count++;
        logMessage("CHATGPT: Failed to queue response, attempt " + String(prompt.retry_count) + "/3");

        if (prompt.retry_count >= 3) {
            logMessage("CHATGPT: All 3 queue attempts failed for '" + String(prompt.name) + "'");
            chatgpt_log_activity(prompt.name, prompt.prompt, "Queue failed after 3 attempts", true, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop, model.c_str());

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
            chatgpt_log_activity(prompt.name, prompt.prompt, response.c_str(), true, false, prompt.capcode, prompt.frequency, prompt_index, prompt.mail_drop, model.c_str());
            return false;
        }
    }
}

static String chatgpt_query(const String& prompt, const String& api_key, const String& model) {
    chatgpt_last_model_unavailable = false;
    chatgpt_last_http_code = 0;

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

    JsonDocument request;
    request["model"] = model;
    JsonArray messages = request["messages"].to<JsonArray>();
    JsonObject system_message = messages.add<JsonObject>();
    system_message["role"] = "system";
    system_message["content"] = "You are an assistant that must always reply with plain text only, no emojis, no hashtags, no special symbols. IMPORTANT: Every response must be 248 characters or fewer.";
    JsonObject user_message = messages.add<JsonObject>();
    user_message["role"] = "user";
    user_message["content"] = prompt;

    String json_request;
    serializeJson(request, json_request);

    logMessage("CHATGPT: Sending query to OpenAI API");
    logMessage("CHATGPT: Query payload: " + json_request);
    int httpCode = http.POST(json_request);
    chatgpt_last_http_code = httpCode;

    String response = "";
    if (httpCode == 200) {
        String payload = http.getString();

        JsonDocument filter;
        filter["model"] = true;
        filter["choices"][0]["message"]["content"] = true;
        filter["usage"]["total_tokens"] = true;
        filter["usage"]["completion_tokens"] = true;

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

        if (!error) {
            if (doc["choices"].size() > 0) {
                response = String(doc["choices"][0]["message"]["content"] | "");
                response.trim();

                String token_info = "";
                if (doc["usage"]["total_tokens"]) {
                    int total_tokens = doc["usage"]["total_tokens"];
                    int completion_tokens = doc["usage"]["completion_tokens"];
                    token_info = ", " + String(completion_tokens) + "/" + String(total_tokens) + " tokens";
                }

                String responding_model = doc["model"] | model.c_str();
                logMessage("CHATGPT: Received response from " + responding_model + " (" + String(response.length()) + " chars" + token_info + "): " + response);
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

            JsonDocument error_filter;
            error_filter["error"]["code"] = true;
            JsonDocument error_doc;
            deserializeJson(error_doc, error_response, DeserializationOption::Filter(error_filter));
            String error_code = error_doc["error"]["code"] | "";
            chatgpt_last_model_unavailable = (httpCode == 404 || error_code == "model_not_found");
        } else {
            logMessage("CHATGPT: Negative HTTP code indicates internal HTTPClient error");
        }
    }

    http.end();
    return response;
}

// =============================================================================
// MODEL LIST (GET /v1/models)
// =============================================================================
class ChatGPTStreamReader {
public:
    ChatGPTStreamReader(Client& client, unsigned long timeout_ms) : client_(client), timeout_ms_(timeout_ms) {}

    int read() {
        unsigned long start = millis();
        while (client_.available() <= 0) {
            if (!client_.connected() || millis() - start >= timeout_ms_) {
                return -1;
            }
            delay(1);
        }
        int c = client_.read();
        if (c >= 0) {
            bytes_read_++;
        }
        return c;
    }

    size_t readBytes(char* buffer, size_t length) {
        size_t count = 0;
        while (count < length) {
            int c = read();
            if (c < 0) {
                break;
            }
            buffer[count++] = static_cast<char>(c);
        }
        return count;
    }

    size_t bytes_read() const {
        return bytes_read_;
    }

private:
    Client& client_;
    unsigned long timeout_ms_;
    size_t bytes_read_ = 0;
};

static bool chatgpt_is_chat_model(const String& id) {
    bool chat_family = id.startsWith("gpt-") || id.startsWith("chatgpt-") ||
                       (id.length() > 1 && id[0] == 'o' && isDigit(id[1]));
    if (!chat_family) {
        return false;
    }

    static const char* const non_chat_markers[] = {
        "audio", "realtime", "tts", "transcribe", "image", "embedding", "instruct", "search", "moderation"
    };
    for (const char* marker : non_chat_markers) {
        if (id.indexOf(marker) >= 0) {
            return false;
        }
    }
    return true;
}

bool chatgpt_get_models(bool refresh, std::vector<String>& models, String& error) {
    if (!refresh && !chatgpt_model_cache.empty()) {
        models = chatgpt_model_cache;
        return true;
    }

    String api_key = chatgpt_decode_api_key(String(chatgpt_config.api_key_b64));
    if (api_key.length() == 0) {
        error = "No API key configured";
        logMessage("CHATGPT: Model list failed - " + error);
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        error = "No WiFi connection (model list needs WiFi transport)";
        logMessage("CHATGPT: Model list failed - " + error);
        return false;
    }

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    http.useHTTP10(true);

    if (!http.begin(client, "https://api.openai.com/v1/models")) {
        error = "Could not open connection to api.openai.com";
        logMessage("CHATGPT: Model list failed - " + error);
        http.end();
        return false;
    }

    http.addHeader("Authorization", "Bearer " + api_key);
    http.setTimeout(15000);

    int httpCode = http.GET();
    if (httpCode != 200) {
        if (httpCode < 0) {
            error = "Connection error: " + HTTPClient::errorToString(httpCode);
        } else if (httpCode == 401) {
            error = "OpenAI rejected the API key (HTTP 401)";
        } else if (httpCode == 429) {
            error = "OpenAI rate limit or quota exceeded (HTTP 429)";
        } else {
            error = "OpenAI returned HTTP " + String(httpCode);
        }
        logMessage("CHATGPT: Model list failed - " + error);
        http.end();
        return false;
    }

    int content_length = http.getSize();

    JsonDocument filter;
    filter["data"][0]["id"] = true;
    filter["data"][0]["created"] = true;

    JsonDocument doc;
    ChatGPTStreamReader reader(http.getStream(), 15000);
    DeserializationError parse_error = deserializeJson(doc, reader, DeserializationOption::Filter(filter));
    http.end();

    logMessage("CHATGPT: Model list response " + String(reader.bytes_read()) + " of " + String(content_length) +
               " bytes read, largest free 8-bit block " + String(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)) + " bytes");

    if (parse_error == DeserializationError::IncompleteInput) {
        error = "Model list download stopped after " + String(reader.bytes_read()) + " of " + String(content_length) + " bytes (connection closed or timed out)";
        logMessage("CHATGPT: Model list failed - " + error);
        return false;
    }
    if (parse_error) {
        error = "Could not parse model list (" + String(parse_error.c_str()) + ")";
        logMessage("CHATGPT: Model list failed - " + error);
        return false;
    }

    JsonArray data = doc["data"].as<JsonArray>();
    std::vector<std::pair<uint32_t, String>> chat_models;
    for (JsonObject entry : data) {
        String id = entry["id"] | "";
        if (chatgpt_is_chat_model(id)) {
            chat_models.emplace_back(entry["created"].as<uint32_t>(), id);
        }
    }

    std::sort(chat_models.begin(), chat_models.end(),
              [](const std::pair<uint32_t, String>& a, const std::pair<uint32_t, String>& b) { return a.first > b.first; });

    chatgpt_model_cache.clear();
    for (const auto& entry : chat_models) {
        chatgpt_model_cache.push_back(entry.second);
    }

    logMessage("CHATGPT: Model list loaded, " + String(chatgpt_model_cache.size()) + " chat models of " + String(data.size()) + " available");
    models = chatgpt_model_cache;
    return true;
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

// =============================================================================
// MANUAL TEST (POST /chatgpt/test_prompt)
// =============================================================================
bool chatgpt_test_prompt(const String& prompt_text, const String& override_model, uint64_t capcode, float frequency,
                         bool mail_drop, String& model, String& response, String& error) {
    String api_key = chatgpt_decode_api_key(String(chatgpt_config.api_key_b64));
    if (api_key.length() == 0) {
        error = "No API key configured";
        return false;
    }

    model = chatgpt_resolve_model(override_model);
    if (model.length() == 0) {
        error = "No default model selected and no model set for this prompt";
        return false;
    }

    logMessage("CHATGPT: Testing prompt with model '" + model + "'");
    response = chatgpt_query_with_fallback(prompt_text, api_key, override_model, model, "test prompt");

    if (response.length() == 0) {
        if (chatgpt_last_model_unavailable) {
            error = "Model " + model + " unavailable";
        } else if (chatgpt_last_http_code == 200) {
            error = "Model returned an empty or unreadable response";
        } else if (chatgpt_last_http_code == 0) {
            error = "No WiFi connection or could not reach api.openai.com";
        } else if (chatgpt_last_http_code < 0) {
            error = "Connection error: " + HTTPClient::errorToString(chatgpt_last_http_code);
        } else {
            error = "OpenAI returned HTTP " + String(chatgpt_last_http_code);
        }
        logMessage("CHATGPT: Test prompt failed - " + error);
        return false;
    }

    response = truncate_message_with_ellipsis(response);

    if (!queue_add_message(capcode, frequency, settings.default_txpower, mail_drop, response.c_str())) {
        error = "Transmission queue is full";
        logMessage("CHATGPT: Test prompt failed - " + error);
        return false;
    }

    logMessage("CHATGPT: Test prompt response queued for transmission to " + String(capcode));
    return true;
}

#endif // ENABLE_CHATGPT
