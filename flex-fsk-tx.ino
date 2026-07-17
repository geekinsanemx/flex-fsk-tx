/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Entry point - setup()/loop() orchestration only
 */

#include <SPI.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <esp_task_wdt.h>
#include <WiFi.h>

#include "src/version.h"
#include "src/core/config.h"
#include "include/boards/boards.h"

#include "src/core/storage.h"
#include "src/core/logging.h"
#include "src/core/hardware.h"
#include "src/core/display.h"
#include "src/core/utils.h"

#include "src/network/wifi.h"
#include "src/network/ntp_time.h"
#include "src/network/gsm.h"
#include "src/network/network.h"
#include "src/services/mqtt.h"
#include "src/services/imap.h"
#include "src/services/chatgpt.h"
#include "src/services/grafana.h"
#include "src/protocol/flex_protocol.h"
#include "src/protocol/transmission.h"
#include "src/protocol/at_commands.h"
#include "src/web/web_server.h"

// =============================================================================
// GLOBALS (file-local, used only within loop()'s periodic battery check)
// =============================================================================
static bool last_power_connected = false;
static bool last_charging_active = false;
static int last_percent_bracket = -1;
static bool battery_first_check = true;

void setup() {
    Serial.setTxBufferSize(1024);
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
    append_to_log_file("SYSTEM: Device boot started");
    logMessage("SYSTEM: SPIFFS initialized successfully");

    load_core_config();
    append_to_log_file("SYSTEM: Core config loaded");
    load_runtime_settings();
    append_to_log_file("SYSTEM: Runtime settings loaded");

#ifdef ENABLE_CHATGPT
    chatgpt_load_config();
#endif // ENABLE_CHATGPT

#ifdef ENABLE_IMAP
    load_imap_config();
#endif // ENABLE_IMAP

    current_tx_frequency = settings.default_frequency;

    if (settings.default_txpower >= 2.0 && settings.default_txpower <= 20.0) {
        tx_power = settings.default_txpower;
    } else {
        tx_power = TX_POWER_DEFAULT;
        settings.default_txpower = TX_POWER_DEFAULT;
    }

    display_setup();
    display_status();

    pinMode(LED_PIN, OUTPUT);
    LED_OFF();

    rfamp_init();

    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);

#ifdef ENABLE_GSM
    if (gsm_config.enable_gsm) {
        pinMode(gsm_config.power_pin, OUTPUT);
        digitalWrite(gsm_config.power_pin, LOW);
        SerialGSM.begin(gsm_config.baudrate, SERIAL_8N1, gsm_config.tx_pin, gsm_config.rx_pin);
        logMessagef("GSM: Serial initialized (TX=%u RX=%u @ %lu baud)",
                    gsm_config.tx_pin,
                    gsm_config.rx_pin,
                    (unsigned long)gsm_config.baudrate);
    }
#endif

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
        logMessagef("RADIO: Init failed with power=%.1f dBm (error=%d), trying fallback to %d dBm",
                    tx_power, radio_init_state, TX_POWER_DEFAULT);

        tx_power = TX_POWER_DEFAULT;
        settings.default_txpower = TX_POWER_DEFAULT;
        save_runtime_settings();

        radio_init_state = radio.beginFSK(corrected_init_freq,
                                         TX_BITRATE,
                                         TX_DEVIATION,
                                         RX_BANDWIDTH,
                                         tx_power,
                                         PREAMBLE_LENGTH,
                                         false);

        if (radio_init_state != RADIOLIB_ERR_NONE) {
            logMessagef("RADIO: Fallback init also failed (error=%d)", radio_init_state);
            panic();
        } else {
            logMessage("RADIO: Fallback init successful, device recovered");
        }
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

    web_server_init();

    // Boot failure detection
    check_boot_failure_history();

    // MQTT reboot limit check
    load_mqtt_reboot_count();
    if (mqtt_reboot_count >= MQTT_MAX_REBOOTS) {
        mqtt_suspended = true;
        mqtt_next_retry_time = millis();
        logMessagef("BOOT: MQTT reboot limit (%u) reached, MQTT starting suspended", MQTT_MAX_REBOOTS);
    }

    // Initialize Core 0 transmission task
    init_transmission_core();

    // Set initial boot phase
    boot_phase = BOOT_NETWORK_PENDING;
    boot_phase_start = millis();

    String startup_msg = "STARTUP: FLEX Paging Message Transmitter " + String(FIRMWARE_VERSION);
    logMessage(startup_msg.c_str());

    Serial.print("AT READY\r\n");
    Serial.print("WIFI ENABLED\r\n");
    Serial.flush();

    logMessage("STARTUP: Boot sequence started (staged initialization)");
    logMessage("STARTUP: Phase -> BOOT_NETWORK_PENDING");
}

void loop() {

    unsigned long now = millis();
    bool wifi_runtime_enabled = (network_mode == NETWORK_MODE_AUTO || network_mode == NETWORK_MODE_WIFI);
#ifdef ENABLE_GSM
    bool gsm_runtime_enabled = (network_mode == NETWORK_MODE_AUTO || network_mode == NETWORK_MODE_GSM) && gsm_config.enable_gsm;
#endif

    flush_log_buffer_if_due();

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
            if (!system_time_initialized) {
                if (!ntp_sync_in_progress) {
                    logMessage("BOOT: Starting NTP sync");
                    ntp_sync_start();
                }
                boot_phase = BOOT_NTP_SYNCING;
                logMessage("BOOT: Phase -> BOOT_NTP_SYNCING");
            } else {
                logMessage("BOOT: Time already initialized (RTC) - starting watchdog");
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
#ifdef ENABLE_GSM
            if (active_network == NETWORK_GSM_ACTIVE) {
                logMessage("BOOT: GSM transport active - IMAP/ChatGPT disabled ");
            } else {
#endif
#ifdef ENABLE_IMAP
                if (imap_config.enabled && imap_config.account_count > 0) {
                    init_imap_scheduler();
                    logMessage("BOOT: IMAP scheduler initialized");
                } else {
                    logMessage("BOOT: IMAP disabled, skipping");
                }
#endif // ENABLE_IMAP
#ifdef ENABLE_CHATGPT
                if (chatgpt_config.enabled) {
                    logMessage("BOOT: ChatGPT enabled, will start on schedule");
                } else {
                    logMessage("BOOT: ChatGPT disabled");
                }
#endif // ENABLE_CHATGPT
#ifdef ENABLE_GSM
            }
#endif
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

        if (wifi_runtime_enabled) {
            check_wifi_connection();
        }

#ifdef ENABLE_GSM
        if (gsm_runtime_enabled) {
            check_gsm_connection();
        }
#endif

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
        } else if (!ap_mode_active && !wifi_connected) {
            static unsigned long last_reconnect_check = 0;
#ifdef ENABLE_GSM
            unsigned long check_interval = gsm_connected ? 60000UL : 30000UL;
#else
            unsigned long check_interval = 30000UL;
#endif

            if ((unsigned long)(millis() - last_reconnect_check) > check_interval) {
                last_reconnect_check = millis();
                network_reconnect();
            }
        }

        network_update_active_state();
#ifdef ENABLE_GSM
        network_available_cached = wifi_connected || (gsm_connected && gsm_internet_verified);
#else
        network_available_cached = wifi_connected;
#endif

        if (network_available_cached &&
            (millis() - last_ntp_sync) > NTP_SYNC_INTERVAL_MS && !ntp_sync_in_progress) {
            logMessage("NTP: Performing periodic sync (1 hour interval)");
            ntp_sync_start();
        }

        if (network_available_cached) {
            ntp_sync_process();
        }
    }

    static unsigned long last_web_handle = 0;
    if (wifi_connected || ap_mode_active) {
        if ((unsigned long)(millis() - last_web_handle) >= 20) {
            webServer.handleClient();
            last_web_handle = millis();
        }
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

    if (!guard_active) {
#ifdef ENABLE_IMAP
        if (boot_phase >= BOOT_COMPLETE &&
            imap_config.enabled &&
            network_available_cached &&
            imap_config.account_count > 0 &&
            !imap_suspended_on_gsm) {
            imap_scheduler_loop();
        }
#endif // ENABLE_IMAP

#ifdef ENABLE_CHATGPT
        if (boot_phase >= BOOT_COMPLETE &&
            network_available_cached &&
            ntp_synced &&
            !chatgpt_suspended_on_gsm) {
            unsigned long current_time = millis();
            if ((current_time - last_chatgpt_check) >= CHATGPT_CHECK_INTERVAL) {
                chatgpt_check_schedules();
                last_chatgpt_check = current_time;
            }
        }
#endif // ENABLE_CHATGPT

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
