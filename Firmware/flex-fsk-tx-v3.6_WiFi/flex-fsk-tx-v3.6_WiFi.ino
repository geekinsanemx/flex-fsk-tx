
/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Enhanced FSK transmitter with WiFi, Web Interface and REST API
 *
 * v3.6.0  - CSS Consolidation: unified styling system with centralized CSS classes, eliminated 1000+ lines of redundant inline styles, improved theme consistency
 * v3.6.1  - ChatGPT toggle consistency fix: removed flex-row-wrap class to match other tabs' toggle styling
 * v3.6.2  - IMAP page memory fix: converted to chunked HTTP responses to prevent incomplete loading due to memory constraints
 * v3.6.3  - ChatGPT card layout fix: balanced three elements (toggles + API key button) in grid layout for consistent card height
 * v3.6.4  - SECURITY & STABILITY FIXES: timing attack protection, non-blocking NTP, input validation, certificate validation
 * v3.6.5  - CRITICAL FIXES: Buffer overflow protection, memory management, IMAP callback fix,
 *            removed blocking delays, state machine improvements, security hardening
 * v3.6.7  - COMPLETE CRITICAL FIXES: Watchdog timer, heap monitoring, HTML buffers,
 *            Unicode optimization, interrupt-safe queues, full security hardening
 * v3.6.8  - TRANSMISSION TIMING CRITICAL FIXES: WiFi isolation during TX, watchdog web feeds,
 *            eliminated all interrupts during transmission for perfect timing
 * v3.6.9  - BOOT LOOP FIX: Fixed watchdog double initialization causing startup hangs,
 *            proper task registration prevents 30s timeout during setup
 * v3.6.10 - WATCHDOG ARCHITECTURE FIX: removed race condition causing premature resets during NTP sync,
 *            enhanced watchdog feeding eliminates delete/add manipulation
 * v3.6.11 - Transmission watchdog protection: single feed per message before transmission, removed periodic feeding during TX
 * v3.6.12 - Discrete heap monitoring (warnings/critical only) and 10-second message send button debounce protection
 * v3.6.13 - Added orange warning style for debounce wait message popup
 * v3.6.14 - Fixed popup hide animation to completely move off-screen using calc(100% + 50px)
 * v3.6.15 - MAJOR LONG-TERM STABILITY FIXES: millis() rollover protection (49+ days), memory leak prevention, string safety, connection health validation
 * v3.6.16 - Serial message consistency validation and code comment cleanup (preserving HTML/JS comments)
 * v3.6.17 - Battery percentage display restored with || separator on OLED status line
 * v3.6.18 - UNIFIED BATTERY MONITORING: 60s polling, display updates when active, low battery alerts (10% threshold with hysteresis),
 *            removed global battery_percentage for thread safety, configurable alert toggle in System Alerts section
 * v3.6.19 - Power disconnect alert: state-based detection (charging→discharging) with 3-reading confirmation, hysteresis prevents false positives
 * v3.6.20 - ChatGPT retry logic fix: separated retry timing from schedule timing with independent retry delays,
 *            enhanced HTTP error diagnostics with connection state, WiFi status, and heap monitoring
 * v3.6.21 - TIMEZONE BUG FIX: Fixed "Next execution" display using UTC instead of local time,
 *            causing incorrect "Today" display when execution time already passed in local timezone
 * v3.6.22 - UI LAYOUT FIX: Modified ChatGPT prompt grid to 3-column layout with Schedule spanning 2 columns,
 *            preventing Schedule text wrapping on multiple lines
 * v3.6.23 - CHATGPT UI ENHANCEMENT: Replaced Edit button with toggle switch for enabling/disabling prompts;
 *            added Status display in prompt cards; toggle appears in edit form with "Enable #N" label
 * v3.6.24 - CHATGPT UI FIX: Corrected grid layout to backup's 3-column structure; moved toggle switch from
 *            edit button position to edit form title area; removed checkbox from form body
 * v3.6.25 - BACKUP FORMAT RESTRUCTURE: Updated JSON backup/restore format to match desired structure with device, wifi.enable,
 *            alerts (low_battery, power_disconnect), api.enable/http_port, and grafana.enable sections for improved organization
 * v3.6.26 - FACTORY RESET FIX: Added SPIFFS.format() to factory reset operations to completely clear IMAP settings, ChatGPT prompts,
 *            and MQTT certificates; backup restore operations properly recreate all SPIFFS files
 * v3.6.27 - EEPROM TO SPIFFS MIGRATION: Migrated non-critical settings from EEPROM to SPIFFS, reducing EEPROM usage by 92%
 *            (4096→325 bytes). Network essentials remain in EEPROM (CoreConfig), application settings moved to /settings.json.
 *            Transparent backup/restore maintains same user experience. Renamed /chatgpt_prompts.json to /chatgpt_settings.json.
 * v3.6.28 - UI THEME FIX: Fixed OpenAI API Key button visibility in clear theme by removing inline color styling and applying
 *            consistent 'button' class, matching Add New Prompt button styling for proper theme compatibility
 * v3.6.29 - IMAP OPTIMIZATION: Removed unnecessary global variable manipulation in IMAP processing by eliminating
 *            save/override/restore pattern for current_frequency, current_tx_power, and current_mail_drop variables
 * v3.6.30 - UI THEME REDUCTION: Reduced CSS themes from 10 to 2 (Minimal White + Carbon Black), eliminated 67 lines
 *            of redundant theme definitions, renumbered Carbon Black from theme 5 to theme 1 for cleaner indexing
 * v3.6.31 - DEAD CODE CLEANUP: Fixed undefined CSS variables (--theme-background, --theme-nav-hover), corrected
 *            status page theme switch for removed themes, removed /logs polling from global header (Status page only)
 * v3.6.32 - CONFIG PAGE HEAP FIX: Converted handle_configuration() from single String allocation to chunked HTTP responses,
 *            eliminates heap fragmentation issues caused by SSL client state after IMAP operations
 * v3.6.33 - IMAP SYSTEM REWRITE: Complete memory-efficient architecture with SSL cleanup, on-demand config loading,
 *            lightweight scheduler, eliminates global SSL state and memory overhead
 * v3.6.34 - IMAP DEAD CODE CLEANUP: Removed all dummy/legacy IMAP functions, implemented real message fetching with
 *            proper From/Subject/Body parsing, eliminates process_email_message and old scheduler functions
 * v3.6.35 - IMAP SEARCH FIX + DYNAMIC IP: Fixed IMAP search failure with alternative search methods and debugging,
 *            replaced [DEVICE-IP] placeholders with WiFi.localIP().toString() for dynamic API URLs
 * v3.6.36 - IMAP MESSAGE ORDER FIX: Process messages chronologically (oldest first) instead of newest first,
 *            prevents older messages from being starved by continuous new arrivals
 * v3.6.37 - IMAP BATCH SIZE INCREASE: Changed from 5 to 10 messages per check cycle for improved throughput
 * v3.6.38 - STATUS PAGE LAYOUT REDESIGN: Consolidated all status sections into single "System Status" card with reordered
 *            sections (Device→Battery→Network→Time→MQTT→IMAP), separate cards for Recent Serial Messages and Device Management
 * v3.6.39 - BATTERY PERCENTAGE & CHARGING DETECTION: Changed percentage mapping to 3.2V-4.15V range with ≥4.15V = 100%,
 *            added separate active charging detection (>4.20V), battery logs now show both Power (Connected/Battery) and
 *            Charging (Yes/No) states for better charge cycle visibility
 * v3.6.40 - BATTERY STATUS PAGE UPDATE: Updated status page Battery Status section to display separate "Power Status"
 *            (Connected/On Battery) and "Charging" (Yes/No) fields matching the enhanced battery logging format
 * v3.6.41 - TRANSMISSION TIMING ISOLATION: Moved STATE_TRANSMITTING protection before send_emr_if_needed() to prevent
 *            WiFi/MQTT/IMAP interference during EMR transmission (2.1s window), deferred display_status() I2C operations
 *            to after radio.startTransmit() completes, eliminates timing jitter during critical transmission phase
 * v3.6.42 - IMAP UID-BASED FIX: Changed to UID SEARCH/FETCH/STORE commands to use persistent message UIDs instead of
 *            sequence numbers, fixes marking wrong messages as read (sequence numbers are relative to search results),
 *            added mark-as-unread for messages that fail to enqueue (enables retry on next check)
 * v3.6.82 - WIFI NETWORK UI FIX: Removed STORED:/SCANNED: prefixes causing networks not to connect after save,
 *            added Add button with validation (SSID, password, static IP fields), simplified dropdown logic
 * v3.6.83 - WIFI NETWORK ADD ENDPOINT: New /api/wifi/add endpoint for adding networks separately from full config save,
 *            Add button now uses dedicated endpoint (no restart), Save Configuration triggers device restart
 * v3.6.43 - STATUS PAGE IMPROVEMENTS: Human-readable uptime format ("X days, Y hours, Z mins"), heap percentage display,
 *            reorganized layout: Device Info|FLEX Config, Network|Battery, MQTT|Time Sync, IMAP (full-width)
 * v3.6.44 - REMOTE SYSLOG LOGGING: RFC 3164 syslog support with UDP/TCP transport, auto-severity detection,
 *            configurable filtering (0-7), uses banner as hostname, facility hardcoded to local0 (16),
 *            config page: combined WiFi+IP into Network Settings, status page: Remote Logging (row 3), MQTT|IMAP (row 4)
 * v3.6.45 - BATTERY BOOT FIX: Fixed false power disconnect alerts on cold boot by initializing state from actual voltage
 *            on first check, lowered hysteresis thresholds (4.08V/4.12V) to handle USB float charge (4.15-4.18V),
 *            skips disconnect alert on first battery check to prevent boot false positives
 * v3.6.46 - STATUS PAGE MEMORY OPTIMIZATION: Converted handle_device_status() to chunked HTTP responses (87% peak memory reduction),
 *            fixed Live Logs feature to dynamically update serial message area without page reload (DOM insertion every 2s)
 * v3.6.47 - MQTT PAGE MEMORY OPTIMIZATION: Converted handle_mqtt() to chunked HTTP responses (87% peak memory reduction),
 *            certificate upload system verified functional (independent transport layer)
 * v3.6.48 - REMAINING PAGES MEMORY OPTIMIZATION: Converted handle_grafana() and handle_api_config() to chunked HTTP responses,
 *            completes web interface memory optimization (all major pages now use chunked transfer encoding)
 * v3.6.49 - FINAL PAGES MEMORY OPTIMIZATION: Converted handle_root() and handle_flex_config() to chunked HTTP responses,
 *            all web interface pages now use chunked transfer encoding (memory optimization complete)
 * v3.6.50 - UTILITY PAGES MEMORY OPTIMIZATION: Converted handle_web_factory_reset() and handle_restore_settings() to chunked responses,
 *            eliminates all remaining string concatenation in web interface (100% chunked HTTP)
 * v3.6.51 - SECURITY HARDENING: XSS protection (HTML escaping), MAC-based AP password generation, removed hardcoded defaults,
 *            default password warning banner on API config page, masked log output for sensitive data
 * v3.6.52 - ESP32 board mapping support: pin abstraction, display power control, SPI/I2C init, branding updates
 *            broker now queues messages while device offline, rsyslog MAC-based prefix, consolidated logging functions
 * v3.6.53 - CSS CENTRALIZATION: Removed ~100+ inline style attributes, added 28 CSS utility classes (.text-success/danger/warning/info,
 *            .toggle-switch.is-active/is-inactive, .button-compact/medium/large), updated JavaScript toggle functions to use classList,
 *            replaced hardcoded colors/sizes with reusable classes for maintainability and theme consistency
 * v3.6.54 - WATCHDOG IMPROVEMENTS: Simplified initialization (always deinit first), doubled timeouts (60s watchdog, 120s boot grace),
 *            added transmission_guard_active() for multi-state protection (TX/WAIT_DATA/WAIT_MSG), removed complex status polling from feed_watchdog()
 * v3.6.55 - LOGGING ACCURACY FIX: Corrected save operation messages (removed "EEPROM" references), save functions now handle their own logging,
 *            removed redundant caller logs, certificates saved to SPIFFS not EEPROM, Preferences (NVS) stores CoreConfig only
 * v3.6.56 - UNIFIED BOARD SUPPORT: Created boards.h with conditional compilation for TTGO_LORA32_V21 and HELTEC_WIFI_LORA32_V2,
 *            eliminates manual header switching, select board via single #define statement
 * v3.6.57 - AP MODE PERFORMANCE FIX: Increased loop delay from 1ms to 10ms for proper WiFi stack operation,
 *            eliminates laggy navigation and WiFi transmission issues in AP mode by reducing CPU saturation
 * v3.6.58 - WEB PERFORMANCE OPTIMIZATION: Increased loop delay to 20ms and throttled webServer.handleClient() to 20ms intervals,
 *            eliminates lag in heavy pages (ChatGPT/MQTT/IMAP) by reducing HTTP polling overhead and improving WiFi stack efficiency
 * v3.6.59 - TRANSMISSION TIMING FIX: Reverted loop delay from 20ms to 1ms to restore FLEX transmission timing,
 *            kept 20ms webServer.handleClient() throttling for web performance without affecting time-critical radio operations
 * v3.6.60 - RTC DS3231 INTEGRATION: Optional RTC support via RTC_ENABLED define, boot from RTC clock when enabled,
 *            NTP verification and sync updates RTC when WiFi available, fallback to NTP-at-boot when disabled
 * v3.6.61 - DOCUMENTATION: Added comprehensive compilation flags and external library requirements section,
 *            consolidated all build flags in one location with library URLs and descriptions
 * v3.6.62 - HEADER REORGANIZATION: Improved code organization with clean board selection, compilation flags,
 *            and grouped library includes with inline comments (external/built-in/project separation)
 * v3.6.63 - ENHANCED DOCUMENTATION: Added detailed explanations for board selection (pin mappings) and
 *            compilation flags (behavior, requirements, hardware details)
 * v3.6.64 - WIFI SETTINGS FIX: Corrected WiFi field references (wifi_ssid, wifi_password, use_dhcp, static_ip,
 *            netmask, gateway, dns) from core_config to settings struct after EEPROM→SPIFFS migration
 * v3.6.65 - FACTORY RESET UNIFICATION: Created perform_factory_reset() function to ensure SPIFFS.format() is called
 *            from all reset paths (physical button, web interface, AT+FACTORYRESET command)
 * v3.6.66 - FACTORY RESET OPTIMIZATION: Simplified to only format SPIFFS and reboot (boot recreates settings.json),
 *            preserves frequency_correction_ppm in NVS, added esp_task_wdt_delete() to eliminate watchdog errors
 * v3.6.67 - FACTORY RESET UI: Added centered "FACTORY RESET..." display message during factory reset operation
 * v3.6.68 - GLOBAL SAVE FIX: Removed redundant save_core_config() calls from all web handlers and AT commands
 *            (9 locations) since save_settings() already calls it internally (eliminates duplicate CONFIG saves)
 * v3.6.69 - FACTORY RESET NVS PRESERVATION: Fixed load_default_settings() to preserve non-zero frequency_correction_ppm
 *            from NVS instead of overwriting with 0.0 during factory reset (maintains calibrated PPM values)
 * v3.6.70 - IMAP BOOT DELAY: Added 60s boot delay before first IMAP check to prevent blocking during system startup,
 *            increased WiFiClientSecure timeout from 5s to 30s with explicit timeout detection and logging
 * v3.6.71 - BOOT SEQUENCE REDESIGN & TRANSMISSION CORE ISOLATION: Watchdog moved to post-NTP, staged service
 *            initialization with 60s delays (NTP→Watchdog→60s→MQTT→60s→IMAP/ChatGPT), Core 0 dedicated
 *            transmission task with portMUX thread-safety, boot failure auto-disable after 3 resets, display
 *            updates via flag to prevent I2C conflicts, NTP failure mode keeps webserver accessible
 * v3.6.72 - MQTT GUARD OPTIMIZATION: Removed redundant state flags, silent defer with logging only on actual
 *            publish, volatile device_state for multi-core safety, eliminated duplicate code in sendDeliveryAck,
 *            simplified mqtt_flush_deferred with informative logging extracted from JSON payloads
 * v3.6.73 - backported from v3.8.23 without GSM code
 * v3.6.74 - MULTIPLE WIFI NETWORK SUPPORT: Device now stores and scans for up to 10 WiFi networks,
 *            automatically connects to best available stored network, eliminates blind connection attempts,
 *            backup/restore format updated to support multiple networks, web UI manages first network
 * v3.6.75 - NETWORK SETTINGS UX IMPROVEMENT: Replaced input+datalist with traditional dropdown for SSID selection,
 *            integrated WiFi scan into dropdown ("Scan for networks..." option), added custom SSID input option,
 *            fixes issue where stored networks weren't visible when one was already selected
 * v3.6.76 - IP SETTINGS VISIBILITY OPTIMIZATION: Hide IP/Netmask/Gateway/DNS fields when DHCP is enabled and network
 *            is not currently connected, show current DHCP values when connected, show editable fields only for static IP,
 *            eliminates confusing default/fake values (0.0.0.0, 192.168.1.100), cleaner form with less visual noise
 * v3.6.77 - PASSWORD VISIBILITY TOGGLE: Added eye icon button to password field for toggling between hidden and
 *            plaintext display, improves UX when entering WiFi passwords
 * v3.6.78 - TIMEZONE DROPDOWN: Replaced numeric input with dropdown menu covering UTC-12:00 to UTC+14:00 in
 *            30-minute increments (41 options), supports half-hour timezones like India and Afghanistan
 * v3.6.79 - LIVE CLOCK DISPLAY: Added side-by-side clock cards showing Hardware Clock (UTC) and Local Time
 *            with live updates, client-side increment from device timestamp, updates on timezone change
 * v3.6.80 - RF CHIP SHUTDOWN FIX: Added radio.standby() after transmission completes in Core 0 task, fixes RF chip staying in TX mode and transmitting continuous noise after first message, regression from commit ceffdb4 when transmission logic moved to Core 0 without migrating hardware state management
 * v3.6.81 - EXTERNAL RF AMPLIFIER: Complete implementation of configurable external RF amplifier control - configurable GPIO pin (default: TTGO GPIO32, Heltec GPIO22), stabilization delay (20-5000ms, default 200ms), polarity selection toggle (Active-High for NPN driver+P-MOSFET like 2N2222+IRF4905, Active-Low for direct P-MOSFET), enable/disable toggle in FLEX settings page with visual feedback (fields disabled/grayed when off), reserved pin validation (prevents selection of GPIO 0, LoRa pins CS/IRQ/RST/GPIO/SCK/MOSI/MISO, OLED pins SDA/SCL/RST, Battery ADC, LED, VEXT) with real-time UI error display and backend validation, GPIO activated before transmission with configurable delay for bias stabilization, deactivated after transmission complete, fully integrated in settings persistence (save_settings/load_settings/config_to_json/json_to_config) and factory defaults, board-specific pin assignments adapt at compile-time
 * v3.6.84 - BOOT STATE MACHINE REFACTOR: Renamed BOOT_WIFI_* to BOOT_NETWORK_* for transport-agnostic naming,
 *           added BOOT_AP_COMPLETE state to prevent NTP sync attempts in AP-only mode (avoiding RTC corruption),
 *           restored WiFi health check with conditional network_reconnect() calls (fixes webserver slowness regression)
 * v3.6.85 - WIFI CONFIG UX IMPROVEMENT: Removed automatic page refresh after adding network, dropdown and storedNetworks
 *           array updated dynamically via DOM manipulation, newly added network auto-selected with onSSIDChange() callback
 * v3.6.86 - WIFI SCAN UX FIX: Added onSSIDChange() after scan completion/error to properly reset form fields
 * v3.6.87 - TIMEZONE DROPDOWN IMPROVEMENT: Replaced 41 generic options with 31 common timezones with descriptive
 *           names (cities/regions), removed rarely-used intermediate offsets, changed label to "Local Timezone"
 * v3.6.88 - BOOT INITIALIZATION FIX: Disabled default watchdog before SPIFFS format, early display initialization
 *           shows "Initializing Device..." message during first boot, eliminates 60s watchdog errors and black screen
 * v3.6.89 - FLEX CAPCODE VALIDATION: Implemented proper FLEX protocol capcode validation with gap detection,
 *           valid ranges: 1-1933312, 1998849-2031614, 2101249-4291000000, updated max from 4294967295 to 4291000000
*/

#define CURRENT_VERSION "v3.6.89"

/*
 * ============================================================================
 * BOARD SELECTION - Choose one:
 * ============================================================================
 *
 * HELTEC_WIFI_LORA32_V2:
 *   - LoRa: SX1276 on GPIO18(CS), GPIO26(IRQ), GPIO14(RST), GPIO35(DIO1)
 *   - OLED: SSD1306 I2C on GPIO4(SDA), GPIO15(SCL), GPIO16(RST)
 *   - Battery: ADC GPIO37
 *   - RTC Pins: Share I2C with OLED (GPIO4/GPIO15)
 *
 * TTGO_LORA32_V21:
 *   - LoRa: SX1276 on GPIO18(CS), GPIO26(IRQ), GPIO23(RST), GPIO33(DIO1)
 *   - OLED: SSD1306 I2C on GPIO21(SDA), GPIO22(SCL)
 *   - Battery: ADC GPIO35
 *   - RTC Pins: Share I2C with OLED (GPIO21/GPIO22)
 *
 */
#if !defined(TTGO_LORA32_V21) && !defined(HELTEC_WIFI_LORA32_V2)
 #define TTGO_LORA32_V21
#endif
/*
 * ============================================================================
 * COMPILATION FLAGS
 * ============================================================================
 *
 * RTC_ENABLED (true/false):
 *   - true:  Enable DS3231 RTC support, boot from RTC time, NTP sync updates RTC
 *   - false: Disable RTC code, boot with invalid time until NTP sync via WiFi
 *   - Requires: RTClib library (Adafruit)
 *   - Hardware: DS3231 connected to board's I2C pins (shared with OLED)
 *
 * ENABLE_IMAP:
 *   - Enable IMAP email monitoring and message-to-pager functionality
 *   - Fetches emails, parses commands, queues FLEX transmissions
 *   - Requires: ReadyMail library, WiFi connection, configured IMAP accounts
 *   - Comment out to disable IMAP features and reduce memory usage
 *
 * ENABLE_DEBUG:
 *   - Enable verbose debug output to Serial console
 *   - Shows IMAP operations, NTP sync, RTC status, memory usage
 *   - Comment out for production builds to reduce serial traffic
 *
 */
#define RTC_ENABLED true
#define ENABLE_IMAP
#define ENABLE_DEBUG

/*
 * ============================================================================
 * EXTERNAL LIBRARIES - Must be installed via Arduino Library Manager
 * ============================================================================
 *
 * RadioLib (by Jan Gromeš) - https://github.com/jgromes/RadioLib
 * U8g2 (by olikraus) - https://github.com/olikraus/u8g2
 * ArduinoJson (by Benoit Blanchon) - https://arduinojson.org
 * PubSubClient (by Nick O'Leary) - https://github.com/knolleary/pubsubclient
 * ReadyMail (by Suwatchai Kuanchai) - https://github.com/kasamdh/ReadyMail
 * RTClib (by Adafruit) - https://github.com/adafruit/RTClib
 *
 * ============================================================================
 */

#include <RadioLib.h>          // SX1276 LoRa radio FSK/FLEX transmission
#include <U8g2lib.h>            // OLED display (SSD1306) driver
#include <ArduinoJson.h>        // JSON parsing/serialization
#include <PubSubClient.h>       // MQTT client

#define READYMAIL_DEBUG_PORT Serial
#include <ReadyMail.h>          // IMAP email client

#if RTC_ENABLED
#include <RTClib.h>             // DS3231 RTC support
#endif

#include <Wire.h>               // I2C communication (built-in)
#include <SPI.h>                // SPI communication (built-in)
#include <WiFi.h>               // WiFi connectivity (built-in)
#include <WiFiUdp.h>            // UDP for NTP (built-in)
#include <WebServer.h>          // HTTP web server (built-in)
#include <Preferences.h>        // NVS preferences (built-in)
#include <WiFiClientSecure.h>  // TLS/SSL client (built-in)
#include <HTTPClient.h>         // HTTP client (built-in)
#include <SPIFFS.h>             // Flash filesystem (built-in)
#include <vector>               // STL vector (built-in)
#include <memory>               // STL smart pointers
#include <utility>              // STL forwarding helpers
#include <type_traits>          // Type trait utilities for template helpers
#include "esp_task_wdt.h"       // Watchdog timer (built-in)

#include "tinyflex/tinyflex.h"           // Project: FLEX protocol
#include "boards/boards.h"               // Project: Board pin definitions


#define MAX_CHATGPT_PROMPTS 10

#define SERIAL_BAUD 115200

#define TX_FREQ_DEFAULT 931.9375
#define TX_BITRATE 1.6
#define TX_DEVIATION 5
#define TX_POWER_DEFAULT 2
#define RX_BANDWIDTH 10.4
#define PREAMBLE_LENGTH 0

#define AT_BUFFER_SIZE 512
#define AT_CMD_TIMEOUT 5000
#define AT_MAX_RETRIES 3
#define AT_INTER_CMD_DELAY 100

#define OLED_TIMEOUT_MS (5 * 60 * 1000)
#define FONT_BANNER u8g2_font_10x20_tr
#define BANNER_HEIGHT 16
#define BANNER_MARGIN 2
#define FONT_DEFAULT u8g2_font_7x13_tr
#define FONT_BOLD u8g2_font_7x13B_tr
#define FONT_LINE_HEIGHT 14
#define FONT_TAB_START 42

#define FLEX_MSG_TIMEOUT 30000
#define MAX_FLEX_MESSAGE_LENGTH 248

#define IMAP_BATCH_SIZE 10
#define IMAP_CONTENT_LIMIT 248
#define IMAP_RECONNECT_INTERVAL 30000
#define IMAP_CHECK_INTERVAL 60000
#define IMAP_MAX_ACCOUNTS 5
#define IMAP_MIN_CHECK_INTERVAL 5
#define IMAP_JITTER_OFFSET 60000

#define WEB_SERVER_PORT 80
#define WIFI_CONNECT_TIMEOUT 30000
#define WIFI_AP_TIMEOUT 300000
#define WIFI_RETRY_ATTEMPTS 3

#define HEARTBEAT_INTERVAL 60000
#define HEARTBEAT_BLINK_DURATION 100

#define FACTORY_RESET_PIN 0
#define FACTORY_RESET_HOLD_TIME 30000

#define DEFAULT_BANNER "flex-fsk-tx"

#define FREQUENCY_CORRECTION_PPM 0.0

#define CONFIG_MAGIC 0xF1E7
#define CONFIG_VERSION 3

#define SERIAL_LOG_SIZE 20

#define CHECK_HEAP(size) (ESP.getFreeHeap() > (size + 8192))


struct ChatGPTPrompt {
    uint8_t id;
    char name[32];
    char prompt[251];
    bool days[7];
    uint8_t hour;
    uint8_t minute;
    uint32_t capcode;
    float frequency;
    bool mail_drop;
    bool enabled;
    uint8_t retry_count;
    unsigned long next_retry_time;
};

struct ChatGPTConfig {
    bool enabled;
    char api_key_b64[300];
    bool notify_on_failure;
    std::vector<ChatGPTPrompt> prompts;
    uint8_t prompt_count;
};


struct IMAPAccount {
    uint8_t id;
    char name[32];
    char server[64];
    uint16_t port;
    bool use_ssl;
    char username[64];
    char password[64];
    uint16_t check_interval_min;
    uint32_t capcode;
    float frequency;
    bool mail_drop;
    unsigned long last_check;
    uint8_t failed_check_cycles;
    bool suspended;
};

struct IMAPConfig {
    bool enabled;
    std::vector<IMAPAccount> accounts;
    uint8_t account_count;
};

struct CoreConfig {
    uint32_t magic;
    uint8_t version;
    float frequency_correction_ppm;
    uint8_t reserved[200];
};

struct DeviceSettings {
    uint8_t theme;
    char banner_message[17];
    float timezone_offset_hours;
    char ntp_server[64];
    bool enable_low_battery_alert;
    bool enable_power_disconnect_alert;
    bool enable_rf_amplifier;
    uint8_t rf_amplifier_power_pin;
    uint16_t rf_amplifier_delay_ms;
    bool rf_amplifier_active_high;
    float default_frequency;
    uint64_t default_capcode;
    float default_txpower;
    float frequency_correction_ppm;
    bool api_enabled;
    uint16_t http_port;
    char api_username[33];
    char api_password[65];
    bool mqtt_enabled;
    uint32_t mqtt_boot_delay_ms;
    bool imap_enabled;
    bool grafana_enabled;
    char mqtt_server[128];
    uint16_t mqtt_port;
    char mqtt_thing_name[32];
    char mqtt_subscribe_topic[64];
    char mqtt_publish_topic[64];
    bool rsyslog_enabled;
    char rsyslog_server[51];
    uint16_t rsyslog_port;
    bool rsyslog_use_tcp;
    uint8_t rsyslog_min_severity;
};

struct WiFiNetwork {
    char ssid[33];
    char password[65];
    bool use_dhcp;
    uint8_t static_ip[4];
    uint8_t netmask[4];
    uint8_t gateway[4];
    uint8_t dns[4];
};

#define MAX_WIFI_NETWORKS 10
WiFiNetwork stored_networks[MAX_WIFI_NETWORKS];
int stored_networks_count = 0;
String current_connected_ssid = "";

#define MQTT_CA_CERT_FILE "/mqtt_ca.pem"
#define MQTT_DEVICE_CERT_FILE "/mqtt_cert.pem"
#define MQTT_DEVICE_KEY_FILE "/mqtt_key.pem"

#define LED_OFF()  digitalWrite(LED_PIN, LOW)
#define LED_ON()   digitalWrite(LED_PIN, HIGH)

SX1276 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, LORA_RST_PIN, LORA_GPIO_PIN);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

#if RTC_ENABLED
RTC_DS3231 rtc;
bool rtc_available = false;
#endif

float tx_power = TX_POWER_DEFAULT;
uint64_t imap_last_uid = 0;

float current_frequency = TX_FREQ_DEFAULT;
int8_t current_tx_power = TX_POWER_DEFAULT;
bool current_mail_drop = false;

WebServer webServer(WEB_SERVER_PORT);

WiFiClientSecure wifiClientSecure;
PubSubClient mqttClient;
unsigned long mqttLastReconnectAttempt = 0;

struct IMAPScheduleEntry {
    uint8_t account_id;
    unsigned long next_check_time;
    uint8_t failed_attempts;
    bool suspended;
};

std::vector<IMAPScheduleEntry> imap_schedule;
bool imap_system_enabled = false;

unsigned long last_imap_check = 0;
int imap_failed_cycles = 0;
std::vector<uint32_t>* message_nums_ptr = nullptr;
String from_str, subject_str, body_str;


void imap_status_callback(const char* message) {
    (void)message;
}

int mqtt_failed_cycles = 0;
bool mqtt_suspended = false;
const int MAX_CONNECTION_FAILURES = 3;
const unsigned long IMAP_RECONNECT_INTERVAL_MS = 30000UL;

struct ChatGPTActivity {
    unsigned long timestamp;
    char prompt_name[51];
    char query[101];
    char response[101];
    bool query_success;
    bool transmission_success;
    uint32_t capcode;
    float frequency;
    int prompt_index;
    bool mail_drop;
};
ChatGPTActivity chatgpt_activity_log[10];
int chatgpt_activity_count = 0;
int chatgpt_last_http_code = 0;

unsigned long last_ntp_sync = 0;
bool ntp_synced = false;
bool mqtt_initialized = false;
unsigned long mqtt_initialized_time = 0;
const unsigned long NTP_SYNC_INTERVAL_MS = 3600000UL;
const unsigned long IMAP_DELAY_AFTER_MQTT_MS = 5000UL;

bool watchdog_task_registered = false;
bool watchdog_initialized = false;

bool ntp_sync_in_progress = false;
unsigned long ntp_sync_last_attempt = 0;
int ntp_sync_attempts = 0;
const int NTP_MAX_ATTEMPTS = 10;
const unsigned long NTP_SYNC_TIMEOUT_MS = 1000;

// Boot state machine
enum BootPhase {
    BOOT_INIT,              // setup() running
    BOOT_NETWORK_PENDING,   // Network connecting (WiFi or GSM)
    BOOT_NETWORK_READY,     // Network connected, webserver active
    BOOT_NTP_SYNCING,       // NTP attempts in progress
    BOOT_NTP_FAILED,        // NTP failed, retry mode (60s intervals)
    BOOT_WATCHDOG_ACTIVE,   // NTP synced, watchdog started
    BOOT_MQTT_PENDING,      // 60s delay before MQTT
    BOOT_MQTT_READY,        // MQTT initialized
    BOOT_SERVICES_PENDING,  // 60s delay before IMAP/ChatGPT
    BOOT_COMPLETE,          // All services running
    BOOT_AP_COMPLETE        // AP-only mode (no internet)
};

BootPhase boot_phase = BOOT_INIT;
unsigned long boot_phase_start = 0;

// Core 0 transmission task
TaskHandle_t tx_task_handle = NULL;
portMUX_TYPE queue_mux = portMUX_INITIALIZER_UNLOCKED;
volatile unsigned long core0_last_heartbeat = 0;
volatile bool display_update_requested = false;

// Boot failure tracking (NVS/Preferences)
struct BootFailureTracker {
    uint8_t consecutive_resets;
    uint32_t last_reset_phase;
};

BootFailureTracker boot_tracker = {0, 0};

struct SerialLogEntry {
    unsigned long timestamp;
    char message[100];
};
SerialLogEntry serial_log[SERIAL_LOG_SIZE];
int serial_log_index = 0;
int serial_log_count = 0;

CoreConfig core_config;
DeviceSettings settings;

ChatGPTConfig chatgpt_config;
unsigned long last_chatgpt_check = 0;
const unsigned long CHATGPT_CHECK_INTERVAL = 60000UL;

IMAPConfig imap_config;
Preferences preferences;


typedef enum {
    STATE_IDLE,
    STATE_WAITING_FOR_DATA,
    STATE_WAITING_FOR_MSG,
    STATE_TRANSMITTING,
    STATE_ERROR,
    STATE_WIFI_CONNECTING,
    STATE_WIFI_AP_MODE,
    STATE_IMAP_PROCESSING,
    STATE_NTP_SYNC,
    STATE_MQTT_CONNECTING
} device_state_t;

volatile device_state_t device_state = STATE_IDLE;
device_state_t previous_state = STATE_IDLE;
unsigned long state_timeout = 0;

#define MAX_QUEUE_SIZE 25

struct QueuedMessage {
    uint32_t capcode;
    float frequency;
    int power;
    bool mail_drop;
    char message[MAX_FLEX_MESSAGE_LENGTH + 1];
};

QueuedMessage message_queue[MAX_QUEUE_SIZE];
volatile int queue_head = 0;
volatile int queue_tail = 0;
volatile int queue_count = 0;

char at_buffer[AT_BUFFER_SIZE];
int at_buffer_pos = 0;
bool at_command_ready = false;

unsigned long last_activity_time = 0;
bool oled_active = true;

volatile bool console_loop_enable = true;
volatile bool fifo_empty = false;
volatile bool transmission_processing_complete = false;
volatile bool transmission_in_progress = false;

String current_message_id = "";

uint8_t tx_data_buffer[2048] = {0};
int current_tx_total_length = 0;
int current_tx_remaining_length = 0;
int16_t radio_start_transmit_status = RADIOLIB_ERR_NONE;
int expected_data_length = 0;
unsigned long data_receive_timeout = 0;

uint64_t flex_capcode = 0;
uint64_t current_tx_capcode = 0;
char flex_message_buffer[MAX_FLEX_MESSAGE_LENGTH + 1] = {0};

int flex_message_pos = 0;
unsigned long flex_message_timeout = 0;
bool flex_mail_drop = false;

float current_tx_frequency = TX_FREQ_DEFAULT;

bool wifi_connected = false;
bool ap_mode_active = false;
unsigned long wifi_connect_start = 0;
int wifi_retry_count = 0;
static bool wifi_retry_silent = false;
bool wifi_scan_available = false;
bool wifi_auth_failed = false;
bool network_boot_complete = false;
unsigned long last_wifi_scan_ms = 0;
IPAddress device_ip;
String ap_ssid = "";
String ap_password = "";
String mac_suffix = "";

unsigned long last_heartbeat = 0;
bool heartbeat_state = false;
int heartbeat_blink_count = 0;

unsigned long factory_reset_start = 0;
bool factory_reset_pressed = false;

bool battery_present = false;
static bool power_disconnect_alert_sent = false;

static bool last_power_connected = false;
static bool last_charging_active = false;
static int last_percent_bracket = -1;
static bool battery_first_check = true;

unsigned long last_emr_transmission = 0;
bool first_message_sent = false;
const unsigned long EMR_TIMEOUT_MS = 600000UL;

unsigned long mqttReconnectBackoff = 10000;

const uint32_t WATCHDOG_TIMEOUT_MS = 120000UL;
const uint32_t IMAP_CONNECTION_TIMEOUT_MS = 30000UL;
const uint32_t IMAP_BOOT_DELAY_MS = 60000UL;

void setup_watchdog();
void feed_watchdog();
void check_heap_health();

#define TRANSMISSION_GUARD_ACTIVE() (device_state == STATE_TRANSMITTING || device_state == STATE_WAITING_FOR_DATA || device_state == STATE_WAITING_FOR_MSG)
inline bool transmission_guard_active() {
    return TRANSMISSION_GUARD_ACTIVE();
}

static String mqtt_deferred_status_payload = "";
static String mqtt_deferred_ack_payload = "";

static device_state_t ntp_previous_state = STATE_IDLE;
static bool ntp_state_active = false;
static device_state_t mqtt_previous_state = STATE_IDLE;
static bool mqtt_state_active = false;

enum ActiveNetwork {
    NETWORK_NONE = 0,
    NETWORK_WIFI_ACTIVE = 1
};

ActiveNetwork active_network = NETWORK_NONE;

bool network_connect_pending = false;
static bool network_available_cached = false;
static bool system_time_initialized = false;
static bool system_time_from_rtc = false;
static bool system_time_from_ntp = false;

uint8_t mqtt_connection_attempt = 0;

static const char* active_network_label(ActiveNetwork network);
static bool network_is_connected();
static void on_network_changed(ActiveNetwork previous, ActiveNetwork current);
static void network_update_active_state();
static bool network_can_mutate();

const char* state_to_string(device_state_t state);
void change_device_state(device_state_t new_state);
bool is_valid_state_transition(device_state_t from, device_state_t to);
void imap_scheduler_loop();
void async_delay(unsigned long ms);
static bool wifi_ssid_scan();
static void network_boot();
static void network_reconnect();

static inline void restore_ntp_state() {
    if (ntp_state_active) {
        change_device_state(ntp_previous_state);
        ntp_state_active = false;
    }
}

static inline void restore_mqtt_state() {
    if (mqtt_state_active) {
        change_device_state(mqtt_previous_state);
        mqtt_state_active = false;
    }
}

String loadCertificateFromSPIFFS(const char* filename) {
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        return "";
    }

    String content = file.readString();
    file.close();
    return content;
}

bool saveCertificateToSPIFFS(const char* filename, const String& cert) {
    File file = SPIFFS.open(filename, "w");
    if (!file) {
        return false;
    }

    size_t bytesWritten = file.print(cert);
    file.close();
    return (bytesWritten > 0);
}

bool certificateExistsInSPIFFS(const char* filename) {
    return SPIFFS.exists(filename);
}

String getCertificateStatusFromSPIFFS(const char* filename) {
    if (certificateExistsInSPIFFS(filename)) {
        File file = SPIFFS.open(filename, "r");
        if (file) {
            size_t size = file.size();
            file.close();
            return "<span class='text-success'>✅ Uploaded (" + String(size) + " bytes)</span>";
        }
    }
    return "<span class='text-danger'>❌ Not uploaded</span>";
}

void deleteAllCertificatesFromSPIFFS() {
    SPIFFS.remove(MQTT_CA_CERT_FILE);
    SPIFFS.remove(MQTT_DEVICE_CERT_FILE);
    SPIFFS.remove(MQTT_DEVICE_KEY_FILE);
}

String getCertificateFilename(const String& certType) {
    if (certType == "mqtt_ca") return MQTT_CA_CERT_FILE;
    if (certType == "mqtt_cert") return MQTT_DEVICE_CERT_FILE;
    if (certType == "mqtt_key") return MQTT_DEVICE_KEY_FILE;
    return "";
}

void load_default_core_config() {
    core_config.magic = CONFIG_MAGIC;
    core_config.version = CONFIG_VERSION;
    core_config.frequency_correction_ppm = 0.0;
    memset(core_config.reserved, 0, sizeof(core_config.reserved));

    deleteAllCertificatesFromSPIFFS();
}

const char* state_to_string(device_state_t state) {
    switch(state) {
        case STATE_IDLE: return "IDLE";
        case STATE_WAITING_FOR_DATA: return "WAITING_FOR_DATA";
        case STATE_WAITING_FOR_MSG: return "WAITING_FOR_MSG";
        case STATE_TRANSMITTING: return "TRANSMITTING";
        case STATE_ERROR: return "ERROR";
        case STATE_WIFI_CONNECTING: return "WIFI_CONNECTING";
        case STATE_WIFI_AP_MODE: return "WIFI_AP_MODE";
        case STATE_IMAP_PROCESSING: return "IMAP_PROCESSING";
        case STATE_NTP_SYNC: return "NTP_SYNC";
        case STATE_MQTT_CONNECTING: return "MQTT_CONNECTING";
        default: return "UNKNOWN";
    }
}

void change_device_state(device_state_t new_state) {
    if (!is_valid_state_transition(device_state, new_state)) {
        logMessagef("STATE: Invalid transition %s -> %s", state_to_string(device_state), state_to_string(new_state));
        return;
    }

    previous_state = device_state;
    device_state = new_state;
    logMessagef("STATE: %s -> %s", state_to_string(previous_state), state_to_string(new_state));

    if (new_state != STATE_IDLE &&
        new_state != STATE_NTP_SYNC &&
        new_state != STATE_IMAP_PROCESSING) {
        reset_oled_timeout();
    }

    display_status();
}

bool is_valid_state_transition(device_state_t from, device_state_t to) {
    switch(from) {
        case STATE_IDLE:
            return true;
        case STATE_TRANSMITTING:
            return (to == STATE_IDLE || to == STATE_ERROR);
        case STATE_WAITING_FOR_DATA:
        case STATE_WAITING_FOR_MSG:
            return (to == STATE_TRANSMITTING || to == STATE_IDLE || to == STATE_ERROR);
        case STATE_ERROR:
            return (to == STATE_IDLE);
        default:
            return true;
    }
}

void async_delay(unsigned long ms) {
    unsigned long start = millis();
    while ((unsigned long)(millis() - start) < ms) {
        feed_watchdog();
        yield();
        if (wifi_connected || ap_mode_active) {
            webServer.handleClient();
        }
    }
}

static const char* active_network_label(ActiveNetwork network) {
    switch (network) {
        case NETWORK_WIFI_ACTIVE: return "WiFi";
        default: return "None";
    }
}

static bool network_can_mutate() {
    return !transmission_guard_active();
}

static void on_network_changed(ActiveNetwork previous, ActiveNetwork current) {
    if (previous == current) {
        return;
    }

    logMessagef("NETWORK: Active transport changed %s -> %s",
                active_network_label(previous), active_network_label(current));

    if (mqtt_initialized) {
        mqtt_initialized = false;
        mqtt_suspended = false;
        mqtt_failed_cycles = 0;
    }

    if (previous == NETWORK_WIFI_ACTIVE) {
        wifiClientSecure.stop();
        logMessage("NETWORK: WiFi SSL client cleaned up");
    }

    ntp_sync_in_progress = false;
}

static void network_update_active_state() {
    ActiveNetwork new_network = wifi_connected ? NETWORK_WIFI_ACTIVE : NETWORK_NONE;

    ActiveNetwork previous = active_network;
    active_network = new_network;
    on_network_changed(previous, active_network);
}

static bool network_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

static void network_boot() {
    logMessage("NETWORK: Boot sequence starting");

    if (stored_networks_count == 0) {
        logMessage("NETWORK: No WiFi networks configured - entering AP mode");
        start_ap_mode();
        network_boot_complete = true;
        return;
    }

    wifi_ssid_scan();

    if (!wifi_scan_available) {
        logMessage("NETWORK: WiFi not available - entering AP mode");
        start_ap_mode();
        network_boot_complete = true;
        return;
    }

    logMessage("NETWORK: WiFi available, attempting connection");
    wifi_connect();

    unsigned long wifi_boot_start = millis();
    const unsigned long wifi_boot_timeout = 30000;

    while (!wifi_connected && (millis() - wifi_boot_start < wifi_boot_timeout)) {
        delay(500);
        check_wifi_connection();

        if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
            wifi_auth_failed = true;
            logMessage("NETWORK: WiFi authentication failed after 3 attempts - entering AP mode");
            start_ap_mode();
            network_boot_complete = true;
            return;
        }
    }

    if (wifi_connected) {
        logMessage("NETWORK: Boot completed with WiFi");
    } else {
        logMessage("NETWORK: WiFi boot timeout - entering AP mode");
        start_ap_mode();
    }

    boot_phase = BOOT_NETWORK_READY;
    network_boot_complete = true;
    network_update_active_state();
}

static void network_reconnect() {
    if (!network_boot_complete || ap_mode_active) {
        return;
    }

    if (!network_can_mutate()) {
        network_connect_pending = true;
        return;
    }

    if (stored_networks_count == 0 || wifi_connected) {
        return;
    }

    const unsigned long scan_interval_ms = 60000UL;
    if ((millis() - last_wifi_scan_ms) < scan_interval_ms) {
        return;
    }

    wifi_ssid_scan();

    if (wifi_scan_available) {
        logMessage("NETWORK: WiFi available, attempting connection");
        wifi_retry_count = 0;
        wifi_connect();
        wifi_retry_silent = true;

        unsigned long reconnect_start = millis();
        const unsigned long reconnect_timeout = 15000;

        while (!wifi_connected && (millis() - reconnect_start < reconnect_timeout)) {
            delay(500);
            check_wifi_connection();

            if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
                logMessage("NETWORK: WiFi reconnection failed after 3 attempts");
                break;
            }
        }

        wifi_retry_silent = false;
    } else {
        logMessage("NETWORK: WiFi networks unavailable, retrying soon");
    }

    network_update_active_state();
}

void setup_watchdog() {
    esp_task_wdt_config_t config = {
        .timeout_ms = WATCHDOG_TIMEOUT_MS,
        .idle_core_mask = 0,
        .trigger_panic = true
    };

    esp_err_t init_result = esp_task_wdt_init(&config);
    if (init_result == ESP_OK) {
        watchdog_initialized = true;
    } else {
        watchdog_initialized = false;
        logMessagef("WATCHDOG: Init failed (err=%d), using default configuration", init_result);
    }

    esp_err_t add_result = esp_task_wdt_add(NULL);
    if (add_result == ESP_OK) {
        logMessagef("WATCHDOG: Loop task registered with %lus timeout",
                    (unsigned long)(WATCHDOG_TIMEOUT_MS / 1000UL));
        watchdog_task_registered = true;
    } else {
        logMessagef("WATCHDOG: Failed to register loop task (err=%d)", add_result);
        watchdog_task_registered = false;
    }
}

void feed_watchdog() {
    if (watchdog_task_registered) {
        esp_task_wdt_reset();
    }
}


void check_heap_health() {
    static unsigned long last_warning = 0;
    static uint32_t last_min_heap = 0;
    static bool first_run = true;

    uint32_t free_heap = ESP.getFreeHeap();
    uint32_t min_heap = ESP.getMinFreeHeap();

    if (first_run) {
        last_min_heap = min_heap;
        first_run = false;
        return;
    }

    bool show_warning = false;
    char warning_msg[128];


    if (free_heap < 30000) {
        snprintf(warning_msg, sizeof(warning_msg), "HEAP CRITICAL: Free: %lu bytes", free_heap);
        show_warning = true;
    }

    else if (free_heap < 50000) {
        snprintf(warning_msg, sizeof(warning_msg), "HEAP WARNING: Free: %lu bytes", free_heap);
        show_warning = true;
    }

    else if (last_min_heap > min_heap && (last_min_heap - min_heap) > 10000) {
        snprintf(warning_msg, sizeof(warning_msg), "HEAP: Min dropped by %lu bytes (now: %lu)", last_min_heap - min_heap, min_heap);
        show_warning = true;
        last_min_heap = min_heap;
    }


    if (show_warning && ((unsigned long)(millis() - last_warning) > 30000)) {
        logMessage(warning_msg);
        last_warning = millis();
    }
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


void imap_scheduler_loop() {
    if (!imap_system_enabled || imap_schedule.empty()) {
        return;
    }

    unsigned long current_time = millis();

    for (auto& entry : imap_schedule) {
        if (entry.suspended || current_time < entry.next_check_time) {
            continue;
        }

        logMessagef("IMAP: Processing account %d", entry.account_id);

        if (imap_check_account_clean(entry.account_id)) {
            entry.failed_attempts = 0;
            entry.next_check_time = current_time + (10 * 60000UL);
        } else {
            entry.failed_attempts++;
            unsigned long backoff = min(entry.failed_attempts * 30000UL, 300000UL);
            entry.next_check_time = current_time + backoff;

            if (entry.failed_attempts >= 5) {
                entry.suspended = true;
                logMessagef("IMAP: Account %d suspended after %d failures", entry.account_id, entry.failed_attempts);
            }
        }

        return;
    }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    logMessagef("MQTT: Message received on topic '%s' (%d bytes)", topic, length);

    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        logMessagef("MQTT: JSON parse error: %s", error.c_str());
        logMessagef("MQTT: Problematic JSON: %s", message);

        String raw_msg = String(message);
        int id_start = raw_msg.indexOf("\"id\":\"");
        if (id_start != -1) {
            id_start += 6;
            int id_end = raw_msg.indexOf("\"", id_start);
            if (id_end != -1) {
                String msg_id = raw_msg.substring(id_start, id_end);
                if (msg_id.length() > 0) {
                    sendDeliveryAck(msg_id, "failed");
                }
            }
        }
        return;
    }

    String type = doc["type"] | "";
    String from = doc["from"] | "";
    String msg = doc["message"] | "";
    String subject = doc["subject"] | "";
    String id = doc["id"] | "";
    uint64_t timestamp = doc["ts"] | 0;

    bool capcode_from_msg = doc.containsKey("capcode");
    bool frequency_from_msg = doc.containsKey("frequency");
    bool power_from_msg = doc.containsKey("power");

    uint64_t capcode = capcode_from_msg ? doc["capcode"] : settings.default_capcode;
    float frequency = frequency_from_msg ? doc["frequency"] : settings.default_frequency;
    float power = power_from_msg ? doc["power"] : settings.default_txpower;
    bool mail_drop_from_msg = doc.containsKey("mail_drop");
    bool mail_drop = mail_drop_from_msg ? doc["mail_drop"] : false;

    if (msg.length() == 0) {
        logMessage("MQTT: Message rejected - missing mandatory 'message' field");
        if (id.length() > 0) {
            sendDeliveryAck(id, "failed");
        }
        return;
    }

    if (type.length() == 0 || (type != "paging" && type != "webhook" && type != "email")) {
        if (type.length() > 0) {
            logMessage("MQTT: Unknown type '" + type + "' - defaulting to 'paging'");
        }
        type = "paging";
    }

    float original_frequency = frequency;
    if (frequency >= 1000.0) {
        frequency = frequency / 1000000.0;
        logMessagef("MQTT: Converted frequency from %.0f Hz to %.6f MHz", original_frequency, frequency);
    }

    String paging_message;
    if (type == "email") {
        paging_message = from + "\n" + subject + "\n" + msg;
    } else {
        paging_message = msg;
    }

    paging_message = truncate_message_with_ellipsis(paging_message);

    current_message_id = id;

    DynamicJsonDocument debugDoc(256);
    debugDoc["type"] = type;
    debugDoc["from"] = from;
    debugDoc["subject"] = subject;
    debugDoc["message"] = msg;
    String debugJson;
    serializeJson(debugDoc, debugJson);

    String param_sources = "";
    if (!capcode_from_msg) param_sources += "capcode=default,";
    if (!frequency_from_msg) param_sources += "freq=default,";
    if (!power_from_msg) param_sources += "power=default,";
    String param_summary = "default";
    if (param_sources.length() > 0) {
        param_sources = param_sources.substring(0, param_sources.length() - 1);
        param_summary = param_sources;
    }

    logMessagef("MQTT: Processing %s message id=%s from=%s (params:%s)",
                  type.c_str(), id.length() ? id.c_str() : "none",
                  from.length() ? from.c_str() : "unknown",
                  param_summary.c_str());

    if (queue_add_message(capcode, frequency, power, mail_drop, paging_message.c_str())) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "MQTT: Message queued (id=%s, from=%s, capcode=%llu)",
                 id.length() ? id.c_str() : "none", from.c_str(), capcode);
        logMessage(log_msg);
        char status_msg[128];
        snprintf(status_msg, sizeof(status_msg), "Message queued from %s", from.c_str());
        mqtt_publish_status(status_msg);
    } else {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "MQTT: Queue full, message id=%s from %s rejected",
                 id.length() ? id.c_str() : "none", from.c_str());
        logMessage(log_msg);
        mqtt_publish_status("Queue full - message rejected");
        if (id.length() > 0) {
            sendDeliveryAck(id, "failed");
        }
        current_message_id = "";
    }
}

void ntp_sync_start() {
    if (ntp_sync_in_progress) {
        return;
    }

    logMessage("NTP: Starting non-blocking time synchronization...");

    String ntp_server1 = (strlen(settings.ntp_server) > 0) ? String(settings.ntp_server) : "pool.ntp.org";

    if (!ntp_state_active) {
        ntp_previous_state = device_state;
    }
    change_device_state(STATE_NTP_SYNC);
    ntp_state_active = true;

    delay(100);
    configTime(0, 0, ntp_server1.c_str(), "time.nist.gov", "216.239.35.4");
    delay(500);

    feed_watchdog();

    ntp_sync_in_progress = true;
    ntp_sync_last_attempt = 0;
    ntp_sync_attempts = 0;

    logMessagef("NTP: Using server: %s", ntp_server1.c_str());
}

void ntp_sync_process() {
    if (!ntp_sync_in_progress) {
        return;
    }

    if (millis() - ntp_sync_last_attempt < NTP_SYNC_TIMEOUT_MS) {
        return;
    }


    feed_watchdog();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        time_t now;
        time(&now);
        ntp_synced = true;
        last_ntp_sync = millis();
        ntp_sync_in_progress = false;
        system_time_initialized = true;
        system_time_from_ntp = true;

        restore_ntp_state();

        logMessagef("NTP: Time synchronized! Attempts: %d, Timestamp: %ld", ntp_sync_attempts + 1, (long)now);
        logMessagef("NTP: Current time: %s", asctime(&timeinfo));

#if RTC_ENABLED
        rtc_sync_from_ntp();
#endif

    } else {
        ntp_sync_attempts++;
        ntp_sync_last_attempt = millis();
        logMessagef("NTP: Sync attempt %d/%d...", ntp_sync_attempts, NTP_MAX_ATTEMPTS);

        if (ntp_sync_attempts >= NTP_MAX_ATTEMPTS) {
            logMessage("NTP: Time sync failed after maximum attempts");
            ntp_synced = false;
            ntp_sync_in_progress = false;

            restore_ntp_state();
        }
    }
}

bool ntp_sync_time() {
    ntp_sync_start();

    for (int i = 0; i < 3 && ntp_sync_in_progress; i++) {
        ntp_sync_process();
        yield();
        delay(10);
    }

    return ntp_synced;
}

#if RTC_ENABLED
void rtc_sync_from_ntp() {
    if (!rtc_available) {
        return;
    }

    time_t now;
    time(&now);
    rtc.adjust(DateTime(now));
    logMessage("RTC: Updated from NTP sync");
}
#endif

unsigned long getUnixTimestamp() {
    time_t now;
    time(&now);
    return (unsigned long)now;
}

time_t getLocalTimestamp() {
    time_t now;
    time(&now);
    return now + (long)(settings.timezone_offset_hours * 3600);
}

bool mqtt_connect() {
    logMessage("MQTT: connect start");

    if (!settings.mqtt_enabled || strlen(settings.mqtt_server) == 0) {
        logMessage("MQTT: Disabled or no server configured");
        return false;
    }

    time_t now;
    time(&now);
    logMessagef("MQTT: Current timestamp: %ld", (long)now);

    if (!ntp_synced || now < 1600000000) {
        logMessage("MQTT: System time not synchronized! SSL connection will likely fail.");
        logMessage("MQTT: NTP should be synced in main loop before MQTT initialization");
        return false;
    } else {
        logMessagef("MQTT: System time appears synchronized: %ld (delta %ld s)",
                    (long)now, (long)(now - (time_t)(last_ntp_sync / 1000UL)));
    }

    if (!network_is_connected()) {
        logMessage("MQTT: No network connection available");
        return false;
    }

    logMessage("=== MQTT Connection Debug ===");
    logMessagef("MQTT Server: '%s'", settings.mqtt_server);
    logMessagef("MQTT Port: %d", settings.mqtt_port);
    logMessagef("MQTT Thing Name: '%s'", settings.mqtt_thing_name);
    logMessagef("MQTT Subscribe Topic: '%s'", settings.mqtt_subscribe_topic);
    logMessagef("MQTT Publish Topic: '%s'", settings.mqtt_publish_topic);
    logMessagef("Network Type: %s", active_network_label(active_network));

    String mqtt_ca_cert = loadCertificateFromSPIFFS(MQTT_CA_CERT_FILE);
    String mqtt_device_cert = loadCertificateFromSPIFFS(MQTT_DEVICE_CERT_FILE);
    String mqtt_device_key = loadCertificateFromSPIFFS(MQTT_DEVICE_KEY_FILE);

    if (mqtt_ca_cert.length() == 0 || mqtt_device_cert.length() == 0 || mqtt_device_key.length() == 0) {
        logMessage("MQTT: Missing certificates in SPIFFS - cannot connect to AWS IoT without proper authentication");
        logMessage("   Please upload Root CA, Device Certificate, and Private Key files in the MQTT configuration page");
        return false;
    }

    logMessage("MQTT: Configuring certificate authentication...");

    if (mqtt_ca_cert.length() < 100 || mqtt_ca_cert.indexOf("-----BEGIN CERTIFICATE-----") == -1) {
        logMessage("MQTT: Invalid CA certificate format");
        return false;
    }
    if (mqtt_device_cert.length() < 100 || mqtt_device_cert.indexOf("-----BEGIN CERTIFICATE-----") == -1) {
        logMessage("MQTT: Invalid device certificate format");
        return false;
    }
    if (mqtt_device_key.length() < 100 || mqtt_device_key.indexOf("-----BEGIN") == -1) {
        logMessage("MQTT: Invalid private key format");
        return false;
    }

    if (!mqtt_state_active) {
        mqtt_previous_state = device_state;
    }
    change_device_state(STATE_MQTT_CONNECTING);
    mqtt_state_active = true;

    bool connection_result = false;
    uint16_t socket_timeout = 5;

    wifiClientSecure.setCACert(mqtt_ca_cert.c_str());
    wifiClientSecure.setCertificate(mqtt_device_cert.c_str());
    wifiClientSecure.setPrivateKey(mqtt_device_key.c_str());
    wifiClientSecure.setTimeout(socket_timeout);
    mqttClient.setClient(wifiClientSecure);
    logMessage("MQTT: Using WiFi TLS client");

    logMessage("MQTT: SSL Configuration:");
    logMessagef("  - CA cert loaded from SPIFFS (%d chars)", mqtt_ca_cert.length());
    logMessagef("  - Client cert loaded from SPIFFS (%d chars)", mqtt_device_cert.length());
    logMessagef("  - Private key loaded from SPIFFS (%d chars)", mqtt_device_key.length());

    logMessage("MQTT: Certificates configured successfully");

    logMessage("MQTT: Setting server and callback...");
    mqttClient.setServer(settings.mqtt_server, settings.mqtt_port);
    mqttClient.setBufferSize(2048);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(socket_timeout);
    logMessagef("MQTT: socket timeout set to %u s", socket_timeout);
    logMessage("MQTT: Buffer size set to 2048 bytes");
    mqttClient.setCallback(mqtt_callback);

    logMessage("MQTT: Testing SSL connection...");
    logMessagef("MQTT: Attempting connection to %s:%d with client ID %s over %s...",
                settings.mqtt_server, settings.mqtt_port, settings.mqtt_thing_name,
                active_network_label(active_network));

    feed_watchdog();

    unsigned long mqtt_connect_start = millis();
    unsigned long connect_timeout_ms = 10000UL;

    bool connected = mqttClient.connect(settings.mqtt_thing_name, NULL, NULL, NULL, 0, 0, NULL, false);
    unsigned long mqtt_connect_time = millis() - mqtt_connect_start;

    yield();

    logMessagef("MQTT: connect returned %s in %lu ms", connected ? "true" : "false", mqtt_connect_time);
    logMessagef("MQTT: MQTT state code %d", mqttClient.state());

    if (connected) {
        logMessage("MQTT: Connection successful!");

        if (strlen(settings.mqtt_subscribe_topic) > 0) {
            logMessagef("MQTT: Subscribing to topic: '%s' with QoS 1", settings.mqtt_subscribe_topic);
            bool subscribed = mqttClient.subscribe(settings.mqtt_subscribe_topic, 1);
            logMessagef("MQTT: Subscription %s", subscribed ? "successful" : "failed");
        } else {
            logMessage("MQTT: No subscribe topic configured");
        }

        connection_result = true;
    } else {
        int error_code = mqttClient.state();
        logMessagef("MQTT: Connection failed with error code: %d", error_code);

        switch (error_code) {
            case -4: logMessage("MQTT Error: Connection timeout"); break;
            case -3: logMessage("MQTT Error: Connection lost"); break;
            case -2: logMessage("MQTT Error: Connect failed"); break;
            case -1: logMessage("MQTT Error: Disconnected"); break;
            case 1: logMessage("MQTT Error: Bad protocol version"); break;
            case 2: logMessage("MQTT Error: Bad client ID"); break;
            case 3: logMessage("MQTT Error: Server unavailable"); break;
            case 4: logMessage("MQTT Error: Bad username/password"); break;
            case 5: logMessage("MQTT Error: Not authorized"); break;
            default: logMessagef("MQTT Error: Unknown error code %d", error_code); break;
        }

        logMessagef("MQTT: Active transport: %s", active_network_label(active_network));
        logMessagef("MQTT: Free heap: %d bytes", ESP.getFreeHeap());

        logMessagef("MQTT: WiFi Status: %d (should be WL_CONNECTED)", WiFi.status());
    }

    if (!connection_result) {
        mqtt_connection_attempt = 0;
    }

    restore_mqtt_state();
    return connection_result;
}

void mqtt_initialize() {
    if (!settings.mqtt_enabled || strlen(settings.mqtt_server) == 0) {
        logMessage("MQTT: Initialization skipped - disabled or not configured");
        return;
    }

    if (mqtt_initialized) {
        logMessage("MQTT: Already initialized");
        return;
    }

    if (!certificateExistsInSPIFFS(MQTT_CA_CERT_FILE) || !certificateExistsInSPIFFS(MQTT_DEVICE_CERT_FILE) || !certificateExistsInSPIFFS(MQTT_DEVICE_KEY_FILE)) {
        logMessage("MQTT: Initialization failed - missing certificates in SPIFFS");
        return;
    }

    logMessage("MQTT: Starting initialization...");

    if (!ntp_synced) {
        logMessage("MQTT: NTP sync required before MQTT initialization");
        logMessage("MQTT: Initialization deferred - waiting for NTP sync");
        return;
    }

    if (mqtt_connect()) {
        mqtt_initialized = true;
        mqtt_initialized_time = millis();
        logMessage("MQTT: Initialization successful - connected");
    } else {
        mqtt_failed_cycles++;
        logMessagef("MQTT: Initial connection failed, cycle failure (%d/%d failures)", mqtt_failed_cycles, MAX_CONNECTION_FAILURES);

        if (mqtt_failed_cycles >= MAX_CONNECTION_FAILURES) {
            mqtt_suspended = true;
            logMessagef("MQTT: %d failures, suspending MQTT until reboot", MAX_CONNECTION_FAILURES);
        }

        mqtt_initialized = true;
        mqtt_initialized_time = millis();
    }
}

void mqtt_loop() {
    static unsigned long lastMqttCheck = 0;
    static unsigned long lastReconnectAttempt = 0;
    static unsigned long connectionEstablishedTime = 0;
    static unsigned long reconnectInterval = 10000;
    static bool wasConnected = false;

    if (!mqtt_initialized) {
        return;
    }

    if (mqtt_suspended) {
        return;
    }

    unsigned long now = millis();

    if ((unsigned long)(now - lastMqttCheck) < 100) return;
    lastMqttCheck = now;

    bool isConnected = mqttClient.connected();

    if (isConnected && !wasConnected) {
        connectionEstablishedTime = now;
        wasConnected = true;
        logMessagef("MQTT: Connection re-established at %lu ms", now);
    }
    else if (!isConnected && wasConnected) {
        unsigned long duration = now - connectionEstablishedTime;
        wasConnected = false;
        logMessagef("MQTT: Connection lost after %lu ms", duration);
        lastReconnectAttempt = now;
    }

    if (!isConnected) {
        if ((unsigned long)(now - lastReconnectAttempt) >= reconnectInterval) {
            logMessagef("MQTT: Attempting reconnection... (last attempt %lu ms ago)",
                          now - lastReconnectAttempt);
            lastReconnectAttempt = now;
            if (mqtt_connect()) {
                connectionEstablishedTime = now;
                wasConnected = true;
                mqtt_failed_cycles = 0;
                mqtt_suspended = false;
                reconnectInterval = 10000;
                logMessage("MQTT: Connected successfully");
            } else {
                reconnectInterval = random(1000, 30001);

                mqtt_failed_cycles++;
                logMessagef("MQTT: Connection failed, cycle failure (%d/%d failures), next retry in %lu ms",
                           mqtt_failed_cycles, MAX_CONNECTION_FAILURES, reconnectInterval);

                if (mqtt_failed_cycles >= MAX_CONNECTION_FAILURES) {
                    mqtt_suspended = true;
                    logMessagef("MQTT: %d failures, suspending MQTT until reboot", MAX_CONNECTION_FAILURES);
                }
            }
        }
    }
    else {
        if (!mqttClient.loop()) {
            logMessagef("MQTT: WARNING - loop() returned false at %lu ms", now);
        }
    }
}

void mqtt_publish_status(const String& status) {
    if (!settings.mqtt_enabled || strlen(settings.mqtt_publish_topic) == 0) {
        return;
    }

    DynamicJsonDocument doc(256);
    doc["device"] = settings.mqtt_thing_name;
    doc["status"] = status;
    doc["timestamp"] = millis();

    String output;
    serializeJson(doc, output);

    if (transmission_guard_active()) {
        mqtt_deferred_status_payload = output;
        return;
    }

    if (mqttClient.connected()) {
        if (mqttClient.publish(settings.mqtt_publish_topic, output.c_str())) {
            logMessagef("MQTT: Status published: %s", status.c_str());
        } else {
            mqtt_deferred_status_payload = output;
        }
    } else {
        mqtt_deferred_status_payload = output;
    }
}

void sendDeliveryAck(String messageId, String status) {
    if (!settings.mqtt_enabled || strlen(settings.mqtt_publish_topic) == 0 || messageId.length() == 0) {
        return;
    }

    DynamicJsonDocument ackDoc(256);
    ackDoc["type"] = "delivery_ack";
    ackDoc["message_id"] = messageId;
    ackDoc["status"] = status;
    ackDoc["timestamp"] = getUnixTimestamp();
    ackDoc["device"] = String(settings.mqtt_thing_name);

    String ackPayload;
    serializeJson(ackDoc, ackPayload);

    if (transmission_guard_active() || !mqttClient.connected()) {
        mqtt_deferred_ack_payload = ackPayload;
        return;
    }

    if (mqttClient.publish(settings.mqtt_publish_topic, ackPayload.c_str())) {
        logMessagef("MQTT: Delivery ACK sent for %s (%s)", messageId.c_str(), status.c_str());
    } else {
        mqtt_deferred_ack_payload = ackPayload;
    }
}

void mqtt_flush_deferred() {
    if (!settings.mqtt_enabled || !mqttClient.connected()) {
        return;
    }

    if (mqtt_deferred_ack_payload.length() > 0) {
        if (mqttClient.publish(settings.mqtt_publish_topic, mqtt_deferred_ack_payload.c_str())) {
            DynamicJsonDocument doc(256);
            deserializeJson(doc, mqtt_deferred_ack_payload);
            const char* msgId = doc["message_id"];
            const char* msgStatus = doc["status"];
            logMessagef("MQTT: Delivery ACK sent for %s (%s)", msgId, msgStatus);
            mqtt_deferred_ack_payload = "";
        } else {
            return;
        }
    }

    if (mqtt_deferred_status_payload.length() > 0) {
        if (mqttClient.publish(settings.mqtt_publish_topic, mqtt_deferred_status_payload.c_str())) {
            DynamicJsonDocument doc(256);
            deserializeJson(doc, mqtt_deferred_status_payload);
            const char* statusMsg = doc["status"];
            logMessagef("MQTT: Status published: %s", statusMsg);
            mqtt_deferred_status_payload = "";
        }
    }
}


IMAPAccount load_account_from_config(uint8_t account_id) {
    IMAPAccount account = {};

    if (!SPIFFS.exists("/imap_settings.json")) {
        logMessage("IMAP: Config file not found");
        return account;
    }

    File file = SPIFFS.open("/imap_settings.json", "r");
    if (!file) {
        logMessage("IMAP: Failed to open config file");
        return account;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        logMessage("IMAP: Failed to parse config JSON");
        return account;
    }

    JsonArray accounts = doc["accounts"];
    for (JsonVariant account_data : accounts) {
        if (account_data["id"] == account_id) {
            account.id = account_data["id"];
            strlcpy(account.name, account_data["name"] | "", sizeof(account.name));
            strlcpy(account.server, account_data["server"] | "", sizeof(account.server));
            account.port = account_data["port"] | 993;
            account.use_ssl = account_data["use_ssl"] | true;
            strlcpy(account.username, account_data["username"] | "", sizeof(account.username));

            String encoded_password = account_data["password"] | "";
            String decoded_password = base64_decode_string(encoded_password);
            strlcpy(account.password, decoded_password.c_str(), sizeof(account.password));

            account.check_interval_min = account_data["check_interval_min"] | IMAP_MIN_CHECK_INTERVAL;
            account.capcode = account_data["capcode"] | settings.default_capcode;
            account.frequency = account_data["frequency"] | settings.default_frequency;
            account.mail_drop = account_data["mail_drop"] | false;
            break;
        }
    }

    return account;
}




bool imap_connect() {
    logMessage("IMAP: Old system disabled - use IMAP configuration");
    return false;
}




bool imap_check_account_clean(uint8_t account_id) {
    IMAPAccount account = load_account_from_config(account_id);

    if (strlen(account.server) == 0 || strlen(account.username) == 0 || strlen(account.password) == 0) {
        logMessagef("IMAP: Account %d - missing server or credentials", account_id);
        return false;
    }

    uint32_t heap_before = ESP.getFreeHeap();
    logMessagef("HEAP: Before IMAP check - Free: %u bytes", heap_before);

    WiFiClientSecure ssl_client;
    ssl_client.setInsecure();
    ssl_client.setTimeout(30000);

    ReadyMailIMAP::IMAPClient imap_client(ssl_client);

    logMessagef("IMAP: Account %d ('%s') - Connecting to %s:%d", account_id, account.name, account.server, account.port);

    unsigned long connect_start = millis();
    bool connect_result = imap_client.connect(account.server, account.port);
    unsigned long connect_duration = millis() - connect_start;

    if (!connect_result) {
        if (connect_duration >= IMAP_CONNECTION_TIMEOUT_MS) {
            logMessagef("IMAP: Account %d - Connection timeout after %lu ms", account_id, connect_duration);
        } else {
            logMessagef("IMAP: Account %d - Connection failed after %lu ms", account_id, connect_duration);
        }
        ssl_client.stop();
        return false;
    }

    if (!imap_client.authenticate(account.username, account.password, readymail_auth_password)) {
        logMessagef("IMAP: Account %d - Authentication failed", account_id);
        imap_client.close();
        ssl_client.stop();
        return false;
    }

    if (!imap_client.select("INBOX", false)) {
        logMessagef("IMAP: Account %d - Failed to select INBOX", account_id);
        imap_client.close();
        ssl_client.stop();
        return false;
    }

    std::vector<uint32_t> message_nums;
    message_nums_ptr = &message_nums;

    auto collection_callback = [](ReadyMailIMAP::IMAPCallbackData &data) -> void {
        if (data.event() == imap_data_event_search && message_nums_ptr) {
            if (message_nums_ptr->size() < 10) {
                message_nums_ptr->push_back(data.messageNum());
            }
        }
    };

    if (!imap_client.search("UID SEARCH UNSEEN", 10, true, collection_callback, true)) {
        logMessagef("IMAP: Account %d - Search failed", account_id);
        imap_client.close();
        ssl_client.stop();
        return false;
    }

    logMessagef("IMAP: Account %d - Found %d unread messages", account_id, message_nums.size());

    std::sort(message_nums.begin(), message_nums.end());

    for (uint32_t msg_num : message_nums) {
        if (queue_is_full()) {
            logMessage("IMAP: Queue full, stopping message processing");
            break;
        }

        if (process_message_clean(imap_client, msg_num, account)) {
            logMessagef("IMAP: Message %d queued successfully", msg_num);
        }

        yield();
        feed_watchdog();
    }

    imap_client.close();
    ssl_client.stop();

    uint32_t heap_after = ESP.getFreeHeap();
    logMessagef("HEAP: After IMAP cleanup - Free: %u bytes (diff: %d)", heap_after, (int32_t)heap_after - (int32_t)heap_before);

    return true;
}



bool process_message_clean(ReadyMailIMAP::IMAPClient& imap_client, uint32_t msg_num, const IMAPAccount& account) {
    from_str = "";
    subject_str = "";
    body_str = "";

    auto fetch_callback = [](ReadyMailIMAP::IMAPCallbackData &data) {
        extern String from_str, subject_str, body_str;

        if (data.event() == imap_data_event_fetch_envelope) {
            for (size_t i = 0; i < data.headerCount(); i++) {
                String header_name = data.getHeader(i).first;
                String header_value = data.getHeader(i).second;

                if (header_name.equalsIgnoreCase("From")) {
                    from_str = header_value;
                    int bracket_pos = from_str.indexOf('<');
                    if (bracket_pos > 0) {
                        from_str = from_str.substring(0, bracket_pos);
                        from_str.trim();
                    }
                } else if (header_name.equalsIgnoreCase("Subject")) {
                    subject_str = header_value;
                }
            }

            for (size_t i = 0; i < data.fileCount(); i++) {
                if (data.fileInfo(i).mime == "text/plain") {
                    data.fetchOption(i) = true;
                }
            }
        } else if (data.event() == imap_data_event_fetch_body) {
            if (data.fileInfo().mime == "text/plain" && data.fileChunk().size > 0) {
                char temp_body[513];
                size_t copy_size = min((size_t)512, (size_t)data.fileChunk().size);
                strncpy(temp_body, (char*)data.fileChunk().data, copy_size);
                temp_body[copy_size] = '\0';
                body_str = String(temp_body);
            }
        }
    };

    if (!imap_client.fetchUID(msg_num, fetch_callback, nullptr, true, 8192)) {
        logMessagef("IMAP: Failed to fetch message %d", msg_num);
        return false;
    }

    String email_message = from_str + ": " + subject_str + "\n" + body_str;
    String truncated_message = truncate_message_with_ellipsis(email_message);

    uint32_t capcode = settings.default_capcode;
    float frequency = account.frequency > 0 ? account.frequency : settings.default_frequency;
    int power = settings.default_txpower;

    if (queue_add_message(capcode, frequency, power, account.mail_drop, truncated_message.c_str())) {
        logMessagef("IMAP: Message %d from '%s' subject '%s' queued", msg_num, from_str.c_str(), subject_str.c_str());
        imap_client.sendCommand("UID STORE " + String(msg_num) + " +FLAGS (\\Seen)", nullptr, true);
        return true;
    }

    logMessagef("IMAP: Failed to queue message %d - queue may be full", msg_num);
    imap_client.sendCommand("UID STORE " + String(msg_num) + " -FLAGS (\\Seen)", nullptr, true);
    return false;
}

void init_imap_scheduler() {
    imap_schedule.clear();
    imap_system_enabled = imap_config.enabled;

    if (!imap_system_enabled) {
        return;
    }

    unsigned long current_time = millis();

    for (uint8_t i = 0; i < imap_config.account_count; i++) {
        IMAPScheduleEntry entry;
        entry.account_id = i + 1;
        entry.next_check_time = current_time + IMAP_BOOT_DELAY_MS + (i * 10000UL);
        entry.failed_attempts = 0;
        entry.suspended = false;
        imap_schedule.push_back(entry);
    }

    logMessagef("IMAP: Scheduler initialized with %d accounts", imap_schedule.size());
}


uint8_t detectSeverity(const String& message) {
    if (message.startsWith("ERROR:") || message.startsWith("FATAL:")) return 3;
    if (message.startsWith("WARN:") || message.startsWith("WARNING:")) return 4;
    if (message.startsWith("SYSTEM:") || message.startsWith("CRITICAL:")) return 2;
    if (message.startsWith("IMAP:") || message.startsWith("MQTT:")) return 5;
    if (message.startsWith("DEBUG:")) return 7;
    return 6;
}

String formatSyslogMessage(const String& message, uint8_t severity) {
    uint8_t priority = (16 * 8) + severity;

    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timestamp[16];
    strftime(timestamp, sizeof(timestamp), "%b %d %H:%M:%S", &timeinfo);

    String hostname = String(settings.banner_message);
    if (hostname.length() == 0) hostname = "FLEX";

    return "<" + String(priority) + ">" + String(timestamp) + " " + hostname + " " + mac_suffix + ": " + message;
}

void sendSyslog(const String& message, uint8_t severity) {
    if (!settings.rsyslog_enabled) return;
    if (severity > settings.rsyslog_min_severity) return;
    if (strlen(settings.rsyslog_server) == 0) return;
    if (!wifi_connected) return;

    String syslogMsg = formatSyslogMessage(message, severity);

    if (settings.rsyslog_use_tcp) {
        WiFiClient client;
        if (client.connect(settings.rsyslog_server, settings.rsyslog_port)) {
            client.print(syslogMsg);
            client.stop();
        }
    } else {
        WiFiUDP udp;
        udp.beginPacket(settings.rsyslog_server, settings.rsyslog_port);
        udp.print(syslogMsg);
        udp.endPacket();
    }
}


float apply_frequency_correction(float base_freq) {
    return base_freq * (1.0 + settings.frequency_correction_ppm / 1000000.0);
}


void logMessage(const char* message) {
    log_serial_message(message);
    uint8_t severity = detectSeverity(String(message));
    sendSyslog(String(message), severity);
}

void logMessage(const String& message) {
    logMessage(message.c_str());
}

void logMessagef(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[200];
    vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[sizeof(buffer) - 1] = '\0';

    va_end(args);

    logMessage(buffer);
}

bool save_core_config() {
    logMessagef("CONFIG: Saving core config - magic=0x%X, version=%d",
                  core_config.magic, core_config.version);
    logMessagef("CONFIG: CoreConfig struct size: %d bytes", sizeof(CoreConfig));

    if (!preferences.begin("flex-fsk", false)) {
        logMessage("CONFIG: Failed to open preferences for writing");
        return false;
    }

    size_t bytes_written = preferences.putBytes("config", &core_config, sizeof(CoreConfig));
    bool result = (bytes_written == sizeof(CoreConfig));

    preferences.end();

    logMessagef("CONFIG: Preferences write: %s (%d bytes)", result ? "SUCCESS" : "FAILED", bytes_written);

    return result;
}

bool load_core_config() {
    if (!preferences.begin("flex-fsk", true)) {
        logMessage("CONFIG: Failed to open preferences for reading, using defaults");
        load_default_core_config();
        save_core_config();
        return false;
    }

    CoreConfig temp_core_config;
    size_t bytes_read = preferences.getBytes("config", &temp_core_config, sizeof(CoreConfig));
    preferences.end();

    logMessagef("CONFIG: Loading core config - bytes_read=%d, expected=%d", bytes_read, sizeof(CoreConfig));

    if (bytes_read != sizeof(CoreConfig)) {
        logMessage("CONFIG: Core config size mismatch, using defaults");
        load_default_core_config();
        save_core_config();
        return false;
    }

    logMessagef("CONFIG: Loaded core config - magic=0x%X (expected 0x%X), version=%d, freq_correction=%.2f ppm",
                  temp_core_config.magic, CONFIG_MAGIC, temp_core_config.version, temp_core_config.frequency_correction_ppm);

    if (temp_core_config.magic != CONFIG_MAGIC) {
        logMessage("CONFIG: Invalid magic number, using defaults");
        load_default_core_config();
        save_core_config();
        return false;
    }

    if (temp_core_config.version != CONFIG_VERSION) {
        logMessagef("CONFIG: Core config version mismatch (found %d, expected %d), using factory defaults", temp_core_config.version, CONFIG_VERSION);
        load_default_core_config();
        save_core_config();
        return false;
    }

    core_config = temp_core_config;

    if (core_config.frequency_correction_ppm != 0.0) {
        settings.frequency_correction_ppm = core_config.frequency_correction_ppm;
        logMessagef("CONFIG: Using NVS frequency_correction_ppm: %.2f", core_config.frequency_correction_ppm);
    }

    logMessage("CONFIG: Core config loaded successfully");
    return true;
}


bool save_imap_config() {
    logMessage("IMAP: Saving configuration");

    File file = SPIFFS.open("/imap_settings.json", "w");
    if (!file) {
        logMessage("IMAP: Failed to create IMAP config file");
        return false;
    }

    DynamicJsonDocument doc(4096);
    doc["enabled"] = imap_config.enabled;
    doc["account_count"] = imap_config.account_count;

    JsonArray accounts = doc.createNestedArray("accounts");
    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        JsonObject account = accounts.createNestedObject();
        account["id"] = imap_config.accounts[i].id;
        account["name"] = imap_config.accounts[i].name;
        account["server"] = imap_config.accounts[i].server;
        account["port"] = imap_config.accounts[i].port;
        account["use_ssl"] = imap_config.accounts[i].use_ssl;
        account["username"] = imap_config.accounts[i].username;
        account["password"] = base64_encode_string(String(imap_config.accounts[i].password));
        account["check_interval_min"] = imap_config.accounts[i].check_interval_min;
        account["capcode"] = imap_config.accounts[i].capcode;
        account["frequency"] = imap_config.accounts[i].frequency;
        account["mail_drop"] = imap_config.accounts[i].mail_drop;
    }

    if (serializeJson(doc, file) == 0) {
        logMessage("IMAP: Failed to write IMAP config JSON");
        file.close();
        return false;
    }

    file.close();
    logMessage("IMAP: Config saved successfully");
    return true;
}

bool load_imap_config() {
    logMessage("IMAP: Loading configuration");

    if (!SPIFFS.exists("/imap_settings.json")) {
        logMessage("IMAP: Config file not found, using defaults");
        load_default_imap_config();
        save_imap_config();
        return false;
    }

    File file = SPIFFS.open("/imap_settings.json", "r");
    if (!file) {
        logMessage("IMAP: Failed to open config file");
        load_default_imap_config();
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        logMessage("IMAP: Failed to parse config JSON, using defaults");
        load_default_imap_config();
        save_imap_config();
        return false;
    }

    imap_config.accounts.clear();

    imap_config.enabled = doc["enabled"] | false;
    imap_config.account_count = doc["account_count"] | 0;

    JsonArray accounts = doc["accounts"];
    for (JsonObject account : accounts) {
        if (imap_config.accounts.size() >= IMAP_MAX_ACCOUNTS) {
            logMessage("IMAP: Maximum accounts reached, skipping additional accounts");
            break;
        }

        IMAPAccount temp_account;
        temp_account.id = account["id"] | (imap_config.accounts.size() + 1);
        strlcpy(temp_account.name, account["name"] | "", sizeof(temp_account.name));
        strlcpy(temp_account.server, account["server"] | "", sizeof(temp_account.server));
        temp_account.port = account["port"] | 993;
        temp_account.use_ssl = account["use_ssl"] | true;
        strlcpy(temp_account.username, account["username"] | "", sizeof(temp_account.username));
        String encoded_password = account["password"] | "";
        String decoded_password = base64_decode_string(encoded_password);
        strlcpy(temp_account.password, decoded_password.c_str(), sizeof(temp_account.password));
        temp_account.check_interval_min = max((uint16_t)(account["check_interval_min"] | IMAP_MIN_CHECK_INTERVAL), (uint16_t)IMAP_MIN_CHECK_INTERVAL);
        temp_account.capcode = account["capcode"] | settings.default_capcode;
        temp_account.frequency = account["frequency"] | settings.default_frequency;
        temp_account.mail_drop = account["mail_drop"] | false;

        temp_account.last_check = 0;
        temp_account.failed_check_cycles = 0;
        temp_account.suspended = false;

        imap_config.accounts.push_back(temp_account);
    }

    imap_config.account_count = imap_config.accounts.size();
    logMessagef("IMAP: Config loaded successfully, %d accounts configured", imap_config.account_count);
    return true;
}

void load_default_imap_config() {
    logMessage("IMAP: Loading default IMAP configuration");
    imap_config.enabled = false;
    imap_config.accounts.clear();
    imap_config.account_count = 0;

    DynamicJsonDocument doc(4096);
    doc["enabled"] = false;
    doc["account_count"] = 0;

    JsonArray accounts = doc.createNestedArray("accounts");


    save_imap_config();
}

bool save_settings() {
    logMessage("SETTINGS: Saving configuration to /settings.json");

    File file = SPIFFS.open("/settings.json", "w");
    if (!file) {
        logMessage("SETTINGS: Failed to create settings file");
        return false;
    }

    DynamicJsonDocument doc(4096);

    JsonObject device = doc.createNestedObject("device");
    device["theme"] = settings.theme;
    device["banner_message"] = settings.banner_message;
    device["timezone_offset_hours"] = settings.timezone_offset_hours;
    device["ntp_server"] = settings.ntp_server;

    JsonObject alerts = doc.createNestedObject("alerts");
    alerts["low_battery"] = settings.enable_low_battery_alert;
    alerts["power_disconnect"] = settings.enable_power_disconnect_alert;

    JsonObject rf_amp = doc.createNestedObject("rf_amplifier");
    rf_amp["enabled"] = settings.enable_rf_amplifier;
    rf_amp["power_pin"] = settings.rf_amplifier_power_pin;
    rf_amp["delay_ms"] = settings.rf_amplifier_delay_ms;
    rf_amp["active_high"] = settings.rf_amplifier_active_high;

    JsonObject flex = doc.createNestedObject("flex");
    flex["default_frequency"] = settings.default_frequency;
    flex["default_capcode"] = String(settings.default_capcode);
    flex["default_txpower"] = settings.default_txpower;
    flex["frequency_correction_ppm"] = settings.frequency_correction_ppm;

    JsonObject api = doc.createNestedObject("api");
    api["enabled"] = settings.api_enabled;
    api["http_port"] = settings.http_port;
    api["username"] = settings.api_username;
    api["password"] = base64_encode_string(String(settings.api_password));

    JsonObject services = doc.createNestedObject("services");
    services["mqtt_enabled"] = settings.mqtt_enabled;
    services["mqtt_boot_delay_ms"] = settings.mqtt_boot_delay_ms;
    services["imap_enabled"] = settings.imap_enabled;
    services["grafana_enabled"] = settings.grafana_enabled;

    JsonObject mqtt = doc.createNestedObject("mqtt");
    mqtt["server"] = settings.mqtt_server;
    mqtt["port"] = settings.mqtt_port;
    mqtt["thing_name"] = settings.mqtt_thing_name;
    mqtt["subscribe_topic"] = settings.mqtt_subscribe_topic;
    mqtt["publish_topic"] = settings.mqtt_publish_topic;

    JsonObject rsyslog = doc.createNestedObject("rsyslog");
    rsyslog["enabled"] = settings.rsyslog_enabled;
    rsyslog["server"] = settings.rsyslog_server;
    rsyslog["port"] = settings.rsyslog_port;
    rsyslog["use_tcp"] = settings.rsyslog_use_tcp;
    rsyslog["min_severity"] = settings.rsyslog_min_severity;

    JsonObject wifi = doc.createNestedObject("wifi");
    for (int i = 0; i < stored_networks_count; i++) {
        JsonObject net = wifi.createNestedObject(stored_networks[i].ssid);
        net["password"] = base64_encode_string(String(stored_networks[i].password));
        net["use_dhcp"] = stored_networks[i].use_dhcp;
        net["static_ip"] = String(stored_networks[i].static_ip[0]) + "." + String(stored_networks[i].static_ip[1]) + "." + String(stored_networks[i].static_ip[2]) + "." + String(stored_networks[i].static_ip[3]);
        net["netmask"] = String(stored_networks[i].netmask[0]) + "." + String(stored_networks[i].netmask[1]) + "." + String(stored_networks[i].netmask[2]) + "." + String(stored_networks[i].netmask[3]);
        net["gateway"] = String(stored_networks[i].gateway[0]) + "." + String(stored_networks[i].gateway[1]) + "." + String(stored_networks[i].gateway[2]) + "." + String(stored_networks[i].gateway[3]);
        net["dns"] = String(stored_networks[i].dns[0]) + "." + String(stored_networks[i].dns[1]) + "." + String(stored_networks[i].dns[2]) + "." + String(stored_networks[i].dns[3]);
    }

    core_config.frequency_correction_ppm = settings.frequency_correction_ppm;
    save_core_config();

    if (serializeJson(doc, file) == 0) {
        logMessage("SETTINGS: Failed to write settings JSON");
        file.close();
        return false;
    }

    file.close();
    logMessage("SETTINGS: Configuration saved successfully");
    return true;
}

bool load_settings() {
    logMessage("SETTINGS: Loading configuration from /settings.json");

    if (!SPIFFS.exists("/settings.json")) {
        logMessage("SETTINGS: Config file not found, using defaults");
        load_default_settings();
        save_settings();
        return false;
    }

    File file = SPIFFS.open("/settings.json", "r");
    if (!file) {
        logMessage("SETTINGS: Failed to open config file");
        load_default_settings();
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        logMessage("SETTINGS: Failed to parse config JSON, using defaults");
        load_default_settings();
        save_settings();
        return false;
    }

    settings.mqtt_boot_delay_ms = 0;

    if (doc.containsKey("device")) {
        JsonObject device = doc["device"];
        settings.theme = device["theme"] | 0;
        strlcpy(settings.banner_message, device["banner_message"] | "flex-fsk-tx", sizeof(settings.banner_message));
        settings.timezone_offset_hours = device["timezone_offset_hours"] | 0.0;
        strlcpy(settings.ntp_server, device["ntp_server"] | "pool.ntp.org", sizeof(settings.ntp_server));
    }

    if (doc.containsKey("alerts")) {
        JsonObject alerts = doc["alerts"];
        settings.enable_low_battery_alert = alerts["low_battery"] | true;
        settings.enable_power_disconnect_alert = alerts["power_disconnect"] | true;
    }

    if (doc.containsKey("rf_amplifier")) {
        JsonObject rf_amp = doc["rf_amplifier"];
        settings.enable_rf_amplifier = rf_amp["enabled"] | false;
        settings.rf_amplifier_power_pin = rf_amp["power_pin"] | RFAMP_PWR_PIN;
        settings.rf_amplifier_delay_ms = rf_amp["delay_ms"] | 200;
        settings.rf_amplifier_active_high = rf_amp["active_high"] | true;
    }

    if (doc.containsKey("flex")) {
        JsonObject flex = doc["flex"];
        settings.default_frequency = flex["default_frequency"] | 931.9375;
        settings.default_capcode = strtoull(flex["default_capcode"] | "37137", nullptr, 10);
        settings.default_txpower = flex["default_txpower"] | 10.0;
        settings.frequency_correction_ppm = flex["frequency_correction_ppm"] | 0.0;
    }

    if (doc.containsKey("api")) {
        JsonObject api = doc["api"];
        settings.api_enabled = api["enabled"] | true;
        settings.http_port = api["http_port"] | 80;
        strlcpy(settings.api_username, api["username"] | "admin", sizeof(settings.api_username));
        String decoded_password = base64_decode_string(api["password"] | "");
        strlcpy(settings.api_password, decoded_password.c_str(), sizeof(settings.api_password));
    }

    if (doc.containsKey("services")) {
        JsonObject services = doc["services"];
        settings.mqtt_enabled = services["mqtt_enabled"] | false;
        settings.mqtt_boot_delay_ms = services["mqtt_boot_delay_ms"] | 0;
        settings.imap_enabled = services["imap_enabled"] | true;
        settings.grafana_enabled = services["grafana_enabled"] | true;
    }

    if (doc.containsKey("mqtt")) {
        JsonObject mqtt = doc["mqtt"];
        strlcpy(settings.mqtt_server, mqtt["server"] | "", sizeof(settings.mqtt_server));
        settings.mqtt_port = mqtt["port"] | 8883;
        strlcpy(settings.mqtt_thing_name, mqtt["thing_name"] | "", sizeof(settings.mqtt_thing_name));
        strlcpy(settings.mqtt_subscribe_topic, mqtt["subscribe_topic"] | "", sizeof(settings.mqtt_subscribe_topic));
        strlcpy(settings.mqtt_publish_topic, mqtt["publish_topic"] | "", sizeof(settings.mqtt_publish_topic));
    }

    if (doc.containsKey("rsyslog")) {
        JsonObject rsyslog = doc["rsyslog"];
        settings.rsyslog_enabled = rsyslog["enabled"] | false;
        strlcpy(settings.rsyslog_server, rsyslog["server"] | "", sizeof(settings.rsyslog_server));
        settings.rsyslog_port = rsyslog["port"] | 514;
        settings.rsyslog_use_tcp = rsyslog["use_tcp"] | false;
        settings.rsyslog_min_severity = rsyslog["min_severity"] | 6;
    }

    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];

        int net_index = 0;
        for (JsonPair kv : wifi) {
            if (net_index >= MAX_WIFI_NETWORKS) break;

            const char* ssid = kv.key().c_str();
            JsonObject network = kv.value();

            strlcpy(stored_networks[net_index].ssid, ssid, sizeof(stored_networks[net_index].ssid));

            String password = base64_decode_string(network["password"] | "");
            strlcpy(stored_networks[net_index].password, password.c_str(), sizeof(stored_networks[net_index].password));

            stored_networks[net_index].use_dhcp = network["use_dhcp"] | true;

            String static_ip_str = network["static_ip"] | "192.168.1.100";
            sscanf(static_ip_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[net_index].static_ip[0],
                   &stored_networks[net_index].static_ip[1],
                   &stored_networks[net_index].static_ip[2],
                   &stored_networks[net_index].static_ip[3]);

            String netmask_str = network["netmask"] | "255.255.255.0";
            sscanf(netmask_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[net_index].netmask[0],
                   &stored_networks[net_index].netmask[1],
                   &stored_networks[net_index].netmask[2],
                   &stored_networks[net_index].netmask[3]);

            String gateway_str = network["gateway"] | "192.168.1.1";
            sscanf(gateway_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[net_index].gateway[0],
                   &stored_networks[net_index].gateway[1],
                   &stored_networks[net_index].gateway[2],
                   &stored_networks[net_index].gateway[3]);

            String dns_str = network["dns"] | "8.8.8.8";
            sscanf(dns_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[net_index].dns[0],
                   &stored_networks[net_index].dns[1],
                   &stored_networks[net_index].dns[2],
                   &stored_networks[net_index].dns[3]);

            net_index++;
        }
        stored_networks_count = net_index;

        logMessagef("WIFI: Loaded %d network(s) from settings", stored_networks_count);
    }

    logMessage("SETTINGS: Configuration loaded successfully");
    return true;
}

void load_default_settings() {
    logMessage("SETTINGS: Loading default settings configuration");

    settings.theme = 0;
    strlcpy(settings.banner_message, "flex-fsk-tx", sizeof(settings.banner_message));
    settings.timezone_offset_hours = 0.0;
    strlcpy(settings.ntp_server, "pool.ntp.org", sizeof(settings.ntp_server));

    settings.enable_low_battery_alert = true;
    settings.enable_power_disconnect_alert = true;

    settings.enable_rf_amplifier = false;
    settings.rf_amplifier_power_pin = RFAMP_PWR_PIN;
    settings.rf_amplifier_delay_ms = 200;
    settings.rf_amplifier_active_high = true;

    settings.default_frequency = 931.9375;
    settings.default_capcode = 37137;
    settings.default_txpower = 10.0;
    settings.frequency_correction_ppm = (core_config.frequency_correction_ppm != 0.0)
        ? core_config.frequency_correction_ppm
        : 0.0;

    settings.api_enabled = true;
    settings.http_port = 80;
    strlcpy(settings.api_username, "admin", sizeof(settings.api_username));
    strlcpy(settings.api_password, "passw0rd", sizeof(settings.api_password));

    settings.mqtt_enabled = false;
    settings.mqtt_boot_delay_ms = 0;
    settings.imap_enabled = true;
    settings.grafana_enabled = true;

    strlcpy(settings.mqtt_server, "", sizeof(settings.mqtt_server));
    settings.mqtt_port = 8883;
    strlcpy(settings.mqtt_thing_name, "", sizeof(settings.mqtt_thing_name));
    strlcpy(settings.mqtt_subscribe_topic, "", sizeof(settings.mqtt_subscribe_topic));
    strlcpy(settings.mqtt_publish_topic, "", sizeof(settings.mqtt_publish_topic));

    settings.rsyslog_enabled = false;
    strlcpy(settings.rsyslog_server, "", sizeof(settings.rsyslog_server));
    settings.rsyslog_port = 514;
    settings.rsyslog_use_tcp = false;
    settings.rsyslog_min_severity = 6;

}


bool add_imap_account(const String& name, const String& server, uint16_t port, bool use_ssl,
                      const String& username, const String& password, uint16_t check_interval_min,
                      uint32_t capcode, float frequency, bool mail_drop) {
    if (imap_config.accounts.size() >= IMAP_MAX_ACCOUNTS) {
        logMessage("IMAP: Cannot add IMAP account - maximum accounts reached");
        return false;
    }

    if (name.length() == 0 || server.length() == 0 || username.length() == 0) {
        logMessage("IMAP: Cannot add IMAP account - required fields empty");
        return false;
    }

    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        if (String(imap_config.accounts[i].name) == name) {
            logMessage("IMAP: Cannot add IMAP account - name already exists");
            return false;
        }
    }

    IMAPAccount temp_account;
    temp_account.id = imap_config.accounts.size() + 1;
    strlcpy(temp_account.name, name.c_str(), sizeof(temp_account.name));
    strlcpy(temp_account.server, server.c_str(), sizeof(temp_account.server));
    temp_account.port = port;
    temp_account.use_ssl = use_ssl;
    strlcpy(temp_account.username, username.c_str(), sizeof(temp_account.username));
    strlcpy(temp_account.password, password.c_str(), sizeof(temp_account.password));
    temp_account.check_interval_min = max(check_interval_min, (uint16_t)IMAP_MIN_CHECK_INTERVAL);
    temp_account.capcode = capcode;
    temp_account.frequency = frequency;
    temp_account.mail_drop = mail_drop;
    temp_account.last_check = 0;
    temp_account.failed_check_cycles = 0;
    temp_account.suspended = false;

    imap_config.accounts.push_back(temp_account);
    imap_config.account_count = imap_config.accounts.size();

    logMessagef("IMAP: Account '%s' added successfully (ID: %d)", name.c_str(), temp_account.id);
    return true;
}

bool edit_imap_account(uint8_t id, const String& name, const String& server, uint16_t port, bool use_ssl,
                       const String& username, const String& password, uint16_t check_interval_min,
                       uint32_t capcode, float frequency, bool mail_drop) {
    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        if (imap_config.accounts[i].id == id) {
            for (size_t j = 0; j < imap_config.accounts.size(); j++) {
                if (j != i && String(imap_config.accounts[j].name) == name) {
                    logMessage("IMAP: Cannot edit IMAP account - name already exists");
                    return false;
                }
            }

            strlcpy(imap_config.accounts[i].name, name.c_str(), sizeof(imap_config.accounts[i].name));
            strlcpy(imap_config.accounts[i].server, server.c_str(), sizeof(imap_config.accounts[i].server));
            imap_config.accounts[i].port = port;
            imap_config.accounts[i].use_ssl = use_ssl;
            strlcpy(imap_config.accounts[i].username, username.c_str(), sizeof(imap_config.accounts[i].username));
            strlcpy(imap_config.accounts[i].password, password.c_str(), sizeof(imap_config.accounts[i].password));
            imap_config.accounts[i].check_interval_min = max(check_interval_min, (uint16_t)IMAP_MIN_CHECK_INTERVAL);
            imap_config.accounts[i].capcode = capcode;
            imap_config.accounts[i].frequency = frequency;
            imap_config.accounts[i].mail_drop = mail_drop;

            logMessagef("IMAP: account ID %d updated successfully", id);
            return true;
        }
    }

    logMessagef("IMAP: account ID %d not found for editing", id);
    return false;
}

bool delete_imap_account(uint8_t id) {
    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        if (imap_config.accounts[i].id == id) {
            String account_name = String(imap_config.accounts[i].name);
            imap_config.accounts.erase(imap_config.accounts.begin() + i);
            imap_config.account_count = imap_config.accounts.size();

            for (size_t j = 0; j < imap_config.accounts.size(); j++) {
                imap_config.accounts[j].id = j + 1;
            }

            logMessagef("IMAP: account '%s' (ID: %d) deleted successfully", account_name.c_str(), id);
            return true;
        }
    }

    logMessagef("CONFIG: account ID %d not found for deletion", id);
    return false;
}



bool any_imap_accounts_suspended() {
    for (size_t i = 0; i < imap_config.accounts.size(); i++) {
        if (imap_config.accounts[i].suspended) {
            return true;
        }
    }
    return false;
}

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

String config_to_json() {
    DynamicJsonDocument doc(8192);

    doc["version"] = CURRENT_VERSION;
    doc["timestamp"] = getLocalTimestamp();

    JsonObject cfg = doc.createNestedObject("config");

    JsonObject device = cfg.createNestedObject("device");
    device["theme"] = settings.theme;
    device["banner_message"] = String(settings.banner_message);
    device["timezone_offset_hours"] = settings.timezone_offset_hours;
    device["ntp_server"] = String(settings.ntp_server);

    JsonObject wifi = cfg.createNestedObject("wifi");
    for (int i = 0; i < stored_networks_count; i++) {
        JsonObject net = wifi.createNestedObject(stored_networks[i].ssid);
        net["password"] = base64_encode_string(String(stored_networks[i].password));
        net["use_dhcp"] = stored_networks[i].use_dhcp;
        net["static_ip"] = String(stored_networks[i].static_ip[0]) + "." + String(stored_networks[i].static_ip[1]) + "." + String(stored_networks[i].static_ip[2]) + "." + String(stored_networks[i].static_ip[3]);
        net["netmask"] = String(stored_networks[i].netmask[0]) + "." + String(stored_networks[i].netmask[1]) + "." + String(stored_networks[i].netmask[2]) + "." + String(stored_networks[i].netmask[3]);
        net["gateway"] = String(stored_networks[i].gateway[0]) + "." + String(stored_networks[i].gateway[1]) + "." + String(stored_networks[i].gateway[2]) + "." + String(stored_networks[i].gateway[3]);
        net["dns"] = String(stored_networks[i].dns[0]) + "." + String(stored_networks[i].dns[1]) + "." + String(stored_networks[i].dns[2]) + "." + String(stored_networks[i].dns[3]);
    }

    JsonObject alerts = cfg.createNestedObject("alerts");
    alerts["low_battery"] = settings.enable_low_battery_alert;
    alerts["power_disconnect"] = settings.enable_power_disconnect_alert;

    JsonObject rf_amp = cfg.createNestedObject("rf_amplifier");
    rf_amp["enabled"] = settings.enable_rf_amplifier;
    rf_amp["power_pin"] = settings.rf_amplifier_power_pin;
    rf_amp["delay_ms"] = settings.rf_amplifier_delay_ms;
    rf_amp["active_high"] = settings.rf_amplifier_active_high;

    JsonObject flex = cfg.createNestedObject("flex");
    flex["default_frequency"] = settings.default_frequency;
    flex["default_capcode"] = String(settings.default_capcode);
    flex["default_txpower"] = settings.default_txpower;
    flex["frequency_correction_ppm"] = settings.frequency_correction_ppm;

    JsonObject api = cfg.createNestedObject("api");
    api["enable"] = settings.api_enabled;
    api["http_port"] = settings.http_port;
    api["username"] = String(settings.api_username);
    api["password"] = base64_encode_string(String(settings.api_password));

    JsonObject grafana = cfg.createNestedObject("grafana");
    grafana["enable"] = settings.grafana_enabled;

    JsonObject mqtt = cfg.createNestedObject("mqtt");
    mqtt["enabled"] = settings.mqtt_enabled;
    mqtt["server"] = String(settings.mqtt_server);
    mqtt["port"] = settings.mqtt_port;
    mqtt["thing_name"] = String(settings.mqtt_thing_name);
    mqtt["subscribe_topic"] = String(settings.mqtt_subscribe_topic);
    mqtt["publish_topic"] = String(settings.mqtt_publish_topic);

    JsonObject certs = mqtt.createNestedObject("certificates_b64");

    certs["root_ca"] = base64_encode_string(loadCertificateFromSPIFFS(MQTT_CA_CERT_FILE));
    certs["device_cert"] = base64_encode_string(loadCertificateFromSPIFFS(MQTT_DEVICE_CERT_FILE));
    certs["device_key"] = base64_encode_string(loadCertificateFromSPIFFS(MQTT_DEVICE_KEY_FILE));

    JsonObject rsyslog = cfg.createNestedObject("rsyslog");
    rsyslog["enabled"] = settings.rsyslog_enabled;
    rsyslog["server"] = String(settings.rsyslog_server);
    rsyslog["port"] = settings.rsyslog_port;
    rsyslog["use_tcp"] = settings.rsyslog_use_tcp;
    rsyslog["min_severity"] = settings.rsyslog_min_severity;

    JsonObject imap = cfg.createNestedObject("imap");

    File imap_file = SPIFFS.open("/imap_settings.json", "r");
    if (imap_file) {
        String imap_content = imap_file.readString();
        imap_file.close();

        DynamicJsonDocument imap_doc(8192);
        DeserializationError error = deserializeJson(imap_doc, imap_content);
        if (!error) {
            JsonObject source = imap_doc.as<JsonObject>();
            for (JsonPair kv : source) {
                imap[kv.key()] = kv.value();
            }
            logMessage("BACKUP: IMAP settings packed directly in backup");
        } else {
            logMessage("BACKUP ERROR: Failed to parse IMAP settings JSON");
        }
    } else {
        logMessage("BACKUP: No IMAP settings file found");
    }

    JsonObject chatgpt = cfg.createNestedObject("chatgpt");

    File chatgpt_file = SPIFFS.open("/chatgpt_settings.json", "r");
    if (chatgpt_file) {
        String chatgpt_content = chatgpt_file.readString();
        chatgpt_file.close();

        DynamicJsonDocument chatgpt_doc(8192);
        DeserializationError error = deserializeJson(chatgpt_doc, chatgpt_content);
        if (!error) {
            JsonObject source = chatgpt_doc.as<JsonObject>();
            for (JsonPair kv : source) {
                chatgpt[kv.key()] = kv.value();
            }
            logMessage("BACKUP: ChatGPT prompts packed directly in backup");
        } else {
            logMessage("BACKUP ERROR: Failed to parse ChatGPT prompts JSON");
        }
    } else {
        logMessage("BACKUP: No ChatGPT prompts file found");
    }

    String temp_json;
    serializeJson(doc, temp_json);
    String checksum = calculate_crc32(temp_json);
    doc["checksum"] = checksum;

    String json_string;
    serializeJsonPretty(doc, json_string);

    return json_string;
}

bool json_to_config(const String& json_string, String& error_msg) {
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, json_string);

    if (error) {
        error_msg = "Invalid JSON format: " + String(error.c_str());
        return false;
    }

    if (!doc.containsKey("checksum") || !doc.containsKey("config")) {
        error_msg = "Missing required fields in backup file";
        return false;
    }

    String stored_checksum = doc["checksum"].as<String>();
    doc.remove("checksum");

    String temp_json;
    serializeJson(doc, temp_json);
    String calculated_checksum = calculate_crc32(temp_json);

    if (stored_checksum != calculated_checksum) {
        error_msg = "Checksum validation failed - file may be corrupted";
        return false;
    }

    JsonObject cfg = doc["config"];
    CoreConfig temp_core_config = core_config;
    DeviceSettings temp_settings = settings;

    if (cfg.containsKey("device")) {
        JsonObject device = cfg["device"];
        if (device.containsKey("theme"))
            temp_settings.theme = device["theme"];
        if (device.containsKey("banner_message"))
            strncpy(temp_settings.banner_message, device["banner_message"].as<String>().c_str(), sizeof(temp_settings.banner_message) - 1);
        if (device.containsKey("timezone_offset_hours"))
            temp_settings.timezone_offset_hours = device["timezone_offset_hours"];
        if (device.containsKey("ntp_server"))
            strncpy(temp_settings.ntp_server, device["ntp_server"].as<String>().c_str(), sizeof(temp_settings.ntp_server) - 1);
    }

    if (cfg.containsKey("wifi")) {
        JsonObject wifi = cfg["wifi"];

        stored_networks_count = 0;
        for (JsonPair kv : wifi) {
            if (stored_networks_count >= MAX_WIFI_NETWORKS) break;

            const char* ssid = kv.key().c_str();
            JsonObject network = kv.value();

            strlcpy(stored_networks[stored_networks_count].ssid, ssid, sizeof(stored_networks[stored_networks_count].ssid));

            String password = base64_decode_string(network["password"] | "");
            strlcpy(stored_networks[stored_networks_count].password, password.c_str(), sizeof(stored_networks[stored_networks_count].password));

            stored_networks[stored_networks_count].use_dhcp = network["use_dhcp"] | true;

            String static_ip_str = network["static_ip"] | "192.168.1.100";
            sscanf(static_ip_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[stored_networks_count].static_ip[0],
                   &stored_networks[stored_networks_count].static_ip[1],
                   &stored_networks[stored_networks_count].static_ip[2],
                   &stored_networks[stored_networks_count].static_ip[3]);

            String netmask_str = network["netmask"] | "255.255.255.0";
            sscanf(netmask_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[stored_networks_count].netmask[0],
                   &stored_networks[stored_networks_count].netmask[1],
                   &stored_networks[stored_networks_count].netmask[2],
                   &stored_networks[stored_networks_count].netmask[3]);

            String gateway_str = network["gateway"] | "192.168.1.1";
            sscanf(gateway_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[stored_networks_count].gateway[0],
                   &stored_networks[stored_networks_count].gateway[1],
                   &stored_networks[stored_networks_count].gateway[2],
                   &stored_networks[stored_networks_count].gateway[3]);

            String dns_str = network["dns"] | "8.8.8.8";
            sscanf(dns_str.c_str(), "%hhu.%hhu.%hhu.%hhu",
                   &stored_networks[stored_networks_count].dns[0],
                   &stored_networks[stored_networks_count].dns[1],
                   &stored_networks[stored_networks_count].dns[2],
                   &stored_networks[stored_networks_count].dns[3]);

            stored_networks_count++;
        }

        logMessagef("RESTORE: Loaded %d WiFi network(s)", stored_networks_count);
    }


    if (cfg.containsKey("alerts")) {
        JsonObject alerts = cfg["alerts"];
        if (alerts.containsKey("low_battery"))
            temp_settings.enable_low_battery_alert = alerts["low_battery"];
        if (alerts.containsKey("power_disconnect"))
            temp_settings.enable_power_disconnect_alert = alerts["power_disconnect"];
    }

    if (cfg.containsKey("rf_amplifier")) {
        JsonObject rf_amp = cfg["rf_amplifier"];
        if (rf_amp.containsKey("enabled"))
            temp_settings.enable_rf_amplifier = rf_amp["enabled"];
        if (rf_amp.containsKey("power_pin"))
            temp_settings.rf_amplifier_power_pin = rf_amp["power_pin"];
        if (rf_amp.containsKey("delay_ms"))
            temp_settings.rf_amplifier_delay_ms = rf_amp["delay_ms"];
        if (rf_amp.containsKey("active_high"))
            temp_settings.rf_amplifier_active_high = rf_amp["active_high"];
    }

    if (cfg.containsKey("flex")) {
        JsonObject flex = cfg["flex"];
        if (flex.containsKey("default_frequency"))
            temp_settings.default_frequency = flex["default_frequency"];
        if (flex.containsKey("default_capcode"))
            temp_settings.default_capcode = strtoull(flex["default_capcode"].as<String>().c_str(), NULL, 10);
        if (flex.containsKey("default_txpower"))
            temp_settings.default_txpower = flex["default_txpower"];
        if (flex.containsKey("frequency_correction_ppm")) {
            float backup_freq_corr = flex["frequency_correction_ppm"];
            if (core_config.frequency_correction_ppm != 0.0) {
                temp_settings.frequency_correction_ppm = core_config.frequency_correction_ppm;
                logMessagef("RESTORE: Using NVS frequency_correction_ppm: %.2f (ignoring backup)", core_config.frequency_correction_ppm);
            } else if (backup_freq_corr != 0.0) {
                temp_settings.frequency_correction_ppm = backup_freq_corr;
                temp_core_config.frequency_correction_ppm = backup_freq_corr;
                logMessagef("RESTORE: Using backup frequency_correction_ppm: %.2f", backup_freq_corr);
            } else {
                temp_settings.frequency_correction_ppm = 0.0;
            }
        }
    }

    if (cfg.containsKey("api")) {
        JsonObject api = cfg["api"];
        if (api.containsKey("enable"))
            temp_settings.api_enabled = api["enable"];
        if (api.containsKey("http_port"))
            temp_settings.http_port = api["http_port"];
        if (api.containsKey("username"))
            strncpy(temp_settings.api_username, api["username"].as<String>().c_str(), sizeof(temp_settings.api_username) - 1);
        if (api.containsKey("password")) {
            String decoded_password = base64_decode_string(api["password"].as<String>());
            strncpy(temp_settings.api_password, decoded_password.c_str(), sizeof(temp_settings.api_password) - 1);
        }
    }

    if (cfg.containsKey("grafana")) {
        JsonObject grafana = cfg["grafana"];
        if (grafana.containsKey("enable"))
            temp_settings.grafana_enabled = grafana["enable"];
    }

    if (cfg.containsKey("mqtt")) {
        JsonObject mqtt = cfg["mqtt"];
        if (mqtt.containsKey("enabled"))
            temp_settings.mqtt_enabled = mqtt["enabled"];
        if (mqtt.containsKey("server"))
            strncpy(temp_settings.mqtt_server, mqtt["server"].as<String>().c_str(), sizeof(temp_settings.mqtt_server) - 1);
        if (mqtt.containsKey("port"))
            temp_settings.mqtt_port = mqtt["port"];
        if (mqtt.containsKey("thing_name"))
            strncpy(temp_settings.mqtt_thing_name, mqtt["thing_name"].as<String>().c_str(), sizeof(temp_settings.mqtt_thing_name) - 1);
        if (mqtt.containsKey("subscribe_topic"))
            strncpy(temp_settings.mqtt_subscribe_topic, mqtt["subscribe_topic"].as<String>().c_str(), sizeof(temp_settings.mqtt_subscribe_topic) - 1);
        if (mqtt.containsKey("publish_topic"))
            strncpy(temp_settings.mqtt_publish_topic, mqtt["publish_topic"].as<String>().c_str(), sizeof(temp_settings.mqtt_publish_topic) - 1);

        if (mqtt.containsKey("certificates_b64")) {
            JsonObject certs = mqtt["certificates_b64"];
            if (certs.containsKey("root_ca") && !certs["root_ca"].as<String>().isEmpty()) {
                String decoded = base64_decode_string(certs["root_ca"].as<String>());
                if (saveCertificateToSPIFFS(MQTT_CA_CERT_FILE, decoded)) {
                    logMessage("RESTORE: Root CA certificate restored to SPIFFS (" + String(decoded.length()) + " bytes)");
                } else {
                    logMessage("RESTORE ERROR: Failed to save Root CA to SPIFFS");
                }
            }
            if (certs.containsKey("device_cert") && !certs["device_cert"].as<String>().isEmpty()) {
                String decoded = base64_decode_string(certs["device_cert"].as<String>());
                if (saveCertificateToSPIFFS(MQTT_DEVICE_CERT_FILE, decoded)) {
                    logMessage("RESTORE: Device certificate restored to SPIFFS (" + String(decoded.length()) + " bytes)");
                } else {
                    logMessage("RESTORE ERROR: Failed to save Device certificate to SPIFFS");
                }
            }
            if (certs.containsKey("device_key") && !certs["device_key"].as<String>().isEmpty()) {
                String decoded = base64_decode_string(certs["device_key"].as<String>());
                if (saveCertificateToSPIFFS(MQTT_DEVICE_KEY_FILE, decoded)) {
                    logMessage("RESTORE: Device private key restored to SPIFFS (" + String(decoded.length()) + " bytes)");
                } else {
                    logMessage("RESTORE ERROR: Failed to save Device private key to SPIFFS");
                }
            }
        }
    }

    if (cfg.containsKey("rsyslog")) {
        JsonObject rsyslog = cfg["rsyslog"];
        if (rsyslog.containsKey("enabled"))
            temp_settings.rsyslog_enabled = rsyslog["enabled"];
        if (rsyslog.containsKey("server"))
            strncpy(temp_settings.rsyslog_server, rsyslog["server"].as<String>().c_str(), sizeof(temp_settings.rsyslog_server) - 1);
        if (rsyslog.containsKey("port"))
            temp_settings.rsyslog_port = rsyslog["port"];
        if (rsyslog.containsKey("use_tcp"))
            temp_settings.rsyslog_use_tcp = rsyslog["use_tcp"];
        if (rsyslog.containsKey("min_severity"))
            temp_settings.rsyslog_min_severity = rsyslog["min_severity"];
    }

    temp_settings.imap_enabled = false;

    if (cfg.containsKey("chatgpt")) {
        JsonObject chatgpt = cfg["chatgpt"];

        if (chatgpt.containsKey("enabled")) {
            chatgpt_config.enabled = chatgpt["enabled"];
        }
        if (chatgpt.containsKey("api_key_b64")) {
            String api_key_b64 = chatgpt["api_key_b64"].as<String>();
            strncpy(chatgpt_config.api_key_b64, api_key_b64.c_str(), sizeof(chatgpt_config.api_key_b64) - 1);
        }

        String chatgpt_json;
        serializeJsonPretty(chatgpt, chatgpt_json);

        File chatgpt_file = SPIFFS.open("/chatgpt_settings.json", "w");
        if (chatgpt_file) {
            chatgpt_file.print(chatgpt_json);
            chatgpt_file.close();

            chatgpt_load_config();
            logMessage("RESTORE: ChatGPT prompts restored to SPIFFS (" + String(chatgpt_json.length()) + " bytes)");
        } else {
            logMessage("RESTORE ERROR: Failed to save ChatGPT prompts to SPIFFS");
        }
    }

    if (cfg.containsKey("imap")) {
        JsonObject imap = cfg["imap"];

        String imap_json;
        serializeJsonPretty(imap, imap_json);

        File imap_file = SPIFFS.open("/imap_settings.json", "w");
        if (imap_file) {
            imap_file.print(imap_json);
            imap_file.close();

            logMessage("RESTORE: IMAP settings restored to SPIFFS (" + String(imap_json.length()) + " bytes)");
        } else {
            logMessage("RESTORE ERROR: Failed to save IMAP settings to SPIFFS");
        }
    }

    core_config = temp_core_config;
    settings = temp_settings;

    return true;
}


void display_turn_off() {
    if (oled_active) {
        display.setPowerSave(1);
        VextOFF();
        oled_active = false;
    }
}

void display_turn_on() {
    if (!oled_active) {
        VextON();
        delay(10);
        display.setPowerSave(0);
        oled_active = true;
    }
}

void getBatteryInfo(uint16_t *voltage_mv, int *percentage) {
    int adc_value = analogRead(BATTERY_ADC_PIN);

    float battery_voltage_local = (float)(adc_value) / 4095.0 * 2.0 * 3.3 * 1.1;

    battery_present = (battery_voltage_local > 2.5);

    int battery_percentage_local = 0;
    if (battery_present) {
        if (battery_voltage_local >= 4.15) {
            battery_percentage_local = 100;
        } else {
            float voltage_clamped = constrain(battery_voltage_local, 3.2, 4.15);
            battery_percentage_local = map(voltage_clamped * 100, 320, 415, 0, 100);
            battery_percentage_local = constrain(battery_percentage_local, 0, 100);
        }
    }

    *voltage_mv = (uint16_t)(battery_voltage_local * 1000);
    *percentage = battery_percentage_local;
}


void VextON(void) {
    if (VEXT_PIN >= 0) {
        pinMode(VEXT_PIN, OUTPUT);
        digitalWrite(VEXT_PIN, LOW);
    }
}

void VextOFF(void) {
    if (VEXT_PIN >= 0) {
        pinMode(VEXT_PIN, OUTPUT);
        digitalWrite(VEXT_PIN, HIGH);
    }
}

float readBatteryVoltage() {
    int adc_value = analogRead(BATTERY_ADC_PIN);
    return (float)(adc_value) / 4095.0 * 2.0 * 3.3 * 1.1;
}

void handle_led_heartbeat() {
    unsigned long current_time = millis();

    if ((unsigned long)(current_time - last_heartbeat) >= HEARTBEAT_INTERVAL) {
        heartbeat_blink_count = 0;
        heartbeat_state = true;
        LED_ON();
        last_heartbeat = current_time;
    }

    if (heartbeat_state) {
        static unsigned long last_blink = 0;

        if (current_time - last_blink >= HEARTBEAT_BLINK_DURATION) {
            if (heartbeat_blink_count < 4) {
                if (heartbeat_blink_count % 2 == 0) {
                    LED_OFF();
                } else {
                    LED_ON();
                }
                heartbeat_blink_count++;
                last_blink = current_time;
            } else {
                LED_OFF();
                heartbeat_state = false;
                heartbeat_blink_count = 0;
            }
        }
    }
}

String generate_ap_ssid() {
    WiFi.mode(WIFI_STA);
    delay(100);

    uint8_t mac[6];
    WiFi.macAddress(mac);

    uint32_t unique_id = (mac[3] << 16) | (mac[4] << 8) | mac[5];

    String suffix = String(unique_id & 0xFFFF, HEX);
    suffix.toUpperCase();
    while (suffix.length() < 4) {
        suffix = "0" + suffix;
    }

    String macStr = "SYSTEM: Device MAC: ";
    for (int i = 0; i < 6; i++) {
        if (i > 0) macStr += ":";
        if (mac[i] < 16) macStr += "0";
        macStr += String(mac[i], HEX);
    }
    logMessage(macStr);
    logMessage("SYSTEM: Generated AP SSID: FLEX_" + suffix);

    return "FLEX_" + suffix;
}

String generate_ap_password() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    char password[9];
    sprintf(password, "%02X%02X%02X%02X",
            mac[2], mac[3], mac[4], mac[5]);

    return String(password);
}

void perform_factory_reset() {
    display_turn_on();
    const int centerX = display.getWidth() / 2;
    const int centerY = display.getHeight() / 2;
    const char *message = "FACTORY RESET";

    display.clearBuffer();
    display.setFont(u8g2_font_nokiafc22_tr);
    int width = display.getStrWidth(message);
    display.drawStr(centerX - (width / 2), centerY, message);
    display.sendBuffer();

    esp_task_wdt_deinit();
    logMessage("SYSTEM: Formatting SPIFFS...");
    SPIFFS.format();

    delay(1000);
    ESP.restart();
}

void handle_factory_reset() {
    bool button_pressed = (digitalRead(FACTORY_RESET_PIN) == LOW);
    unsigned long current_time = millis();

    if (button_pressed && !factory_reset_pressed) {
        factory_reset_pressed = true;
        factory_reset_start = current_time;
        logMessage("SYSTEM: Factory reset button pressed - hold for 30 seconds");
    } else if (!button_pressed && factory_reset_pressed) {
        factory_reset_pressed = false;
        logMessage("SYSTEM: Factory reset cancelled");
    } else if (button_pressed && factory_reset_pressed) {
        if (current_time - factory_reset_start >= FACTORY_RESET_HOLD_TIME) {
            logMessage("SYSTEM: Factory reset initiated!");

            display.clearBuffer();
            display.setFont(u8g2_font_6x10_tr);
            display.drawStr(10, 20, "FACTORY RESET");
            display.drawStr(10, 35, "Restoring defaults...");
            display.sendBuffer();

            delay(2000);

            perform_factory_reset();
        } else {
            unsigned long remaining = FACTORY_RESET_HOLD_TIME - (current_time - factory_reset_start);
            if ((current_time - factory_reset_start) % 5000 < 100) {
                logMessagef("SYSTEM: Factory reset in: %d seconds", remaining / 1000);
            }
        }
    }
}

void reset_oled_timeout() {
    last_activity_time = millis();
    display_turn_on();
}

void display_panic() {
    const int centerX = display.getWidth() / 2;
    const int centerY = display.getHeight() / 2;
    const char *message = "System halted";

    display.clearBuffer();
    display.setFont(u8g2_font_open_iconic_check_4x_t);
    display.drawGlyph(centerX - (32 / 2), centerY + (32 / 2), 66);
    display.setFont(u8g2_font_nokiafc22_tr);
    int width = display.getStrWidth(message);
    display.drawStr(centerX - (width / 2), centerY + 30, message);
    display.sendBuffer();
}

void display_setup() {
    VextON();
    delay(10);
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    if (OLED_RST_PIN >= 0) {
        pinMode(OLED_RST_PIN, OUTPUT);
        digitalWrite(OLED_RST_PIN, LOW);
        delay(50);
        digitalWrite(OLED_RST_PIN, HIGH);
        delay(50);
    }
    display.begin();
    display.clearBuffer();

#if RTC_ENABLED
    if (rtc.begin()) {
        rtc_available = true;
        logMessage("RTC: DS3231 initialized successfully");

        if (!rtc.lostPower()) {
            DateTime now = rtc.now();
            struct timeval tv;
            tv.tv_sec = now.unixtime();
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            system_time_initialized = true;
            system_time_from_rtc = true;
            logMessagef("RTC: System time set from RTC: %04d-%02d-%02d %02d:%02d:%02d",
                       now.year(), now.month(), now.day(),
                       now.hour(), now.minute(), now.second());
        } else {
            logMessage("RTC: WARNING - RTC lost power, time may be incorrect");
        }
    } else {
        rtc_available = false;
        logMessage("RTC: Failed to initialize DS3231");
    }
#endif
}

void display_ap_info() {
    if (!oled_active) return;

    display.clearBuffer();

    int info_start_y = 12;

    display.setFont(u8g2_font_7x13_tr);

    display.drawStr(0, info_start_y, "AP Mode Active");

    info_start_y += 14;
    display.setFont(u8g2_font_6x10_tr);
    String ssid_display = "SSID: " + ap_ssid;
    display.drawStr(0, info_start_y, ssid_display.c_str());

    display.setFont(u8g2_font_7x13_tr);
    info_start_y += 12;
    String pass_display = "Pass: " + ap_password;
    display.drawStr(0, info_start_y, pass_display.c_str());

    info_start_y += 12;
    String ip_str = "AP: " + WiFi.softAPIP().toString();
    display.drawStr(0, info_start_y, ip_str.c_str());

    display.sendBuffer();
}

void display_status() {
    if (!oled_active) return;

    if (ap_mode_active && WiFi.softAPgetStationNum() == 0) {
        display_ap_info();
        return;
    }

    uint16_t battery_voltage_mv;
    int battery_percentage_temp;
    getBatteryInfo(&battery_voltage_mv, &battery_percentage_temp);

    String tx_power_str;
    if (battery_present) {
        tx_power_str = String(tx_power, 1) + "dBm || " + String(battery_percentage_temp) + "%";
    } else {
        tx_power_str = String(tx_power, 1) + "dBm";
    }
    String tx_frequency_str = String(current_tx_frequency, 4) + " MHz";
    String status_str;
    String wifi_str;

    switch (device_state) {
        case STATE_IDLE:
            status_str = "Ready";
            break;
        case STATE_WAITING_FOR_DATA:
            status_str = "Receiving Data...";
            break;
        case STATE_WAITING_FOR_MSG:
            status_str = "Receiving Msg...";
            break;
        case STATE_TRANSMITTING:
            status_str = "Transmitting...";
            break;
        case STATE_ERROR:
            status_str = "Error";
            break;
        case STATE_WIFI_CONNECTING:
            status_str = "WiFi Connecting...";
            break;
        case STATE_WIFI_AP_MODE:
            status_str = "WiFi AP Mode";
            break;
        case STATE_IMAP_PROCESSING:
            status_str = "IMAP Sync...";
            break;
        case STATE_NTP_SYNC:
            status_str = "NTP Sync...";
            break;
        case STATE_MQTT_CONNECTING:
            if (mqtt_connection_attempt > 0) {
                status_str = "MQTT [" + String(mqtt_connection_attempt) + "]...";
            } else {
                status_str = "MQTT Connecting...";
            }
            break;
        default:
            status_str = "Unknown";
            break;
    }

    if (wifi_connected) {
        wifi_str = "WiFi: " + WiFi.localIP().toString();
    } else if (ap_mode_active) {
        wifi_str = "AP: " + WiFi.softAPIP().toString();
    } else {
        wifi_str = "WiFi: Connecting...";
    }

    display.clearBuffer();

    display.setFont(FONT_BANNER);
    int banner_width = display.getStrWidth(settings.banner_message);
    int banner_x = (display.getWidth() - banner_width) / 2;
    display.drawStr(banner_x, BANNER_HEIGHT, settings.banner_message);

    int status_start_y = BANNER_HEIGHT + BANNER_MARGIN + 10;

    display.setFont(u8g2_font_6x10_tr);

    display.drawStr(0, status_start_y, "State: ");
    display.drawStr(40, status_start_y, status_str.c_str());

    status_start_y += 10;

    display.drawStr(0, status_start_y, "Pwr: ");
    display.drawStr(30, status_start_y, tx_power_str.c_str());

    status_start_y += 10;
    display.drawStr(0, status_start_y, "Freq: ");
    display.drawStr(35, status_start_y, tx_frequency_str.c_str());

    status_start_y += 10;
    display.drawStr(0, status_start_y, wifi_str.c_str());

    display.sendBuffer();
}


static int str2uint64(uint64_t *out, const char *s) {
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

bool flex_encode_and_store(uint64_t capcode, const char *message, bool mail_drop) {
    uint8_t flex_buffer[FLEX_BUFFER_SIZE];
    struct tf_message_config config = {0};
    config.mail_drop = mail_drop ? 1 : 0;

    int error = 0;
    size_t encoded_size = tf_encode_flex_message_ex(message, capcode, flex_buffer,
                                                   sizeof(flex_buffer), &error, &config);

    if (error < 0 || encoded_size == 0 || encoded_size > sizeof(tx_data_buffer)) {
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
        uint8_t emr_pattern[] = {0xA5, 0x5A, 0xA5, 0x5A};
        radio.startTransmit(emr_pattern, sizeof(emr_pattern));

        unsigned long emr_start = millis();
        while (radio.getPacketLength() > 0 && ((unsigned long)(millis() - emr_start) < 2000)) {
            delay(1);
        }
        delay(100);

        last_emr_transmission = millis();
        first_message_sent = true;
    }
}

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



String chatgpt_encode_api_key(String api_key) {
    return base64_encode_string(api_key);
}

String chatgpt_decode_api_key(String encoded_key) {
    return base64_decode_string(encoded_key);
}

String chatgpt_mask_api_key(String api_key) {
    if (api_key.length() < 10) return "Invalid";

    String masked = api_key.substring(0, 3);
    masked += "***...***";
    masked += api_key.substring(api_key.length() - 3);
    return masked;
}

bool validate_flex_capcode(uint32_t capcode) {
    if (capcode < 1 || capcode > 4291000000UL) {
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

bool chatgpt_validate_api_key(String api_key) {
    return api_key.startsWith("sk-") && api_key.length() >= 20 && api_key.length() <= 200;
}


bool chatgpt_load_config() {
    File file = SPIFFS.open("/chatgpt_settings.json", "r");
    if (!file) {
        logMessage("CHATGPT: No configuration file found, using defaults");
        chatgpt_config.enabled = false;
        chatgpt_config.notify_on_failure = false;
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
    chatgpt_config.notify_on_failure = doc["notify_on_failure"] | false;
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
    DynamicJsonDocument doc(4096);

    doc["enabled"] = chatgpt_config.enabled;
    doc["notify_on_failure"] = chatgpt_config.notify_on_failure;
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
        file.close();
        logMessage("CHATGPT: Failed to write configuration file");
        return false;
    }

    file.close();
    logMessage("CHATGPT: Configuration saved successfully");
    return true;
}

bool chatgpt_validate_json(String json_content) {
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

void chatgpt_create_default_config() {
    chatgpt_config.enabled = false;
    chatgpt_config.notify_on_failure = false;
    chatgpt_config.prompts.clear();
    chatgpt_config.prompt_count = 0;
    strncpy(chatgpt_config.api_key_b64, "", sizeof(chatgpt_config.api_key_b64) - 1);
    chatgpt_config.api_key_b64[sizeof(chatgpt_config.api_key_b64) - 1] = '\0';
    logMessage("CHATGPT: Default configuration created");
}


void chatgpt_log_activity(const char* prompt_name, const char* query, const char* response,
                          bool query_success, bool transmission_success, uint32_t capcode, float frequency, int prompt_index, bool mail_drop) {
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

    chatgpt_activity_count++;
}


bool chatgpt_execute_prompt(ChatGPTPrompt& prompt, int prompt_index) {
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

            if (chatgpt_config.notify_on_failure) {
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

            if (chatgpt_config.notify_on_failure) {
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

String chatgpt_query(String prompt, String api_key) {
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
            "{\"role\":\"user\",\"content\":\"" + prompt + "\"}"
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


bool chatgpt_is_time_to_execute(ChatGPTPrompt& prompt) {
    if (!ntp_synced || !chatgpt_config.enabled || !prompt.enabled) {
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

    if (!ntp_synced) {
        return "NTP not synced";
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

bool wifi_ssid_scan() {
    logMessage("WiFi: Scanning for stored networks...");

    int n = WiFi.scanNetworks();
    wifi_scan_available = false;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < stored_networks_count; j++) {
            if (WiFi.SSID(i) == String(stored_networks[j].ssid)) {
                wifi_scan_available = true;
                logMessagef("WiFi: Stored network '%s' found (RSSI: %d dBm)",
                           stored_networks[j].ssid, WiFi.RSSI(i));
                break;
            }
        }
        if (wifi_scan_available) break;
    }

    if (!wifi_scan_available) {
        logMessagef("WiFi: No stored networks found (%d networks scanned)", n);
    }

    WiFi.scanDelete();
    last_wifi_scan_ms = millis();
    return wifi_scan_available;
}

void wifi_connect() {
    if (stored_networks_count == 0) {
        start_ap_mode();
        network_connect_pending = true;
        return;
    }

    device_state = STATE_WIFI_CONNECTING;
    display_status();

    WiFi.mode(WIFI_STA);

    WiFi.disconnect(true, true);
    delay(50);

    if (mac_suffix == "") {
        delay(100);
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char suffix[5];
        sprintf(suffix, "%02X%02X", mac[4], mac[5]);
        mac_suffix = String(suffix);
    }

    scan_and_connect_wifi();
    wifi_connect_start = millis();
    wifi_retry_count = 0;
    wifi_retry_silent = false;
}

bool scan_and_connect_wifi() {
    if (stored_networks_count == 0) {
        logMessage("WIFI: No stored networks configured");
        return false;
    }

    logMessagef("WIFI: Scanning for networks (have %d stored)", stored_networks_count);

    int n = WiFi.scanNetworks();

    if (n == 0) {
        logMessage("WIFI: No networks found in scan");
        return false;
    }

    logMessagef("WIFI: Scan complete, found %d network(s)", n);

    for (int j = 0; j < n; j++) {
        logMessagef("  [%d] %s (RSSI: %d dBm, Ch: %d, Enc: %s)",
                    j + 1,
                    WiFi.SSID(j).c_str(),
                    WiFi.RSSI(j),
                    WiFi.channel(j),
                    (WiFi.encryptionType(j) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted");
    }

    for (int i = 0; i < stored_networks_count; i++) {
        String stored_ssid = String(stored_networks[i].ssid);

        bool found = false;
        int rssi = 0;

        for (int j = 0; j < n; j++) {
            if (WiFi.SSID(j) == stored_ssid) {
                found = true;
                rssi = WiFi.RSSI(j);
                break;
            }
        }

        if (!found) {
            logMessagef("WIFI: Stored network '%s' not in range", stored_ssid.c_str());
            continue;
        }

        logMessagef("WIFI: Attempting connection to '%s' (RSSI: %d dBm, Priority: %d)",
                    stored_ssid.c_str(), rssi, i + 1);

        WiFi.disconnect();
        WiFi.mode(WIFI_STA);
        delay(100);

        if (!stored_networks[i].use_dhcp) {
            logMessage("WIFI: Configuring static IP");

            IPAddress ip(stored_networks[i].static_ip[0],
                         stored_networks[i].static_ip[1],
                         stored_networks[i].static_ip[2],
                         stored_networks[i].static_ip[3]);

            IPAddress netmask(stored_networks[i].netmask[0],
                              stored_networks[i].netmask[1],
                              stored_networks[i].netmask[2],
                              stored_networks[i].netmask[3]);

            IPAddress gateway(stored_networks[i].gateway[0],
                              stored_networks[i].gateway[1],
                              stored_networks[i].gateway[2],
                              stored_networks[i].gateway[3]);

            IPAddress dns(stored_networks[i].dns[0],
                          stored_networks[i].dns[1],
                          stored_networks[i].dns[2],
                          stored_networks[i].dns[3]);

            if (!WiFi.config(ip, gateway, netmask, dns)) {
                logMessage("WIFI: Static IP configuration failed, skipping");
                continue;
            }

            logMessagef("WIFI: Static IP: %s, Gateway: %s", ip.toString().c_str(), gateway.toString().c_str());
        } else {
            logMessage("WIFI: Using DHCP");
        }

        WiFi.begin(stored_networks[i].ssid, stored_networks[i].password);

        unsigned long connect_start = millis();
        int dots = 0;

        while (WiFi.status() != WL_CONNECTED && (millis() - connect_start) < 15000) {
            delay(500);
            dots++;
            if (dots % 2 == 0) {
                logMessage("WIFI: Connecting...");
            }
        }

        if (WiFi.status() == WL_CONNECTED) {
            current_connected_ssid = stored_ssid;
            logMessage("WIFI: ========================================");
            logMessagef("WIFI: ✓ CONNECTED to '%s'", stored_ssid.c_str());
            logMessagef("WIFI: IP Address: %s", WiFi.localIP().toString().c_str());
            logMessagef("WIFI: Subnet Mask: %s", WiFi.subnetMask().toString().c_str());
            logMessagef("WIFI: Gateway: %s", WiFi.gatewayIP().toString().c_str());
            logMessagef("WIFI: DNS: %s", WiFi.dnsIP().toString().c_str());
            logMessagef("WIFI: RSSI: %d dBm", WiFi.RSSI());
            logMessage("WIFI: ========================================");
            return true;
        } else {
            logMessagef("WIFI: ✗ Failed to connect to '%s' (timeout)", stored_ssid.c_str());
        }
    }

    logMessage("WIFI: No stored networks could be reached");
    return false;
}

void start_ap_mode() {
    device_state = STATE_WIFI_AP_MODE;
    ap_mode_active = true;

    if (ap_ssid == "") {
        ap_ssid = generate_ap_ssid();
        ap_password = generate_ap_password();
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());

    device_ip = WiFi.softAPIP();
    display_status();

    logMessage("WIFI: AP mode started");
    logMessage("WIFI: AP SSID: " + String(ap_ssid));
    logMessage("WIFI: AP password: " + ap_password + " (MAC-based)");
    logMessage("WIFI: AP IP address: " + device_ip.toString());
}

void check_wifi_connection() {
    if (device_state == STATE_WIFI_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            wifi_connected = true;
            device_ip = WiFi.localIP();
            device_state = STATE_IDLE;
            display_status();

            logMessage("WIFI: Connected successfully");
            logMessage("WIFI: IP address: " + device_ip.toString());

            if (boot_phase == BOOT_NETWORK_PENDING) {
                boot_phase = BOOT_NETWORK_READY;
                logMessage("BOOT: Phase -> BOOT_NETWORK_READY");
            }

            wifi_retry_silent = false;
        } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
            wifi_retry_count++;
            if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
                logMessage("WIFI: Authentication failed");
                device_state = STATE_IDLE;
                wifi_retry_silent = false;
            } else {
                if (!wifi_retry_silent) {
                    logMessagef("WIFI: Retry attempt %d", wifi_retry_count);
                }
                scan_and_connect_wifi();
                wifi_connect_start = millis();
            }
        } else if ((unsigned long)(millis() - wifi_connect_start) > WIFI_CONNECT_TIMEOUT) {
            wifi_retry_count++;
            if (wifi_retry_count >= WIFI_RETRY_ATTEMPTS) {
                logMessage("WIFI: Connection timeout");
                device_state = STATE_IDLE;
                wifi_retry_silent = false;
            } else {
                if (!wifi_retry_silent) {
                    logMessagef("WIFI: Timeout retry attempt %d", wifi_retry_count);
                }
                scan_and_connect_wifi();
                wifi_connect_start = millis();
            }
        }
    }
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
           " | <span style='color:var(--theme-nav-inactive);'>" + String(CURRENT_VERSION) + "</span>"
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
            "<a href='/chatgpt' class='tab-inactive" + String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            "<a href='/imap' class='tab-inactive" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
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
            "<a href='/chatgpt' class='tab-active" + String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            "<a href='/imap' class='tab-inactive" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
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
             "<span style='text-large'>Notify in case of Failure</span>"
             "<div class='toggle-switch " + String(chatgpt_config.notify_on_failure ? "is-active" : "is-inactive") + "' onclick='toggleChatGPTNotifications()'>"
             "<div class='toggle-slider " + String(chatgpt_config.notify_on_failure ? "is-active" : "is-inactive") + "'></div>"
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
            unsigned long elapsed = millis() - activity.timestamp;
            String timeAgo;
            if (elapsed < 60000) {
                timeAgo = String(elapsed / 1000) + "s ago";
            } else if (elapsed < 3600000) {
                timeAgo = String(elapsed / 60000) + "m ago";
            } else {
                timeAgo = String(elapsed / 3600000) + "h ago";
            }
            String queryStatus = activity.query_success ? "✅" : "❌";
            String transmissionStatus = activity.transmission_success ? "✅" : "❌";

            chunk = "<div style='border: 1px solid var(--theme-border); border-radius: 8px; padding: 12px; margin-bottom: 10px; background-color: var(--theme-input);'>"
                    "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;'>"
                    "<div style='font-weight: bold; color: var(--theme-text);'>" + queryStatus + " Query " + transmissionStatus + " Transmission</div>"
                    "<div style='font-size: 0.9em; color: var(--theme-nav-inactive);'>#" + String(activity.prompt_index) + " | " + String(activity.mail_drop ? "📧" : "📟") + " " + String(activity.capcode) + " | 📡 " + String(activity.frequency, 4) + " MHz | " + timeAgo + "</div>"
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
            "    body: newState ? 'notify_on_failure=1' : ''"
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

    bool notify_requested = webServer.hasArg("notify_on_failure");
    chatgpt_config.notify_on_failure = notify_requested;

    if (chatgpt_save_config()) {
        logMessage("CHATGPT: Failure notifications " + String(chatgpt_config.notify_on_failure ? "enabled" : "disabled"));
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
    uint32_t capcode = doc["capcode"];
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
    strlcpy(newPrompt.prompt, promptText.c_str(), sizeof(newPrompt.prompt));

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
    uint32_t capcode = doc["capcode"];
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
    strlcpy(editPrompt.prompt, promptText.c_str(), sizeof(editPrompt.prompt));

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
                        "<a href='/chatgpt' class='tab-inactive" + String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>"
                        "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
                        "<a href='/imap' class='tab-inactive" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
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
#if RTC_ENABLED
    timezone_section += "<div style='margin-top:15px;padding:15px;border:1px dashed var(--theme-border);border-radius:8px;background-color:var(--theme-input);color:var(--theme-text);'>";
    timezone_section += "<strong>RTC Module:</strong> ";
    timezone_section += String(rtc_available ? "✅ Detected" : "⚠️ Not detected");
    timezone_section += "<p style='margin:8px 0 0 0;font-size:0.9em;color:var(--theme-secondary);'>Hardware RTC presence is determined by the DS3231 wiring on the I²C bus. When detected, it seeds the system clock at boot and is refreshed automatically after WiFi (NTP) synchronization.</p>";
    timezone_section += "</div>";
#else
    timezone_section += "<div style='margin-top:15px;padding:15px;border:1px dashed var(--theme-border);border-radius:8px;background-color:var(--theme-input);color:var(--theme-text);'>"
                        "<strong>RTC Module:</strong> 🚫 Disabled at build time"
                        "<p style='margin:8px 0 0 0;font-size:0.9em;color:var(--theme-secondary);'>Recompile firmware with RTC support enabled to use an external DS3231 hardware clock.</p>"
                        "</div>";
#endif
    timezone_section += "</div>";
    webServer.sendContent(timezone_section);

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

    webServer.sendContent(get_html_footer());
    webServer.sendContent("");
}

String getReservedPinsJson() {
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
            "<a href='/chatgpt' class='tab-inactive" + String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            "<a href='/imap' class='tab-inactive" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
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
            "<input type='number' id='tx_power' name='tx_power' value='" + String((int)settings.default_txpower) + "' min='0' max='20' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<small style='color: var(--theme-secondary); display: block; margin-top: 5px;'>Range: 0 - 20 dBm</small>"
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

            "<div class='form-section' style='margin: 0; border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>"
            "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;'>"
            "<h4 style='margin: 0; color: var(--theme-text); display: flex; align-items: center; gap: 8px; font-size: 1.1em;'>📡 External RF Amplifier</h4>"
            "<div id='toggle_rf_enable' class='toggle-switch " + String(settings.enable_rf_amplifier ? "is-active" : "is-inactive") + "' onclick='toggleRFAmplifier()'>"
            "<div class='toggle-slider " + String(settings.enable_rf_amplifier ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "</div>"
            "<div style='margin-bottom: 16px;'>"
            "<label for='rf_amplifier_power_pin' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Power Pin (GPIO):</label>"
            "<input type='number' id='rf_amplifier_power_pin' name='rf_amplifier_power_pin' value='" + String((settings.rf_amplifier_power_pin == 0) ? RFAMP_PWR_PIN : settings.rf_amplifier_power_pin) + "' min='0' max='39' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div>"
            "<label for='rf_amplifier_delay_ms' style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Stabilization Delay (ms):</label>"
            "<input type='number' id='rf_amplifier_delay_ms' name='rf_amplifier_delay_ms' value='" + String(settings.rf_amplifier_delay_ms) + "' min='20' max='5000' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "</div>"
            "<div id='rf_amp_polarity_section' style='margin-top: 20px; margin-bottom: 16px;'>"
            "<label style='display: block; margin-bottom: 8px; font-weight: 500; color: var(--theme-text);'>Amplifier Control Logic:</label>"
            "<div style='display: flex; align-items: center; gap: 12px; margin-bottom: 8px;'>"
            "<span style='font-size: 14px; color: var(--theme-text); min-width: 85px; text-align: right;'>Active-Low</span>"
            "<div id='toggle_rf_polarity' class='toggle-switch " + String(settings.rf_amplifier_active_high ? "is-active" : "is-inactive") + "' onclick='toggleRFAmpPolarity()'>"
            "<div class='toggle-slider " + String(settings.rf_amplifier_active_high ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "<span style='font-size: 14px; color: var(--theme-text); min-width: 85px;'>Active-High</span>"
            "</div>"
            "<div style='font-size: 12px; margin: 8px 0 0 0; padding: 10px; background: var(--theme-card); border-radius: 4px; border-left: 3px solid #28a745;'>"
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
            "<a href='/chatgpt' class='tab-inactive" + String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-active" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            "<a href='/imap' class='tab-inactive" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
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
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<form action='/save_mqtt' method='post' onsubmit='return submitFormAjax(this, \"MQTT settings saved successfully!\", \"MQTT settings saved, restarting in 5 seconds...\")'>"

            "<input type='hidden' id='mqtt_enabled' name='mqtt_enabled' value='" + String(settings.mqtt_enabled ? "1" : "0") + "'>"

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
            "<div style='margin-bottom: 20px;'>"
            "<label for='mqtt_boot_delay' style='display:block;margin-bottom:8px;font-weight:500;color:var(--theme-text);'>Boot Delay Before MQTT (seconds):</label>"
            "<input type='number' id='mqtt_boot_delay' name='mqtt_boot_delay' value='" + String(settings.mqtt_boot_delay_ms / 1000UL) + "' min='0' max='600' style='width:100%;padding:12px 16px;border:2px solid var(--theme-border);border-radius:8px;font-size:16px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);transition:all 0.3s ease;'>"
            "<p style='margin:6px 0 0 0;font-size:14px;color:var(--theme-secondary);'>Set to 0 for immediate MQTT initialization after boot.</p>"
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

    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

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
            "<a href='/chatgpt' class='tab-inactive" + String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            "<a href='/imap' class='tab-active" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
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
            "<a href='/chatgpt' class='tab-inactive'" + String(chatgpt_config.enabled ? " style='color:#28a745;'" : "") + ">🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-inactive'" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " style='color:#dc3545;'" : (settings.mqtt_enabled ? " style='color:#28a745;'" : "")) + ">📡 MQTT</a>"
            "<a href='/imap' class='tab-inactive'" + String(any_imap_accounts_suspended() ? " style='color:#dc3545;'" : (imap_config.enabled ? " style='color:#28a745;'" : "")) + ">📧 IMAP</a>"
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

    if (save_settings()) {
        webServer.send(200, "application/json", "{\"success\":true,\"message\":\"API settings saved successfully!\"}");
        logMessage("CONFIG: API settings saved - HTTP:" + String(http_port));

        delay(1000);
        ESP.restart();
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save configuration\"}");
    }
}

void handle_grafana() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("Grafana Integration");

    chunk += "<div class='header'>"
            "<h1>🚨 Grafana Integration</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive'" + String(settings.api_enabled ? " style='color:#28a745;'" : "") + ">🔗 API</a>"
            "<a href='/grafana' class='tab-active'" + String(settings.grafana_enabled ? " style='color:#28a745;'" : "") + ">🚨 Grafana</a>"
            "<a href='/chatgpt' class='tab-inactive'" + String(chatgpt_config.enabled ? " style='color:#28a745;'" : "") + ">🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-inactive'" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " style='color:#dc3545;'" : (settings.mqtt_enabled ? " style='color:#28a745;'" : "")) + ">📡 MQTT</a>"
            "<a href='/imap' class='tab-inactive'" + String(any_imap_accounts_suspended() ? " style='color:#dc3545;'" : (imap_config.enabled ? " style='color:#28a745;'" : "")) + ">📧 IMAP</a>"
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div id='temp-message'></div>";

    chunk += "<div class='form-section'>"
            "<div class='flex-space-between mb-20'>"
            "<div class='flex-center'>"
            "<span style='text-large'>Enable Grafana</span>"
            "<div class='toggle-switch " + String(settings.grafana_enabled ? "is-active" : "is-inactive") + "' onclick='toggleGrafana()'>"
            "<div class='toggle-slider " + String(settings.grafana_enabled ? "is-active" : "is-inactive") + "'></div>"
            "</div>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='padding:20px;'>";

    chunk += "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;margin:20px 0;border-left:5px solid #ff6b35;'>"
            "<h3 style='margin:0 0 15px 0;color:#ff6b35;'>📡 Webhook Endpoint</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Configure Grafana to send alert notifications to this device:</p>"
            "<div style='background-color:var(--theme-input);padding:15px;border-radius:8px;border:2px solid var(--theme-border);'>"
            "<code style='font-size:16px;font-weight:bold;color:var(--theme-text);word-break:break-all;'>"
            "http://" + WiFi.localIP().toString() + "/api/v1/alerts"
            "</code>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>⚙️ Grafana Alertmanager Configuration</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Add this webhook configuration to your alertmanager.yml:</p>"
            "<textarea readonly style='width:100%;height:302px;padding:15px;border:2px solid var(--theme-border);border-radius:8px;font-family:monospace;font-size:13px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:vertical;'>"
            "# alertmanager.yml\n"
            "route:\n"
            "  group_by: ['alertname']\n"
            "  group_wait: 10s\n"
            "  group_interval: 10s\n"
            "  repeat_interval: 1h\n"
            "  receiver: 'flex-pager'\n\n"
            "receivers:\n"
            "- name: 'flex-pager'\n"
            "  webhook_configs:\n"
            "  - url: 'http://" + WiFi.localIP().toString() + "/api/v1/alerts'\n"
            "    http_config:\n"
            "      basic_auth:\n"
            "        username: '" + String(settings.api_username) + "'\n"
            "        password: '" + String(settings.api_password) + "'"
            "</textarea>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>🏷️ Alert Field Mapping</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Configure alert labels and annotations to control paging behavior:</p>"

            "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:20px 0;'>"
            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;border-left:4px solid #28a745;'>"
            "<h4 style='margin:0 0 15px 0;color:#28a745;'>📋 Labels (Paging Parameters)</h4>"
            "<div style='font-family:monospace;font-size:12px;line-height:1.6;'>"
            "<div><strong>capcode</strong> → Target pager capcode</div>"
            "<div><strong>pager_capcode</strong> → Alternative capcode field</div><br>"
            "<div><strong>frequency</strong> → TX frequency (MHz/Hz)</div>"
            "<div><strong>pager_frequency</strong> → Alternative frequency field</div><br>"
            "<div><strong>mail_drop</strong> → Enable mail drop flag</div>"
            "<div><strong>pager_mail_drop</strong> → Alternative mail drop field</div><br>"
            "<div style='color:var(--theme-secondary);font-size:11px;font-style:italic;'>"
            "All fields are optional. Missing values use FLEX tab defaults.</div>"
            "</div>"
            "</div>"

            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;border-left:4px solid #007bff;'>"
            "<h4 style='margin:0 0 15px 0;color:#007bff;'>💬 Annotations (Message Content)</h4>"
            "<div style='font-family:monospace;font-size:12px;line-height:1.6;'>"
            "<div><strong>1. summary</strong> → Primary message content</div>"
            "<div><strong>2. description</strong> → Secondary content</div>"
            "<div><strong>3. message</strong> → Tertiary content</div>"
            "<div><strong>4. \"Alert triggered\"</strong> → Default fallback</div><br>"
            "<div style='color:var(--theme-secondary);font-size:11px;'>Priority order: 1 → 2 → 3 → 4</div>"
            "</div>"
            "</div>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>📝 Message Format</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Alert messages are automatically formatted with status and content:</p>"
            "<div style='background-color:var(--theme-input);padding:15px;border-radius:8px;border:2px solid var(--theme-border);font-family:monospace;'>"
            "<div style='color:#dc3545;'>[FIRING] AlertName: Message content</div>"
            "<div style='color:#28a745;'>[RESOLVED] AlertName: Message content</div>"
            "</div>"
            "<p style='font-size:12px;color:var(--theme-secondary);margin:10px 0;'><em>Status determined by alert 'endsAt' field: ongoing alerts show FIRING, resolved alerts show RESOLVED.</em></p>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>🧪 Test Webhook</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Test the Grafana webhook endpoint with this curl command:</p>"
            "<textarea readonly style='width:100%;height:272px;padding:15px;border:2px solid var(--theme-border);border-radius:8px;font-family:monospace;font-size:12px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:vertical;'>"
            "curl -X POST \\\n"
            "     -u " + String(settings.api_username) + ":" + String(settings.api_password) + " \\\n"
            "     -H \"Content-Type: application/json\" \\\n"
            "     http://" + WiFi.localIP().toString() + "/api/v1/alerts \\\n"
            "     -d '[{\n"
            "       \"labels\": {\n"
            "         \"alertname\": \"TestAlert\",\n"
            "         \"capcode\": \"1234567\",\n"
            "         \"frequency\": \"931937500\"\n"
            "       },\n"
            "       \"annotations\": {\n"
            "         \"summary\": \"Test notification from curl\"\n"
            "       },\n"
            "       \"endsAt\": \"0001-01-01T00:00:00Z\"\n"
            "     }]'"
            "</textarea>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>📨 Webhook Response</h3>"
            "<p style='font-size:14px;color:var(--theme-secondary);margin:10px 0;'>Multi-alert processing response with detailed status:</p>"
            "<textarea readonly style='width:100%;height:269px;padding:15px;border:2px solid var(--theme-border);border-radius:8px;font-family:monospace;font-size:12px;box-sizing:border-box;background-color:var(--theme-input);color:var(--theme-text);resize:vertical;'>"
            "{\n"
            "  \"status\": \"completed\",\n"
            "  \"total_alerts\": 1,\n"
            "  \"successful\": 1,\n"
            "  \"failed\": 0,\n"
            "  \"results\": [{\n"
            "    \"alert_index\": 1,\n"
            "    \"alert_name\": \"TestAlert\",\n"
            "    \"capcode\": 1234567,\n"
            "    \"frequency\": 931.9375,\n"
            "    \"message\": \"[FIRING] TestAlert: Test notification\",\n"
            "    \"truncated\": false,\n"
            "    \"success\": true\n"
            "  }]\n"
            "}"
            "</textarea>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<h3>🔧 Integration Notes</h3>"
            "<div style='background-color:var(--theme-card);padding:20px;border-radius:12px;margin:20px 0;'>"
            "<ul style='margin:0;padding-left:20px;line-height:1.8;'>"
            "<li><strong>Queue System:</strong> Grafana alerts use the same message queue as other API calls</li>"
            "<li><strong>EMR Support:</strong> Messages benefit from Emergency Message Resynchronization</li>"
            "<li><strong>Multi-Alert:</strong> Single webhook call can process multiple alerts</li>"
            "<li><strong>Auto-Truncation:</strong> Long messages automatically truncated to 248 characters</li>"
            "<li><strong>FLEX Tab Defaults:</strong> Missing fields (capcode, frequency, mail_drop) use FLEX tab configuration</li>"
            "<li><strong>Authentication:</strong> Same HTTP Basic Auth as standard API endpoint</li>"
            "</ul>"
            "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<script>"
            "function toggleGrafana() {"
            "  const toggleSwitch = document.querySelector('.toggle-switch');"
            "  const toggleSlider = document.querySelector('.toggle-slider');"
            "  "
            "  const currentEnabled = toggleSwitch.style.backgroundColor === 'rgb(40, 167, 69)';"
            "  const newEnabled = !currentEnabled;"
            "  "
            "  if (newEnabled) {"
            "    toggleSwitch.style.backgroundColor = '#28a745';"
            "    toggleSlider.style.left = '26px';"
            "  } else {"
            "    toggleSwitch.style.backgroundColor = '#ccc';"
            "    toggleSlider.style.left = '2px';"
            "  }"
            "  "
            "  fetch('/grafana_toggle', {"
            "    method: 'POST',"
            "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
            "    body: 'grafana_enabled=' + (newEnabled ? '1' : '0')"
            "  })"
            "  .then(response => response.text())"
            "  .then(data => {"
            "    console.log('Grafana toggle response:', data);"
            "  })"
            "  .catch(error => {"
            "    console.error('Error toggling Grafana:', error);"
            "  });"
            "}"
            "</script>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_grafana_toggle() {
    reset_oled_timeout();

    if (webServer.hasArg("grafana_enabled")) {
        settings.grafana_enabled = (webServer.arg("grafana_enabled") == "1");

        if (save_settings()) {
            webServer.send(200, "text/plain", "OK");
            logMessage("CONFIG: Grafana toggled - Enabled:" + String(settings.grafana_enabled ? "true" : "false"));
        } else {
            webServer.send(500, "text/plain", "Error saving configuration");
        }
    } else {
        webServer.send(400, "text/plain", "Missing parameter");
    }
}

void handle_logs() {
    reset_oled_timeout();

    String response = "{\"logs\":[";

    for (int i = 0; i < serial_log_count && i < 20; i++) {
        int index = (serial_log_index - 1 - i + SERIAL_LOG_SIZE) % SERIAL_LOG_SIZE;
        if (index < 0) index += SERIAL_LOG_SIZE;

        if (i > 0) response += ",";
        response += "{";
        response += "\"message\":\"" + String(serial_log[index].message) + "\",";
        response += "\"type\":\"info\",";
        response += "\"timestamp\":" + String(serial_log[index].timestamp);
        response += "}";
    }

    response += "]}";

    webServer.send(200, "application/json", response);
}

void handle_device_status() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("Device Status");

    chunk += "<div class='header'>"
            "<h1>📊 Device Status</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            "<a href='/chatgpt' class='tab-inactive" + String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            "<a href='/imap' class='tab-inactive" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
            "<a href='/status' class='tab-active'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card); margin-bottom: 20px;'>";
    chunk += "<h2 style='margin-top:0; margin-bottom:25px; border-bottom: 2px solid var(--theme-border); padding-bottom:10px;'>📊 System Status</h2>";

    chunk += "<div style='display:flex;gap:20px;flex-wrap:wrap;'>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📡 Device Information</h3>";
    unsigned long uptimeSeconds = millis() / 1000;
    unsigned long days = uptimeSeconds / 86400;
    unsigned long hours = (uptimeSeconds % 86400) / 3600;
    unsigned long mins = (uptimeSeconds % 3600) / 60;
    String uptime = "";
    if (days > 0) uptime += String(days) + " days, ";
    if (hours > 0 || days > 0) uptime += String(hours) + " hours, ";
    uptime += String(mins) + " mins";
    chunk += "<p><strong>Uptime:</strong> " + uptime + "</p>";
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint8_t heapPercent = (freeHeap * 100) / totalHeap;
    chunk += "<p><strong>Free Heap:</strong> " + String(freeHeap) + " bytes (" + String(heapPercent) + "%)</p>";
    String theme_name = "";
    switch(settings.theme) {
        case 0: theme_name = "🌞 Minimal White"; break;
        case 1: theme_name = "🌙 Carbon Black"; break;
        default: theme_name = "Unknown"; break;
    }
    chunk += "<p><strong>Theme:</strong> " + theme_name + "</p>";
    String wifi_status = "Enabled";
    chunk += "<p><strong>WiFi Status:</strong> " + wifi_status + "</p>";
    chunk += "<p><strong>Chip Model:</strong> " + String(ESP.getChipModel()) + "</p>";
    chunk += "<p><strong>CPU Frequency:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>";
    chunk += "</div>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📻 FLEX Configuration</h3>";
    chunk += "<p><strong>Banner:</strong> " + String(settings.banner_message) + "</p>";
    chunk += "<p><strong>Frequency:</strong> " + String(current_tx_frequency, 4) + " MHz</p>";
    chunk += "<p><strong>TX Power:</strong> " + String(tx_power, 1) + " dBm</p>";
    chunk += "<p><strong>Default Capcode:</strong> " + String(settings.default_capcode) + "</p>";
    chunk += "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='display:flex;gap:20px;flex-wrap:wrap;margin-top:20px;'>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📶 Network Information</h3>";

    if (wifi_connected) {
        chunk += "<p><strong>WiFi SSID:</strong> " + WiFi.SSID() + "</p>";
        chunk += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
        chunk += "<p><strong>Subnet Mask:</strong> " + WiFi.subnetMask().toString() + "</p>";
        chunk += "<p><strong>Gateway:</strong> " + WiFi.gatewayIP().toString() + "</p>";
        chunk += "<p><strong>DNS Server:</strong> " + WiFi.dnsIP().toString() + "</p>";
        chunk += "<p><strong>MAC Address:</strong> " + WiFi.macAddress() + "</p>";
        chunk += "<p><strong>RSSI:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
        chunk += "<p><strong>DHCP:</strong> DHCP</p>";
    } else if (ap_mode_active) {
        chunk += "<p><strong>Mode:</strong> Access Point (Configuration Mode)</p>";
        chunk += "<p><strong>AP SSID:</strong> " + ap_ssid + "</p>";
        chunk += "<p><strong>AP IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
        chunk += "<p><strong>AP MAC:</strong> " + WiFi.softAPmacAddress() + "</p>";
        chunk += "<p><strong>Connected Clients:</strong> " + String(WiFi.softAPgetStationNum()) + "</p>";
    } else {
        chunk += "<p><strong>Status:</strong> WiFi Disabled</p>";
        chunk += "<p><strong>MAC Address:</strong> " + WiFi.macAddress() + "</p>";
    }
    chunk += "</div>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>🔋 Battery Status</h3>";

    uint16_t battery_voltage_mv_status;
    int battery_percentage_status;
    getBatteryInfo(&battery_voltage_mv_status, &battery_percentage_status);

    int adc_raw = analogRead(BATTERY_ADC_PIN);
    float battery_voltage_status = battery_voltage_mv_status / 1000.0;
    bool is_connected = (battery_voltage_status > 4.17);
    bool is_actively_charging = (battery_voltage_status > 4.20);

    if (battery_present) {
        chunk += "<p><strong>Battery Present:</strong> ✅ Yes</p>";
        chunk += "<p><strong>Voltage:</strong> " + String(battery_voltage_status, 3) + "V (" + String(battery_voltage_mv_status) + " mV)</p>";
        chunk += "<p><strong>Percentage:</strong> " + String(battery_percentage_status) + "%</p>";
        chunk += "<p><strong>ADC Raw Value:</strong> " + String(adc_raw) + " (0-4095)</p>";

        String power_status = is_connected ? "<span style='color:#28a745;'>🔌 Connected</span>" : "<span style='color:#ffc107;'>🔋 On Battery</span>";
        chunk += "<p><strong>Power Status:</strong> " + power_status + "</p>";

        String charging_status = is_actively_charging ? "<span style='color:#17a2b8;'>⚡ Yes</span>" : "<span style='color:#6c757d;'>○ No</span>";
        chunk += "<p><strong>Charging:</strong> " + charging_status + "</p>";

        chunk += "<p><strong>Check Interval:</strong> 60 seconds</p>";

        if (settings.enable_low_battery_alert) {
            chunk += "<p><strong>Low Battery Alert:</strong> ✅ Enabled (≤10%)</p>";
        } else {
            chunk += "<p><strong>Low Battery Alert:</strong> ❌ Disabled</p>";
        }

        if (settings.enable_power_disconnect_alert) {
            chunk += "<p><strong>Power Disconnect Alert:</strong> ✅ Enabled</p>";
        } else {
            chunk += "<p><strong>Power Disconnect Alert:</strong> ❌ Disabled</p>";
        }
    } else {
        chunk += "<p><strong>Battery Present:</strong> ❌ No</p>";
        chunk += "<p><strong>Voltage:</strong> " + String(battery_voltage_status, 3) + "V (below 2.5V threshold)</p>";
        chunk += "<p><strong>ADC Raw Value:</strong> " + String(adc_raw) + " (0-4095)</p>";
    }

    chunk += "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='display:flex;gap:20px;flex-wrap:wrap;margin-top:20px;'>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>🕐 Time Synchronization</h3>";

    time_t now;
    time(&now);
    time_t local_time = getLocalTimestamp();
    chunk += "<p><strong>Current Time:</strong> " + String(ctime(&local_time)) + "</p>";
    chunk += "<p><strong>Unix Timestamp:</strong> " + String((long)now) + "</p>";
    chunk += "<p><strong>NTP Synchronized:</strong> " + String(ntp_synced ? "✅ Yes" : "❌ No") + "</p>";

    if (last_ntp_sync > 0) {
        unsigned long time_since_sync = (millis() - last_ntp_sync) / 1000;
        unsigned long minutes_since = time_since_sync / 60;
        unsigned long hours_since = minutes_since / 60;

        String last_sync_str;
        if (hours_since > 0) {
            last_sync_str = String(hours_since) + "h " + String(minutes_since % 60) + "m ago";
        } else {
            last_sync_str = String(minutes_since) + "m " + String(time_since_sync % 60) + "s ago";
        }
        chunk += "<p><strong>Last NTP Sync:</strong> " + last_sync_str + "</p>";

        unsigned long next_sync = NTP_SYNC_INTERVAL_MS - (millis() - last_ntp_sync);
        if (next_sync > NTP_SYNC_INTERVAL_MS) next_sync = 0;
        unsigned long next_minutes = next_sync / 60000;
        chunk += "<p><strong>Next Sync In:</strong> " + String(next_minutes) + " minutes</p>";
    } else {
        chunk += "<p><strong>Last NTP Sync:</strong> Never</p>";
        chunk += "<p><strong>Next Sync In:</strong> On WiFi connection</p>";
    }

    if (now > 1600000000) {
        chunk += "<p><strong>Time Quality:</strong> ✅ Good (SSL ready)</p>";
    } else {
        chunk += "<p><strong>Time Quality:</strong> ❌ Poor (SSL may fail)</p>";
    }

    chunk += "</div>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>🖥️ Remote Logging</h3>";

    if (!settings.rsyslog_enabled) {
        chunk += "<p><strong>Status:</strong> ❌ Disabled</p>";
    } else if (strlen(settings.rsyslog_server) == 0) {
        chunk += "<p><strong>Status:</strong> ⚠️ Not Configured</p>";
    } else {
        chunk += "<p><strong>Status:</strong> ✅ Enabled</p>";
        chunk += "<p><strong>Server:</strong> " + String(settings.rsyslog_server) + ":" + String(settings.rsyslog_port) + "</p>";
        chunk += "<p><strong>Protocol:</strong> " + String(settings.rsyslog_use_tcp ? "TCP" : "UDP") + "</p>";
        String severity_name;
        switch (settings.rsyslog_min_severity) {
            case 0: severity_name = "Emergency"; break;
            case 1: severity_name = "Alert"; break;
            case 2: severity_name = "Critical"; break;
            case 3: severity_name = "Error"; break;
            case 4: severity_name = "Warning"; break;
            case 5: severity_name = "Notice"; break;
            case 6: severity_name = "Informational"; break;
            case 7: severity_name = "Debug"; break;
            default: severity_name = "Unknown"; break;
        }
        chunk += "<p><strong>Min Severity:</strong> " + severity_name + " (" + String(settings.rsyslog_min_severity) + ")</p>";
        chunk += "<p><strong>Hostname:</strong> " + String(settings.banner_message) + "</p>";
        chunk += "<p><strong>Facility:</strong> local0 (16)</p>";
    }
    chunk += "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='display:flex;gap:20px;flex-wrap:wrap;margin-top:20px;'>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📡 MQTT Status</h3>";

    if (!settings.mqtt_enabled) {
        chunk += "<p><strong>Status:</strong> ❌ Disabled</p>";
    } else if (strlen(settings.mqtt_server) == 0) {
        chunk += "<p><strong>Status:</strong> ⚠️ Not Configured</p>";
    } else {
        String status;
        if (mqtt_suspended) {
            status = "<span style='color:#dc3545;'>🚫 Suspended</span>";
        } else if (mqttClient.connected()) {
            status = "<span style='color:#28a745;'>✅ Connected</span>";
        } else {
            status = "<span style='color:#dc3545;'>❌ Disconnected</span>";
        }
        chunk += "<p><strong>Status:</strong> " + status + "</p>";
        chunk += "<p><strong>Server:</strong> " + String(settings.mqtt_server) + ":" + String(settings.mqtt_port) + "</p>";
        chunk += "<p><strong>Thing Name:</strong> " + String(settings.mqtt_thing_name) + "</p>";
        chunk += "<p><strong>Subscribe Topic:</strong> " + String(settings.mqtt_subscribe_topic) + "</p>";
        chunk += "<p><strong>Publish Topic:</strong> " + String(settings.mqtt_publish_topic) + "</p>";
        chunk += "<p><strong>Initialized:</strong> " + String(mqtt_initialized ? "✅ Yes" : "❌ No") + "</p>";

        bool has_certs = (certificateExistsInSPIFFS(MQTT_CA_CERT_FILE) && certificateExistsInSPIFFS(MQTT_DEVICE_CERT_FILE) && certificateExistsInSPIFFS(MQTT_DEVICE_KEY_FILE));
        chunk += "<p><strong>SSL Certificates:</strong> " + String(has_certs ? "✅ Configured" : "⚠️ Using Insecure Connection") + "</p>";
    }
    chunk += "</div>";

    chunk += "<div style='flex:1;min-width:350px;'>";
    chunk += "<h3 style='margin-top:0;'>📧 IMAP Status</h3>";

    if (!imap_config.enabled) {
        chunk += "<p><strong>Status:</strong> ❌ IMAP System Disabled</p>";
    } else if (imap_config.account_count == 0) {
        chunk += "<p><strong>Status:</strong> ⚠️ No Accounts Configured</p>";
    } else {
        String suspended_accounts = "";
        int suspended_count = 0;

        for (size_t i = 0; i < imap_config.accounts.size(); i++) {
            if (imap_config.accounts[i].suspended) {
                if (suspended_count > 0) suspended_accounts += ", ";
                suspended_accounts += String(imap_config.accounts[i].name);
                suspended_count++;
            }
        }

        String status;
        if (suspended_count > 0) {
            status = "<span style='color:#dc3545;'>⚠️ " + String(imap_config.account_count - suspended_count) + "/" + String(imap_config.account_count) + " Active</span>";
        } else {
            status = "<span style='color:#28a745;'>✅ Active</span>";
        }
        chunk += "<p><strong>Status:</strong> " + status + "</p>";
        chunk += "<p><strong>Accounts:</strong> " + String(imap_config.account_count) + "</p>";

        if (suspended_count > 0) {
            chunk += "<div style='background-color:#fff3cd;border:1px solid #ffeaa7;padding:10px;border-radius:5px;margin:10px 0;'>";
            chunk += "<strong style='color:#856404;'>🚫 Suspended Accounts:</strong> " + suspended_accounts;
            chunk += "</div>";
        }
        chunk += "<p><strong>Alert Mode:</strong> 📢 All Unread Emails</p>";

        if (queue_count > 0) {
            chunk += "<p><strong>Queue:</strong> " + String(queue_count) + " message(s) pending</p>";
        }

        if (last_imap_check > 0) {
            unsigned long time_since_check = (millis() - last_imap_check) / 1000;
            unsigned long minutes_since = time_since_check / 60;
            chunk += "<p><strong>Last Global Check:</strong> " + String(minutes_since) + " minutes ago</p>";
        }

        chunk += "<div style='margin-top:15px;'>";
        chunk += "<h4 style='margin:10px 0 8px 0;color:var(--theme-text);'>Account Details:</h4>";

        webServer.sendContent(chunk);
        chunk = "";

        for (size_t i = 0; i < imap_config.accounts.size(); i++) {
            const IMAPAccount& account = imap_config.accounts[i];
            String ssl_icon = account.use_ssl ? "🔒" : "🔓";
            String account_status;
            String status_color;

            if (account.suspended) {
                account_status = "🚫 Suspended";
                status_color = "#dc3545";
            } else {
                account_status = "✅ Active";
                status_color = "#28a745";
            }

            chunk += "<div style='background-color:var(--theme-card);border:1px solid var(--theme-border);border-radius:6px;padding:12px;margin:5px 0;'>";
            chunk += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
            chunk += "<div>";
            chunk += "<strong>" + ssl_icon + " " + String(account.name) + "</strong>";
            chunk += "<div style='font-size:0.9em;color:var(--theme-secondary);margin-top:2px;'>";

            if (account.last_check > 0) {
                unsigned long account_time_since = (millis() - account.last_check) / 1000;
                unsigned long account_minutes = account_time_since / 60;
                chunk += "Last: " + String(account_minutes) + "m ago";
            } else {
                chunk += "Last: Never";
            }

            if (account.suspended && account.failed_check_cycles > 0) {
                chunk += " | Failures: " + String(account.failed_check_cycles);
            }

            chunk += "</div>";
            chunk += "</div>";
            chunk += "<span style='color:" + status_color + ";font-weight:500;'>" + account_status + "</span>";
            chunk += "</div>";
            chunk += "</div>";
            webServer.sendContent(chunk);
            chunk = "";
        }
        chunk += "</div>";
    }
    chunk += "</div>";
    chunk += "</div>";

    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card); margin-bottom: 20px;'>";
    chunk += "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;'>";
    chunk += "<h3 style='margin: 0;'>📡 Recent Serial Messages</h3>";
    chunk += "<div style='display: flex; align-items: center; gap: 8px;'>";
    chunk += "<div class='toggle-switch' id='live-logs-toggle' onclick='toggleLiveLogs()' style='background-color: #ccc;'>";
    chunk += "<div class='toggle-slider' style='left: 2px;'></div>";
    chunk += "</div>";
    chunk += "<span style='font-size: 0.9em; color: var(--theme-text);'>Live Logs</span>";
    chunk += "</div>";
    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    if (serial_log_count == 0) {
        chunk += "<p>No serial messages logged yet.</p>";
    } else {
        chunk += "<div class='serial-log'>";
        webServer.sendContent(chunk);
        chunk = "";

        for (int i = 0; i < serial_log_count; i++) {
            int index = (serial_log_index - 1 - i + SERIAL_LOG_SIZE) % SERIAL_LOG_SIZE;
            if (index < 0) index += SERIAL_LOG_SIZE;

            time_t timestamp = serial_log[index].timestamp + (settings.timezone_offset_hours * 3600);
            struct tm *timeinfo = localtime(&timestamp);
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);

            chunk += "<div>";
            chunk += "<span class='timestamp'>[" + String(timeStr) + "]</span> ";
            chunk += String(serial_log[index].message);
            chunk += "</div>";

            if ((i + 1) % 10 == 0) {
                webServer.sendContent(chunk);
                chunk = "";
            }
        }
        if (chunk.length() > 0) {
            webServer.sendContent(chunk);
            chunk = "";
        }
        chunk += "</div>";
    }
    chunk += "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div style='border: 2px solid var(--theme-border); border-radius: 8px; padding: 20px; background-color: var(--theme-card);'>";
    chunk += "<h3 style='margin-top:0;'>⚙️ Device Management</h3>";
    chunk += "<p>Backup, restore, or reset device configuration:</p>";
    chunk += "<div style='display:flex;gap:8px;flex-wrap:wrap;'>";

    chunk += "<form action='/backup_settings' method='get' style='margin:0;'>";
    chunk += "<button type='submit' style='padding:6px 10px;font-size:0.8em;background-color:#28a745;color:white;border-radius:4px;border:none;cursor:pointer;'>💾 Backup</button>";
    chunk += "</form>";

    chunk += "<form action='/restore_settings' method='get' style='margin:0;'>";
    chunk += "<button type='submit' style='padding:6px 10px;font-size:0.8em;background-color:#fd7e14;color:white;border-radius:4px;border:none;cursor:pointer;'>📁 Restore</button>";
    chunk += "</form>";

    chunk += "<form action='/factory_reset' method='post' onsubmit='return confirm(\"Are you sure you want to reset to factory defaults? This will clear all configuration and restart the device.\")' style='margin:0;'>";
    chunk += "<button type='submit' style='padding:6px 10px;font-size:0.8em;background-color:#dc3545;color:white;border-radius:4px;border:none;cursor:pointer;'>🔄 Reset</button>";
    chunk += "</form>";

    chunk += "</div>";
    chunk += "<div style='margin-top:10px;font-size:0.9em;color:#666;'>";
    chunk += "<strong>Backup:</strong> Download all settings to JSON file<br>";
    chunk += "<strong>Restore:</strong> Upload and apply settings from backup file<br>";
    chunk += "<strong>Factory Reset:</strong> Clear all settings and return to defaults";
    chunk += "</div>";
    chunk += "</div>";
    chunk += "<script>"
           "let lastLogTimestamp = 0;"
           "let liveLogsEnabled = false;"
           "let liveLogsInterval = null;"
           "function pollLogs() {"
           "  fetch('/logs')"
           "    .then(response => response.json())"
           "    .then(data => {"
           "      if (data.logs && data.logs.length > 0) {"
           "        const container = document.querySelector('.serial-log');"
           "        if (!container) return;"
           "        let newLogs = [];"
           "        data.logs.forEach(log => {"
           "          if (log.timestamp > lastLogTimestamp) {"
           "            newLogs.push(log);"
           "            lastLogTimestamp = Math.max(lastLogTimestamp, log.timestamp);"
           "          }"
           "        });"
           "        newLogs.reverse().forEach(log => {"
           "          const logDiv = document.createElement('div');"
           "          const date = new Date(log.timestamp * 1000);"
           "          const timeStr = date.toLocaleTimeString('en-US', {hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit'});"
           "          logDiv.innerHTML = \"<span class='timestamp'>[\" + timeStr + \"]</span> \" + log.message;"
           "          container.insertBefore(logDiv, container.firstChild);"
           "        });"
           "      }"
           "    })"
           "    .catch(() => {});"
           "}"
           "function toggleLiveLogs() {"
           "  liveLogsEnabled = !liveLogsEnabled;"
           "  const toggle = document.getElementById('live-logs-toggle');"
           "  const slider = toggle.querySelector('.toggle-slider');"
           "  if (liveLogsEnabled) {"
           "    toggle.style.backgroundColor = '#28a745';"
           "    slider.style.left = '26px';"
           "    liveLogsInterval = setInterval(pollLogs, 2000);"
           "  } else {"
           "    toggle.style.backgroundColor = '#ccc';"
           "    slider.style.left = '2px';"
           "    if (liveLogsInterval) {"
           "      clearInterval(liveLogsInterval);"
           "      liveLogsInterval = null;"
           "    }"
           "  }"
           "}"
           "pollLogs();"
           "</script>";

    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_web_factory_reset() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("Factory Reset");
    chunk += "<div class='header'>"
            "<h1>🔄 Factory Reset</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            "<a href='/chatgpt' class='tab-inactive" + String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            "<a href='/imap' class='tab-inactive" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div class='status success'>✅ Factory reset completed! Device will restart in 3 seconds...</div>";
    chunk += "<script>setTimeout(function() { window.location.href = '/'; }, 3000);</script>";
    chunk += get_html_footer();

    webServer.sendContent(chunk);
    webServer.sendContent("");

    delay(3000);
    perform_factory_reset();
}

void handle_backup_settings() {
    reset_oled_timeout();

    String json_backup = config_to_json();

    char filename[80];
    sprintf(filename, "flex-settings-backup-%s-%lu.json", CURRENT_VERSION, millis());

    webServer.sendHeader("Content-Disposition", "attachment; filename=\"" + String(filename) + "\"");
    webServer.sendHeader("Content-Type", "application/json");
    webServer.send(200, "application/json", json_backup);

    logMessage("BACKUP: Settings exported to JSON file");
}

void handle_restore_settings() {
    reset_oled_timeout();

    webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    webServer.send(200, "text/html; charset=utf-8", "");

    String chunk = get_html_header("Restore Settings");
    chunk += "<div class='header'>"
            "<h1>📁 Restore Settings</h1>"
            "</div>";

    chunk += "<div class='nav'>"
            "<a href='/' class='tab-inactive'>📡 Message</a>"
            "<a href='/config' class='tab-inactive'>⚙️ Config</a>"
            "<a href='/flex' class='tab-inactive'>📻 FLEX</a>"
            "<a href='/api_config' class='tab-inactive" + String(settings.api_enabled ? " nav-status-enabled" : "") + "'>🔗 API</a>"
            "<a href='/grafana' class='tab-inactive" + String(settings.grafana_enabled ? " nav-status-enabled" : "") + "'>🚨 Grafana</a>"
            "<a href='/chatgpt' class='tab-inactive" + String(chatgpt_config.enabled ? " nav-status-enabled" : "") + "'>🤖 ChatGPT</a>"
            "<a href='/mqtt' class='tab-inactive" + String(mqtt_suspended || (settings.mqtt_enabled && !mqttClient.connected()) ? " nav-status-disabled" : (settings.mqtt_enabled ? " nav-status-enabled" : "")) + "'>📡 MQTT</a>"
            "<a href='/imap' class='tab-inactive" + String(any_imap_accounts_suspended() ? " nav-status-disabled" : (imap_config.enabled ? " nav-status-enabled" : "")) + "'>📧 IMAP</a>"
            "<a href='/status' class='tab-inactive'>📊 Status</a>"
            "</div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<div class='container'>";
    chunk += "<div class='status warning'>⚠️ <strong>Warning:</strong> Restoring settings will completely override your current configuration. This action cannot be undone.</div>";

    chunk += "<h3>📁 Upload Backup File</h3>";
    chunk += "<p>Select a JSON backup file previously exported from this device:</p>";

    chunk += "<form id='restore-form' enctype='multipart/form-data' method='post' action='/upload_restore'>";
    chunk += "<div style='margin-bottom:20px;'>";
    chunk += "<input type='file' name='backup' accept='.json' required style='padding:10px;border:2px solid #ddd;border-radius:5px;'>";
    chunk += "</div>";
    chunk += "<button type='submit' class='button' style='background-color:#dc3545;' onclick='return confirm(\"Are you sure you want to restore settings? This will override ALL current configuration and restart the device.\")'>🔄 Restore Settings</button>";
    chunk += "</form>";

    chunk += "<div id='status-message' style='margin-top:20px;'></div>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "<script>";
    chunk += "document.getElementById('restore-form').addEventListener('submit', function(e) {";
    chunk += "  e.preventDefault();";
    chunk += "  var formData = new FormData(this);";
    chunk += "  var statusDiv = document.getElementById('status-message');";
    chunk += "  statusDiv.innerHTML = '<div class=\"status info\">⏳ Uploading and validating backup file...</div>';";
    chunk += "  fetch('/upload_restore', { method: 'POST', body: formData })";
    chunk += "    .then(response => {";
    chunk += "      if (response.ok) {";
    chunk += "        return response.json();";
    chunk += "      } else {";
    chunk += "        throw new Error('HTTP ' + response.status);";
    chunk += "      }";
    chunk += "    })";
    chunk += "    .then(data => {";
    chunk += "      if (data.success) {";
    chunk += "        statusDiv.innerHTML = '<div class=\"status success\">✅ ' + data.message + '</div>';";
    chunk += "        if (data.restart) {";
    chunk += "          statusDiv.innerHTML += '<br><div class=\"status info\">⏳ Device is restarting...</div>';";
    chunk += "          setTimeout(() => { window.location.href = '/'; }, 5000);";
    chunk += "        }";
    chunk += "      } else {";
    chunk += "        statusDiv.innerHTML = '<div class=\"status error\">❌ ' + data.message + '</div>';";
    chunk += "      }";
    chunk += "    })";
    chunk += "    .catch(error => {";
    chunk += "      if (error.message.includes('Failed to fetch')) {";
    chunk += "        statusDiv.innerHTML = '<div class=\"status success\">✅ Settings restored successfully. Device is restarting...</div>';";
    chunk += "        setTimeout(() => { window.location.href = '/'; }, 5000);";
    chunk += "      } else {";
    chunk += "        statusDiv.innerHTML = '<div class=\"status error\">❌ Upload failed: ' + error.message + '</div>';";
    chunk += "      }";
    chunk += "    });";
    chunk += "});";
    chunk += "</script>";

    webServer.sendContent(chunk);
    chunk = "";

    chunk += "</div>";
    chunk += get_html_footer();
    webServer.sendContent(chunk);
    webServer.sendContent("");
}

void handle_upload_restore() {
    reset_oled_timeout();

    HTTPUpload& upload = webServer.upload();
    static String restore_content = "";

    if (upload.status == UPLOAD_FILE_START) {
        restore_content = "";
        logMessage("RESTORE: Starting backup file upload");

    } else if (upload.status == UPLOAD_FILE_WRITE) {
        restore_content += String((char*)upload.buf).substring(0, upload.currentSize);

    } else if (upload.status == UPLOAD_FILE_END) {
        logMessage("RESTORE: Backup file upload completed, validating...");

        String error_msg = "";
        bool success = json_to_config(restore_content, error_msg);

        if (success) {
            if (save_settings()) {
                logMessage("RESTORE: Settings restored successfully from backup");
                webServer.send(200, "application/json",
                    "{\"success\":true,\"message\":\"Settings restored successfully. Device will restart in 5 seconds.\",\"restart\":true}");

                for (int i = 0; i < 10; i++) {
                    webServer.handleClient();
                    delay(100);
                }

                ESP.restart();
            } else {
                webServer.send(500, "application/json",
                    "{\"success\":false,\"message\":\"Failed to save restored settings\"}");
            }
        } else {
            logMessage("RESTORE: Backup validation failed - " + error_msg);
            webServer.send(400, "application/json",
                "{\"success\":false,\"message\":\"" + error_msg + "\"}");
        }

        restore_content = "";

    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        restore_content = "";
        webServer.send(400, "application/json",
            "{\"success\":false,\"message\":\"File upload was aborted\"}");
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

    if (save_settings()) {
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
        if (power >= 0 && power <= 20) {
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

    if (save_settings()) {
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

    if (save_settings()) {
        display_status();

        webServer.send(200, "application/json", "{\"success\":true,\"restart\":true,\"message\":\"MQTT configuration and certificates saved successfully. Device will restart in 5 seconds.\"}");
        delay(5000);
        ESP.restart();
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save MQTT configuration\"}");
    }
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
    uint32_t capcode = doc["capcode"] | settings.default_capcode;
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


String uploaded_cert_data = "";

String get_cert_status(const char* saved_cert) {
    return has_valid_certificate(saved_cert) ? "✅" : "❌";
}


void handle_file_upload() {
    HTTPUpload& upload = webServer.upload();

    if (upload.status == UPLOAD_FILE_START) {
        uploaded_cert_data = "";
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        uploaded_cert_data += String((const char*)upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
    }
}

void handle_upload_certificate() {
    reset_oled_timeout();

    String cert_data = uploaded_cert_data;
    cert_data.trim();

    if (cert_data.length() == 0) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"No certificate data received\"}");
        return;
    }

    String cert_type = "";
    if (webServer.hasArg("cert_type")) {
        cert_type = webServer.arg("cert_type");
    } else {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Certificate type not specified\"}");
        return;
    }

    if (!has_valid_certificate(cert_data.c_str())) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid certificate format\"}");
        return;
    }

    if (cert_data.length() > 2048) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Certificate too large (max 2KB)\"}");
        return;
    }

    String filename = getCertificateFilename(cert_type);
    if (filename == "") {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid certificate type: " + cert_type + "\"}");
        return;
    }

    if (!saveCertificateToSPIFFS(filename.c_str(), cert_data)) {
        webServer.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save certificate to SPIFFS\"}");
        return;
    }

    String cert_name = cert_type;
    cert_name.replace("_", " ");
    String protocol = (cert_type.startsWith("mqtt_") || cert_type == "root_ca" || cert_type == "device_cert" || cert_type == "device_key") ? "MQTT" : "HTTPS";
    logMessage(protocol + ": " + cert_name + " certificate uploaded and saved to SPIFFS");
    webServer.send(200, "application/json", "{\"success\":true,\"message\":\"" + cert_name + " certificate saved successfully\"}");

    uploaded_cert_data = "";
}










void log_serial_message(const char* message) {
    Serial.println(message);

    serial_log[serial_log_index].timestamp = getUnixTimestamp();
    strncpy(serial_log[serial_log_index].message, message, sizeof(serial_log[serial_log_index].message) - 1);
    serial_log[serial_log_index].message[sizeof(serial_log[serial_log_index].message) - 1] = '\0';

    serial_log_index = (serial_log_index + 1) % SERIAL_LOG_SIZE;
    if (serial_log_count < SERIAL_LOG_SIZE) {
        serial_log_count++;
    }
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

bool has_valid_certificate(const char* cert) {
    if (!cert || strlen(cert) < 50) {
        return false;
    }

    return (strstr(cert, "-----BEGIN") != NULL && strstr(cert, "-----END") != NULL);
}



static unsigned long last_auth_attempt = 0;
static int auth_failures = 0;
const int MAX_AUTH_FAILURES = 5;
const unsigned long AUTH_LOCKOUT_TIME = 300000;

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

bool is_using_default_api_password() {
    String encoded_current = base64_encode_string(String(settings.api_password));
    return (encoded_current == "cGFzc3cwcmQ=");
}

void handle_api_wifi_scan() {
    logMessage("API: WiFi scan requested");

    int n = WiFi.scanNetworks();

    DynamicJsonDocument doc(2048);
    doc["success"] = (n >= 0);

    if (n > 0) {
        JsonArray networks = doc.createNestedArray("networks");

        for (int i = 0; i < n; i++) {
            JsonObject net = networks.createNestedObject();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["channel"] = WiFi.channel(i);
            net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted";

            bool is_stored = false;
            for (int j = 0; j < stored_networks_count; j++) {
                if (String(stored_networks[j].ssid) == WiFi.SSID(i)) {
                    is_stored = true;
                    break;
                }
            }
            net["stored"] = is_stored;
        }

        logMessagef("API: Scan found %d networks", n);
    } else {
        logMessage("API: Scan found no networks");
    }

    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
}

void handle_api_wifi_delete() {
    if (!webServer.hasArg("ssid")) {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"Missing SSID parameter\"}");
        return;
    }

    String ssid_to_delete = webServer.arg("ssid");
    logMessagef("API: Delete network '%s' requested", ssid_to_delete.c_str());

    int network_idx = -1;
    for (int i = 0; i < stored_networks_count; i++) {
        if (String(stored_networks[i].ssid) == ssid_to_delete) {
            network_idx = i;
            break;
        }
    }

    if (network_idx == -1) {
        webServer.send(404, "application/json", "{\"success\":false,\"message\":\"Network not found\"}");
        return;
    }

    // Shift remaining networks down
    for (int i = network_idx; i < stored_networks_count - 1; i++) {
        stored_networks[i] = stored_networks[i + 1];
    }
    stored_networks_count--;

    if (save_settings()) {
        logMessagef("API: Network '%s' deleted successfully", ssid_to_delete.c_str());
        webServer.send(200, "application/json", "{\"success\":true,\"message\":\"Network deleted\"}");
    } else {
        logMessage("API: Failed to save settings after network deletion");
        webServer.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save settings\"}");
    }
}

void handle_api_wifi_add() {
    reset_oled_timeout();

    String ssid = webServer.arg("ssid");
    String password = webServer.arg("password");
    ssid.trim();
    password.trim();

    if (ssid.length() == 0) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"SSID required\"}");
        return;
    }

    if (password.length() == 0) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Password required\"}");
        return;
    }

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
    } else if (network_idx == -1) {
        webServer.send(400, "application/json", "{\"success\":false,\"error\":\"Maximum networks reached\"}");
        return;
    }

    strlcpy(stored_networks[network_idx].ssid, ssid.c_str(), sizeof(stored_networks[network_idx].ssid));
    strlcpy(stored_networks[network_idx].password, password.c_str(), sizeof(stored_networks[network_idx].password));
    stored_networks[network_idx].use_dhcp = (webServer.arg("use_dhcp") == "1");

    if (webServer.hasArg("static_ip")) {
        IPAddress parsed_ip;
        if (parsed_ip.fromString(webServer.arg("static_ip"))) {
            for (int i = 0; i < 4; i++) stored_networks[network_idx].static_ip[i] = parsed_ip[i];
        }
    }

    if (webServer.hasArg("netmask")) {
        IPAddress parsed_netmask;
        if (parsed_netmask.fromString(webServer.arg("netmask"))) {
            for (int i = 0; i < 4; i++) stored_networks[network_idx].netmask[i] = parsed_netmask[i];
        }
    }

    if (webServer.hasArg("gateway")) {
        IPAddress parsed_gateway;
        if (parsed_gateway.fromString(webServer.arg("gateway"))) {
            for (int i = 0; i < 4; i++) stored_networks[network_idx].gateway[i] = parsed_gateway[i];
        }
    }

    if (webServer.hasArg("dns")) {
        IPAddress parsed_dns;
        if (parsed_dns.fromString(webServer.arg("dns"))) {
            for (int i = 0; i < 4; i++) stored_networks[network_idx].dns[i] = parsed_dns[i];
        }
    }

    if (save_settings()) {
        webServer.send(200, "application/json", "{\"success\":true}");
    } else {
        webServer.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save\"}");
    }
}

void handle_api_message() {
    reset_oled_timeout();

    if (!settings.api_enabled) {
        webServer.send(503, "application/json", "{\"error\":\"API service is disabled\"}");
        return;
    }

    if (!authenticate_api_request()) {
        webServer.sendHeader("WWW-Authenticate", "Basic realm=\"FLEX API\"");
        webServer.send(401, "application/json", "{\"error\":\"Authentication required\"}");
        return;
    }

    if (webServer.method() != HTTP_POST) {
        webServer.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
        return;
    }

    if (!webServer.hasArg("plain")) {
        webServer.send(400, "application/json", "{\"error\":\"No JSON payload\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

    if (error) {
        webServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc["message"].is<String>()) {
        webServer.send(400, "application/json", "{\"error\":\"Missing required field: message\"}");
        return;
    }

    uint64_t capcode = settings.default_capcode;
    if (doc["capcode"].is<uint64_t>()) {
        capcode = doc["capcode"];
    } else if (doc["capcode"].is<String>()) {
        capcode = strtoul(doc["capcode"].as<String>().c_str(), nullptr, 10);
    }

    float frequency = settings.default_frequency;
    if (doc["frequency"].is<float>()) {
        frequency = doc["frequency"];
    } else if (doc["frequency"].is<double>()) {
        frequency = (float)doc["frequency"].as<double>();
    } else if (doc["frequency"].is<String>()) {
        frequency = atof(doc["frequency"].as<String>().c_str());
    }

    String message = doc["message"].as<String>();

    int power = settings.default_txpower;
    if (doc["power"].is<int>()) {
        power = doc["power"];
    } else if (doc["power"].is<String>()) {
        power = atoi(doc["power"].as<String>().c_str());
    } else if (doc["tx_power"].is<int>()) {
        power = doc["tx_power"];
    } else if (doc["tx_power"].is<String>()) {
        power = atoi(doc["tx_power"].as<String>().c_str());
    }

    bool mail_drop = false;
    if (doc["mail_drop"].is<bool>()) {
        mail_drop = doc["mail_drop"];
    } else if (doc["mail_drop"].is<String>()) {
        String mail_drop_str = doc["mail_drop"].as<String>();
        mail_drop_str.toLowerCase();
        mail_drop = (mail_drop_str == "true" || mail_drop_str == "1");
    }

    if (frequency > 1000.0) {
        frequency = frequency / 1000000.0;
    }

    if (frequency < 400.0 || frequency > 1000.0) {
        webServer.send(400, "application/json", "{\"error\":\"Frequency must be between 400.0-1000.0 MHz or 400000000-1000000000 Hz\"}");
        return;
    }

    if (power < 0 || power > 20) {
        webServer.send(400, "application/json", "{\"error\":\"TX Power must be between 0 and 20 dBm\"}");
        return;
    }

    if (message.length() == 0) {
        webServer.send(400, "application/json", "{\"error\":\"Message cannot be empty\"}");
        return;
    }

    bool message_was_truncated = false;
    if (message.length() > MAX_FLEX_MESSAGE_LENGTH) {
        message = truncate_message_with_ellipsis(message);
        message_was_truncated = true;
    }

    if (queue_add_message(capcode, frequency, power, mail_drop, message.c_str())) {
        JsonDocument response;
        response["frequency"] = frequency;
        response["power"] = power;
        response["capcode"] = capcode;
        response["text"] = message;
        response["truncated"] = message_was_truncated;

        if (device_state == STATE_IDLE) {
            response["status"] = "queued";
            if (message_was_truncated) {
                response["message"] = "Message truncated to 248 chars and queued for immediate transmission";
            } else {
                response["message"] = "Message queued for immediate transmission";
            }
        } else {
            response["status"] = "queued";
            if (message_was_truncated) {
                response["message"] = "Message truncated to 248 chars and queued for transmission";
            } else {
                response["message"] = "Message queued for transmission";
            }
            response["queue_position"] = queue_count;
        }

        String response_str;
        serializeJson(response, response_str);
        webServer.send(200, "application/json", response_str);
    } else {
        JsonDocument response;
        response["status"] = "error";
        response["message"] = "Queue is full. Please try again later.";
        response["max_queue_size"] = MAX_QUEUE_SIZE;

        String response_str;
        serializeJson(response, response_str);
        webServer.send(503, "application/json", response_str);
    }
}

void handle_grafana_webhook() {
    reset_oled_timeout();

    if (!settings.grafana_enabled) {
        webServer.send(503, "application/json", "{\"error\":\"Grafana webhook service is disabled\"}");
        return;
    }

    if (!authenticate_api_request()) {
        webServer.sendHeader("WWW-Authenticate", "Basic realm=\"FLEX API\"");
        webServer.send(401, "application/json", "{\"error\":\"Authentication required\"}");
        return;
    }

    if (webServer.method() != HTTP_POST) {
        webServer.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
        return;
    }

    if (!webServer.hasArg("plain")) {
        webServer.send(400, "application/json", "{\"error\":\"No JSON payload\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

    if (error) {
        webServer.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.is<JsonArray>()) {
        webServer.send(400, "application/json", "{\"error\":\"Expected JSON array of alerts\"}");
        return;
    }

    JsonArray alerts = doc.as<JsonArray>();
    int total_alerts = alerts.size();
    int successful = 0;
    int failed = 0;

    logMessage("GRAFANA: Processing " + String(total_alerts) + " alerts");

    JsonDocument response;
    response["status"] = "completed";
    response["total_alerts"] = total_alerts;

    JsonArray results = response["results"].to<JsonArray>();

    for (int i = 0; i < total_alerts; i++) {
        JsonObject alert = alerts[i];
        JsonObject result = results.createNestedObject();

        result["alert_index"] = i + 1;

        JsonObject labels = alert["labels"];
        JsonObject annotations = alert["annotations"];

        String ends_at = alert["endsAt"].as<String>();
        String status = (ends_at == "0001-01-01T00:00:00Z") ? "FIRING" : "RESOLVED";

        String alert_name = labels["alertname"].as<String>();
        if (alert_name.isEmpty()) {
            alert_name = "Unknown Alert";
        }
        result["alert_name"] = alert_name;

        uint64_t capcode = settings.default_capcode;
        if (labels["capcode"].is<uint64_t>()) {
            capcode = labels["capcode"];
        } else if (labels["capcode"].is<String>()) {
            capcode = strtoul(labels["capcode"].as<String>().c_str(), nullptr, 10);
        } else if (labels["pager_capcode"].is<uint64_t>()) {
            capcode = labels["pager_capcode"];
        } else if (labels["pager_capcode"].is<String>()) {
            capcode = strtoul(labels["pager_capcode"].as<String>().c_str(), nullptr, 10);
        }
        result["capcode"] = capcode;

        float frequency = settings.default_frequency;
        if (labels["frequency"].is<float>()) {
            frequency = labels["frequency"];
        } else if (labels["frequency"].is<String>()) {
            frequency = atof(labels["frequency"].as<String>().c_str());
        } else if (labels["pager_frequency"].is<float>()) {
            frequency = labels["pager_frequency"];
        } else if (labels["pager_frequency"].is<String>()) {
            frequency = atof(labels["pager_frequency"].as<String>().c_str());
        }

        if (frequency > 1000.0) {
            frequency = frequency / 1000000.0;
        }
        result["frequency"] = frequency;

        bool mail_drop = false;
        if (labels["mail_drop"].is<bool>()) {
            mail_drop = labels["mail_drop"];
        } else if (labels["mail_drop"].is<String>()) {
            String mail_drop_str = labels["mail_drop"].as<String>();
            mail_drop = (mail_drop_str == "true" || mail_drop_str == "1");
        } else if (labels["pager_mail_drop"].is<bool>()) {
            mail_drop = labels["pager_mail_drop"];
        } else if (labels["pager_mail_drop"].is<String>()) {
            String mail_drop_str = labels["pager_mail_drop"].as<String>();
            mail_drop = (mail_drop_str == "true" || mail_drop_str == "1");
        }

        String message_content = "";
        if (annotations["summary"].is<String>() && !annotations["summary"].as<String>().isEmpty()) {
            message_content = annotations["summary"].as<String>();
        } else if (annotations["description"].is<String>() && !annotations["description"].as<String>().isEmpty()) {
            message_content = annotations["description"].as<String>();
        } else if (annotations["message"].is<String>() && !annotations["message"].as<String>().isEmpty()) {
            message_content = annotations["message"].as<String>();
        } else {
            message_content = "Alert triggered";
        }

        String final_message = "[" + status + "] " + alert_name + ": " + message_content;

        bool message_was_truncated = false;
        if (final_message.length() > MAX_FLEX_MESSAGE_LENGTH) {
            final_message = truncate_message_with_ellipsis(final_message);
            message_was_truncated = true;
        }

        result["message"] = final_message;
        result["truncated"] = message_was_truncated;

        if (queue_add_message(capcode, frequency, settings.default_txpower, mail_drop, final_message.c_str())) {
            result["success"] = true;
            successful++;
            logMessage("GRAFANA: Alert " + String(i + 1) + " queued - " + alert_name + " (capcode=" + String(capcode) + ")");
        } else {
            result["success"] = false;
            result["error"] = "Queue is full";
            failed++;
            logMessage("GRAFANA: Alert " + String(i + 1) + " failed - Queue full");
        }
    }

    response["successful"] = successful;
    response["failed"] = failed;

    String response_str;
    serializeJson(response, response_str);

    int status_code = (failed == 0) ? 200 : 207;
    webServer.send(status_code, "application/json", response_str);

    logMessage("GRAFANA: Completed processing - " + String(successful) + " successful, " + String(failed) + " failed");
}


void at_send_ok() {
    Serial.print("OK\r\n");
    Serial.flush();
    delay(AT_INTER_CMD_DELAY);
}

void at_send_error() {
    Serial.print("ERROR\r\n");
    Serial.flush();
    delay(AT_INTER_CMD_DELAY);
}

void at_send_response(const char* cmd, const char* value) {
    Serial.print("+");
    Serial.print(cmd);
    Serial.print(": ");
    Serial.print(value);
    Serial.print("\r\n");
    at_send_ok();
}

void at_send_response_float(const char* cmd, float value, int decimals) {
    Serial.print("+");
    Serial.print(cmd);
    Serial.print(": ");
    Serial.print(value, decimals);
    Serial.print("\r\n");
    at_send_ok();
}

void at_send_response_int(const char* cmd, int value) {
    Serial.print("+");
    Serial.print(cmd);
    Serial.print(": ");
    Serial.print(value);
    Serial.print("\r\n");
    at_send_ok();
}

void at_reset_state() {
    if (device_state != STATE_WIFI_CONNECTING && device_state != STATE_WIFI_AP_MODE) {
        device_state = STATE_IDLE;
    }
    current_tx_total_length = 0;
    current_tx_remaining_length = 0;
    expected_data_length = 0;
    data_receive_timeout = 0;
    state_timeout = 0;
    transmission_processing_complete = false;
    console_loop_enable = true;


    flex_capcode = 0;
    current_tx_capcode = 0;
    flex_message_pos = 0;
    flex_message_timeout = 0;
    flex_mail_drop = false;
    memset(flex_message_buffer, 0, sizeof(flex_message_buffer));
}

void at_flush_serial_buffers() {
    while (Serial.available()) {
        Serial.read();
        delay(1);
    }
    delay(50);
}

bool at_parse_command(char* cmd_buffer) {
    reset_oled_timeout();

    int len = strlen(cmd_buffer);
    while (len > 0 && (cmd_buffer[len-1] == '\r' || cmd_buffer[len-1] == '\n')) {
        cmd_buffer[--len] = '\0';
    }

    if (len == 0) {
        return true;
    }

    if (strncmp(cmd_buffer, "AT", 2) != 0) {
        return false;
    }

    if (strcmp(cmd_buffer, "AT") == 0) {
        at_reset_state();
        display_status();
        at_send_ok();
        return true;
    }

    if (strncmp(cmd_buffer, "AT+", 3) != 0) {
        return false;
    }

    char* cmd_start = cmd_buffer + 3;
    char* equals_pos = strchr(cmd_start, '=');
    char* query_pos = strchr(cmd_start, '?');

    char cmd_name[32];
    int cmd_name_len;

    if (equals_pos != NULL) {
        cmd_name_len = equals_pos - cmd_start;
    } else if (query_pos != NULL) {
        cmd_name_len = query_pos - cmd_start;
    } else {
        cmd_name_len = strlen(cmd_start);
    }

    if (cmd_name_len >= sizeof(cmd_name) || cmd_name_len <= 0) {
        return false;
    }

    strncpy(cmd_name, cmd_start, cmd_name_len);
    cmd_name[cmd_name_len] = '\0';

    if (transmission_guard_active()) {
        if (!(strcmp(cmd_buffer, "AT") == 0 || strcmp(cmd_name, "STATUS") == 0 || strcmp(cmd_name, "ABORT") == 0)) {
            at_send_error();
            return true;
        }
    }

    if (strcmp(cmd_name, "FREQ") == 0) {
        if (query_pos != NULL) {
            at_send_response_float("FREQ", current_tx_frequency, 4);
        } else if (equals_pos != NULL) {
            float freq = atof(equals_pos + 1);
            if (freq < 400.0 || freq > 1000.0) {
                at_send_error();
                return true;
            }

            int state = radio.setFrequency(apply_frequency_correction(freq));
            if (state != RADIOLIB_ERR_NONE) {
                at_send_error();
                return true;
            }

            current_tx_frequency = freq;
            display_status();
            at_send_ok();
        }
        return true;
    }

    if (strcmp(cmd_name, "FREQPPM") == 0) {
        if (query_pos != NULL) {
            at_send_response_float("FREQPPM", settings.frequency_correction_ppm, 2);
        } else if (equals_pos != NULL) {
            float ppm = atof(equals_pos + 1);
            if (ppm < -50.0 || ppm > 50.0) {
                at_send_error();
                return true;
            }

            settings.frequency_correction_ppm = ppm;
            save_settings();

            if (current_tx_frequency > 0) {
                float corrected_freq = apply_frequency_correction(current_tx_frequency);
                radio.setFrequency(corrected_freq);
            }

            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "POWER") == 0) {
        if (query_pos != NULL) {
            at_send_response_int("POWER", (int)tx_power);
        } else if (equals_pos != NULL) {
            int power = atoi(equals_pos + 1);
            if (power < -9 || power > 20) {
                at_send_error();
                return true;
            }

            int state = radio.setOutputPower(power);
            if (state != RADIOLIB_ERR_NONE) {
                at_send_error();
                return true;
            }

            tx_power = power;
            display_status();
            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "SEND") == 0) {
        if (equals_pos != NULL) {
            int bytes_to_read = atoi(equals_pos + 1);

            if (bytes_to_read <= 0 || bytes_to_read > 2048) {
                at_send_error();
                return true;
            }

            at_reset_state();

            device_state = STATE_WAITING_FOR_DATA;
            expected_data_length = bytes_to_read;
            current_tx_total_length = 0;
            data_receive_timeout = millis() + 15000;
            console_loop_enable = false;

            at_flush_serial_buffers();

            Serial.print("+SEND: READY\r\n");
            Serial.flush();

            display_status();
        }
        return true;
    }

    else if (strcmp(cmd_name, "MSG") == 0) {
        if (equals_pos != NULL) {
            uint64_t capcode;
            if (str2uint64(&capcode, equals_pos + 1) < 0) {
                at_send_error();
                return true;
            }

            at_reset_state();

            device_state = STATE_WAITING_FOR_MSG;
            flex_capcode = capcode;
            current_tx_capcode = capcode;
            flex_message_pos = 0;
            flex_message_timeout = millis() + FLEX_MSG_TIMEOUT;
            console_loop_enable = false;
            memset(flex_message_buffer, 0, sizeof(flex_message_buffer));

            at_flush_serial_buffers();

            Serial.print("+MSG: READY\r\n");
            Serial.flush();

            display_status();
        }
        return true;
    }


    else if (strcmp(cmd_name, "STATUS") == 0) {
        const char* status_str;
        switch (device_state) {
            case STATE_IDLE:
                status_str = "READY";
                break;
            case STATE_WAITING_FOR_DATA:
                status_str = "WAITING_DATA";
                break;
            case STATE_WAITING_FOR_MSG:
                status_str = "WAITING_MSG";
                break;
            case STATE_TRANSMITTING:
                status_str = "TRANSMITTING";
                break;
            case STATE_ERROR:
                status_str = "ERROR";
                break;
            case STATE_WIFI_CONNECTING:
                status_str = "WIFI_CONNECTING";
                break;
            case STATE_WIFI_AP_MODE:
                status_str = "WIFI_AP_MODE";
                break;
            case STATE_NTP_SYNC:
                status_str = "NTP_SYNC";
                break;
            case STATE_MQTT_CONNECTING:
                status_str = "MQTT_CONNECTING";
                break;
            default:
                status_str = "UNKNOWN";
                break;
        }
        at_send_response("STATUS", status_str);
        return true;
    }

    else if (strcmp(cmd_name, "ABORT") == 0) {
        radio.standby();
        LED_OFF();
        at_reset_state();
        display_status();
        at_send_ok();
        return true;
    }

    else if (strcmp(cmd_name, "RESET") == 0) {
        at_send_ok();
        delay(100);
        ESP.restart();
        return true;
    }

    else if (strcmp(cmd_name, "WIFI") == 0) {
        if (query_pos != NULL) {
            String status = "DISCONNECTED";
            if (wifi_connected) {
                status = "CONNECTED," + WiFi.localIP().toString();
            } else if (ap_mode_active) {
                status = "AP_MODE," + WiFi.softAPIP().toString();
            }
            at_send_response("WIFI", status.c_str());
        } else if (equals_pos != NULL) {
            // AT+WIFI=<ssid>,<password> - Add or update network
            String params = String(equals_pos + 1);
            int comma_pos = params.indexOf(',');
            if (comma_pos > 0) {
                String ssid = params.substring(0, comma_pos);
                String password = params.substring(comma_pos + 1);
                ssid.trim();
                password.trim();

                if (ssid.length() > 0 && ssid.length() <= 32 && password.length() <= 64) {
                    int network_idx = -1;

                    // Find existing network or add new one
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
                        stored_networks[network_idx].use_dhcp = true;  // Default to DHCP

                        if (save_settings()) {
                            at_send_ok();
                        } else {
                            at_send_error();
                        }
                    } else {
                        at_send_error();  // Max networks reached
                    }
                } else {
                    at_send_error();  // Invalid SSID/password length
                }
            } else {
                at_send_error();  // Invalid format
            }
        }
        return true;
    }













    else if (strcmp(cmd_name, "DEVICE") == 0) {
        if (query_pos != NULL) {
            Serial.print("+DEVICE_FIRMWARE: ");
            Serial.print(CURRENT_VERSION);
            Serial.print("\r\n");

            uint16_t battery_voltage_mv;
            int battery_percentage_temp;
            getBatteryInfo(&battery_voltage_mv, &battery_percentage_temp);
            Serial.print("+DEVICE_BATTERY: ");
            if (battery_present) {
                Serial.print(battery_percentage_temp);
                Serial.print("%");
            } else {
                Serial.print("N/A");
            }
            Serial.print("\r\n");

            String wifi_status = "Disconnected";
            if (wifi_connected) {
                wifi_status = "Connected";
            } else if (ap_mode_active) {
                wifi_status = "AP_Mode";
            }
            Serial.print("+DEVICE_WIFI: ");
            Serial.print(wifi_status);
            Serial.print("\r\n");

            String mqtt_status = settings.mqtt_enabled ? (mqttClient.connected() ? "Connected" : "Disconnected") : "Disabled";
            Serial.print("+DEVICE_MQTT: ");
            Serial.print(mqtt_status);
            Serial.print("\r\n");

            String imap_status = imap_config.enabled ? "Active" : "Disabled";
            Serial.print("+DEVICE_IMAP: ");
            Serial.print(imap_status);
            Serial.print("\r\n");

            for (int i = 0; i < imap_config.account_count; i++) {
                String account_status = !imap_config.accounts[i].suspended ? "Active" : "Suspended";
                Serial.print("+DEVICE_IMAP_ACCOUNT");
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(account_status);
                Serial.print("\r\n");
            }

            String api_status = settings.api_enabled ? "Enabled" : "Disabled";
            Serial.print("+DEVICE_API: ");
            Serial.print(api_status);
            Serial.print("\r\n");

            String grafana_status = settings.grafana_enabled ? "Enabled" : "Disabled";
            Serial.print("+DEVICE_GRAFANA: ");
            Serial.print(grafana_status);
            Serial.print("\r\n");

            Serial.print("+DEVICE_MEMORY: ");
            Serial.print(ESP.getFreeHeap());
            Serial.print(" bytes\r\n");

            Serial.print("+DEVICE_FLEX_CAPCODE: ");
            Serial.print(settings.default_capcode);
            Serial.print("\r\n");

            Serial.print("+DEVICE_FLEX_FREQUENCY: ");
            Serial.print(settings.default_frequency, 4);
            Serial.print("\r\n");

            Serial.print("+DEVICE_FLEX_POWER: ");
            Serial.print(settings.default_txpower, 1);
            Serial.print("\r\n");

            at_send_ok();
        }
        return true;
    }

    else if (strcmp(cmd_name, "FLEX") == 0) {
        if (query_pos != NULL) {
            Serial.print("+FLEX_CAPCODE: ");
            Serial.print(settings.default_capcode);
            Serial.print("\r\n");
            Serial.print("+FLEX_FREQUENCY: ");
            Serial.print(settings.default_frequency, 4);
            Serial.print("\r\n");
            Serial.print("+FLEX_POWER: ");
            Serial.print(settings.default_txpower, 1);
            Serial.print("\r\n");
            at_send_ok();
        } else if (equals_pos != NULL) {
            String params = String(equals_pos + 1);
            int comma_pos = params.indexOf(',');
            if (comma_pos > 0) {
                String param_name = params.substring(0, comma_pos);
                String value_str = params.substring(comma_pos + 1);
                param_name.toUpperCase();

                if (param_name == "CAPCODE") {
                    uint64_t capcode = strtoull(value_str.c_str(), NULL, 10);
                    if (capcode > 0) {
                        settings.default_capcode = capcode;
                        save_settings();
                        at_send_ok();
                    } else {
                        at_send_error();
                    }
                }
                else if (param_name == "FREQUENCY") {
                    float freq = atof(value_str.c_str());
                    if (freq >= 400.0 && freq <= 1000.0) {
                        settings.default_frequency = freq;
                        save_settings();
                        at_send_ok();
                    } else {
                        at_send_error();
                    }
                }
                else if (param_name == "POWER") {
                    float power = atof(value_str.c_str());
                    if (power >= 0.0 && power <= 20.0) {
                        settings.default_txpower = power;
                        save_settings();
                        at_send_ok();
                    } else {
                        at_send_error();
                    }
                }
                else {
                    at_send_error();
                }
            } else {
                at_send_error();
            }
        }
        return true;
    }

    else if (strcmp(cmd_name, "FACTORYRESET") == 0) {
        at_send_ok();
        delay(100);

        logMessage("SYSTEM: Factory reset initiated via AT command");
        perform_factory_reset();
        return true;
    }

    return false;
}

void at_handle_binary_data() {
    reset_oled_timeout();

    if (device_state != STATE_WAITING_FOR_DATA) {
        return;
    }

    if (millis() > data_receive_timeout) {
        at_reset_state();
        display_status();
        at_send_error();
        return;
    }

    while (Serial.available() && current_tx_total_length < expected_data_length) {
        tx_data_buffer[current_tx_total_length++] = Serial.read();
        data_receive_timeout = millis() + 5000;
    }

    if (current_tx_total_length >= expected_data_length) {
        device_state = STATE_TRANSMITTING;
        LED_ON();

        fifo_empty = true;
        current_tx_remaining_length = current_tx_total_length;
        radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);

        display_status();
    }
}

void at_handle_flex_message() {
    reset_oled_timeout();

    if (device_state != STATE_WAITING_FOR_MSG) {
        return;
    }

    if (millis() > flex_message_timeout) {
        at_reset_state();
        display_status();
        at_send_error();
        return;
    }

    while (Serial.available() && flex_message_pos < MAX_FLEX_MESSAGE_LENGTH) {
        char c = Serial.read();

        if (c == '\r' || c == '\n') {
            flex_message_buffer[flex_message_pos] = '\0';

            if (queue_add_message(flex_capcode, current_tx_frequency, tx_power, flex_mail_drop, flex_message_buffer)) {
                at_reset_state();
                at_send_ok();
                display_status();
            } else {
                device_state = STATE_ERROR;
                at_send_error();
                at_reset_state();
                display_status();
            }
            return;
        }

        if (c >= 32 && c <= 126) {
            flex_message_buffer[flex_message_pos++] = c;
            flex_message_timeout = millis() + FLEX_MSG_TIMEOUT;
        }
    }

    if (flex_message_pos >= MAX_FLEX_MESSAGE_LENGTH) {
        String truncated_message = truncate_message_with_ellipsis(String(flex_message_buffer));
        strncpy(flex_message_buffer, truncated_message.c_str(), MAX_FLEX_MESSAGE_LENGTH);
        flex_message_buffer[MAX_FLEX_MESSAGE_LENGTH] = '\0';

        if (queue_add_message(flex_capcode, current_tx_frequency, tx_power, flex_mail_drop, flex_message_buffer)) {
            at_reset_state();
            at_send_ok();
            display_status();
        } else {
            device_state = STATE_ERROR;
            at_send_error();
            at_reset_state();
            display_status();
        }
    }
}

void at_process_serial() {
    if (device_state == STATE_WAITING_FOR_DATA) {
        at_handle_binary_data();
        return;
    }

    if (device_state == STATE_WAITING_FOR_MSG) {
        at_handle_flex_message();
        return;
    }

    while (Serial.available()) {
        char c = Serial.read();

        if (at_buffer_pos >= AT_BUFFER_SIZE - 1) {
            at_buffer_pos = 0;
            at_send_error();
            continue;
        }

        at_buffer[at_buffer_pos++] = c;

        if (c == '\n' || (c == '\r' && at_buffer_pos > 1)) {
            at_buffer[at_buffer_pos] = '\0';
            at_command_ready = true;
            break;
        }
    }

    if (at_command_ready) {
        if (!at_parse_command(at_buffer)) {
            at_send_error();
        }

        at_buffer_pos = 0;
        at_command_ready = false;
    }
}


void panic() {
    display_panic();
    Serial.print("ERROR\r\n");
    while (true) {
        delay(100000);
    }
}


#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void on_interrupt_fifo_has_space() {
    fifo_empty = true;
}

void transmission_task(void* parameter);
void init_transmission_core();

bool queue_is_empty() {
    return queue_count == 0;
}

bool queue_is_full() {
    return queue_count >= MAX_QUEUE_SIZE;
}

bool queue_add_message(uint32_t capcode, float frequency, int power, bool mail_drop, const char* message) {

    portENTER_CRITICAL(&queue_mux);

    if (queue_count >= MAX_QUEUE_SIZE) {
        portEXIT_CRITICAL(&queue_mux);
        return false;
    }

    QueuedMessage* msg = &message_queue[queue_tail];
    msg->capcode = capcode;
    msg->frequency = frequency;
    msg->power = power;
    msg->mail_drop = mail_drop;

    String converted_message = convert_unicode_to_ascii(String(message));
    converted_message = truncate_message_with_ellipsis(converted_message);
    strncpy(msg->message, converted_message.c_str(), MAX_FLEX_MESSAGE_LENGTH);
    msg->message[MAX_FLEX_MESSAGE_LENGTH] = '\0';

    queue_tail = (queue_tail + 1) % MAX_QUEUE_SIZE;
    queue_count++;

    portEXIT_CRITICAL(&queue_mux);

    if (tx_task_handle != NULL) {
        xTaskNotifyGive(tx_task_handle);
    }

    return true;
}

struct QueuedMessage* queue_get_next_message() {
    portENTER_CRITICAL(&queue_mux);
    if (queue_count == 0) {
        portEXIT_CRITICAL(&queue_mux);
        return nullptr;
    }
    QueuedMessage* msg = &message_queue[queue_head];
    portEXIT_CRITICAL(&queue_mux);
    return msg;
}

void queue_remove_message() {
    portENTER_CRITICAL(&queue_mux);
    if (queue_count > 0) {
        queue_head = (queue_head + 1) % MAX_QUEUE_SIZE;
        queue_count--;
    }
    portEXIT_CRITICAL(&queue_mux);
}

void queue_process_next() {
    if (queue_is_empty() || (device_state != STATE_IDLE && device_state != STATE_IMAP_PROCESSING)) {
        return;
    }

    QueuedMessage* msg = queue_get_next_message();
    if (msg == nullptr) {
        return;
    }

    if (abs(msg->frequency - current_tx_frequency) > 0.0001) {
        int state = radio.setFrequency(apply_frequency_correction(msg->frequency));
        if (state != RADIOLIB_ERR_NONE) {
            queue_remove_message();
            return;
        }
        current_tx_frequency = msg->frequency;
    }

    if (abs(msg->power - tx_power) > 0.1) {
        int state = radio.setOutputPower(msg->power);
        if (state != RADIOLIB_ERR_NONE) {
            queue_remove_message();
            return;
        }
        tx_power = msg->power;
    }

    feed_watchdog();

    if (!flex_encode_and_store(msg->capcode, msg->message, msg->mail_drop)) {
        queue_remove_message();
        return;
    }

    current_tx_capcode = msg->capcode;
    device_state = STATE_TRANSMITTING;
    LED_ON();

    send_emr_if_needed();

    int radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);
    if (radio_start_transmit_status != RADIOLIB_ERR_NONE) {
        device_state = STATE_IDLE;
        LED_OFF();
        display_status();
    } else {
        display_status();
    }

    queue_remove_message();
}

void transmission_task(void* parameter) {
    while (true) {
        core0_last_heartbeat = millis();

        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));

        while (true) {
            QueuedMessage* msg = queue_get_next_message();
            if (msg == nullptr) {
                break;
            }

            if (abs(msg->frequency - current_tx_frequency) > 0.0001) {
                int state = radio.setFrequency(apply_frequency_correction(msg->frequency));
                if (state != RADIOLIB_ERR_NONE) {
                    queue_remove_message();
                    continue;
                }
                current_tx_frequency = msg->frequency;
            }

            if (abs(msg->power - tx_power) > 0.1) {
                int state = radio.setOutputPower(msg->power);
                if (state != RADIOLIB_ERR_NONE) {
                    queue_remove_message();
                    continue;
                }
                tx_power = msg->power;
            }

            if (!flex_encode_and_store(msg->capcode, msg->message, msg->mail_drop)) {
                queue_remove_message();
                continue;
            }

            current_tx_capcode = msg->capcode;

            if (settings.enable_rf_amplifier) {
                int actual_rfamp_pin = (settings.rf_amplifier_power_pin == 0) ? RFAMP_PWR_PIN : settings.rf_amplifier_power_pin;
                digitalWrite(actual_rfamp_pin, settings.rf_amplifier_active_high ? HIGH : LOW);
                delay(settings.rf_amplifier_delay_ms);
            }

            device_state = STATE_TRANSMITTING;
            LED_ON();

            display_update_requested = true;

            send_emr_if_needed();

            fifo_empty = true;
            current_tx_remaining_length = current_tx_total_length;
            radio_start_transmit_status = radio.startTransmit(tx_data_buffer, current_tx_total_length);

            if (radio_start_transmit_status != RADIOLIB_ERR_NONE) {
                device_state = STATE_IDLE;
                LED_OFF();
                display_update_requested = true;
                queue_remove_message();
                continue;
            }

            display_update_requested = true;

            bool transmission_complete = false;
            while (!transmission_complete) {
                if (fifo_empty && current_tx_remaining_length > 0) {
                    fifo_empty = false;
                    transmission_complete = radio.fifoAdd(tx_data_buffer, current_tx_total_length, &current_tx_remaining_length);
                }
                delay(1);
            }

            if (radio_start_transmit_status == RADIOLIB_ERR_NONE) {
                logMessagef("FLEX: Message sent successfully (capcode=%llu, freq=%.4f MHz, power=%.1f dBm)",
                          current_tx_capcode, current_tx_frequency, tx_power);
            }

            radio.standby();

            device_state = STATE_IDLE;
            LED_OFF();

            if (settings.enable_rf_amplifier) {
                int actual_rfamp_pin = (settings.rf_amplifier_power_pin == 0) ? RFAMP_PWR_PIN : settings.rf_amplifier_power_pin;
                digitalWrite(actual_rfamp_pin, settings.rf_amplifier_active_high ? LOW : HIGH);
            }

            display_update_requested = true;

            queue_remove_message();
        }
    }
}

void init_transmission_core() {
    xTaskCreatePinnedToCore(
        transmission_task,
        "TX_Core0",
        4096,
        NULL,
        configMAX_PRIORITIES - 1,
        &tx_task_handle,
        0
    );

    logMessage("TX: Core 0 task created - isolated transmission");
}

void load_boot_tracker() {
    Preferences prefs;
    prefs.begin("boot_tracker", true);
    boot_tracker.consecutive_resets = prefs.getUChar("resets", 0);
    boot_tracker.last_reset_phase = prefs.getUInt("phase", 0);
    prefs.end();
}

void save_boot_tracker() {
    Preferences prefs;
    prefs.begin("boot_tracker", false);
    prefs.putUChar("resets", boot_tracker.consecutive_resets);
    prefs.putUInt("phase", boot_tracker.last_reset_phase);
    prefs.end();
}

void check_boot_failure_history() {
    load_boot_tracker();

    if (boot_tracker.consecutive_resets >= 3 &&
        boot_tracker.last_reset_phase == BOOT_SERVICES_PENDING) {

        logMessage("BOOT: Detected 3 consecutive failures during IMAP init");
        logMessage("BOOT: Auto-disabling IMAP for safe mode");

        imap_config.enabled = false;
        save_imap_config();

        boot_tracker.consecutive_resets = 0;
        save_boot_tracker();
    } else {
        boot_tracker.last_reset_phase = boot_phase;
        boot_tracker.consecutive_resets++;
        save_boot_tracker();
    }
}

void mark_boot_success() {
    boot_tracker.consecutive_resets = 0;
    save_boot_tracker();
    logMessage("BOOT: Complete - boot failure tracker reset");
}

void check_transmission_task_health() {
    static unsigned long last_check = 0;

    if (millis() - last_check > 10000) {
        if (millis() - core0_last_heartbeat > 15000) {
            logMessage("CRITICAL: Core 0 transmission task unresponsive for 15s");
        }
        last_check = millis();
    }
}



void setup() {
    Serial.begin(SERIAL_BAUD);

    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_CS_PIN);

    esp_task_wdt_deinit();

    if (!SPIFFS.begin(false)) {
        VextON();
        delay(10);
        Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
        if (OLED_RST_PIN >= 0) {
            pinMode(OLED_RST_PIN, OUTPUT);
            digitalWrite(OLED_RST_PIN, LOW);
            delay(50);
            digitalWrite(OLED_RST_PIN, HIGH);
            delay(50);
        }
        display.begin();
        display.clearBuffer();

        const int centerX = display.getWidth() / 2;

        display.setFont(u8g2_font_open_iconic_embedded_4x_t);
        display.drawGlyph(centerX - 16, 28, 78);

        display.setFont(u8g2_font_7x13B_tr);
        const char* line1 = "INITIALIZING";
        const char* line2 = "DEVICE";
        int width1 = display.getStrWidth(line1);
        int width2 = display.getStrWidth(line2);
        display.drawStr(centerX - (width1 / 2), 45, line1);
        display.drawStr(centerX - (width2 / 2), 60, line2);

        display.sendBuffer();

        if (!SPIFFS.begin(true)) {
            logMessage("SYSTEM: SPIFFS initialization failed!");
            while(1) {
                delay(1000);
            }
        }
    }
    logMessage("SYSTEM: SPIFFS initialized successfully");

    load_core_config();
    load_settings();

    chatgpt_load_config();

    load_imap_config();

    current_tx_frequency = settings.default_frequency;

    if (settings.default_txpower >= -4.0 && settings.default_txpower <= 20.0) {
        tx_power = settings.default_txpower;
    } else {
        tx_power = TX_POWER_DEFAULT;
        settings.default_txpower = TX_POWER_DEFAULT;
    }

    display_setup();
    display_status();

    pinMode(LED_PIN, OUTPUT);
    LED_OFF();

    int actual_rfamp_pin = (settings.rf_amplifier_power_pin == 0) ? RFAMP_PWR_PIN : settings.rf_amplifier_power_pin;
    pinMode(actual_rfamp_pin, OUTPUT);
    digitalWrite(actual_rfamp_pin, settings.rf_amplifier_active_high ? LOW : HIGH);

    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);

    uint16_t battery_voltage_mv;
    int battery_percentage_temp;
    getBatteryInfo(&battery_voltage_mv, &battery_percentage_temp);

    last_heartbeat = millis();

    float corrected_init_freq = apply_frequency_correction(current_tx_frequency);
    int radio_init_state = radio.beginFSK(corrected_init_freq,
                                         TX_BITRATE,
                                         TX_DEVIATION,
                                         RX_BANDWIDTH,
                                         tx_power,
                                         PREAMBLE_LENGTH,
                                         false);

    if (radio_init_state != RADIOLIB_ERR_NONE) {
        panic();
    }

    radio.setFifoEmptyAction(on_interrupt_fifo_has_space);

    int packet_mode_state = radio.fixedPacketLengthMode(0);
    if (packet_mode_state != RADIOLIB_ERR_NONE) {
        panic();
    }

    at_reset_state();

    reset_oled_timeout();

    WiFi.mode(WIFI_STA);
    network_boot();

    webServer.on("/", handle_root);
        webServer.on("/send", HTTP_POST, handle_send_message);
        webServer.on("/config", handle_configuration);
        webServer.on("/save_config", HTTP_POST, handle_save_config);
        webServer.on("/flex", handle_flex_config);
        webServer.on("/save_flex", HTTP_POST, handle_save_flex);
        webServer.on("/mqtt", handle_mqtt);
        webServer.on("/save_mqtt", HTTP_POST, handle_save_mqtt);
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
        webServer.on("/upload_certificate", HTTP_POST, handle_upload_certificate, handle_file_upload);
        webServer.on("/status", handle_device_status);
        webServer.on("/logs", handle_logs);
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

    // Boot failure detection
    check_boot_failure_history();

    // Initialize Core 0 transmission task
    init_transmission_core();

    // Set initial boot phase
    boot_phase = BOOT_NETWORK_PENDING;
    boot_phase_start = millis();

    webServer.begin();
    logMessage("STARTUP: HTTP server started on port " + String(settings.http_port));

    String startup_msg = "STARTUP: FLEX Paging Message Transmitter " + String(CURRENT_VERSION);
    log_serial_message(startup_msg.c_str());

    Serial.print("AT READY\r\n");
    Serial.print("WIFI ENABLED\r\n");
    Serial.flush();

    logMessage("STARTUP: Boot sequence started (staged initialization)");
    logMessage("STARTUP: Phase -> BOOT_NETWORK_PENDING");
}

static bool low_battery_alert_sent = false;

void check_low_battery_alert(int battery_pct) {
    if (battery_pct <= 10 && !low_battery_alert_sent) {
        String alert_msg = "LOW BATTERY: " + String(battery_pct) + "% remaining";

        if (queue_add_message(
            settings.default_capcode,
            settings.default_frequency,
            settings.default_txpower,
            false,
            alert_msg.c_str()
        )) {
            low_battery_alert_sent = true;
            logMessage("ALERT: Low battery warning queued (" + String(battery_pct) + "%)");
        }
    }
    else if (battery_pct > 15) {
        low_battery_alert_sent = false;
    }
}

void check_power_disconnect_alert(bool power_connected, bool charging_active) {
    static bool was_power_connected = true;
    static uint8_t disconnect_confirm_count = 0;

    if (was_power_connected && !power_connected) {
        disconnect_confirm_count++;
        if (disconnect_confirm_count >= 3 && !power_disconnect_alert_sent) {
            if (queue_add_message(
                settings.default_capcode,
                settings.default_frequency,
                settings.default_txpower,
                false,
                "POWER DISCONNECTED: Battery discharging"
            )) {
                power_disconnect_alert_sent = true;
                logMessage("ALERT: Power disconnect warning queued");
            }
        }
    } else if (power_connected) {
        disconnect_confirm_count = 0;
        power_disconnect_alert_sent = false;
        was_power_connected = true;
    } else if (!was_power_connected) {
        disconnect_confirm_count = 0;
    }
}

void loop() {

    unsigned long now = millis();

    switch (boot_phase) {
        case BOOT_INIT:
            break;

        case BOOT_NETWORK_PENDING:
            if (network_is_connected()) {
                boot_phase = BOOT_NETWORK_READY;
                logMessage("BOOT: Phase -> BOOT_NETWORK_READY");
            } else if (ap_mode_active) {
                logMessage("BOOT: AP-only mode - internet services disabled");
                setup_watchdog();
                boot_phase = BOOT_AP_COMPLETE;
                logMessage("BOOT: Phase -> BOOT_AP_COMPLETE");
            }
            break;

        case BOOT_NETWORK_READY:
            if (!ntp_synced) {
                if (!ntp_sync_in_progress) {
                    logMessage("BOOT: Starting NTP sync");
                    ntp_sync_start();
                }
                boot_phase = BOOT_NTP_SYNCING;
                logMessage("BOOT: Phase -> BOOT_NTP_SYNCING");
            } else {
                logMessage("BOOT: NTP already synced - starting watchdog");
                setup_watchdog();
                boot_phase = BOOT_WATCHDOG_ACTIVE;
                boot_phase_start = millis();
                if (settings.mqtt_boot_delay_ms > 0) {
                    logMessagef("BOOT: MQTT initialization in %lu seconds",
                                settings.mqtt_boot_delay_ms / 1000UL);
                } else {
                    logMessage("BOOT: MQTT initialization immediately");
                }
            }
            break;

        case BOOT_NTP_SYNCING:
            if (ntp_synced) {
                logMessage("BOOT: NTP synced, starting watchdog");
                setup_watchdog();
                boot_phase = BOOT_WATCHDOG_ACTIVE;
                boot_phase_start = millis();
                if (settings.mqtt_boot_delay_ms > 0) {
                    logMessagef("BOOT: MQTT initialization in %lu seconds",
                                settings.mqtt_boot_delay_ms / 1000UL);
                } else {
                    logMessage("BOOT: MQTT initialization immediately");
                }
            } else if (ntp_sync_attempts >= NTP_MAX_ATTEMPTS && !ntp_sync_in_progress) {
                logMessage("BOOT: NTP failed after all attempts, entering degraded mode");
                boot_phase = BOOT_NTP_FAILED;
                boot_phase_start = millis();
            }
            break;

        case BOOT_NTP_FAILED:
            if (millis() - boot_phase_start > 60000) {
                ntp_sync_start();
                boot_phase = BOOT_NTP_SYNCING;
            }
            break;

        case BOOT_WATCHDOG_ACTIVE:
            if ((unsigned long)(millis() - boot_phase_start) >= settings.mqtt_boot_delay_ms) {
                boot_phase = BOOT_MQTT_PENDING;
            }
            break;

        case BOOT_MQTT_PENDING:
            if (settings.mqtt_enabled && strlen(settings.mqtt_server) > 0) {
                mqtt_initialize();
            }
            boot_phase = BOOT_MQTT_READY;
            boot_phase_start = millis();
            break;

        case BOOT_MQTT_READY:
            if (millis() - boot_phase_start > 60000) {
                logMessage("BOOT: 60s delay complete, initializing IMAP/ChatGPT");
                boot_phase = BOOT_SERVICES_PENDING;
            }
            break;

        case BOOT_SERVICES_PENDING:
            if (imap_config.enabled && imap_config.account_count > 0) {
                init_imap_scheduler();
                logMessage("BOOT: IMAP scheduler initialized");
            } else {
                logMessage("BOOT: IMAP disabled, skipping");
            }
            if (chatgpt_config.enabled) {
                logMessage("BOOT: ChatGPT enabled, will start on schedule");
            } else {
                logMessage("BOOT: ChatGPT disabled");
            }
            boot_phase = BOOT_COMPLETE;
            mark_boot_success();
            logMessage("BOOT: All services initialized - boot complete");
            break;

        case BOOT_AP_COMPLETE:
            if (network_is_connected() && !ap_mode_active) {
                logMessage("BOOT: Network now available - upgrading to full services");
                boot_phase = BOOT_NETWORK_READY;
                boot_phase_start = millis();
            }
            break;

        case BOOT_COMPLETE:
            break;
    }


    bool guard_active = transmission_guard_active();
    if (!guard_active && (mqtt_deferred_ack_payload.length() > 0 || mqtt_deferred_status_payload.length() > 0)) {
        mqtt_flush_deferred();
    }

    if ((boot_phase >= BOOT_WATCHDOG_ACTIVE || boot_phase == BOOT_AP_COMPLETE) && !guard_active) {
        feed_watchdog();
        check_heap_health();
        at_process_serial();
    } else if (!guard_active) {

        check_heap_health();
        at_process_serial();
    }


    if (!guard_active) {
        if (network_connect_pending) {
            network_connect_pending = false;
            network_reconnect();
        }

        check_wifi_connection();

        if (wifi_connected) {
            if (WiFi.status() != WL_CONNECTED) {
                wifi_connected = false;
                wifi_retry_count = 0;
                logMessage("WIFI: Disconnected - connection health check failed");
                network_reconnect();
            } else {
                static unsigned long last_ip_check = 0;
                if ((unsigned long)(millis() - last_ip_check) > 300000) {
                    last_ip_check = millis();
                    if (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
                        wifi_connected = false;
                        wifi_retry_count = 0;
                        logMessage("WIFI: Invalid IP detected - triggering reconnection");
                        network_reconnect();
                    }
                }
            }
        }

        network_update_active_state();
        network_available_cached = wifi_connected;

        static unsigned long last_web_handle = 0;
        if (wifi_connected || ap_mode_active) {
            if ((unsigned long)(millis() - last_web_handle) >= 20) {
                webServer.handleClient();
                last_web_handle = millis();
            }
        }

        if (network_available_cached &&
            (millis() - last_ntp_sync) > NTP_SYNC_INTERVAL_MS && !ntp_sync_in_progress) {
            logMessage("NTP: Performing periodic sync (1 hour interval)");
            ntp_sync_start();
        }

        if (network_available_cached) {
            ntp_sync_process();
        }

        if (boot_phase >= BOOT_MQTT_READY &&
            settings.mqtt_enabled &&
            network_available_cached &&
            strlen(settings.mqtt_server) > 0) {
            if (!mqtt_initialized) {
                mqtt_initialize();
                mqtt_loop();
            } else {
                mqtt_loop();
            }
        }

        if (boot_phase >= BOOT_COMPLETE &&
            imap_config.enabled &&
            network_available_cached &&
            imap_config.account_count > 0) {
            imap_scheduler_loop();
        }

        if (boot_phase >= BOOT_COMPLETE &&
            network_available_cached &&
            ntp_synced) {
            unsigned long current_time = millis();
            if ((current_time - last_chatgpt_check) >= CHATGPT_CHECK_INTERVAL) {
                chatgpt_check_schedules();
                last_chatgpt_check = current_time;
            }
        }
    }

    if (!guard_active) {
        if (oled_active && (millis() - last_activity_time > OLED_TIMEOUT_MS)) {
            display_turn_off();
        }

        handle_factory_reset();

        static unsigned long last_battery_check = 0;
        if (millis() - last_battery_check > 60000) {
            uint16_t battery_voltage_mv;
            int battery_percentage_temp;
            getBatteryInfo(&battery_voltage_mv, &battery_percentage_temp);

            int adc_raw = analogRead(BATTERY_ADC_PIN);
            float battery_voltage_v = battery_voltage_mv / 1000.0;

            bool is_power_connected = last_power_connected;
            if (battery_first_check) {
                is_power_connected = (battery_voltage_v > 4.12);
            } else if (last_power_connected) {
                if (battery_voltage_v < 4.08) {
                    is_power_connected = false;
                }
            } else {
                if (battery_voltage_v > 4.12) {
                    is_power_connected = true;
                }
            }

            bool is_actively_charging = (battery_voltage_v > 4.20);
            int current_percent_bracket = battery_percentage_temp / 10;

            bool should_log = battery_first_check ||
                             (is_power_connected != last_power_connected) ||
                             (is_actively_charging != last_charging_active) ||
                             (current_percent_bracket != last_percent_bracket);

            if (should_log) {
                char battery_log[200];
                snprintf(battery_log, sizeof(battery_log),
                    "BATTERY: V=%dmV (%.2fV), ADC=%d, %%=%d, Power=%s, Charging=%s, Present=%s",
                    battery_voltage_mv, battery_voltage_v, adc_raw, battery_percentage_temp,
                    is_power_connected ? "Connected" : "Battery",
                    is_actively_charging ? "Yes" : "No",
                    battery_present ? "Yes" : "No");
                logMessage(battery_log);

                last_power_connected = is_power_connected;
                last_charging_active = is_actively_charging;
                last_percent_bracket = current_percent_bracket;
                battery_first_check = false;
            }

            if (oled_active) {
                display_status();
            }

            if (settings.enable_low_battery_alert && battery_present) {
                check_low_battery_alert(battery_percentage_temp);
            }

            if (settings.enable_power_disconnect_alert && battery_present && !battery_first_check) {
                check_power_disconnect_alert(is_power_connected, is_actively_charging);
            }

            last_battery_check = millis();
        }
    }

    if (!guard_active) {
        handle_led_heartbeat();
    }


    if (display_update_requested && !guard_active) {
        reset_oled_timeout();
        display_status();
        display_update_requested = false;
    }

    if (boot_phase >= BOOT_COMPLETE || boot_phase == BOOT_AP_COMPLETE) {
        check_transmission_task_health();
    }

    delay(1);
}
