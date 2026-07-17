# Changelog

All notable changes to this firmware are recorded here. This file mirrors the
changelog embedded in [`version.h`](version.h), which remains the single
source of truth for `FIRMWARE_VERSION`.

## v3.8.67
MQTT counter reset fix + display UX: fixed `mqtt_connection_attempt` counter resetting on failure instead of success (display stuck at "[1]"), counter now properly increments across retry attempts and resets only when connection succeeds; display shows "Ready (M)" when MQTT connected (otherwise just "Ready"), removed "*" asterisk from all transport indicators (GSM/WiFi/AP) for cleaner display.

## v3.8.66
Log retention improved: doubled log file limits (`MAX_LOG_FILE_SIZE` 32KB→64KB, `LOG_TRUNCATE_SIZE` 8KB→32KB), added rotation notification message to Serial output, retains 4x more history before truncation.

## v3.8.65
MQTT transport crash fix: added `!mqtt_suspended` guard to `mqttClient.disconnect()` in `on_network_changed()`, prevents LoadProhibited crash when switching transports with MQTT suspended (client never connected), separated disconnect logic from counter reset (counter reset always executes, disconnect only when client valid).

## v3.8.64
MQTT retry limits extended + transport reset fix: changed limits from 3×3 to 5×5 (25 total attempts before suspension), `mqtt_reboot_count` now resets on transport switch (GSM↔WiFi) and `AT+RESET` command, fixes permanent MQTT suspension after transport change due to stale counter from previous transport.

## v3.8.63
MQTT unified failure counter: single `mqtt_failed_cycles` counter for both transports, single decision point in `mqtt_loop()`, transport-aware threshold (WiFi/GSM: 3 attempts→reboot), persistent reboot loop guard (3 reboots→suspend), removed `mqtt_gsm_attempt_counter`/`mqtt_gsm_max_attempts`, `mqtt_connect()` now only performs diagnostics and transport-specific cleanup (`gsm_reset_ssl_client`), all counter/reboot/suspend logic centralized in `mqtt_loop()`.

## v3.8.62
SPIFFS write hardening: added `file.flush()` before `file.close()` in all config write functions (`saveCertificateToSPIFFS`, `save_imap_config`, `save_runtime_settings`, `chatgpt_save_config`, restore handler); restore handler now checks `print()` return value and logs error on 0-byte write.

## v3.8.61
SPIFFS log size fix + log write buffer: corrected `MAX_LOG_FILE_SIZE` (256000→32768) and `LOG_TRUNCATE_SIZE` (51200→8192) to fit within 128KB SPIFFS partition (min_spiffs scheme); replaced per-message open/write/flush/close with 2KB RAM buffer; flushes to SPIFFS as single write when full (>=1536B) or every 1s via `loop()`.

## v3.8.60
Status page UX fix: lines count input now refreshes log display immediately when changed, works even with live logs disabled (calls `pollLogs()` after updating `linesToShow` variable).

## v3.8.59
RTC time integration: log timestamps now use `system_time_initialized` flag instead of `ntp_synced`, enables immediate timestamped logging when RTC provides time at boot (eliminates 0000-00-00 timestamps for RTC-equipped devices), boot sequence skips NTP blocking when RTC time valid (faster boot), MQTT/ChatGPT use `system_time_initialized` for time validation (works with RTC or NTP), periodic NTP still runs to verify/update RTC when network available.

## v3.8.58
Persistent SPIFFS log system: implemented `/serial.log` file (250KB limit, 50KB keep on rotate), single `logMessage()` function handles all logging (Serial + file + syslog), deleted old log buffers (12KB freed), pre-NTP timestamps show uptime (0000-00-00 HH:MM:SS), post-NTP show datetime (YYYY-MM-DD HH:MM:SS), `file.flush()` ensures writes committed to flash, `AT+LOGS?N` (default 25) and `AT+RMLOG` commands, status page displays 100 lines with auto-scroll to bottom, chronological order (oldest→newest), live logs toggle with configurable refresh interval (1-60s, default 5s).

## v3.8.57
ChatGPT JSON escape fix: added `json_escape_string()` for proper JSON encoding, `sanitize_chatgpt_prompt()` for input normalization (whitespace/newlines), prevents HTTP 400 errors from malformed JSON with special characters.

## v3.8.56
MQTT auto-retry & failure notifications: added auto-retry mechanism (configurable retry interval: 5-1440 minutes) replacing indefinite suspension, added failure notifications toggle (sends pager alert on suspension), added Recent Activity card with detailed event logging including datetime/frequency/capcode, dynamic transmission status tracking in activity log, shortened UI labels ("Boot Delay" and "Retry Time"), added `MQTTActivity` struct with comprehensive event tracking. ChatGPT consistency: renamed `notify_on_failure` → `chatgpt_notify_failures` for consistency with MQTT, changed default to failure notifications enabled, updated UI label to "Failure notifications". UI: added transmission status icons (✅/❌) for MQTT events with conditional display (only for events that transmit RF), backup/restore JSON support for all new fields.

## v3.8.55
Critical service isolation fix: removed `transmission_guard_active()` blocking from `webServer.handleClient()` and `mqtt_loop()` only — these critical services now continue processing during TX since transmission runs isolated on Core 0 with thread-safe queue, fixes HTTP/MQTT request dropping during transmission; IMAP and ChatGPT remain protected (non-critical, heavy SSL/HTTP operations can wait).

## v3.8.54
MQTT counter reset consistency: moved `mqtt_failed_cycles` reset from `mqtt_loop()` to `mqtt_connect()` on successful connection, both GSM (`mqtt_gsm_attempt_counter`) and WiFi (`mqtt_failed_cycles`) counters now reset in same location for full architectural consistency, `mqtt_connect()` now fully self-contained for all connection state management.

## v3.8.53
MQTT failure handling architectural fix: moved WiFi failure counter logic from `mqtt_loop()` to `mqtt_connect()` for architectural consistency, both WiFi (3 attempts→suspend) and GSM (5 attempts→restart) failure handling now centralized in `mqtt_connect()` where connection attempt occurs, `mqtt_loop()` simplified to only coordinate reconnection timing with backoff, cleaner separation of responsibilities (connection logic vs scheduling logic), removed unused `mqtt_transmit_suppressed` variable.

## v3.8.52
Manual network mode control: added `AT+NETWORK` command to force specific transport mode (AUTO/WIFI/GSM/AP) until reboot or manual change, disables automatic transport fallback when mode is locked, locked modes retry connection indefinitely with appropriate intervals (WiFi 60s, GSM 300s, AP no retries), display shows asterisk indicator (WiFi*, GSM*, AP*) when mode is locked, enables dedicated transport testing and prevents unwanted switching in field deployments.

## v3.8.51
Web JavaScript load order fix: moved `static_ip_script` (containing `onSSIDChange` function) to be sent before `network_section` HTML, eliminates "onSSIDChange is not defined" ReferenceError on first page load.

## v3.8.50
Web WiFi scan async fix: changed `/api/wifi/scan` to use async `WiFi.scanNetworks(true)` with `webServer.handleClient()` calls during wait, prevents blocking `loop()` and allows web interface to remain responsive during scan, fixes dropdown never updating issue.

## v3.8.49
Web WiFi scan error handling: added WiFi scan failure validation and recovery in `/api/wifi/scan` endpoint, handles `WIFI_SCAN_FAILED` errors same as `wifi_ssid_scan()`, fixes network dropdown not updating when scan fails.

## v3.8.48
MQTT transport switch order fix: call `network_update_active_state()` BEFORE `gsm_disconnect`/`power_off` during GSM→WiFi switch, ensures `mqttClient.disconnect()` executes while GSM socket still active, eliminates SSL write errors and BR_WRITE_ERROR on transport switch.

## v3.8.47
WiFi scan error handling: added validation and recovery for WiFi scan failures (`WIFI_SCAN_FAILED`/-2), reinitializes WiFi and retries once on failure, prevents infinite loop of failed scans when GSM is active, clearer error messages.

## v3.8.46
MQTT transport switch fix (critical): force `mqttClient.disconnect()` without checking `connected()` state (underlying transport change causes false negative), added 100ms delay after disconnect for socket cleanup, eliminates "Socket dropped unexpectedly" SSL warning.

## v3.8.45
MQTT transport switch fix: added `mqttClient.disconnect()` before SSL client cleanup during transport switch (WiFi↔GSM), prevents SSL BR_WRITE_ERROR and stale connection state causing reconnection failures.

## v3.8.44
MQTT display counter fix: fixed `mqtt_connection_attempt` counter never incrementing, display now shows retry count "MQTT [1]...", "MQTT [2]..." during reconnection attempts.

## v3.8.43
Periodic WiFi reconnect fix: added periodic `network_reconnect()` timer in loop (60s when GSM active, 30s when no network) to restore WiFi availability scanning, fixes issue where device stayed on GSM indefinitely without checking for WiFi; added `STATE_WIFI_CONNECTING` guard to prevent scan interference during WiFi connection.

## v3.8.42
Network fallback fix: WiFi connection failure now attempts GSM before falling back to AP mode (was going directly to AP), fixes issue where device entered AP mode despite GSM being enabled and available, proper fallback sequence: WiFi → GSM (all network modes) → AP_MODE.

## v3.8.41
WiFi transport switch fix: added missing transport switch logic to `check_wifi_connection()`, WiFi reconnection now properly powers off GSM modem and updates active network state, fixes webserver inaccessibility and stale GSM IP display when switching from GSM to WiFi. Display boot fix: network status line now shows correct transport during boot (WiFi vs GSM), uses `device_state` to determine active connection attempt instead of generic fallback logic. Display UX: simplified state messages by removing transport prefixes (WiFi/GSM) from State line, network line already shows explicit transport type, eliminates redundant information.

## v3.8.40
TX power validation fix: enforce 2 dBm minimum power (was 0), add radio init fallback to prevent halt on invalid power settings, fixes device halt on reboot when TX power set below hardware minimum.

## v3.8.39
FLEX capcode validation: implemented proper FLEX protocol capcode validation with gap detection, valid ranges: 1-1933312, 1998849-2031614, 2101249-4291000000, updated max from 4294967295 to 4291000000.

## v3.8.38
Boot initialization fix: disabled default watchdog before SPIFFS format, early display initialization shows "Initializing Device..." message during first boot, eliminates 60s watchdog errors and black screen.

## v3.8.37
Timezone dropdown improvement: replaced 41 generic options with 31 common timezones with descriptive names (cities/regions), removed rarely-used intermediate offsets, changed label to "Local Timezone".

## v3.8.36
WiFi config UX improvements: removed page refresh after adding network (dynamic dropdown update), added `onSSIDChange()` after WiFi scan completion/error to properly reset form fields.

## v3.8.35
Boot state machine refactor: renamed `BOOT_WIFI_*` to `BOOT_NETWORK_*` for transport-agnostic naming, added `BOOT_AP_COMPLETE` state to prevent NTP sync attempts in AP-only mode (no WiFi, no GSM, avoiding RTC corruption), restored WiFi health check with conditional `network_reconnect()` calls (fixes webserver slowness regression).

## v3.8.34
WiFi network UI fix: removed STORED:/SCANNED: prefixes causing networks not to connect after save, added blue Add button with validation (SSID, password, static IP fields), new `/api/wifi/add` endpoint for adding networks without restart, Save Configuration now triggers device restart.

## v3.8.33
External RF amplifier: complete implementation of configurable external RF amplifier control — configurable GPIO pin (default: TTGO GPIO32, Heltec GPIO22), stabilization delay (20-5000ms, default 200ms), polarity selection toggle (Active-High for NPN driver+P-MOSFET like 2N2222+IRF4905, Active-Low for direct P-MOSFET), enable/disable toggle in FLEX settings page with visual feedback (fields disabled/grayed when off), reserved pin validation (prevents selection of GPIO 0, LoRa pins CS/IRQ/RST/GPIO/SCK/MOSI/MISO, OLED pins SDA/SCL/RST, Battery ADC, LED, VEXT) with real-time UI error display and backend validation, GPIO activated before transmission with configurable delay for bias stabilization, deactivated after transmission complete, fully integrated in settings persistence (`save_settings`/`load_settings`/`config_to_json`/`json_to_config`) and factory defaults, board-specific pin assignments adapt at compile-time.

## v3.8.32
RF chip shutdown fix: added `radio.standby()` after transmission completes in Core 0 task, fixes RF chip staying in TX mode and transmitting continuous noise after first message, regression from commit ceffdb4 when transmission logic moved to Core 0 without migrating hardware state management.

## v3.8.31
Live clock display: added side-by-side clock cards showing Hardware Clock (UTC) and Local Time with live updates, client-side increment from device timestamp, updates on timezone change.

## v3.8.30
Timezone dropdown: replaced numeric input with dropdown menu covering UTC-12:00 to UTC+14:00 in 30-minute increments (41 options), supports half-hour timezones like India and Afghanistan.

## v3.8.29
Password visibility toggle: added eye icon button to password field for toggling between hidden and plaintext display, improves UX when entering WiFi passwords.

## v3.8.28
IP settings visibility optimization: hide IP/Netmask/Gateway/DNS fields when DHCP is enabled and network is not currently connected, show current DHCP values when connected, show editable fields only for static IP, eliminates confusing default/fake values (0.0.0.0, 192.168.1.100), cleaner form with less visual noise.

## v3.8.27
Network settings UX improvement: replaced input+datalist with traditional dropdown for SSID selection, integrated WiFi scan into dropdown ("Scan for networks..." option), added custom SSID input option, fixes issue where stored networks weren't visible when one was already selected.

## v3.8.26
Multiple WiFi network support: device now stores and scans for up to 10 WiFi networks, automatically connects to best available stored network, eliminates blind connection attempts, backup/restore format updated to support multiple networks, web UI manages first network.

## v3.8.25
GSM config validation: use hardware defaults when config contains 0 for GPIO pins (power/RX/TX), baudrate, or connection timeout — applies during both GSM initialization and web page display, ensures consistent fallback behavior across UI and runtime.

## v3.8.24
GSM web interface simplification: removed Runtime Status and GSM Modem Detection sections from GSM page, removed modem detection endpoint and related functions, Enable GSM toggle now works freely without detection requirement, cleaner user experience.

## v3.8.23
Display timeout fix (silent states): added silent states (IDLE, NTP_SYNC, IMAP_PROCESSING) to prevent background operations from waking display, only visible state changes (TX, MQTT, GSM, WiFi operations) wake display, improved state logging to show state names instead of numbers (e.g., "STATE: IDLE -> TRANSMITTING").

## v3.8.22
Display timeout fix (final): removed `reset_oled_timeout()` from battery monitoring (battery state is environmental not user activity), fixes display staying on due to noisy battery readings crossing power detection threshold every 60s, display now properly times out after 5min of actual user inactivity.

## v3.8.21
Display timeout fix: separated display content updates from timeout resets, removed `reset_oled_timeout()` from `display_status()` (architectural fix), display now properly turns off after 5min of no visible state changes, battery checks and periodic updates no longer prevent timeout.

## v3.8.20
GSM stability: fixed module name (SIM800L→A7670SA in logs), added 3s stabilization delay after GPRS connection before internet test (fixes first test failure), internet test now succeeds on first attempt.

## v3.8.19
GSM network mode fix (critical): corrected `AT+CNMP` values (38=LTE, 14=3G, 13=2G — was backwards!), moved network mode setting BEFORE GPRS connection (was after, completely ineffective), proper fallback sequence now works: LTE first → 3G → 2G.

## v3.8.18
GSM improvements: fixed IP parsing to remove trailing OK/newlines (clean display), added 1s delay between internet test attempts, implemented network mode fallback (LTE→3G→2G with 3 attempts each for maximum compatibility).

## v3.8.17
GSM power simplification: reverted to simple transistor-based power control (HIGH=ON, LOW=OFF) for N2222 hardware modification, removed all POWERKEY pulse logic, removed smart detection/auto-power-off, clean straightforward power management.

## v3.8.16
GSM A7670SA power management fix: WiFi boot now explicitly powers OFF modem if detected ON (fixes modem staying on when WiFi available), improved IP parsing to strip CID prefix (fixes "1,10.52.129.153" display issue, now shows clean "10.52.129.153").

## v3.8.15
GSM A7670SA power management: modem stays OFF until needed (power saving), smart initialization checks if modem already on and powers it OFF first, manual `AT+CGPADDR=1` IP parsing replaces broken `getLocalIP()` (fixes display showing raw AT commands), clean IP display format.

## v3.8.14
GSM A7670SA driver fix: changed from SIM800 to SIM7600 TinyGSM driver for proper AT command compatibility, implemented correct POWERKEY pulse-based power management (1s ON pulse, 1.5s OFF pulse), fixes GPRS connection failures and power control issues.

## v3.8.13
Network connection redesign: complete architectural rewrite of network connection logic with WiFi scanning before connection attempts, clean separation between boot and runtime reconnection, smart scan intervals (5min with GSM, 1min without), eliminates blind connection attempts and display wake-ups from reconnection spam, replaces convoluted `network_execute_connect()` with `network_boot()` + `network_reconnect()` functions.

## v3.8.12
Display auto-on: added `reset_oled_timeout()` to `display_status()` so display turns on automatically when status updates occur (state changes, MQTT events, GSM registration), display auto-off after 5min idle, improves visibility of system activity.

## v3.8.11
GSM operator name: added readable network name to operator log (queries `AT+COPS?` for alphanumeric name), displays as "Carrier Name (334020)" instead of just numeric code, improves GSM connection visibility.

## v3.8.10
Code cleanup: removed unused ESPping library include, removed misleading ChatGPT ping log message (no actual ping performed), renamed `last_ping_test` to `last_ip_check` (checks WiFi IP validity every 5min, not ping).

## v3.8.9
Transport switch optimization: removed 2s transport switch grace period and 5s post-NTP delay before GSM TLS handshake, eliminates unnecessary blocking delays during WiFi/GSM failover, allows immediate MQTT reconnection after transport switch completes.

## v3.8.7
GSM transport service control: disabled IMAP/ChatGPT services during GSM transport, services automatically suspended when GSM becomes active transport and re-enabled when WiFi restored, boot phase skips service initialization if GSM active.

## v3.8.6
Transport switch fix: removed unnecessary `ntp_synced=false` reset during transport switch (time doesn't break during 2s switch), added explicit SSL client cleanup (`gsm_reset_ssl_client`/`wifiClientSecure.stop`) to prevent SSL errors when switching transports, fixes infinite "MQTT: NTP sync required" loop and SSL connection drops during WiFi↔GSM failover.

## v3.8.5
MQTT GSM buffer size fix: reduced PubSubClient buffer from 2048 to 1024 bytes to prevent BR_WRITE_ERROR SSL handshake timeouts over GSM, empirical testing shows 1024 works reliably while 2048 causes heap fragmentation issues during BearSSL buffer allocation (v4 uses 2048 successfully suggesting different memory management pattern).

## v3.8.4
MQTT GSM critical ordering fix: moved 5s post-NTP delay and GPRS reconnection check to BEFORE certificate loading (was after), GSM modem needs time to stabilize before SSL initialization begins.

## v3.8.3
MQTT GSM architectural fix: replaced `use_gsm_transport` local variable (`active_network && gsm_connected`) with direct `active_network==NETWORK_GSM_ACTIVE` checks matching v4, eliminates double condition causing wrong code path.

## v3.8.2
MQTT GSM SSL sequence fix: moved `setTimeout()` call to after `setMutualAuthParams()` (was before `gsm_reset_ssl_client()`), fixes BR_WRITE_ERROR by preventing timeout reset during SSL client initialization.

## v3.8.1
MQTT GSM fix: reset MQTT client state on transport switch (disconnect client, reset NTP/MQTT flags, 2s grace period), prevents stale WiFi SSL context interfering with GSM TLS handshake.

## v3.8.0
v3.6.91 baseline with integrated GSM transport (SIM800L) and WiFi/GSM failover.

---

## flex-fsk-tx-v2 project restructuring

This project (`flex-fsk-tx-v2`) is a structural port of `flex-fsk-tx-v3.8_GSM`
(v3.8.67) from a single 13,911-line `.ino` sketch into one `.cpp`/`.h` pair
per subsystem. No behavior was changed — every function, global, and struct
was relocated as-is, with cross-module globals promoted to `extern` only
where genuinely used across translation units. See [README.md](README.md)
for the resulting file layout.
