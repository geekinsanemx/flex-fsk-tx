/*
 * FLEX Paging Message Transmitter - ESP32 FSK Transceiver
 * Web Server Module - HTTP server, REST API, web UI
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <functional>

// =============================================================================
// GLOBALS
// =============================================================================
extern WebServer webServer;

// =============================================================================
// FUNCTIONS - web_server.cpp
// =============================================================================
void web_server_init();
String get_html_header(String title);
String get_html_footer();
void send_chunked_html_response(const String& title, const std::function<void()>& body_generator);
bool authenticate_api_request();
void handle_root();
void handle_send_message();
String gsm_nav_tab_html(bool active);
String imap_nav_tab_html(bool active);
String chatgpt_nav_tab_html(bool active);

// =============================================================================
// FUNCTIONS - web_handlers_settings.cpp
// =============================================================================
void handle_configuration();
void handle_save_config();
void handle_flex_config();
void handle_save_flex();
void handle_mqtt();
void handle_save_mqtt();
void handle_imap_config();
void handle_imap_toggle();
void handle_imap_add();
void handle_imap_edit();
void handle_imap_delete();
void handle_imap_account_data();
void handle_imap_update();
void handle_api_config();
void handle_save_api();
void handle_gsm_config();
void handle_save_gsm();
void handle_gsm_status();

// =============================================================================
// FUNCTIONS - web_handlers_device.cpp
// =============================================================================
void handle_device_status();
void handle_logs();
void handle_download_logs();
void handle_web_factory_reset();
void handle_backup_settings();
void handle_restore_settings();
void handle_upload_restore();
void handle_file_upload();
void handle_upload_certificate();

// =============================================================================
// FUNCTIONS - web_handlers_api.cpp
// =============================================================================
void handle_api_message();
void handle_api_wifi_scan();
void handle_api_wifi_delete();
void handle_api_wifi_add();

// =============================================================================
// FUNCTIONS - web_handlers_chatgpt.cpp
// =============================================================================
void handle_chatgpt();
void handle_chatgpt_config();
void handle_chatgpt_notifications();
void handle_chatgpt_api_key();
void handle_chatgpt_add_prompt();
void handle_chatgpt_get_prompt();
void handle_chatgpt_edit_prompt();
void handle_chatgpt_toggle();
void handle_chatgpt_delete();

#endif // WEB_SERVER_H
