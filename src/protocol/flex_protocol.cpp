#include "../protocol/flex_protocol.h"

#include "../../include/tinyflex/tinyflex.h"

#include "../core/config.h"
#include "../core/logging.h"
#include "../core/storage.h"
#include "../protocol/transmission.h"

static unsigned long last_emr_transmission = 0;
static bool first_message_sent = false;
static const unsigned long EMR_TIMEOUT_MS = 600000UL;

// =============================================================================
// FREQUENCY / CAPCODE
// =============================================================================
float apply_frequency_correction(float base_freq) {
    return base_freq * (1.0 + settings.frequency_correction_ppm / 1000000.0);
}

bool validate_flex_capcode(uint64_t capcode) {
    if (capcode < 1 || capcode > 4297068542ULL) {
        return false;
    }

    if (capcode >= 1933313UL && capcode <= 1998848UL) {
        return false;
    }

    if (capcode >= 2031615UL && capcode <= 2101248UL) {
        return false;
    }

    return true;
}

// =============================================================================
// MESSAGE SANITIZATION
// =============================================================================
String truncate_message_with_ellipsis(String message) {
    if (message.length() <= MAX_FLEX_MESSAGE_LENGTH) {
        return message;
    }
    return message.substring(0, 245) + "...";
}

String convert_unicode_to_ascii(String message) {

    static const struct {
        const char* unicode;
        const char* ascii;
    } replacements[] = {
        {"á", "a"}, {"é", "e"}, {"í", "i"}, {"ó", "o"}, {"ú", "u"}, {"ü", "u"},
        {"Á", "A"}, {"É", "E"}, {"Í", "I"}, {"Ó", "O"}, {"Ú", "U"}, {"Ü", "U"},
        {"ñ", "n"}, {"Ñ", "N"},
        {"à", "a"}, {"è", "e"}, {"ì", "i"}, {"ò", "o"}, {"ù", "u"},
        {"À", "A"}, {"È", "E"}, {"Ì", "I"}, {"Ò", "O"}, {"Ù", "U"},
        {"â", "a"}, {"ê", "e"}, {"î", "i"}, {"ô", "o"}, {"û", "u"},
        {"Â", "A"}, {"Ê", "E"}, {"Î", "I"}, {"Ô", "O"}, {"Û", "U"},
        {"¿", "?"}, {"¡", "!"}, {"°", "^"},
        {nullptr, nullptr}
    };

    for (int i = 0; replacements[i].unicode != nullptr; i++) {
        message.replace(replacements[i].unicode, replacements[i].ascii);
    }

    return message;
}

// =============================================================================
// ENCODING / EMR
// =============================================================================
bool flex_encode_and_store(uint64_t capcode, const char *message, bool mail_drop) {
    uint8_t flex_buffer[FLEX_BUFFER_SIZE];
    struct tf_message_config config = {0};
    config.mail_drop = mail_drop ? 1 : 0;

    int error = 0;
    size_t encoded_size = tf_encode_flex_message_ex(message, capcode, flex_buffer,
                                                   sizeof(flex_buffer), &error, &config);

    if (error < 0 || encoded_size == 0 || encoded_size > sizeof(tx_data_buffer)) {
        logMessagef("FLEX: Encoding failed (error=%d, size=%d)", error, (int)encoded_size);
        return false;
    }

    memcpy(tx_data_buffer, flex_buffer, encoded_size);
    current_tx_total_length = encoded_size;
    current_tx_remaining_length = encoded_size;

    return true;
}

void send_emr_if_needed() {
    bool need_emr = !first_message_sent || (millis() - last_emr_transmission) >= EMR_TIMEOUT_MS;

    if (need_emr) {
        uint8_t emr_pattern[EMR_PATTERN_SIZE];
        memcpy(emr_pattern, EMR_PATTERN, EMR_PATTERN_SIZE);
        radio.startTransmit(emr_pattern, EMR_PATTERN_SIZE);

        unsigned long emr_start = millis();
        while (radio.getPacketLength() > 0 && ((unsigned long)(millis() - emr_start) < 2000)) {
            delay(1);
        }
        delay(100);

        last_emr_transmission = millis();
        first_message_sent = true;
    }
}
