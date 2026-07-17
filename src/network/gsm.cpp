/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * GSM Module - A7670SA/SIM800L modem transport (SIM7600/SIM800 TinyGSM drivers)
 */

#include "../core/config.h"
#include "../network/gsm.h"
#include "../network/network.h"
#include "../network/wifi.h"
#include "../core/logging.h"
#include "../core/hardware.h"
#include "../core/display.h"
#include "../protocol/transmission.h"
#include "../network/ntp_time.h"
#include "../web/web_server.h"
#include "../../include/boards/boards.h"

#ifdef ENABLE_GSM

#include <type_traits>
#include <utility>

#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_USE_WIFI false

namespace TinyGsmNS7600 {
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClientSIM7600.h>
#undef TINY_GSM_MODEM_SIM7600
}

#ifdef MODEM_MANUFACTURER
#undef MODEM_MANUFACTURER
#endif
#ifdef MODEM_MODEL
#undef MODEM_MODEL
#endif
#ifdef TINY_GSM_MUX_COUNT
#undef TINY_GSM_MUX_COUNT
#endif
#ifdef TINY_GSM_BUFFER_READ_AND_CHECK_SIZE
#undef TINY_GSM_BUFFER_READ_AND_CHECK_SIZE
#endif
#ifdef AT_NL
#undef AT_NL
#endif
#ifdef TinyGsmFifo_h
#undef TinyGsmFifo_h
#endif

#ifdef SRC_TINYGSMCOMMON_H_
#undef SRC_TINYGSMCOMMON_H_
#endif
#ifdef SRC_TINYGSMMODEM_H_
#undef SRC_TINYGSMMODEM_H_
#endif
#ifdef SRC_TINYGSMTCP_H_
#undef SRC_TINYGSMTCP_H_
#endif
#ifdef SRC_TINYGSMGPRS_H_
#undef SRC_TINYGSMGPRS_H_
#endif
#ifdef SRC_TINYGSMCALLING_H_
#undef SRC_TINYGSMCALLING_H_
#endif
#ifdef SRC_TINYGSMSMS_H_
#undef SRC_TINYGSMSMS_H_
#endif
#ifdef SRC_TINYGSMGSMLOCATION_H_
#undef SRC_TINYGSMGSMLOCATION_H_
#endif
#ifdef SRC_TINYGSMGPS_H_
#undef SRC_TINYGSMGPS_H_
#endif
#ifdef SRC_TINYGSMTIME_H_
#undef SRC_TINYGSMTIME_H_
#endif
#ifdef SRC_TINYGSMNTP_H_
#undef SRC_TINYGSMNTP_H_
#endif
#ifdef SRC_TINYGSMBATTERY_H_
#undef SRC_TINYGSMBATTERY_H_
#endif
#ifdef SRC_TINYGSMTEMPERATURE_H_
#undef SRC_TINYGSMTEMPERATURE_H_
#endif
#ifdef SRC_TINYGSMSSL_H_
#undef SRC_TINYGSMSSL_H_
#endif

namespace TinyGsmNS800 {
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClientSIM800.h>
#undef TINY_GSM_MODEM_SIM800
}

using TinyGsmSim7600 = TinyGsmNS7600::TinyGsmSim7600;
using TinyGsmClientSim7600 = TinyGsmNS7600::TinyGsmSim7600::GsmClientSim7600;
using TinyGsmSim800 = TinyGsmNS800::TinyGsmSim800;
using TinyGsmClientSim800 = TinyGsmNS800::TinyGsmSim800::GsmClientSim800;

#include "../../include/gsm_trust_anchors/gsm_trust_anchors.h"

// =============================================================================
// MODEM/CLIENT OBJECTS (file-local: raw TinyGsm objects never touched outside gsm.cpp)
// =============================================================================
#ifdef ESP32
HardwareSerial SerialGSM(2);
#endif

static TinyGsmSim7600 modem_a7670(SerialGSM);
static TinyGsmClientSim7600 gsm_client_a7670(modem_a7670);
static TinyGsmSim800 modem_sim800(SerialGSM);
static TinyGsmClientSim800 gsm_client_sim800(modem_sim800);

static const int GSM_SSL_RNG_PIN = 34;
static SSLClient gsm_client_secure_a7670(gsm_client_a7670, GSM_TAs, GSM_TAS_NUM, GSM_SSL_RNG_PIN);
static SSLClient gsm_client_secure_sim800(gsm_client_sim800, GSM_TAs, GSM_TAS_NUM, GSM_SSL_RNG_PIN);

// =============================================================================
// GLOBALS
// =============================================================================
GSMConfig gsm_config;

bool gsm_connected = false;
bool gsm_internet_verified = false;
uint8_t gsm_internet_test_attempt = 0;
String gsm_ip_address = "";
bool gsm_boot_modes_exhausted = false;

GsmModuleType gsm_module_type = GSM_MODULE_A7670SA;
bool gsm_module_detected = false;
bool gsm_power_state = false;
bool gsm_modem_ready = false;
bool gsm_registration_complete = false;
String gsm_operator_name = "";
int gsm_signal_quality = 0;
String gsm_gateway_address = "";
String gsm_dns_primary = "";
String gsm_dns_secondary = "";
String gsm_subnet_mask = "";

static unsigned long gsm_power_last_toggle = 0;
static bool gsm_initialized = false;
static bool gsm_tcp_warmed_up = false;
static unsigned long gsm_connect_start = 0;
static uint8_t gsm_retry_count = 0;
static int gsm_current_network_mode_index = 0;
static unsigned long last_gsm_health_check = 0;
static unsigned long last_wifi_retry_from_gsm = 0;

static const int GSM_INTERNET_TEST_MAX_ATTEMPTS = 10;
static const char* GSM_INTERNET_TEST_HOST = "checkip.amazonaws.com";
static const uint16_t GSM_INTERNET_TEST_PORT = 80;
static const char* GSM_INTERNET_TEST_PATH = "/";

enum ModemClockSyncResult {
    MODEM_CLOCK_RESULT_NONE = 0,
    MODEM_CLOCK_RESULT_SET,
    MODEM_CLOCK_RESULT_SKIPPED,
    MODEM_CLOCK_RESULT_INVALID
};
static ModemClockSyncResult last_modem_clock_result = MODEM_CLOCK_RESULT_NONE;

// =============================================================================
// MODULE CAPABILITY TABLES
// =============================================================================
struct GsmModuleCapabilities {
    const int* network_modes;
    const char* const* network_mode_names;
    size_t network_mode_count;
    bool require_internet_verification;
    bool supports_ntp_client;
    bool supports_gnss;
};

static const int GSM_A76XX_NETWORK_MODES[] = {38, 14, 13};
static const char* const GSM_A76XX_NETWORK_MODE_NAMES[] = {"LTE", "3G", "2G"};

static const int GSM_SIM800_NETWORK_MODES[] = {13};
static const char* const GSM_SIM800_NETWORK_MODE_NAMES[] = {"2G"};

static const GsmModuleCapabilities GSM_CAPABILITIES_A76XX = {
    GSM_A76XX_NETWORK_MODES,
    GSM_A76XX_NETWORK_MODE_NAMES,
    sizeof(GSM_A76XX_NETWORK_MODES) / sizeof(GSM_A76XX_NETWORK_MODES[0]),
    true,   // require_internet_verification
    true,   // supports_ntp_client
    true    // supports_gnss
};

static const GsmModuleCapabilities GSM_CAPABILITIES_SIM800 = {
    GSM_SIM800_NETWORK_MODES,
    GSM_SIM800_NETWORK_MODE_NAMES,
    sizeof(GSM_SIM800_NETWORK_MODES) / sizeof(GSM_SIM800_NETWORK_MODES[0]),
    false,  // require_internet_verification
    false,  // supports_ntp_client
    false   // supports_gnss
};

static const GsmModuleCapabilities& gsm_capabilities_for_type(GsmModuleType type) {
    switch (type) {
        case GSM_MODULE_SIM800L:
            return GSM_CAPABILITIES_SIM800;
        case GSM_MODULE_A7670SA:
        default:
            return GSM_CAPABILITIES_A76XX;
    }
}

static const GsmModuleCapabilities& gsm_module_capabilities() {
    return gsm_capabilities_for_type(gsm_module_type);
}

static bool gsm_module_is_sim800() {
    return gsm_module_type == GSM_MODULE_SIM800L;
}

static bool gsm_module_supports_multimode() {
    return gsm_module_capabilities().network_mode_count > 1;
}

bool gsm_module_requires_internet_verification() {
    return gsm_module_capabilities().require_internet_verification;
}

static bool gsm_module_supports_modem_ntp() {
    return gsm_module_capabilities().supports_ntp_client;
}

const char* gsm_module_label(GsmModuleType module) {
    switch (module) {
        case GSM_MODULE_SIM800L:
            return "SIM800L";
        case GSM_MODULE_A7670SA:
        default:
            return "A7670SA";
    }
}

struct GsmModuleSignature {
    const char* keyword;
    GsmModuleType type;
};

static const GsmModuleSignature GSM_MODULE_SIGNATURES[] = {
    {"A7670", GSM_MODULE_A7670SA},
    {"A7600", GSM_MODULE_A7670SA},
    {"SIM7600", GSM_MODULE_A7670SA},
    {"SIM7670", GSM_MODULE_A7670SA},
    {"SIM800", GSM_MODULE_SIM800L},
    {"SIM800L", GSM_MODULE_SIM800L}
};

static void gsm_detect_module_from_info(const String& modem_info) {
    String info_upper = modem_info;
    info_upper.toUpperCase();
    GsmModuleType detected = GSM_MODULE_A7670SA;
    bool matched = false;

    for (const auto& signature : GSM_MODULE_SIGNATURES) {
        if (info_upper.indexOf(signature.keyword) >= 0) {
            detected = signature.type;
            matched = true;
            break;
        }
    }

    if (!gsm_module_detected || gsm_module_type != detected) {
        gsm_module_type = detected;
        gsm_module_detected = true;
        gsm_current_network_mode_index = 0;
        logMessagef("GSM: Module family detected: %s (%s)",
                    gsm_module_label(gsm_module_type),
                    matched ? "ATI match" : "default");
    }
}

// =============================================================================
// DISPATCH TEMPLATES (compile-time A7670SA/SIM800L modem selection)
// =============================================================================
template<typename ReturnType, typename Func, typename Target>
inline ReturnType gsm_dispatch(Func&& func, Target& target) {
    if constexpr (std::is_void<ReturnType>::value) {
        std::forward<Func>(func)(target);
    } else if constexpr (std::is_enum<ReturnType>::value) {
        return static_cast<ReturnType>(static_cast<int>(std::forward<Func>(func)(target)));
    } else {
        return std::forward<Func>(func)(target);
    }
}

template<typename Func>
auto with_gsm_modem(Func&& func) -> decltype(func(modem_a7670)) {
    using ReturnType = decltype(func(modem_a7670));
    if (gsm_module_is_sim800()) {
        if constexpr (std::is_void<ReturnType>::value) {
            gsm_dispatch<ReturnType>(std::forward<Func>(func), modem_sim800);
            return;
        } else {
            return gsm_dispatch<ReturnType>(std::forward<Func>(func), modem_sim800);
        }
    }
    if constexpr (std::is_void<ReturnType>::value) {
        gsm_dispatch<ReturnType>(std::forward<Func>(func), modem_a7670);
        return;
    } else {
        return gsm_dispatch<ReturnType>(std::forward<Func>(func), modem_a7670);
    }
}

template<typename Func>
auto with_gsm_client(Func&& func) -> decltype(func(gsm_client_a7670)) {
    using ReturnType = decltype(func(gsm_client_a7670));
    if (gsm_module_is_sim800()) {
        if constexpr (std::is_void<ReturnType>::value) {
            gsm_dispatch<ReturnType>(std::forward<Func>(func), gsm_client_sim800);
            return;
        } else {
            return gsm_dispatch<ReturnType>(std::forward<Func>(func), gsm_client_sim800);
        }
    }
    if constexpr (std::is_void<ReturnType>::value) {
        gsm_dispatch<ReturnType>(std::forward<Func>(func), gsm_client_a7670);
        return;
    } else {
        return gsm_dispatch<ReturnType>(std::forward<Func>(func), gsm_client_a7670);
    }
}

template<typename Func>
auto with_gsm_ssl_client(Func&& func) -> decltype(func(gsm_client_secure_a7670)) {
    using ReturnType = decltype(func(gsm_client_secure_a7670));
    if (gsm_module_is_sim800()) {
        if constexpr (std::is_void<ReturnType>::value) {
            gsm_dispatch<ReturnType>(std::forward<Func>(func), gsm_client_secure_sim800);
            return;
        } else {
            return gsm_dispatch<ReturnType>(std::forward<Func>(func), gsm_client_secure_sim800);
        }
    }
    if constexpr (std::is_void<ReturnType>::value) {
        gsm_dispatch<ReturnType>(std::forward<Func>(func), gsm_client_secure_a7670);
        return;
    } else {
        return gsm_dispatch<ReturnType>(std::forward<Func>(func), gsm_client_secure_a7670);
    }
}

// =============================================================================
// MODEM/CLIENT WRAPPERS
// =============================================================================
static bool gsm_modem_testAT() {
    return with_gsm_modem([](auto& modem) { return modem.testAT(); });
}

static String gsm_modem_get_info() {
    return with_gsm_modem([](auto& modem) { return modem.getModemInfo(); });
}

static bool gsm_modem_sim_unlock(const char* pin) {
    return with_gsm_modem([pin](auto& modem) { return modem.simUnlock(pin); });
}

static int gsm_modem_get_sim_status() {
    return with_gsm_modem([](auto& modem) -> int { return modem.getSimStatus(); });
}

template<typename... Args>
static void gsm_modem_sendAT(Args&&... args) {
    with_gsm_modem([&](auto& modem) {
        modem.sendAT(std::forward<Args>(args)...);
        return 0;
    });
}

template<typename... Args>
static int gsm_modem_waitResponse(Args&&... args) {
    return with_gsm_modem([&](auto& modem) {
        return modem.waitResponse(std::forward<Args>(args)...);
    });
}

static int gsm_modem_get_registration_status() {
    return with_gsm_modem([](auto& modem) -> int {
        return modem.getRegistrationStatus();
    });
}

static String gsm_modem_get_operator() {
    return with_gsm_modem([](auto& modem) { return modem.getOperator(); });
}

static int gsm_modem_get_signal_quality() {
    return with_gsm_modem([](auto& modem) { return modem.getSignalQuality(); });
}

bool gsm_modem_gprs_connect(const char* apn, const char* user, const char* pass) {
    return with_gsm_modem([&](auto& modem) {
        return modem.gprsConnect(apn, user, pass);
    });
}

static void gsm_modem_gprs_disconnect() {
    with_gsm_modem([](auto& modem) {
        modem.gprsDisconnect();
        return 0;
    });
}

bool gsm_modem_is_gprs_connected() {
    return with_gsm_modem([](auto& modem) { return modem.isGprsConnected(); });
}

static String gsm_modem_get_datetime() {
    if (gsm_module_is_sim800()) {
        return modem_sim800.getGSMDateTime(TinyGsmNS800::DATE_TIME);
    }
    return modem_a7670.getGSMDateTime(TinyGsmNS7600::DATE_TIME);
}

static String gsm_modem_get_local_ip() {
    return with_gsm_modem([](auto& modem) { return modem.getLocalIP(); });
}

static void gsm_modem_set_network_mode(int mode) {
    if (!gsm_module_supports_multimode()) {
        return;
    }
    modem_a7670.setNetworkMode(mode);
}

static Stream& gsm_modem_stream() {
    if (gsm_module_is_sim800()) {
        return modem_sim800.stream;
    }
    return modem_a7670.stream;
}

static bool gsm_client_connect(const char* host, uint16_t port) {
    return with_gsm_client([&](auto& client) {
        return client.connect(host, port);
    });
}

static bool gsm_client_connected() {
    return with_gsm_client([](auto& client) { return client.connected(); });
}

static int gsm_client_available() {
    return with_gsm_client([](auto& client) { return client.available(); });
}

static String gsm_client_read_string_until(char terminator) {
    return with_gsm_client([&](auto& client) {
        return client.readStringUntil(terminator);
    });
}

static int gsm_client_read_byte() {
    return with_gsm_client([](auto& client) {
        return client.read();
    });
}

static void gsm_client_stop() {
    with_gsm_client([](auto& client) {
        client.stop();
        return 0;
    });
}

static void gsm_client_clear_write_error() {
    with_gsm_client([](auto& client) {
        client.clearWriteError();
        return 0;
    });
}

static void gsm_ssl_clear_write_error() {
    with_gsm_ssl_client([](auto& client) {
        client.clearWriteError();
        return 0;
    });
}

static void gsm_ssl_stop() {
    with_gsm_ssl_client([](auto& client) {
        client.stop();
        return 0;
    });
}

void gsm_ssl_set_mutual_params(const SSLClientParameters& params) {
    with_gsm_ssl_client([&](auto& client) {
        client.setMutualAuthParams(params);
        return 0;
    });
}

void gsm_ssl_set_timeout(uint32_t timeout_ms) {
    with_gsm_ssl_client([&](auto& client) {
        client.setTimeout(timeout_ms);
        return 0;
    });
}

int gsm_ssl_get_write_error() {
    return with_gsm_ssl_client([](auto& client) {
        return client.getWriteError();
    });
}

SSLClient& gsm_active_ssl_client() {
    return gsm_module_is_sim800() ? gsm_client_secure_sim800 : gsm_client_secure_a7670;
}

static bool parse_modem_datetime(const String& datetime, time_t& epoch_out) {
    if (datetime.length() < 20) {
        return false;
    }

    int yy = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, tz_quarters = 0;
    char tz_sign = '+';
    if (sscanf(datetime.c_str(), "%d/%d/%d,%d:%d:%d%c%d",
               &yy, &month, &day, &hour, &minute, &second, &tz_sign, &tz_quarters) != 8) {
        return false;
    }

    if (month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 59) {
        return false;
    }

    int full_year = (yy >= 70 ? 1900 + yy : 2000 + yy);
    struct tm tm_time;
    memset(&tm_time, 0, sizeof(tm_time));
    tm_time.tm_year = full_year - 1900;
    tm_time.tm_mon = month - 1;
    tm_time.tm_mday = day;
    tm_time.tm_hour = hour;
    tm_time.tm_min = minute;
    tm_time.tm_sec = second;
    tm_time.tm_isdst = 0;

#if defined(__USE_BSD) || defined(__USE_MISC) || defined(__USE_XOPEN)
    time_t local_epoch = timegm(&tm_time);
#else
    time_t local_epoch = mktime(&tm_time);
#endif

    if (local_epoch == (time_t)-1) {
        return false;
    }

    int tz_sign_mult = (tz_sign == '-') ? -1 : 1;
    long tz_offset_seconds = tz_sign_mult * (long)tz_quarters * 15L * 60L;
    epoch_out = local_epoch - tz_offset_seconds;
    return true;
}

static bool string_is_ipv4_address(const String& value) {
    int segments = 0;
    int current = 0;
    bool has_digit = false;

    for (size_t i = 0; i < value.length(); ++i) {
        char c = value.charAt(i);
        if (c == '.') {
            if (!has_digit) {
                return false;
            }
            segments++;
            current = 0;
            has_digit = false;
            continue;
        }
        if (c < '0' || c > '9') {
            return false;
        }
        has_digit = true;
        current = current * 10 + (c - '0');
        if (current > 255) {
            return false;
        }
    }

    return (segments == 3) && has_digit;
}

// =============================================================================
// NETWORK MODE CYCLING (LTE -> 3G -> 2G fallback for A76xx; single 2G for SIM800)
// =============================================================================
size_t gsm_module_network_mode_count() {
    return gsm_module_capabilities().network_mode_count;
}

static size_t gsm_active_network_mode_slot() {
    size_t count = gsm_module_network_mode_count();
    if (count == 0) {
        return 0;
    }

    if (gsm_current_network_mode_index < 0) {
        gsm_current_network_mode_index = 0;
    }

    size_t slot = static_cast<size_t>(gsm_current_network_mode_index);
    if (slot >= count) {
        gsm_current_network_mode_index = 0;
        slot = 0;
    }
    return slot;
}

static int gsm_current_network_mode_value() {
    const auto& caps = gsm_module_capabilities();
    size_t slot = gsm_active_network_mode_slot();
    if (caps.network_mode_count == 0) {
        return 0;
    }
    return caps.network_modes[slot];
}

const char* gsm_current_network_mode_name() {
    const auto& caps = gsm_module_capabilities();
    size_t slot = gsm_active_network_mode_slot();
    if (caps.network_mode_count == 0) {
        return "GSM";
    }
    return caps.network_mode_names[slot];
}

static bool gsm_has_next_network_mode() {
    size_t count = gsm_module_network_mode_count();
    if (count == 0) {
        return false;
    }
    size_t slot = gsm_active_network_mode_slot();
    return slot < (count - 1);
}

static bool gsm_advance_network_mode() {
    if (!gsm_has_next_network_mode()) {
        return false;
    }
    size_t next_slot = gsm_active_network_mode_slot() + 1;
    gsm_current_network_mode_index = static_cast<int>(next_slot);
    return true;
}

static void gsm_reset_network_mode_cycle() {
    gsm_current_network_mode_index = 0;
}

static String gsm_network_mode_summary_for_type(GsmModuleType type) {
    const auto& caps = gsm_capabilities_for_type(type);
    if (caps.network_mode_count == 0) {
        return "none";
    }

    String summary = caps.network_mode_names[0];
    for (size_t i = 1; i < caps.network_mode_count; ++i) {
        summary += "/";
        summary += caps.network_mode_names[i];
    }
    return summary;
}

static String gsm_network_mode_summary() {
    return gsm_network_mode_summary_for_type(gsm_module_type);
}

// =============================================================================
// ASYNC DELAY (non-blocking wait that keeps watchdog fed and web server alive)
// =============================================================================
static void async_delay(unsigned long ms) {
    unsigned long start = millis();
    while ((unsigned long)(millis() - start) < ms) {
        feed_watchdog();
        yield();
        if (wifi_connected || ap_mode_active) {
            webServer.handleClient();
        }
    }
}

// =============================================================================
// POWER / LIFECYCLE
// =============================================================================
static void gsm_clear_network_info() {
    gsm_ip_address = "";
    gsm_gateway_address = "";
    gsm_dns_primary = "";
    gsm_dns_secondary = "";
    gsm_subnet_mask = "";
}

static void gsm_power_on(bool force = false) {
    if (!gsm_config.enable_gsm && !force) {
        return;
    }

    if (gsm_power_state) {
        return;
    }

    int actual_power_pin = (gsm_config.power_pin == 0) ? GSM_PWR_PIN : gsm_config.power_pin;

    pinMode(actual_power_pin, OUTPUT);
    digitalWrite(actual_power_pin, HIGH);
    gsm_power_state = true;
    gsm_power_last_toggle = millis();
    logMessagef("GSM: Power pin HIGH (GPIO%d)", actual_power_pin);
    async_delay(GSM_BOOT_STABILIZE_MS);
}

void gsm_power_off() {
    if (!gsm_power_state) {
        return;
    }

    int actual_power_pin = (gsm_config.power_pin == 0) ? GSM_PWR_PIN : gsm_config.power_pin;

    digitalWrite(actual_power_pin, LOW);
    gsm_power_state = false;
    gsm_modem_ready = false;
    gsm_initialized = false;
    gsm_connected = false;
    gsm_registration_complete = false;
    gsm_tcp_warmed_up = false;
    gsm_internet_verified = false;
    gsm_internet_test_attempt = 0;
    gsm_clear_network_info();
    gsm_power_last_toggle = millis();
    logMessagef("GSM: Power pin LOW (GPIO%d)", actual_power_pin);
    change_device_state(STATE_IDLE);
}

String gsm_get_ip_address() {
    if (gsm_module_is_sim800()) {
        String ip = gsm_modem_get_local_ip();
        if (ip.length() == 0) {
            ip = "0.0.0.0";
        }
        return ip;
    }

    gsm_modem_sendAT("+CGPADDR=1");
    String response = "";
    if (gsm_modem_waitResponse(10000L, response) == 1) {
        int colon = response.indexOf(':');
        if (colon != -1) {
            String data = response.substring(colon + 1);
            data.trim();

            int comma = data.indexOf(',');
            if (comma != -1) {
                data = data.substring(comma + 1);
                data.trim();
            }

            int quote1 = data.indexOf('"');
            int quote2 = data.lastIndexOf('"');
            if (quote1 != -1 && quote2 != -1 && quote2 > quote1) {
                return data.substring(quote1 + 1, quote2);
            }

            data.replace("\"", "");
            data.replace("\r", "");
            data.replace("\n", "");
            data.replace("OK", "");
            data.trim();

            if (data.length() > 0 && data.indexOf('.') != -1) {
                return data;
            }
        }
    }
    return "0.0.0.0";
}

void gsm_update_network_info() {
    if (!gsm_connected) {
        gsm_clear_network_info();
        return;
    }

    gsm_ip_address = gsm_get_ip_address();
    if (gsm_ip_address == "0.0.0.0" || gsm_ip_address.length() == 0) {
        gsm_ip_address = "0.0.0.0";
    }
}

bool gsm_initialize() {
    if (gsm_initialized && gsm_modem_ready) {
        return true;
    }

    int actual_power_pin = (gsm_config.power_pin == 0) ? GSM_PWR_PIN : gsm_config.power_pin;
    int actual_rx_pin = (gsm_config.rx_pin == 0) ? GSM_RX_PIN : gsm_config.rx_pin;
    int actual_tx_pin = (gsm_config.tx_pin == 0) ? GSM_TX_PIN : gsm_config.tx_pin;
    unsigned long actual_baudrate = (gsm_config.baudrate == 0) ? 115200 : gsm_config.baudrate;

    logMessage("NETWORK: Initializing GSM module...");
    change_device_state(STATE_GSM_INITIALIZING);

    pinMode(actual_power_pin, OUTPUT);
    gsm_power_on();

    if (!gsm_power_state) {
        logMessage("GSM: Unable to enable power");
        change_device_state(STATE_ERROR);
        return false;
    }

    SerialGSM.begin(actual_baudrate, SERIAL_8N1, actual_tx_pin, actual_rx_pin);
    logMessagef("GSM: Serial2 started @ %lu baud (TX=%u RX=%u)",
                (unsigned long)actual_baudrate,
                actual_tx_pin,
                actual_rx_pin);

    unsigned long start = millis();
    bool at_ready = false;
    while ((millis() - start) < GSM_AT_READY_TIMEOUT_MS) {
        feed_watchdog();
        if (gsm_modem_testAT()) {
            at_ready = true;
            break;
        }
        async_delay(100);
    }

    if (!at_ready) {
        logMessage("GSM: AT communication failed within timeout");
        gsm_power_off();
        change_device_state(STATE_ERROR);
        return false;
    }

    logMessage("GSM: AT communication successful");
    gsm_modem_ready = true;

    String modem_info = gsm_modem_get_info();
    logMessage("GSM: Module info: " + modem_info);
    gsm_detect_module_from_info(modem_info);

    if (strlen(gsm_config.pin) > 0) {
        logMessage("GSM: Unlocking SIM with PIN...");
        if (!gsm_modem_sim_unlock(gsm_config.pin)) {
            logMessage("GSM: SIM unlock failed");
            change_device_state(STATE_ERROR);
            return false;
        }
        logMessage("GSM: SIM unlocked successfully");
    }

    logMessage("GSM: Waiting for SIM card...");
    bool sim_ready = false;
    for (int retry = 0; retry < GSM_SIM_READY_RETRIES; retry++) {
        feed_watchdog();
        if (gsm_modem_get_sim_status() == TinyGsmNS7600::SIM_READY) {
            sim_ready = true;
            break;
        }
        async_delay(GSM_SIM_POLL_DELAY_MS);
    }

    if (!sim_ready) {
        logMessage("GSM: SIM card not ready - check SIM insertion");
        change_device_state(STATE_ERROR);
        return false;
    }

    logMessage("GSM: SIM card ready");

    logMessage("GSM: Enabling network time sync (AT+CLTS=1)");
    gsm_modem_sendAT("+CLTS=1");
    gsm_modem_waitResponse(5000L);
    if (gsm_module_supports_modem_ntp()) {
        logMessage("GSM: Configuring modem NTP client (AT+CNTP)");
        gsm_modem_sendAT("+CNTP=\"pool.ntp.org\",0");
        gsm_modem_waitResponse(5000L);
        gsm_modem_sendAT("+CNTP");
        gsm_modem_waitResponse(5000L);
    }
    gsm_modem_sendAT("&W");
    gsm_modem_waitResponse(5000L);

    gsm_initialized = true;
    change_device_state(STATE_IDLE);
    display_status();
    return true;
}

void gsm_connect() {
    if (!gsm_config.enable_gsm) {
        return;
    }

    gsm_boot_modes_exhausted = false;

    if (!network_can_mutate()) {
        network_connect_pending = true;
        return;
    }

    if (!gsm_initialize()) {
        return;
    }

    if (gsm_connected || device_state == STATE_GSM_CONNECTING || device_state == STATE_GSM_REGISTERING) {
        return;
    }

    gsm_connect_start = millis();
    gsm_retry_count = 0;
    gsm_registration_complete = false;
    gsm_tcp_warmed_up = false;
    gsm_internet_verified = false;
    gsm_internet_test_attempt = 0;

    const bool multimode = gsm_module_supports_multimode();
    if (multimode) {
        int mode = gsm_current_network_mode_value();
        const char* mode_name = gsm_current_network_mode_name();
        logMessagef("GSM: Setting network mode to %s (AT+CNMP=%d)", mode_name, mode);
        gsm_modem_set_network_mode(mode);
        async_delay(2000);
        logMessagef("GSM: Starting connection sequence with %s", mode_name);
    } else {
        gsm_current_network_mode_index = 0;
        logMessagef("GSM: Starting connection sequence with %s", gsm_current_network_mode_name());
    }

    change_device_state(STATE_GSM_CONNECTING);
}

void gsm_disconnect() {
    if (!gsm_config.enable_gsm) {
        return;
    }

    logMessage("GSM: Disconnecting from network...");
    if (gsm_connected) {
        gsm_modem_gprs_disconnect();
    }

    gsm_connected = false;
    gsm_registration_complete = false;
    gsm_tcp_warmed_up = false;
    gsm_internet_verified = false;
    gsm_clear_network_info();

    if (device_state == STATE_GSM_CONNECTING || device_state == STATE_GSM_REGISTERING) {
        change_device_state(STATE_IDLE);
    }
}

static bool gsm_sync_modem_clock() {
    String modem_time = gsm_modem_get_datetime();
    if (modem_time.length() == 0) {
        last_modem_clock_result = MODEM_CLOCK_RESULT_INVALID;
        return false;
    }

    logMessage("GSM: Modem clock " + modem_time);

    if (system_time_initialized) {
        logMessage("GSM: System clock already initialized - skipping modem time");
        last_modem_clock_result = MODEM_CLOCK_RESULT_SKIPPED;
        return false;
    }

    time_t epoch = 0;
    if (!parse_modem_datetime(modem_time, epoch)) {
        last_modem_clock_result = MODEM_CLOCK_RESULT_INVALID;
        return false;
    }

    struct timeval tv;
    tv.tv_sec = epoch;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    system_time_initialized = true;
    last_modem_clock_result = MODEM_CLOCK_RESULT_SET;
    return true;
}

const char* sslclient_error_label(int error) {
    switch (error) {
        case SSLClient::SSL_OK: return "OK";
        case SSLClient::SSL_CLIENT_CONNECT_FAIL: return "CLIENT_CONNECT_FAIL";
        case SSLClient::SSL_BR_CONNECT_FAIL: return "BR_CONNECT_FAIL";
        case SSLClient::SSL_CLIENT_WRTIE_ERROR: return "CLIENT_WRITE_ERROR";
        case SSLClient::SSL_BR_WRITE_ERROR: return "BR_WRITE_ERROR";
        case SSLClient::SSL_INTERNAL_ERROR: return "INTERNAL_ERROR";
        case SSLClient::SSL_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        default: return "UNKNOWN";
    }
}

void gsm_reset_ssl_client() {
    gsm_ssl_clear_write_error();
    gsm_ssl_stop();
    gsm_client_stop();
    delay(50);
    while (gsm_client_available()) {
        gsm_client_read_byte();
    }

    gsm_modem_stream().flush();
    while (gsm_modem_stream().available()) {
        gsm_modem_stream().read();
    }
    delay(100);

    gsm_ssl_clear_write_error();
    gsm_client_clear_write_error();
}

void check_gsm_connection() {
    if (!gsm_config.enable_gsm || !gsm_initialized) {
        return;
    }

    bool connection_in_progress = (device_state == STATE_GSM_CONNECTING || device_state == STATE_GSM_REGISTERING);
    if (!connection_in_progress) {
        return;
    }

    if (!gsm_power_state) {
        logMessage("GSM: Power off detected during connection attempt");
        gsm_initialized = false;
        change_device_state(STATE_ERROR);
        return;
    }

    if ((unsigned long)(millis() - gsm_connect_start) > gsm_config.connection_timeout) {
        gsm_retry_count++;
        logMessagef("GSM: Connection timeout (%u)", gsm_retry_count);
        gsm_connect_start = millis();
        if (gsm_retry_count >= 3) {
            logMessage("GSM: Giving up after 3 attempts");
            change_device_state(STATE_IDLE);
            return;
        }
    }

    if (!gsm_registration_complete) {
        int status = gsm_modem_get_registration_status();

        if (status == 1 || status == 5) {
            gsm_registration_complete = true;
            gsm_operator_name = gsm_modem_get_operator();
            gsm_signal_quality = gsm_modem_get_signal_quality();

            String operator_readable = "";
            gsm_modem_sendAT("+COPS?");
            if (gsm_modem_waitResponse(1000, operator_readable) == 1) {
                String network_name = gsm_operator_name;
                if (operator_readable.indexOf(",\"") > 0) {
                    int start = operator_readable.indexOf(",\"") + 2;
                    int end = operator_readable.indexOf("\"", start);
                    if (end > start) {
                        network_name = operator_readable.substring(start, end) + " (" + gsm_operator_name + ")";
                    }
                }
                logMessage("GSM: Operator: " + network_name);
            } else {
                logMessage("GSM: Operator: " + gsm_operator_name);
            }
            logMessagef("GSM: Signal quality: %d/31", gsm_signal_quality);

            if (gsm_config.require_cell_signal &&
                gsm_signal_quality < gsm_config.min_signal_quality) {
                logMessage("GSM: Signal too weak, waiting...");
                delay(2000);
                return;
            }

            change_device_state(STATE_GSM_REGISTERING);

        } else if (status == 2) {
            static unsigned long last_log = 0;
            if (millis() - last_log > 5000) {
                logMessage("GSM: Searching for network...");
                last_log = millis();
            }
        } else {
            logMessagef("GSM: Registration failed, status=%d", status);
            gsm_retry_count++;
            gsm_connect_start = millis();
        }

        return;
    }

    if (strlen(gsm_config.apn_user) == 0 && strlen(gsm_config.apn_pass) == 0) {
        logMessagef("GSM: Connecting to APN '%s' (no credentials)...", gsm_config.apn);
    } else {
        logMessagef("GSM: Connecting to APN '%s' (with credentials)...", gsm_config.apn);
    }

    bool gprs_connected = gsm_modem_gprs_connect(
        gsm_config.apn,
        strlen(gsm_config.apn_user) > 0 ? gsm_config.apn_user : "",
        strlen(gsm_config.apn_pass) > 0 ? gsm_config.apn_pass : ""
    );

    if (gprs_connected) {
        gsm_connected = true;
        gsm_retry_count = 0;
        gsm_update_network_info();

        logMessage("GSM: Data connection established");
        logMessage("GSM: IP address: " + gsm_ip_address);
        logMessage("GSM: Waiting 3s for connection stabilization...");
        async_delay(3000);

        change_device_state(STATE_IDLE);
        network_update_active_state();
        display_status();

        logMessage("NTP: Checking modem clock (AT+CCLK?)");
        bool modem_time_valid = gsm_sync_modem_clock();

        if (modem_time_valid) {
            logMessage("NTP: System time set from modem clock");
        } else if (last_modem_clock_result == MODEM_CLOCK_RESULT_SKIPPED) {
            logMessage("NTP: Modem clock already applied previously - skipping");
        } else {
            logMessage("NTP: Modem clock unavailable - relying on NTP");
        }
        ntp_sync_start();

        if (!gsm_tcp_warmed_up) {
            for (int attempt = 1; attempt <= GSM_INTERNET_TEST_MAX_ATTEMPTS; attempt++) {
                gsm_internet_test_attempt = attempt;
                display_status();
                logMessagef("GSM: Internet test attempt %d/%d", attempt, GSM_INTERNET_TEST_MAX_ATTEMPTS);

                if (gsm_client_connect(GSM_INTERNET_TEST_HOST, GSM_INTERNET_TEST_PORT)) {
                    with_gsm_client([&](auto& client) {
                        client.print("GET ");
                        client.print(GSM_INTERNET_TEST_PATH);
                        client.println(" HTTP/1.0");
                        client.print("Host: ");
                        client.println(GSM_INTERNET_TEST_HOST);
                        client.println();
                        return 0;
                    });

                    bool http_status_ok = false;
                    bool in_body = false;
                    bool body_payload_seen = false;
                    bool ipv4_detected = false;
                    unsigned long timeout_start = millis();
                    while (gsm_client_connected() && millis() - timeout_start < 10000) {
                        if (gsm_client_available()) {
                            String line = gsm_client_read_string_until('\n');
                            line.trim();
                            if (!in_body) {
                                if (line.length() == 0) {
                                    in_body = true;
                                    continue;
                                }
                                if (line.startsWith("HTTP/1.") && line.indexOf("200") > 0) {
                                    http_status_ok = true;
                                }
                            } else if (line.length() > 0) {
                                body_payload_seen = true;
                                if (string_is_ipv4_address(line)) {
                                    logMessage("GSM: Probe response IP " + line);
                                    ipv4_detected = true;
                                    break;
                                }
                            }
                        }
                        yield();
                    }

                    gsm_client_stop();

                    if (ipv4_detected || (http_status_ok && body_payload_seen)) {
                        gsm_tcp_warmed_up = true;
                        gsm_internet_verified = true;
                        logMessage("GSM: Internet connectivity verified");
                        gsm_internet_test_attempt = 0;
                        display_status();
                        break;
                    } else if (!body_payload_seen) {
                        logMessage("GSM: Probe returned no data");
                    } else {
                        logMessage("GSM: Probe response did not contain IP");
                    }
                }

                logMessagef("GSM: Internet test attempt %d/%d failed", attempt, GSM_INTERNET_TEST_MAX_ATTEMPTS);
                unsigned long backoff_ms = (unsigned long)attempt * 1000UL;
                logMessagef("GSM: Waiting %lu ms before retry", backoff_ms);
                async_delay(backoff_ms);

                if (attempt < GSM_INTERNET_TEST_MAX_ATTEMPTS) {
                    logMessage("GSM: Re-initializing data session...");
                    gsm_modem_gprs_disconnect();
                    delay(2000);

                    bool gprs_restored = gsm_modem_gprs_connect(
                        gsm_config.apn,
                        gsm_config.apn_user,
                        gsm_config.apn_pass
                    );

                    if (gprs_restored) {
                        gsm_ip_address = gsm_get_ip_address();
                        logMessage("GSM: Data session re-established");
                        logMessage("GSM: IP address: " + gsm_ip_address);
                        unsigned long session_wait_ms = (unsigned long)attempt * 1000UL;
                        logMessagef("GSM: Waiting %lu ms for data session stabilization...", session_wait_ms);
                        async_delay(session_wait_ms);
                    } else {
                        logMessage("GSM: Failed to re-establish data session");
                        break;
                    }
                }
            }

            if (!gsm_internet_verified) {
                logMessagef("GSM: Internet test failed (%d attempts)", GSM_INTERNET_TEST_MAX_ATTEMPTS);
                gsm_internet_test_attempt = 0;
                display_status();

                if (gsm_module_supports_multimode()) {
                    logMessage("GSM: Internet verification failed - disconnecting and cycling network mode");
                    gsm_disconnect();
                    network_update_active_state();

                    if (gsm_advance_network_mode()) {
                        logMessagef("GSM: Retrying using %s mode", gsm_current_network_mode_name());
                        gsm_connect();
                        return;
                    } else {
                        if (!network_boot_complete) {
                            logMessage("GSM: All network modes exhausted during boot");
                            gsm_boot_modes_exhausted = true;
                            return;
                        }
                        gsm_reset_network_mode_cycle();
                        logMessagef("GSM: All modes failed, restarting with %s", gsm_current_network_mode_name());
                        gsm_connect();
                        return;
                    }
                } else {
                    logMessage("GSM: Internet verification failed on SIM800L - keeping data session active");
                }
            }

            gsm_internet_test_attempt = 0;
            gsm_tcp_warmed_up = true;
            display_status();
        }

    } else {
        gsm_retry_count++;
        logMessage("GSM: GPRS connection failed");

        if (gsm_module_supports_multimode()) {
            if (gsm_retry_count >= 3 && gsm_advance_network_mode()) {
                gsm_retry_count = 0;
                gsm_registration_complete = false;
                logMessage("GSM: Trying next network mode...");
            } else if (gsm_retry_count >= 3) {
                String mode_summary = gsm_network_mode_summary();
                logMessagef("GSM: All network modes exhausted (%s)",
                            mode_summary.c_str());
                gsm_reset_network_mode_cycle();
            }
        }

        gsm_connect_start = millis();
    }
}

#endif // ENABLE_GSM
