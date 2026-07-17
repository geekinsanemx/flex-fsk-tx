/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * ChatGPT Module - scheduled prompt execution via OpenAI API
 */

#ifndef CHATGPT_H
#define CHATGPT_H

#include <Arduino.h>
#include "../core/config.h"

#ifdef ENABLE_CHATGPT

#include <vector>

// =============================================================================
// STRUCTS
// =============================================================================
struct ChatGPTPrompt {
    uint8_t id;
    char name[32];
    char prompt[251];
    bool days[7];
    uint8_t hour;
    uint8_t minute;
    uint64_t capcode;
    float frequency;
    bool mail_drop;
    bool enabled;
    uint8_t retry_count;
    unsigned long next_retry_time;
};

struct ChatGPTConfig {
    bool enabled;
    char api_key_b64[300];
    bool chatgpt_notify_failures;
    std::vector<ChatGPTPrompt> prompts;
    uint8_t prompt_count;
};

struct ChatGPTActivity {
    unsigned long timestamp;
    char prompt_name[51];
    char query[101];
    char response[101];
    char datetime[20];
    bool query_success;
    bool transmission_success;
    uint64_t capcode;
    float frequency;
    int prompt_index;
    bool mail_drop;
};

// =============================================================================
// GLOBALS
// =============================================================================
extern ChatGPTConfig chatgpt_config;
extern ChatGPTActivity chatgpt_activity_log[10];
extern int chatgpt_activity_count;
extern unsigned long last_chatgpt_check;

// =============================================================================
// FUNCTIONS (implemented in Batch 4)
// =============================================================================
bool chatgpt_load_config();
bool chatgpt_save_config();
void chatgpt_check_schedules();
String sanitize_chatgpt_prompt(String input);
String chatgpt_encode_api_key(String api_key);
String chatgpt_decode_api_key(String encoded_key);
bool chatgpt_validate_api_key(String api_key);
String chatgpt_format_next_execution(ChatGPTPrompt& prompt);

#endif // ENABLE_CHATGPT

#endif // CHATGPT_H
