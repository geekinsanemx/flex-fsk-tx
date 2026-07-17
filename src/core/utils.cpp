#include "../core/utils.h"

// =============================================================================
// BASE64
// =============================================================================
String base64_encode_string(const String& input) {
    if (input.length() == 0) return "";

    const char* chars_to_encode = input.c_str();
    int in_len = input.length();
    String ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    const String base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while (in_len--) {
        char_array_3[i++] = *(chars_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++) {
                ret += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++) {
            ret += base64_chars[char_array_4[j]];
        }

        while((i++ < 3)) {
            ret += '=';
        }
    }

    return ret;
}

String base64_decode_string(const String& encoded_string) {
    if (encoded_string.length() == 0) return "";

    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    String result;
    result.reserve((encoded_string.length() * 3) / 4);

    int val = 0;
    int valb = -8;

    for (int i = 0; i < encoded_string.length(); i++) {
        char c = encoded_string[i];
        if (c == '=') break;

        int pos = -1;
        for (int j = 0; j < 64; j++) {
            if (base64_chars[j] == c) {
                pos = j;
                break;
            }
        }

        if (pos == -1) continue;

        val = (val << 6) + pos;
        valb += 6;

        if (valb >= 0) {
            result += char((val >> valb) & 0xFF);
            valb -= 8;
        }
    }

    return result;
}

String base64_decode(String input) {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String result = "";
    int val = 0, valb = -8;

    for (char c : input) {
        if (c == '=') break;
        const char* pos = strchr(chars, c);
        if (!pos) continue;

        val = (val << 6) + (pos - chars);
        valb += 6;
        if (valb >= 0) {
            result += (char)((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return result;
}

// =============================================================================
// HTML / JSON ESCAPING
// =============================================================================
String htmlEscape(const String& str) {
    String escaped = "";
    escaped.reserve(str.length() * 1.2);
    for (unsigned int i = 0; i < str.length(); i++) {
        char c = str[i];
        switch(c) {
            case '&':  escaped += "&amp;"; break;
            case '<':  escaped += "&lt;"; break;
            case '>':  escaped += "&gt;"; break;
            case '"':  escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default:   escaped += c; break;
        }
    }
    return escaped;
}

String json_escape_string(String input) {
    String output = "";
    output.reserve(input.length() + 20);

    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input.charAt(i);
        switch (c) {
            case '"':  output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '/':  output += "\\/"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:
                if (c < 32) {
                    output += "\\u00";
                    if (c < 16) output += "0";
                    output += String(c, HEX);
                } else {
                    output += c;
                }
                break;
        }
    }
    return output;
}

// =============================================================================
// CRC32
// =============================================================================
String calculate_crc32(const String& input) {
    uint32_t crc = 0xFFFFFFFF;
    const char* data = input.c_str();
    int len = input.length();

    for (int i = 0; i < len; i++) {
        uint8_t byte = data[i];
        crc = crc ^ byte;
        for (uint8_t j = 0; j < 8; j++) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    crc = ~crc;

    char hex_string[9];
    sprintf(hex_string, "%08X", crc);
    return String(hex_string);
}

// =============================================================================
// IP ADDRESS STRING HELPERS
// =============================================================================
String ip_array_to_string(uint8_t ip[4]) {
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

void string_to_ip_array(const String& ip_str, uint8_t ip[4]) {
    int parts[4];
    sscanf(ip_str.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);
    for (int i = 0; i < 4; i++) {
        ip[i] = (uint8_t)parts[i];
    }
}

void parse_ip_string(const String& ip_str, uint8_t ip[4]) {
    int parts[4];
    int part_count = sscanf(ip_str.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);

    if (part_count == 4) {
        for (int i = 0; i < 4; i++) {
            if (parts[i] >= 0 && parts[i] <= 255) {
                ip[i] = parts[i];
            }
        }
    }
}

// =============================================================================
// CERTIFICATE STATUS
// =============================================================================
bool has_valid_certificate(const char* cert) {
    if (!cert || strlen(cert) < 50) {
        return false;
    }

    return (strstr(cert, "-----BEGIN") != NULL && strstr(cert, "-----END") != NULL);
}

String get_cert_status(const char* saved_cert) {
    return has_valid_certificate(saved_cert) ? "✅" : "❌";
}
