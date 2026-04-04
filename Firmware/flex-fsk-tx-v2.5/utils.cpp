/*
 * FLEX Paging Message Transmitter - v2.5
 * Utility Functions Implementation
 */

#include "utils.h"
#include "config.h"
#include "storage.h"
#include "boards/boards.h"

// =============================================================================
// STRING CONVERSION
// =============================================================================
int str2uint64(uint64_t *out, const char *s) {
    if (!s || s[0] == '\0') return -1;

    uint64_t result = 0;
    const char *p = s;

    while (*p) {
        if (*p < '0' || *p > '9') return -1;

        if (result > (UINT64_MAX - (*p - '0')) / 10) return -1;

        result = result * 10 + (*p - '0');
        p++;
    }

    *out = result;
    return 0;
}

float apply_frequency_correction(float base_freq) {
    extern CoreConfig core_config;
    return base_freq * (1.0 + core_config.frequency_correction_ppm / 1000000.0);
}

// =============================================================================
// VALIDATION
// =============================================================================
bool validate_flex_capcode(uint64_t capcode) {
    if (capcode >= FLEX_CAPCODE_MIN_1 && capcode <= FLEX_CAPCODE_MAX_1) return true;
    if (capcode >= FLEX_CAPCODE_MIN_2 && capcode <= FLEX_CAPCODE_MAX_2) return true;
    if (capcode >= FLEX_CAPCODE_MIN_3 && capcode <= FLEX_CAPCODE_MAX_3) return true;

    return false;
}

bool is_reserved_pin(uint8_t pin) {
    if (pin == 0) return true;
    if (pin == LORA_CS_PIN) return true;
    if (pin == LORA_IRQ_PIN) return true;
    if (pin == LORA_RST_PIN) return true;
    if (pin == LORA_GPIO_PIN) return true;
    if (pin == LORA_SCK_PIN) return true;
    if (pin == LORA_MOSI_PIN) return true;
    if (pin == LORA_MISO_PIN) return true;
    if (pin == OLED_SDA_PIN) return true;
    if (pin == OLED_SCL_PIN) return true;
    if (pin == LED_PIN) return true;
    if (pin == BATTERY_ADC_PIN) return true;
    if (OLED_RST_PIN != -1 && pin == OLED_RST_PIN) return true;
    if (VEXT_PIN != -1 && pin == VEXT_PIN) return true;

    return false;
}

// =============================================================================
// MESSAGE PROCESSING
// =============================================================================
String truncate_message_with_ellipsis(const String& message) {
    if (message.length() <= MAX_FLEX_MESSAGE_LENGTH) {
        return message;
    }
    return message.substring(0, 245) + "...";
}

// =============================================================================
// TIME UTILITIES
// =============================================================================
String format_uptime(unsigned long milliseconds) {
    unsigned long seconds = milliseconds / 1000;
    unsigned long days = seconds / 86400;
    unsigned long hours = (seconds % 86400) / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    unsigned long secs = seconds % 60;

    char buffer[64];
    if (days > 0) {
        snprintf(buffer, sizeof(buffer), "%lu days, %lu hours, %lu mins", days, hours, minutes);
    } else if (hours > 0) {
        snprintf(buffer, sizeof(buffer), "%lu hours, %lu mins", hours, minutes);
    } else if (minutes > 0) {
        snprintf(buffer, sizeof(buffer), "%lu mins, %lu secs", minutes, secs);
    } else {
        snprintf(buffer, sizeof(buffer), "%lu secs", secs);
    }

    return String(buffer);
}
