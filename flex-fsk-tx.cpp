/*
 * flex-fsk-tx: Send FLEX packets over serial using AT commands.
 * Enhanced version with v3 firmware support and configuration capabilities.
 *
 * Original send_ttgo application Written by Davidson Francis (aka Theldus) and Rodrigo Laneth - 2025.
 * > https://github.com/Theldus/tinyflex
 * > https://github.com/rlaneth/ttgo-fsk-tx
 * send_ttgo adaptation for heltec v3 and standarized AT command renamed to flex-fsk-tx @ geekinsanemx
 * > https://github.com/geekinsanemx/flex-fsk-tx
 * Enhanced with remote encoding and v3 firmware support by GeekInsaneMX - 2025.
 *
 * Features:
 * - Local encoding (v1): Host-side FLEX encoding using tinyflex
 * - Remote encoding (v2): Device-side FLEX encoding via AT+MSG
 * - v3 firmware support: WiFi configuration and advanced AT commands
 * - Interactive configuration wizard via --config/-c flag
 * - Complete AT command protocol implementation
 *
 * This is free and unencumbered software released into the public domain.
 */

// System includes
#define _DEFAULT_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// Project includes
#include "include/tinyflex/tinyflex.h"

// =============================================================================
// CONSTANTS AND CONFIGURATION
// =============================================================================

// Default serial parameters
#define DEFAULT_DEVICE    "/dev/ttyUSB0"
#define DEFAULT_BAUDRATE  115200
#define DEFAULT_FREQUENCY 916.0
#define DEFAULT_POWER     2

// AT Protocol constants
#define AT_BUFFER_SIZE       1024
#define AT_TIMEOUT_MS        8000
#define AT_MAX_RETRIES       5
#define AT_INTER_CMD_DELAY   200
#define AT_DATA_SEND_TIMEOUT 20000
#define AT_MSG_SEND_TIMEOUT  35000  // 35 seconds for remote encoding

// =============================================================================
// TYPE DEFINITIONS
// =============================================================================

// Serial configuration structure
struct serial_config {
    double frequency;
    int baudrate;
    const char *device;
    int power;
};

// Device configuration structure for comprehensive AT command support
struct device_config {
    // Radio parameters
    double frequency;
    int power;
    int mail_drop;
    
    // Default FLEX settings (stored in EEPROM)
    uint64_t default_capcode;
    double default_frequency;
    int default_power;
    
    // WiFi configuration
    char wifi_ssid[64];
    char wifi_password[64];
    int wifi_enabled;
    int use_dhcp;
    char static_ip[16];
    char static_mask[16];
    char static_gateway[16];
    char static_dns[16];
    
    // API configuration
    int api_port;
    char api_username[33];
    char api_password[65];
    
    // Device settings
    char banner_message[17];
    
    // Status information
    char device_status[32];
    char wifi_status[64];
    char battery_info[32];
};

// AT Protocol response types
typedef enum {
    AT_RESP_OK,
    AT_RESP_ERROR,
    AT_RESP_DATA,
    AT_RESP_TIMEOUT,
    AT_RESP_INVALID
} at_response_t;

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

static int loop_enabled = 0;
static int mail_drop_enabled = 0;
static int remote_encoding = 0;
static int config_mode = 0;
static int reset_mode = 0;
static int help_mode = 0;
static int show_help_and_exit = 0;
static int silent_mode = 0;  // Flag to suppress AT command debug output
static struct device_config device_cfg = {};

// Forward declarations for configuration functions
static void collect_wifi_configuration(void);
static void collect_api_configuration(void);
static void collect_device_configuration(void);
static void collect_default_configuration(void);
static void display_configuration_summary(void);
static int apply_wifi_configuration(int fd);
static int apply_api_configuration(int fd);
static int apply_device_configuration(int fd);
static int apply_default_configuration(int fd);

// TTY restoration globals
static struct termios orig_tty;
static int tty_saved = 0;
static int serial_fd = -1;

// Error messages
static const char *msg_errors[] = {
    "Invalid provided error pointer",
    "Invalid message buffer",
    "Invalid provided capcode",
    "Invalid provided flex buffer"
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * Safe string-to-int routine that takes into account:
 * - Overflow and Underflow
 * - No undefined behaviour
 */
static int str2int(int *out, char *s)
{
    char *end;
    if (s[0] == '\0')
        return (-1);
    errno = 0;

    long l = strtol(s, &end, 10);

    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return (-1);
    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return (-1);
    if (*end != '\0')
        return (-1);

    *out = l;
    return (0);
}

/**
 * @brief Safe string-to-uint64_t routine.
 */
static int str2uint64(uint64_t *out, const char *s)
{
    char *end;
    unsigned long ul;
    const char *p = s;

    if (p[0] == '\0')
        return -1;

    errno = 0;
    ul = strtoull(p, &end, 10);

    if (end == p || errno == ERANGE || *end != '\0' || ul > UINT64_MAX)
        return -1;

    *out = (uint64_t)ul;
    return 0;
}

// =============================================================================
// SERIAL COMMUNICATION FUNCTIONS
// =============================================================================

/**
 * @brief Configures the serial port with the specified baudrate.
 */
static int configure_serial(int fd, int baudrate)
{
    struct termios tty;
    speed_t speed;

    if (tcgetattr(fd, &orig_tty) != 0) {
        perror("tcgetattr");
        return -1;
    }
    tty_saved = 1;
    serial_fd = fd;
    tty = orig_tty;

    // Convert baudrate to speed_t
    switch (baudrate) {
    case 9600:   speed = B9600;   break;
    case 19200:  speed = B19200;  break;
    case 38400:  speed = B38400;  break;
    case 57600:  speed = B57600;  break;
    case 115200: speed = B115200; break;
    case 230400: speed = B230400; break;
    default:
        fprintf(stderr, "Unsupported baudrate: %d\n", baudrate);
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    cfmakeraw(&tty);

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;  // Reduced from 10 to 5 (500ms)
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

/**
 * @brief Restores original TTY settings if they were saved.
 */
static void restore_tty(void)
{
    if (tty_saved && serial_fd >= 0) {
        tcsetattr(serial_fd, TCSANOW, &orig_tty);
        tty_saved = 0;
    }
}

/**
 * @brief Flush serial buffers completely.
 */
static void flush_serial_buffers(int fd)
{
    tcflush(fd, TCIOFLUSH);
    usleep(100000); // 100ms delay to ensure buffers are clear

    // Read any remaining data
    char dummy[256];
    int attempts = 0;
    while (attempts < 10) {
        ssize_t bytes = read(fd, dummy, sizeof(dummy));
        if (bytes <= 0) break;
        attempts++;
        usleep(10000); // 10ms between attempts
    }
}

// =============================================================================
// AT COMMAND PROTOCOL FUNCTIONS
// =============================================================================

/**
 * @brief Send AT command with proper flushing.
 */
static int at_send_command(int fd, const char *command)
{
    if (!silent_mode) {
        printf("Sending: %s", command);
    }

    if (write(fd, command, strlen(command)) < 0) {
        perror("write");
        return -1;
    }
    tcdrain(fd);
    usleep(AT_INTER_CMD_DELAY * 1000); // Convert ms to us

    return 0;
}

/**
 * @brief Read AT response with improved parsing and timeout handling.
 */
static at_response_t at_read_response(int fd, char *buffer, size_t buffer_size,
                                     char *data_buffer, size_t data_buffer_size)
{
    (void)data_buffer_size;

    char line_buffer[AT_BUFFER_SIZE];
    size_t line_pos = 0;
    int total_timeout = AT_TIMEOUT_MS;
    bool got_response = false;
    int empty_reads = 0;

    if (buffer) buffer[0] = '\0';
    if (data_buffer) data_buffer[0] = '\0';

    while (total_timeout > 0 && empty_reads < 20) {
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;

        int poll_result = poll(&pfd, 1, 50); // Shorter poll interval

        if (poll_result < 0) {
            perror("poll");
            return AT_RESP_INVALID;
        }

        if (poll_result == 0) {
            total_timeout -= 50;
            empty_reads++;
            continue;
        }

        if (!(pfd.revents & POLLIN)) {
            total_timeout -= 50;
            continue;
        }

        char c;
        ssize_t bytes_read = read(fd, &c, 1);

        if (bytes_read < 0) {
            perror("read");
            return AT_RESP_INVALID;
        }

        if (bytes_read == 0) {
            total_timeout -= 50;
            empty_reads++;
            continue;
        }

        empty_reads = 0; // Reset empty read counter

        // Skip carriage returns
        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            // End of line
            line_buffer[line_pos] = '\0';

            // Skip empty lines
            if (line_pos == 0) {
                continue;
            }

            if (!silent_mode) {
                printf("Received: '%s'\n", line_buffer);
            }

            // Check for responses
            if (strcmp(line_buffer, "OK") == 0) {
                return AT_RESP_OK;
            }
            else if (strcmp(line_buffer, "ERROR") == 0) {
                return AT_RESP_ERROR;
            }
            else if (strncmp(line_buffer, "+", 1) == 0) {
                // Data response
                if (buffer && strlen(line_buffer) < buffer_size) {
                    strcpy(buffer, line_buffer);
                    got_response = true;
                }
                // Continue reading to get OK/ERROR
            }
            else if (strstr(line_buffer, "DEBUG:") != NULL) {
                // Debug message from device
                printf("Device debug: %s\n", line_buffer);
            }
            else if (strstr(line_buffer, "AT READY") != NULL) {
                // Device ready message
                printf("Device ready message: %s\n", line_buffer);
            }

            // Reset line buffer
            line_pos = 0;
        }
        else if (line_pos < sizeof(line_buffer) - 1 && c >= 32 && c <= 126) {
            // Printable character
            line_buffer[line_pos++] = c;
        }
        else if (c < 32 && c != '\r' && c != '\n') {
            // Non-printable character (except CR/LF) - reset line
            if (line_pos > 0) {
                printf("Warning: Non-printable character 0x%02X in response, resetting line\n", (unsigned char)c);
                line_pos = 0;
            }
        }

        // Reset timeout on successful character read
        total_timeout = AT_TIMEOUT_MS;
    }

    if (got_response) {
        return AT_RESP_DATA;
    }

    return AT_RESP_TIMEOUT;
}

/**
 * @brief Ensure device is ready by sending AT command before any other command.
 */
static int at_ensure_device_ready(int fd)
{
    static time_t last_at_time = 0;
    time_t current_time = time(NULL);
    char response[AT_BUFFER_SIZE];
    
    // Send AT command if it's been more than 5 seconds since last AT, or if never sent
    if (current_time - last_at_time > 5 || last_at_time == 0) {
        if (!silent_mode) {
            printf("Ensuring device is ready with AT command...\n");
        }
        flush_serial_buffers(fd);
        usleep(100000); // 100ms
        
        if (at_send_command(fd, "AT\r\n") < 0) {
            return -1;
        }
        
        at_response_t result = at_read_response(fd, response, sizeof(response), NULL, 0);
        if (result != AT_RESP_OK) {
            printf("Device not ready, AT command failed\n");
            return -1;
        }
        
        last_at_time = current_time;
        usleep(200000); // 200ms delay after AT command
    }
    
    return 0;
}

/**
 * @brief Send AT command and wait for response with enhanced retry logic.
 */
static int at_execute_command(int fd, const char *command, char *response,
                             size_t response_size)
{
    int retries = AT_MAX_RETRIES;

    // Always ensure device is ready before sending any AT command
    if (strncmp(command, "AT\r\n", 4) != 0) { // Don't send AT before AT
        if (at_ensure_device_ready(fd) < 0) {
            printf("Failed to ensure device readiness before command: %s", command);
        }
    }

    while (retries-- > 0) {
        // Clear buffers before sending command
        flush_serial_buffers(fd);

        if (at_send_command(fd, command) < 0) {
            return -1;
        }

        at_response_t result = at_read_response(fd, response, response_size, NULL, 0);

        switch (result) {
        case AT_RESP_OK:
            return 0;
        case AT_RESP_ERROR:
            fprintf(stderr, "AT command failed: %s", command);
            if (retries > 0) {
                printf("Retrying command (%d attempts left)...\n", retries);
                usleep(500000); // 500ms delay between retries
                continue;
            }
            return -1;
        case AT_RESP_TIMEOUT:
            fprintf(stderr, "AT command timeout: %s", command);
            if (retries > 0) {
                printf("Retrying command due to timeout (%d attempts left)...\n", retries);
                // Send AT command to reset device state
                flush_serial_buffers(fd);
                at_send_command(fd, "AT\r\n");
                usleep(200000);
                at_read_response(fd, response, response_size, NULL, 0);
                usleep(500000);
                continue;
            }
            return -1;
        case AT_RESP_INVALID:
            fprintf(stderr, "AT communication error: %s", command);
            if (retries > 0) {
                printf("Retrying command due to communication error (%d attempts left)...\n", retries);
                usleep(1000000); // 1 second delay for communication errors
                continue;
            }
            return -1;
        default:
            return -1;
        }
    }

    return -1;
}

/**
 * @brief Initialize device with AT commands and enhanced error recovery.
 */
static int at_initialize_device(int fd)
{
    char response[AT_BUFFER_SIZE];

    printf("Testing device communication...\n");

    // Give device time to boot up
    flush_serial_buffers(fd);
    usleep(1000000); // 1 second

    // Try to establish communication
    for (int i = 0; i < 10; i++) {
        printf("Communication attempt %d/10...\n", i + 1);

        // Clear buffers thoroughly
        flush_serial_buffers(fd);
        usleep(200000); // 200ms

        // Send basic AT command
        if (at_execute_command(fd, "AT\r\n", response, sizeof(response)) == 0) {
            printf("Device communication established\n");

            // Send one more AT command to ensure stability
            usleep(200000);
            if (at_execute_command(fd, "AT\r\n", response, sizeof(response)) == 0) {
                printf("Device communication confirmed stable\n");
                return 0;
            }
        }

        // Progressive delay between attempts
        usleep(500000 * (i + 1)); // 500ms, 1s, 1.5s, etc.
    }

    fprintf(stderr, "Failed to establish communication after 10 attempts\n");
    return -1;
}

// =============================================================================
// COMPREHENSIVE AT COMMAND SUPPORT FUNCTIONS
// =============================================================================

/**
 * @brief Query device status using AT+STATUS?
 */
static int at_query_status(int fd, char *status, size_t status_size)
{
    char response[256] = {0};
    char command[] = "AT+STATUS?\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *status_start = strstr(response, "+STATUS: ");
        if (status_start) {
            status_start += 9; // Skip "+STATUS: "
            char *end = strchr(status_start, '\r');
            if (end) *end = '\0';
            strncpy(status, status_start, status_size - 1);
            status[status_size - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Set device frequency using AT+FREQ=
 */

/**
 * @brief Query device frequency using AT+FREQ?
 */
static int at_query_frequency(int fd, double *frequency)
{
    char response[256] = {0};
    char command[] = "AT+FREQ?\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *freq_start = strstr(response, "+FREQ: ");
        if (freq_start) {
            freq_start += 7; // Skip "+FREQ: "
            *frequency = atof(freq_start);
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Set device power using AT+POWER=
 */

/**
 * @brief Query device power using AT+POWER?
 */
static int at_query_power(int fd, int *power)
{
    char response[256] = {0};
    char command[] = "AT+POWER?\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *power_start = strstr(response, "+POWER: ");
        if (power_start) {
            power_start += 8; // Skip "+POWER: "
            *power = atoi(power_start);
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Set mail drop flag using AT+MAILDROP=
 */


/**
 * @brief Configure WiFi using AT+WIFI=
 */
static int at_set_wifi(int fd, const char *ssid, const char *password)
{
    char command[256];
    snprintf(command, sizeof(command), "AT+WIFI=%s,%s\r\n", ssid, password);
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Query WiFi status using AT+WIFI?
 */
static int at_query_wifi(int fd, char *status, size_t status_size)
{
    char response[256] = {0};
    char command[] = "AT+WIFI?\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *wifi_start = strstr(response, "+WIFI: ");
        if (wifi_start) {
            wifi_start += 7; // Skip "+WIFI: "
            char *end = strchr(wifi_start, '\r');
            if (end) *end = '\0';
            strncpy(status, wifi_start, status_size - 1);
            status[status_size - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Set WiFi enable/disable using AT+WIFIENABLE=
 */
static int at_set_wifi_enable(int fd, int enabled)
{
    char command[64];
    snprintf(command, sizeof(command), "AT+WIFIENABLE=%d\r\n", enabled);
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Query WiFi enable status using AT+WIFIENABLE?
 */
static int at_query_wifi_enable(int fd, int *enabled)
{
    char response[256] = {0};
    char command[] = "AT+WIFIENABLE?\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *enable_start = strstr(response, "+WIFIENABLE: ");
        if (enable_start) {
            enable_start += 13; // Skip "+WIFIENABLE: "
            *enabled = atoi(enable_start);
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Set banner message using AT+BANNER=
 */
static int at_set_banner(int fd, const char *banner)
{
    char command[128];
    snprintf(command, sizeof(command), "AT+BANNER=%s\r\n", banner);
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Query banner message using AT+BANNER?
 */
static int at_query_banner(int fd, char *banner, size_t banner_size)
{
    char response[256] = {0};
    char command[] = "AT+BANNER?\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *banner_start = strstr(response, "+BANNER: ");
        if (banner_start) {
            banner_start += 9; // Skip "+BANNER: "
            char *end = strchr(banner_start, '\r');
            if (end) *end = '\0';
            strncpy(banner, banner_start, banner_size - 1);
            banner[banner_size - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Set API port using AT+APIPORT=
 */
static int at_set_api_port(int fd, int port)
{
    char command[64];
    snprintf(command, sizeof(command), "AT+APIPORT=%d\r\n", port);
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Query API port using AT+APIPORT?
 */
static int at_query_api_port(int fd, int *port)
{
    char response[256] = {0};
    char command[] = "AT+APIPORT?\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *port_start = strstr(response, "+APIPORT: ");
        if (port_start) {
            port_start += 10; // Skip "+APIPORT: "
            *port = atoi(port_start);
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Set API username using AT+APIUSER=
 */
static int at_set_api_username(int fd, const char *username)
{
    char command[128];
    snprintf(command, sizeof(command), "AT+APIUSER=%s\r\n", username);
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Query API username using AT+APIUSER?
 */
static int at_query_api_username(int fd, char *username, size_t username_size)
{
    char response[256] = {0};
    char command[] = "AT+APIUSER?\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *user_start = strstr(response, "+APIUSER: ");
        if (user_start) {
            user_start += 10; // Skip "+APIUSER: "
            char *end = strchr(user_start, '\r');
            if (end) *end = '\0';
            strncpy(username, user_start, username_size - 1);
            username[username_size - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Set API password using AT+APIPASS=
 */
static int at_set_api_password(int fd, const char *password)
{
    char command[128];
    snprintf(command, sizeof(command), "AT+APIPASS=%s\r\n", password);
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Query battery information using AT+BATTERY?
 */
static int at_query_battery(int fd, char *battery_info, size_t info_size)
{
    char response[256] = {0};
    char command[] = "AT+BATTERY?\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *battery_start = strstr(response, "+BATTERY: ");
        if (battery_start) {
            battery_start += 10; // Skip "+BATTERY: "
            char *end = strchr(battery_start, '\r');
            if (end) *end = '\0';
            strncpy(battery_info, battery_start, info_size - 1);
            battery_info[info_size - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Save configuration to EEPROM using AT+SAVE
 */
static int at_save_config(int fd)
{
    char command[] = "AT+SAVE\r\n";
    return at_execute_command(fd, command, NULL, 0);
}


/**
 * @brief Reset device using AT+RESET
 */
static int at_reset_device(int fd)
{
    char command[] = "AT+RESET\r\n";
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Set default capcode using AT+SETDEFAULT=CAPCODE,value
 */
static int at_set_default_capcode(int fd, uint64_t capcode)
{
    char command[64];
    snprintf(command, sizeof(command), "AT+SETDEFAULT=CAPCODE,%lu\r\n", capcode);
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Set default frequency using AT+SETDEFAULT=FREQUENCY,value
 */
static int at_set_default_frequency(int fd, double frequency)
{
    char command[64];
    snprintf(command, sizeof(command), "AT+SETDEFAULT=FREQUENCY,%.4f\r\n", frequency);
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Set default power using AT+SETDEFAULT=POWER,value
 */
static int at_set_default_power(int fd, int power)
{
    char command[64];
    snprintf(command, sizeof(command), "AT+SETDEFAULT=POWER,%d\r\n", power);
    return at_execute_command(fd, command, NULL, 0);
}

/**
 * @brief Get default capcode using AT+GETDEFAULT=CAPCODE
 */
static int at_get_default_capcode(int fd, uint64_t *capcode)
{
    char response[256] = {0};
    char command[] = "AT+GETDEFAULT=CAPCODE\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *capcode_start = strstr(response, "+GETDEFAULT_CAPCODE: ");
        if (capcode_start) {
            capcode_start += 21; // Skip "+GETDEFAULT_CAPCODE: "
            *capcode = strtoull(capcode_start, NULL, 10);
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Get default frequency using AT+GETDEFAULT=FREQUENCY
 */
static int at_get_default_frequency(int fd, double *frequency)
{
    char response[256] = {0};
    char command[] = "AT+GETDEFAULT=FREQUENCY\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *freq_start = strstr(response, "+GETDEFAULT_FREQUENCY: ");
        if (freq_start) {
            freq_start += 23; // Skip "+GETDEFAULT_FREQUENCY: "
            *frequency = atof(freq_start);
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Get default power using AT+GETDEFAULT=POWER
 */
static int at_get_default_power(int fd, int *power)
{
    char response[256] = {0};
    char command[] = "AT+GETDEFAULT=POWER\r\n";
    
    if (at_execute_command(fd, command, response, sizeof(response)) == 0) {
        char *power_start = strstr(response, "+GETDEFAULT_POWER: ");
        if (power_start) {
            power_start += 19; // Skip "+GETDEFAULT_POWER: "
            *power = atoi(power_start);
            return 0;
        }
    }
    return -1;
}


// =============================================================================
// FLEX MESSAGE TRANSMISSION FUNCTIONS
// =============================================================================

/**
 * @brief Send flex message using remote encoding (AT+MSG command).
 */
static int at_send_flex_message_remote(int fd, struct serial_config *config,
                                      uint64_t capcode, const char *message)
{
    char command[128];
    char response[AT_BUFFER_SIZE];
    int send_retries = 3;

    printf("\nConfiguring radio parameters...\n");

    // Set frequency with retries
    snprintf(command, sizeof(command), "AT+FREQ=%.4f\r\n", config->frequency);
    if (at_execute_command(fd, command, response, sizeof(response)) < 0) {
        fprintf(stderr, "Failed to set frequency after all retries\n");
        return -1;
    }

    // Set power with retries
    snprintf(command, sizeof(command), "AT+POWER=%d\r\n", config->power);
    if (at_execute_command(fd, command, response, sizeof(response)) < 0) {
        fprintf(stderr, "Failed to set power after all retries\n");
        return -1;
    }

    // Set mail drop if enabled
    if (mail_drop_enabled) {
        snprintf(command, sizeof(command), "AT+MAILDROP=1\r\n");
        if (at_execute_command(fd, command, response, sizeof(response)) < 0) {
            fprintf(stderr, "Failed to set mail drop flag\n");
            return -1;
        }
    }

    printf("Radio configured successfully.\n");

    // Try to send the message with retries
    while (send_retries-- > 0) {
        printf("\nAttempting remote encoding and transmission (attempt %d/3)...\n", 3 - send_retries);

        // Reset device state before attempting to send
        printf("Resetting device state...\n");
        flush_serial_buffers(fd);
        if (at_execute_command(fd, "AT\r\n", response, sizeof(response)) < 0) {
            printf("Failed to reset device state, continuing anyway...\n");
        }

        // Send the MSG command
        snprintf(command, sizeof(command), "AT+MSG=%" PRIu64 "\r\n", capcode);
        printf("Sending command: %s", command);

        // Clear buffers before sending
        flush_serial_buffers(fd);

        if (write(fd, command, strlen(command)) < 0) {
            perror("write");
            if (send_retries > 0) {
                printf("Write failed, retrying...\n");
                usleep(1000000);
                continue;
            }
            return -1;
        }
        tcdrain(fd);

        // Wait for device to be ready for message
        printf("Waiting for device to be ready for message...\n");
        at_response_t result = at_read_response(fd, response, sizeof(response), NULL, 0);

        if (result != AT_RESP_DATA || strstr(response, "+MSG: READY") == NULL) {
            fprintf(stderr, "Device not ready for message. Got response type %d: '%s'\n", result, response);
            if (send_retries > 0) {
                printf("Device not ready, retrying entire send operation...\n");
                usleep(2000000); // 2 second delay before retry
                continue;
            }
            return -1;
        }

        printf("Device ready! Sending message: '%s'\n", message);

        // Send the message text
        size_t msg_len = strlen(message);
        if (write(fd, message, msg_len) < 0) {
            perror("write message");
            if (send_retries > 0) {
                printf("Message write failed, retrying...\n");
                usleep(2000000);
                continue;
            }
            return -1;
        }

        // Send message terminator
        if (write(fd, "\r\n", 2) < 0) {
            perror("write terminator");
            if (send_retries > 0) {
                printf("Terminator write failed, retrying...\n");
                usleep(2000000);
                continue;
            }
            return -1;
        }

        // Ensure all data is transmitted
        tcdrain(fd);

        printf("Message sent, waiting for encoding and transmission...\n");

        // Wait for final response with extended timeout for remote encoding
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;

        int timeout_remaining = AT_MSG_SEND_TIMEOUT;
        bool transmission_complete = false;

        while (timeout_remaining > 0 && !transmission_complete) {
            int poll_result = poll(&pfd, 1, 1000); // 1 second intervals

            if (poll_result > 0 && (pfd.revents & POLLIN)) {
                result = at_read_response(fd, response, sizeof(response), NULL, 0);
                if (result == AT_RESP_OK) {
                    transmission_complete = true;
                    break;
                } else if (result == AT_RESP_ERROR) {
                    fprintf(stderr, "Remote encoding/transmission failed\n");
                    break;
                }
            }

            timeout_remaining -= 1000;
            if (timeout_remaining % 5000 == 0) {
                printf("Waiting for transmission completion... (%d seconds remaining)\n",
                       timeout_remaining / 1000);
            }
        }

        if (transmission_complete) {
            printf("Remote encoding and transmission completed successfully!\n");
            return 0;
        } else {
            fprintf(stderr, "Remote encoding/transmission timeout or failed\n");
            if (send_retries > 0) {
                printf("Retrying entire operation...\n");
                usleep(2000000);
                continue;
            }
            return -1;
        }
    }

    fprintf(stderr, "Failed to send message after all retry attempts\n");
    return -1;
}

/**
 * @brief Send flex message using local encoding and AT+SEND command.
 */
static int at_send_flex_message_local(int fd, struct serial_config *config,
                                     const uint8_t *data, size_t size)
{
    char command[128];
    char response[AT_BUFFER_SIZE];
    int send_retries = 3; // Retry sending the entire message up to 3 times

    printf("\nConfiguring radio parameters...\n");

    // Set frequency with retries
    snprintf(command, sizeof(command), "AT+FREQ=%.4f\r\n", config->frequency);
    if (at_execute_command(fd, command, response, sizeof(response)) < 0) {
        fprintf(stderr, "Failed to set frequency after all retries\n");
        return -1;
    }

    // Set power with retries
    snprintf(command, sizeof(command), "AT+POWER=%d\r\n", config->power);
    if (at_execute_command(fd, command, response, sizeof(response)) < 0) {
        fprintf(stderr, "Failed to set power after all retries\n");
        return -1;
    }

    printf("Radio configured successfully.\n");

    // Try to send the message with retries
    while (send_retries-- > 0) {
        printf("\nAttempting to send data (attempt %d/3)...\n", 3 - send_retries);

        // Reset device state before attempting to send
        printf("Resetting device state...\n");
        flush_serial_buffers(fd);
        if (at_execute_command(fd, "AT\r\n", response, sizeof(response)) < 0) {
            printf("Failed to reset device state, continuing anyway...\n");
        }

        // Send the SEND command
        snprintf(command, sizeof(command), "AT+SEND=%zu\r\n", size);
        printf("Sending command: %s", command);

        // Clear buffers before sending
        flush_serial_buffers(fd);

        if (write(fd, command, strlen(command)) < 0) {
            perror("write");
            if (send_retries > 0) {
                printf("Write failed, retrying...\n");
                usleep(1000000);
                continue;
            }
            return -1;
        }
        tcdrain(fd);

        // Wait for device to be ready for data
        printf("Waiting for device to be ready for data...\n");
        at_response_t result = at_read_response(fd, response, sizeof(response), NULL, 0);

        if (result != AT_RESP_DATA || strstr(response, "+SEND: READY") == NULL) {
            fprintf(stderr, "Device not ready for data. Got response type %d: '%s'\n", result, response);
            if (send_retries > 0) {
                printf("Device not ready, retrying entire send operation...\n");
                usleep(2000000); // 2 second delay before retry
                continue;
            }
            return -1;
        }

        printf("Device ready! Sending %zu bytes of binary data...\n", size);

        // Send binary data in smaller chunks with progress tracking
        size_t bytes_sent = 0;
        const size_t CHUNK_SIZE = 32; // Smaller chunks for more reliable transmission
        bool send_success = true;
        time_t send_start_time = time(NULL);

        while (bytes_sent < size && send_success) {
            size_t chunk_size = (size - bytes_sent > CHUNK_SIZE) ? CHUNK_SIZE : (size - bytes_sent);

            ssize_t written = write(fd, data + bytes_sent, chunk_size);
            if (written < 0) {
                perror("write binary data");
                send_success = false;
                break;
            }

            bytes_sent += written;
            printf("Sent %zu/%zu bytes (%.1f%%)\r", bytes_sent, size, (float)bytes_sent * 100.0 / size);
            fflush(stdout);

            // Check for timeout
            if ((time(NULL) - send_start_time) > (AT_DATA_SEND_TIMEOUT / 1000)) {
                printf("\nBinary data send timeout\n");
                send_success = false;
                break;
            }

            // Small delay between chunks to avoid overwhelming the device
            usleep(5000); // 5ms
        }

        if (!send_success) {
            if (send_retries > 0) {
                printf("\nBinary data send failed, retrying entire operation...\n");
                usleep(2000000);
                continue;
            }
            return -1;
        }

        printf("\nBinary data sent successfully. Waiting for transmission completion...\n");

        // Ensure all data is transmitted
        tcdrain(fd);
        sleep(5);

        // Wait for final response
        result = at_read_response(fd, response, sizeof(response), NULL, 0);
        if (result != AT_RESP_OK) {
            fprintf(stderr, "Transmission failed. Response type %d: '%s'\n", result, response);
            if (send_retries > 0) {
                printf("Transmission failed, retrying entire operation...\n");
                usleep(2000000);
                continue;
            }
            return -1;
        }

        printf("Transmission completed successfully!\n");
        return 0;
    }

    fprintf(stderr, "Failed to send message after all retry attempts\n");
    return -1;
}

// =============================================================================
// INPUT/OUTPUT HANDLING FUNCTIONS
// =============================================================================

/**
 * @brief Reads a line from stdin, parses capcode and message.
 */
static int read_stdin_message(uint64_t *capcode_ptr, char *message_buf,
    char **line_ptr, size_t *len_ptr)
{
    char *current_message;
    ssize_t read_len;
    char *colon_pos;
    size_t msg_len;

    read_len = getline(line_ptr, len_ptr, stdin);
    if (read_len == -1)
        return 1; /* EOF or error */

    if (read_len > 0 && (*line_ptr)[read_len - 1] == '\n') {
        (*line_ptr)[read_len - 1] = '\0';
        read_len--;
    }

    colon_pos = strchr(*line_ptr, ':');
    if (colon_pos == NULL) {
        fprintf(stderr,
            "Invalid input: '%s', expected 'capcode:message'\n",
            *line_ptr);
        return 2;
    }
    *colon_pos = '\0';

    if (str2uint64(capcode_ptr, *line_ptr) < 0) {
        fprintf(stderr, "Invalid capcode in input: '%s'\n", *line_ptr);
        return 2;
    }

    current_message = colon_pos + 1;
    msg_len = read_len - (current_message - *line_ptr);

    if (msg_len >= MAX_CHARS_ALPHA) {
        fprintf(stderr,
            "Message too long in input: '%s' (max %d chars).\n",
            current_message, MAX_CHARS_ALPHA - 1);
        return 2;
    }
    memcpy(message_buf, current_message, msg_len + 1);
    return 0;
}

/**
 * @brief Display usage information and exit.
 */
/**
 * @brief Display help message.
 */
static void show_help(const char *prgname)
{
    printf("FLEX Paging Message Transmitter v%s - Enhanced Host Application\n", VERSION);
    printf("Build Date: %s\n", BUILD_DATE);
    printf("Comprehensive AT command support and configuration wizard\n\n");
    
    printf("Usage:\n");
    printf("   %s [options] <capcode> <message>\n", prgname);
    printf("   %s [options] [--loop] [--maildrop] [--remote] - (from stdin)\n", prgname);
    printf("   %s --config|-c <device> (interactive configuration)\n", prgname);
    printf("   %s --factoryreset <device> (factory reset device)\n", prgname);
    printf("   %s --help|-h (show this help)\n\n", prgname);
    
    printf("Options:\n");
    printf("   -h, --help         Show this help message and exit\n");
    printf("   -d, --device <dev> Serial device (default: %s)\n", DEFAULT_DEVICE);
    printf("                      Common devices:\n");
    printf("                      /dev/ttyUSB0 - Heltec WiFi LoRa 32 V3\n");
    printf("                      /dev/ttyACM0 - TTGO LoRa32-OLED\n");
    printf("   -b, --baudrate <rate> Baudrate (default: %d)\n", DEFAULT_BAUDRATE);
    printf("   -f, --frequency <MHz> Frequency in MHz (default: %f)\n", DEFAULT_FREQUENCY);
    printf("   -p, --power <dBm>     TX power (default: %d, -9 to 22 for Heltec, 0 to 20 for TTGO)\n", DEFAULT_POWER);
    printf("   -l, --loop            Loop mode: stays open receiving new lines until EOF\n");
    printf("   -m, --maildrop        Mail Drop: sets the Mail Drop Flag in the FLEX message\n");
    printf("   -r, --remote          Remote encoding: use device's AT+MSG command instead of\n");
    printf("                         local encoding. Encoding is performed on the device.\n");
    printf("   -c, --config <device> Configuration mode: interactive setup wizard for v3 devices\n");
    printf("       --factoryreset <device> Factory reset mode: reset device to factory defaults\n\n");
    
    printf("Examples:\n");
    printf("   %s 1234567 \"Hello World\"              # Send basic message\n", prgname);
    printf("   %s --config /dev/ttyUSB0               # Configure device\n", prgname);
    printf("   %s --factoryreset /dev/ttyUSB0         # Factory reset device\n", prgname);
     printf("   %s --help                              # Show this help\n", prgname);
    
    exit(0);
}

static void usage(const char *prgname)
{
    fprintf(stderr,
        "%s [options] <capcode> <message>\n"
        "or:\n"
        "%s [options] [-l] [-m] [-r] - (from stdin)\n"
        "or:\n"
        "%s --config|-c <device> (interactive configuration)\n"
        "or:\n"
        "%s --reset <device> (factory reset device)\n"
        "or:\n"
        "%s --help (show this help)\n\n"

        "Options:\n"
        "   -d <device>    Serial device (default: %s)\n"
        "                  Common devices:\n"
        "                  /dev/ttyUSB0 - Heltec WiFi LoRa 32 V3\n"
        "                  /dev/ttyACM0 - TTGO LoRa32-OLED\n"
        "   -b <baudrate>  Baudrate (default: %d)\n"
        "   -f <frequency> Frequency in MHz (default: %f)\n"
        "   -p <power>     TX power (default: %d, -9 to 22 for Heltec, 0 to 20 for TTGO)\n"
        "   -l             Loop mode: stays open receiving new lines until EOF\n"
        "   -m             Mail Drop: sets the Mail Drop Flag in the FLEX message\n"
        "   -r             Remote encoding: use device's AT+MSG command instead of\n"
        "                  local encoding. Encoding is performed on the device.\n"
        "   -c, --config   Configuration mode: interactive setup wizard for v3 devices\n"
        "   --reset        Factory reset mode: reset device to factory defaults\n"
        "   --help         Show this help message and exit\n\n"

        "Firmware versions:\n"
        "   v1 (Local):   Host encodes FLEX messages using tinyflex library\n"
        "   v2 (Remote):  Device encodes FLEX messages via AT+MSG command\n"
        "   v3 (WiFi):    Device with WiFi, web interface, and REST API support\n\n"

        "Encoding modes:\n"
        "   Default (local):  Encode FLEX message on host using tinyflex library,\n"
        "                     then send binary data with AT+SEND command\n"
        "   Remote (-r):      Send capcode and message text to device using\n"
        "                     AT+MSG command for device-side encoding\n\n"

        "Configuration mode (comprehensive device setup):\n"
        "   %s --config /dev/ttyUSB0    # Configure Heltec device\n"
        "   %s -c /dev/ttyACM0          # Configure TTGO device\n"
        "   \n"
        "   Enhanced interactive wizard with batch questioning:\n"
        "   - Comprehensive AT command support (18+ commands)\n"
        "   - Collects ALL configuration preferences first\n"
        "   - Radio parameters (frequency, power, mail drop)\n"
        "   - WiFi settings (SSID, password, enable/disable, IP configuration)\n"
        "   - REST API configuration (port, username, password)\n"
        "   - Device customization (banner message, themes)\n"
        "   - System management (save, factory reset, device restart)\n"
        "   - Input validation with range checking and error handling\n"
        "   - Displays detailed configuration summary before applying\n"
        "   - Applies all settings in optimized sequence\n"
        "   - Saves configuration to EEPROM automatically\n"
        "   - Firmware version detection (v1/v2/v3) with feature adaptation\n"
        "   - Complete error recovery with detailed status reporting\n\n"

        "Stdin mode:\n"
        "   Example:\n"
        "     printf '1234567:MY MESSAGE'                 | %s -\n"
        "     printf '1234567:MY MSG1\\n1122334:MY MSG2'   | %s -l -\n"
        "     printf '1234567:MY MESSAGE'                 | %s -m -\n"
        "     printf '1234567:MY MESSAGE'                 | %s -r -\n"
        "     printf '1234567:MY MESSAGE'                 | %s -l -m -r -\n\n"

        "Device-specific examples:\n"
        "   # For Heltec WiFi LoRa 32 V3 (local encoding):\n"
        "   %s -d /dev/ttyUSB0 1234567 'MY MESSAGE'\n"
        "   # For TTGO LoRa32-OLED (remote encoding):\n"
        "   %s -d /dev/ttyACM0 -r 1234567 'MY MESSAGE'\n"
        "   # Configure v3 device:\n"
        "   %s --config /dev/ttyUSB0\n"
        "   # Factory reset device:\n"
        "   %s --reset /dev/ttyUSB0\n"
        "   # Show help:\n"
        "   %s --help\n\n"

        "Normal mode examples:\n"
        "   %s 1234567 'MY MESSAGE'\n"
        "   %s -m 1234567 'MY MESSAGE'\n"
        "   %s -r 1234567 'MY MESSAGE'\n"
        "   %s -r -m 1234567 'MY MESSAGE'\n"
        "   %s -d /dev/ttyUSB0 -f 915.5 -r 1234567 'MY MESSAGE'\n",
        prgname, prgname, prgname, prgname, prgname, DEFAULT_DEVICE, DEFAULT_BAUDRATE,
        DEFAULT_FREQUENCY, DEFAULT_POWER, prgname, prgname, prgname, prgname,
        prgname, prgname, prgname, prgname, prgname, prgname, prgname, prgname,
        prgname, prgname, prgname, prgname, prgname);
    exit(1);
}

/**
 * @brief Parses command line options and arguments.
 */
static void read_params(uint64_t *capcode, char *msg, int argc, char **argv,
    struct serial_config *config, int *is_stdin)
{
    int non_opt_start;
    size_t msg_size;
    int opt;

    /* Initialize defaults */
    config->device = strdup(DEFAULT_DEVICE);
    config->baudrate = DEFAULT_BAUDRATE;
    config->frequency = DEFAULT_FREQUENCY;
    config->power = DEFAULT_POWER;

    /* Check for no arguments - show help */
    if (argc == 1) {
        show_help_and_exit = 1;
        show_help(argv[0]);
        return;
    }

    /* Long option definitions */
    static struct option long_options[] = {
        {"help",          no_argument,       0, 'h'},
        {"device",        required_argument, 0, 'd'},
        {"baudrate",      required_argument, 0, 'b'},
        {"frequency",     required_argument, 0, 'f'},
        {"power",         required_argument, 0, 'p'},
        {"loop",          no_argument,       0, 'l'},
        {"maildrop",      no_argument,       0, 'm'},
        {"remote",        no_argument,       0, 'r'},
        {"config",        required_argument, 0, 'c'},
        {"factoryreset",  required_argument, 0, 'R'},
        {0, 0, 0, 0}
    };

    /* Parse options using getopt_long */
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "hd:b:f:p:lmrc:R:", long_options, &option_index)) != -1) {
        switch (opt) {
        case 'h':
            help_mode = 1;
            show_help(argv[0]);
            return;
        case 'd':
            if (strcmp(config->device, DEFAULT_DEVICE) != 0) {
                free((void*)config->device);
            }
            config->device = strdup(optarg);
            break;
        case 'b':
            if (str2int(&config->baudrate, optarg) < 0) {
                fprintf(stderr, "Invalid baudrate: %s\n", optarg);
                usage(argv[0]);
            }
            break;
        case 'f':
            config->frequency = atof(optarg);
            if (config->frequency <= 0) {
                fprintf(stderr, "Invalid frequency: %s\n", optarg);
                usage(argv[0]);
            }
            break;
        case 'p':
            if (str2int(&config->power, optarg) < 0 ||
                config->power < -9 || config->power > 22)
            {
                fprintf(stderr, "Invalid power: %s (range: -9 to 22 dBm)\n", optarg);
                usage(argv[0]);
            }
            break;
        case 'l':
            loop_enabled = 1;
            break;
        case 'm':
            mail_drop_enabled = 1;
            break;
        case 'r':
            remote_encoding = 1;
            break;
        case 'c':
            config_mode = 1;
            if (strcmp(config->device, DEFAULT_DEVICE) != 0) {
                free((void*)config->device);
            }
            config->device = strdup(optarg);
            return; // Config mode doesn't need other params
        case 'R':
            reset_mode = 1;
            if (strcmp(config->device, DEFAULT_DEVICE) != 0) {
                free((void*)config->device);
            }
            config->device = strdup(optarg);
            return; // Reset mode doesn't need other params
        default:
            usage(argv[0]);
        }
    }

    non_opt_start = optind;

    /* Check remaining arguments */
    if (argc - non_opt_start == 2) {
        /* Normal mode: capcode and message */
        if (str2uint64(capcode, argv[non_opt_start]) < 0) {
            fprintf(stderr, "Invalid capcode: %s\n",
                argv[non_opt_start]);
            usage(argv[0]);
        }

        if ((msg_size = strlen(argv[non_opt_start + 1])) >= MAX_CHARS_ALPHA) {
            fprintf(stderr, "Message too long (max %d characters).\n",
                MAX_CHARS_ALPHA - 1);
            usage(argv[0]);
        }
        memcpy(msg, argv[non_opt_start + 1], msg_size + 1);
        *is_stdin = 0;
    }
    else if (argc - non_opt_start == 1 && strcmp(argv[non_opt_start], "-") == 0) {
        /* Stdin mode: requires "-" argument */
        *is_stdin = 1;
    }
    else if (non_opt_start >= argc && !config_mode && !reset_mode && !help_mode) {
        /* No arguments after options - show help */
        show_help(argv[0]);
    }
    else if (non_opt_start < argc) {
        /* Invalid arguments */
        fprintf(stderr, "Invalid arguments provided.\n");
        usage(argv[0]);
    }
}

// =============================================================================
// V3 FIRMWARE CONFIGURATION FUNCTIONS
// =============================================================================

/**
 * @brief Query an AT command and return the response.
 */

/**
 * @brief Set an AT command with a value.
 */

/**
 * @brief Get user input with prompt.
 */

/**
 * @brief Get yes/no confirmation from user.
 */
static int get_yes_no(const char *prompt, int default_yes)
{
    char input[10];
    
    printf("%s [%s]: ", prompt, default_yes ? "Y/n" : "y/N");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return default_yes;
    }
    
    // Remove newline
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }
    
    // Empty input uses default
    if (strlen(input) == 0) {
        return default_yes;
    }
    
    return (tolower(input[0]) == 'y');
}

/**
 * @brief Silently retrieve device information and store in device_cfg.
 */
static int retrieve_device_info_silent(int fd)
{
    int success_count = 0;
    int total_queries = 0;
    
    // Enable silent mode to suppress AT command debug output
    int old_silent_mode = silent_mode;
    silent_mode = 1;
    
    // Basic device status
    total_queries++;
    if (at_query_status(fd, device_cfg.device_status, sizeof(device_cfg.device_status)) == 0) {
        success_count++;
    }
    
    // Radio configuration
    total_queries++;
    if (at_query_frequency(fd, &device_cfg.frequency) == 0) {
        success_count++;
    }
    
    total_queries++;
    if (at_query_power(fd, &device_cfg.power) == 0) {
        success_count++;
    }
    
    // Default FLEX settings (v3 firmware)
    total_queries++;
    if (at_get_default_capcode(fd, &device_cfg.default_capcode) == 0) {
        success_count++;
    }
    
    total_queries++;
    if (at_get_default_frequency(fd, &device_cfg.default_frequency) == 0) {
        success_count++;
    }
    
    total_queries++;
    if (at_get_default_power(fd, &device_cfg.default_power) == 0) {
        success_count++;
    }
    
    // WiFi configuration (v3 firmware)
    total_queries++;
    if (at_query_wifi_enable(fd, &device_cfg.wifi_enabled) == 0) {
        success_count++;
        
        if (device_cfg.wifi_enabled && at_query_wifi(fd, device_cfg.wifi_status, sizeof(device_cfg.wifi_status)) == 0) {
            // WiFi status retrieved successfully
        }
    }
    
    // Device customization (v3 firmware)
    total_queries++;
    if (at_query_banner(fd, device_cfg.banner_message, sizeof(device_cfg.banner_message)) == 0) {
        success_count++;
    }
    
    // Battery status (v3 firmware)
    total_queries++;
    if (at_query_battery(fd, device_cfg.battery_info, sizeof(device_cfg.battery_info)) == 0) {
        success_count++;
    }
    
    // API configuration (v3 firmware)
    total_queries++;
    if (at_query_api_port(fd, &device_cfg.api_port) == 0) {
        success_count++;
    }
    
    total_queries++;
    if (at_query_api_username(fd, device_cfg.api_username, sizeof(device_cfg.api_username)) == 0) {
        success_count++;
    }
    
    // Restore original silent mode setting
    silent_mode = old_silent_mode;
    
    // Return success if we got at least half of the information
    return (success_count >= total_queries / 2) ? 0 : -1;
}



/**
 * @brief Enhanced interactive device configuration wizard with batch questioning.
 */
static int run_configuration_wizard(int fd)
{
    printf("=== FLEX Paging Message Transmitter Configuration Wizard ===\n");
    printf("This wizard will help you configure your device comprehensively.\n");
    printf("We'll collect all configuration information first, then apply settings.\n\n");
    
    // Initialize device configuration structure
    memset(&device_cfg, 0, sizeof(device_cfg));
    
    // Retrieve current device information silently
    printf("Retrieving device information...\n");
    if (retrieve_device_info_silent(fd) < 0) {
        printf("Warning: Could not retrieve some device settings.\n");
    }
    
    // Collect all configuration preferences
    printf("\n=== Configuration Questions ===\n");
    printf("Please answer the following questions. We'll apply all settings together at the end.\n\n");
    
    // 1. Default FLEX Settings Configuration
    int configure_defaults = get_yes_no("Configure default FLEX settings (capcode, frequency, power)?", 1);
    if (configure_defaults) {
        collect_default_configuration();
    }
    
    // 2. WiFi Configuration
    int configure_wifi = get_yes_no("Configure WiFi settings?", 1);
    if (configure_wifi) {
        collect_wifi_configuration();
    }
    
    // 3. API Configuration
    int configure_api = get_yes_no("Configure REST API settings (port, authentication)?", 1);
    if (configure_api) {
        collect_api_configuration();
    }
    
    // 4. Device Settings
    int configure_device = get_yes_no("Configure device banner?", 1);
    if (configure_device) {
        collect_device_configuration();
    }
    
    // Display summary and confirm
    printf("\n=== Configuration Summary ===\n");
    display_configuration_summary();
    
    if (!get_yes_no("Apply these configuration changes?", 1)) {
        printf("Configuration cancelled by user.\n");
        return 0;
    }
    
    // Apply all configurations in sequence
    printf("\n=== Applying Configuration ===\n");
    int success = 1;
    
    if (configure_defaults && apply_default_configuration(fd) < 0) {
        printf("ERROR: Failed to apply default FLEX configuration.\n");
        success = 0;
    }
    
    if (configure_wifi && apply_wifi_configuration(fd) < 0) {
        printf("ERROR: Failed to apply WiFi configuration.\n");
        success = 0;
    }
    
    if (configure_api && apply_api_configuration(fd) < 0) {
        printf("ERROR: Failed to apply API configuration.\n");
        success = 0;
    }
    
    if (configure_device && apply_device_configuration(fd) < 0) {
        printf("ERROR: Failed to apply device configuration.\n");
        success = 0;
    }
    
    if (success) {
        printf("Saving configuration to device EEPROM...\n");
        if (at_save_config(fd) == 0) {
            printf("✓ Configuration saved successfully!\n");
        } else {
            printf("WARNING: Failed to save configuration to EEPROM.\n");
        }
        
        printf("Restarting device to apply all settings...\n");
        at_reset_device(fd);
        printf("✓ Device restart initiated. Please wait for device to reboot.\n");
        printf("Configuration complete!\n");
    } else {
        printf("Configuration completed with errors. Please check settings manually.\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Factory reset device via AT command.
 */
static int run_factory_reset(int fd)
{
    printf("=== FLEX Paging Message Transmitter Factory Reset ===\n");
    printf("This will reset the device to factory defaults and restart it.\n");
    printf("All configuration will be lost (WiFi settings, API config, custom banner, etc.)\n\n");
    
    if (!get_yes_no("Are you sure you want to factory reset this device?", 0)) {
        printf("Factory reset cancelled by user.\n");
        return 0;
    }
    
    if (!get_yes_no("WARNING: This action cannot be undone. Continue with factory reset?", 0)) {
        printf("Factory reset cancelled by user.\n");
        return 0;
    }
    
    printf("\nPerforming factory reset...\n");
    
    // Simple device check - send AT command
    printf("Checking device communication...\n");
    if (at_send_command(fd, "AT\r\n") < 0) {
        printf("ERROR: Device not responding to AT command.\n");
        printf("Please check device connection and try again.\n");
        return -1;
    }
    
    char response[64];
    at_response_t result = at_read_response(fd, response, sizeof(response), NULL, 0);
    if (result != AT_RESP_OK) {
        printf("ERROR: Device not responding properly (got: %s).\n", response);
        printf("Please check device connection and try again.\n");
        return -1;
    }
    
    printf("Device responding. Sending factory reset command...\n");
    
    // Send factory reset command
    if (at_send_command(fd, "AT+FACTORYRESET\r\n") < 0) {
        printf("ERROR: Failed to send factory reset command.\n");
        return -1;
    }
    
    printf("✓ Factory reset command sent successfully!\n");
    printf("Device will restart with default settings.\n");
    printf("Please wait for device to reboot (this may take 10-30 seconds).\n");
    
    return 0;
}

// =============================================================================
// CONFIGURATION COLLECTION FUNCTIONS
// =============================================================================

/**
 * @brief Get string input from user with validation.
 */
static int get_string_input(const char *prompt, char *buffer, size_t buffer_size, const char *default_value)
{
    printf("%s", prompt);
    if (default_value && strlen(default_value) > 0) {
        printf(" [%s]", default_value);
    }
    printf(": ");
    
    if (fgets(buffer, buffer_size, stdin) == NULL) {
        return -1;
    }
    
    // Remove newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    
    // Use default if empty input
    if (strlen(buffer) == 0 && default_value) {
        strncpy(buffer, default_value, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
    
    return 0;
}

/**
 * @brief Get integer input from user with validation.
 */
static int get_int_input(const char *prompt, int *value, int min_val, int max_val, int default_val)
{
    char buffer[64];
    printf("%s (%d-%d) [%d]: ", prompt, min_val, max_val, default_val);
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return -1;
    }
    
    if (strlen(buffer) <= 1) { // Just newline
        *value = default_val;
        return 0;
    }
    
    int input = atoi(buffer);
    if (input < min_val || input > max_val) {
        printf("Error: Value must be between %d and %d\n", min_val, max_val);
        return -1;
    }
    
    *value = input;
    return 0;
}

/**
 * @brief Get frequency input with automatic Hz to MHz conversion.
 */
static int get_frequency_input(const char *prompt, double *value, double min_val, double max_val, double default_val)
{
    char buffer[64];
    printf("%s (%.1f-%.1f MHz) [%.4f]: ", prompt, min_val, max_val, default_val);
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return -1;
    }
    
    // Remove newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    
    // Use default if empty input
    if (strlen(buffer) == 0) {
        *value = default_val;
        return 0;
    }
    
    // Try to parse the input
    double input_val = atof(buffer);
    if (input_val == 0.0 && buffer[0] != '0') {
        return -1; // Invalid input
    }
    
    // Auto-convert Hz to MHz if value is very large (> 100,000)
    if (input_val > 100000.0) {
        input_val = input_val / 1000000.0;  // Convert Hz to MHz
        printf("  (Converted from Hz to MHz: %.4f MHz)\n", input_val);
    }
    
    // Validate range
    if (input_val < min_val || input_val > max_val) {
        return -1;
    }
    
    *value = input_val;
    return 0;
}

/**
 * @brief Get uint64_t input from user with validation.
 */
static int get_uint64_input(const char *prompt, uint64_t *value, uint64_t min_val, uint64_t max_val, uint64_t default_val)
{
    char buffer[64];
    printf("%s (%llu-%llu) [%llu]: ", prompt, (unsigned long long)min_val, (unsigned long long)max_val, (unsigned long long)default_val);
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return -1;
    }
    
    // Remove newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    
    // Use default if empty input
    if (strlen(buffer) == 0) {
        *value = default_val;
        return 0;
    }
    
    // Try to parse the input
    char *endptr;
    unsigned long long input_val = strtoull(buffer, &endptr, 10);
    if (*endptr != '\0') {
        return -1; // Invalid input
    }
    
    // Validate range
    if (input_val < min_val || input_val > max_val) {
        return -1;
    }
    
    *value = (uint64_t)input_val;
    return 0;
}



/**
 * @brief Collect WiFi configuration from user.
 */
static void collect_wifi_configuration(void)
{
    printf("\n--- WiFi Configuration ---\n");
    
    // Use current WiFi enabled status as default
    int wifi_default = device_cfg.wifi_enabled ? 1 : 1; // Default to enabled
    device_cfg.wifi_enabled = get_yes_no("Enable WiFi functionality", wifi_default) ? 1 : 0;
    
    if (device_cfg.wifi_enabled) {
        // Use current SSID as default if available
        const char *ssid_default = (strlen(device_cfg.wifi_ssid) > 0) ? device_cfg.wifi_ssid : "";
        while (get_string_input("WiFi SSID", device_cfg.wifi_ssid, sizeof(device_cfg.wifi_ssid), ssid_default) < 0 ||
               strlen(device_cfg.wifi_ssid) == 0) {
            printf("Please enter a valid WiFi SSID.\n");
        }
        
        // For password, don't show current for security, prompt for new one
        while (get_string_input("WiFi Password", device_cfg.wifi_password, sizeof(device_cfg.wifi_password), "") < 0 ||
               strlen(device_cfg.wifi_password) == 0) {
            printf("Please enter a valid WiFi password.\n");
        }
        
        device_cfg.use_dhcp = get_yes_no("Use DHCP (automatic IP assignment)", 1) ? 1 : 0;
        
        if (!device_cfg.use_dhcp) {
            printf("Note: Static IP configuration not implemented in this wizard.\n");
            printf("You can configure static IP manually via AT commands after setup.\n");
            device_cfg.use_dhcp = 1; // Force DHCP for now
        }
    }
    
    printf("✓ WiFi configuration collected.\n");
}

/**
 * @brief Collect API configuration from user.
 */
static void collect_api_configuration(void)
{
    printf("\n--- REST API Configuration ---\n");
    
    // Use current API port as default, fallback to 16180 if not available
    int port_default = (device_cfg.api_port > 0) ? device_cfg.api_port : 16180;
    while (get_int_input("API Port", &device_cfg.api_port, 1024, 65535, port_default) < 0) {
        printf("Please enter a valid port number.\n");
    }
    
    // For username, always use "admin" as default for simplicity
    while (get_string_input("API Username", device_cfg.api_username, sizeof(device_cfg.api_username), "admin") < 0) {
        printf("Please enter a valid username.\n");
    }
    
    // For password, always use "passw0rd" as default for security
    while (get_string_input("API Password", device_cfg.api_password, sizeof(device_cfg.api_password), "passw0rd") < 0) {
        printf("Please enter a valid password.\n");
    }
    
    printf("✓ API configuration collected.\n");
}

/**
 * @brief Collect device configuration from user.
 */
static void collect_device_configuration(void)
{
    printf("\n--- Device Configuration ---\n");
    
    // Use retrieved banner from device, fallback to "flex-fsk-tx" if device had default/empty
    const char *current_banner = device_cfg.banner_message;
    const char *banner_to_show = (strlen(current_banner) > 0 && strcmp(current_banner, "flex-fsk-tx") != 0) ? current_banner : "flex-fsk-tx";
    
    // Temporarily store the current banner to use as default
    char temp_banner[17];
    strcpy(temp_banner, banner_to_show);
    
    while (get_string_input("Device Banner (max 16 chars)", device_cfg.banner_message, sizeof(device_cfg.banner_message), temp_banner) < 0) {
        printf("Please enter a valid banner message.\n");
    }
    
    // Ensure banner is set if still empty
    if (strlen(device_cfg.banner_message) == 0) {
        strcpy(device_cfg.banner_message, temp_banner);
    }
    
    printf("✓ Device configuration collected.\n");
}

/**
 * @brief Collect default FLEX configuration from user.
 */
static void collect_default_configuration(void)
{
    printf("\n--- Default FLEX Settings Configuration ---\n");
    printf("These settings will be stored in EEPROM as defaults for new transmissions.\n");
    
    // Default capcode
    uint64_t capcode_default = (device_cfg.default_capcode > 0) ? device_cfg.default_capcode : 1234567;
    while (get_uint64_input("Default Capcode", &device_cfg.default_capcode, 1, 4294967295ULL, capcode_default) < 0) {
        printf("Please enter a valid capcode (1-4294967295).\n");
    }
    
    // Default frequency  
    double freq_default = (device_cfg.default_frequency > 0) ? device_cfg.default_frequency : 929.6625;
    while (get_frequency_input("Default Frequency (MHz or Hz)", &device_cfg.default_frequency, 400.0, 1000.0, freq_default) < 0) {
        printf("Please enter a valid frequency (400-1000 MHz or 400000000-1000000000 Hz).\n");
    }
    
    // Default power
    int power_default = (device_cfg.default_power != 0) ? device_cfg.default_power : 2;
    while (get_int_input("Default TX Power (dBm)", &device_cfg.default_power, -9, 22, power_default) < 0) {
        printf("Please enter a valid power level.\n");
    }
    
    printf("✓ Default FLEX configuration collected.\n");
}

/**
 * @brief Display configuration summary before applying.
 */
static void display_configuration_summary(void)
{
    printf("The following configuration will be applied:\n\n");
    
    if (device_cfg.default_capcode > 0 || device_cfg.default_frequency > 0 || device_cfg.default_power != 0) {
        printf("Default FLEX Settings (stored in EEPROM):\n");
        if (device_cfg.default_capcode > 0) {
            printf("  - Default Capcode: %lu\n", device_cfg.default_capcode);
        }
        if (device_cfg.default_frequency > 0) {
            printf("  - Default Frequency: %.4f MHz\n", device_cfg.default_frequency);
        }
        if (device_cfg.default_power != 0) {
            printf("  - Default Power: %d dBm\n", device_cfg.default_power);
        }
        printf("\n");
    }
    
    if (device_cfg.wifi_enabled) {
        printf("WiFi Settings:\n");
        printf("  - WiFi: %s\n", device_cfg.wifi_enabled ? "Enabled" : "Disabled");
        printf("  - SSID: %s\n", device_cfg.wifi_ssid);
        printf("  - Password: %s\n", strlen(device_cfg.wifi_password) > 0 ? "***" : "(not set)");
        printf("  - DHCP: %s\n", device_cfg.use_dhcp ? "Enabled" : "Disabled");
        printf("\n");
    }
    
    if (device_cfg.api_port > 0) {
        printf("API Settings:\n");
        printf("  - Port: %d\n", device_cfg.api_port);
        printf("  - Username: %s\n", strlen(device_cfg.api_username) > 0 ? device_cfg.api_username : "admin");
        printf("  - Password: %s\n", strlen(device_cfg.api_password) > 0 ? "***" : "(not set)");
        printf("\n");
    }
    
    if (strlen(device_cfg.banner_message) > 0) {
        printf("Device Settings:\n");
        printf("  - Banner: %s\n", device_cfg.banner_message);
        printf("\n");
    }
}

// =============================================================================
// CONFIGURATION APPLICATION FUNCTIONS
// =============================================================================


/**
 * @brief Apply WiFi configuration to device.
 */
static int apply_wifi_configuration(int fd)
{
    printf("Applying WiFi configuration...\n");
    
    if (at_set_wifi_enable(fd, device_cfg.wifi_enabled) < 0) {
        printf("  ERROR: Failed to set WiFi enable status\n");
        return -1;
    }
    printf("  ✓ WiFi %s\n", device_cfg.wifi_enabled ? "enabled" : "disabled");
    
    if (device_cfg.wifi_enabled && strlen(device_cfg.wifi_ssid) > 0) {
        if (at_set_wifi(fd, device_cfg.wifi_ssid, device_cfg.wifi_password) < 0) {
            printf("  ERROR: Failed to configure WiFi credentials\n");
            return -1;
        }
        printf("  ✓ WiFi credentials configured for SSID: %s\n", device_cfg.wifi_ssid);
    }
    
    return 0;
}

/**
 * @brief Apply API configuration to device.
 */
static int apply_api_configuration(int fd)
{
    printf("Applying API configuration...\n");
    
    if (device_cfg.api_port > 0) {
        if (at_set_api_port(fd, device_cfg.api_port) < 0) {
            printf("  ERROR: Failed to set API port\n");
            return -1;
        }
        printf("  ✓ API port set to %d\n", device_cfg.api_port);
    }
    
    const char *username_to_apply = (strlen(device_cfg.api_username) > 0) ? device_cfg.api_username : "admin";
    if (at_set_api_username(fd, username_to_apply) < 0) {
        printf("  ERROR: Failed to set API username\n");
        return -1;
    }
    printf("  ✓ API username set to %s\n", username_to_apply);
    
    if (strlen(device_cfg.api_password) > 0) {
        if (at_set_api_password(fd, device_cfg.api_password) < 0) {
            printf("  ERROR: Failed to set API password\n");
            return -1;
        }
        printf("  ✓ API password configured\n");
    }
    
    return 0;
}

/**
 * @brief Apply device configuration to device.
 */
static int apply_device_configuration(int fd)
{
    printf("Applying device configuration...\n");
    
    if (strlen(device_cfg.banner_message) > 0) {
        if (at_set_banner(fd, device_cfg.banner_message) < 0) {
            printf("  ERROR: Failed to set banner message\n");
            return -1;
        }
        printf("  ✓ Banner set to: %s\n", device_cfg.banner_message);
    }
    
    return 0;
}

/**
 * @brief Apply default FLEX configuration to device using AT+SETDEFAULT commands.
 */
static int apply_default_configuration(int fd)
{
    printf("Applying default FLEX configuration...\n");
    
    if (device_cfg.default_capcode > 0) {
        if (at_set_default_capcode(fd, device_cfg.default_capcode) < 0) {
            printf("  ERROR: Failed to set default capcode\n");
            return -1;
        }
        printf("  ✓ Default capcode set to: %lu\n", device_cfg.default_capcode);
    }
    
    if (device_cfg.default_frequency > 0) {
        if (at_set_default_frequency(fd, device_cfg.default_frequency) < 0) {
            printf("  ERROR: Failed to set default frequency\n");
            return -1;
        }
        printf("  ✓ Default frequency set to: %.4f MHz\n", device_cfg.default_frequency);
    }
    
    if (device_cfg.default_power != 0) {
        if (at_set_default_power(fd, device_cfg.default_power) < 0) {
            printf("  ERROR: Failed to set default power\n");
            return -1;
        }
        printf("  ✓ Default power set to: %d dBm\n", device_cfg.default_power);
    }
    
    return 0;
}

/**
 * @brief Retrieve current default settings from device using AT+GETDEFAULT commands.
 */

// =============================================================================
// SIGNAL HANDLING
// =============================================================================

/**
 * @brief Signal handler to restore TTY on interrupt.
 */
static void signal_handler(int sig)
{
    (void)sig; /* Unused */
    restore_tty();
    exit(1);
}

// =============================================================================
// MAIN FUNCTION
// =============================================================================

/**
 * @brief Main function - program entry point.
 */
int main(int argc, char **argv)
{
    struct tf_message_config msg_config = {0};
    uint8_t vec[FLEX_BUFFER_SIZE] = {0};
    char message[MAX_CHARS_ALPHA] = {0};
    struct serial_config config;
    uint64_t capcode;
    size_t read_size;
    int is_stdin;
    char *line;
    int status;
    size_t len;
    int err;
    int ret;
    int fd;

    // Initialize variables
    line = NULL;
    len = 0;
    ret = 1;
    fd = -1;

    // Setup cleanup and signal handlers
    atexit(restore_tty);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Parse command line arguments
    read_params(&capcode, message, argc, argv, &config, &is_stdin);

    // Open and configure serial device
    fd = open(config.device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Unable to open serial device '%s': %s\n",
            config.device, strerror(errno));
        goto error;
    }

    if (configure_serial(fd, config.baudrate) < 0) {
        fprintf(stderr, "Failed to configure serial port\n");
        goto error;
    }

    // Initialize device communication
    usleep(1000000); // 1 second settling time
    if (at_initialize_device(fd) < 0) {
        fprintf(stderr, "Failed to initialize device\n");
        goto error;
    }

    // Handle configuration mode
    if (config_mode) {
        printf("Starting configuration mode for device: %s\n", config.device);
        if (run_configuration_wizard(fd) < 0) {
            fprintf(stderr, "Configuration failed\n");
            goto error;
        }
        printf("Configuration completed successfully.\n");
        goto exit;
    }

    // Handle factory reset mode
    if (reset_mode) {
        printf("Starting factory reset mode for device: %s\n", config.device);
        if (run_factory_reset(fd) < 0) {
            fprintf(stderr, "Factory reset failed\n");
            goto error;
        }
        printf("Factory reset completed successfully.\n");
        goto exit;
    }

    // Display encoding mode
    if (remote_encoding) {
        printf("Using remote encoding mode (device-side encoding)\n");
    } else {
        printf("Using local encoding mode (host-side encoding)\n");
    }

    // Handle normal mode (single message)
    if (!is_stdin) {
        if (remote_encoding) {
            // Use remote encoding
            if (at_send_flex_message_remote(fd, &config, capcode, message) < 0)
                goto error;
            printf("Successfully sent flex message using remote encoding\n");
        } else {
            // Use local encoding
            msg_config.mail_drop = mail_drop_enabled;
            read_size = tf_encode_flex_message_ex(message, capcode, vec,
                sizeof vec, &err, &msg_config);

            if (err >= 0) {
                if (at_send_flex_message_local(fd, &config, vec, read_size) < 0)
                    goto error;
                printf("Successfully sent flex message using local encoding\n");
            }
            else {
                fprintf(stderr, "Error encoding message: %s\n",
                    msg_errors[-err]);
                goto error;
            }
        }
        goto exit;
    }

    // Handle stdin mode (multiple messages)
    do {
        status = read_stdin_message(&capcode, message, &line, &len);
        if (status == 1) /* EOF or read error */
            break;

        if (status == 2) { /* Parsing error */
            if (!loop_enabled)
                goto error;
            continue;
        }

        if (remote_encoding) {
            // Use remote encoding
            if (at_send_flex_message_remote(fd, &config, capcode, message) < 0) {
                if (!loop_enabled)
                    goto error;

                // In loop mode, continue on error
                fprintf(stderr, "Failed to send message using remote encoding, continuing...\n");
            } else {
                printf("Sent message using remote encoding for capcode %" PRId64 "\n", capcode);
            }
        } else {
            // Use local encoding
            msg_config.mail_drop = mail_drop_enabled;
            read_size = tf_encode_flex_message_ex(message, capcode, vec,
                sizeof vec, &err, &msg_config);

            if (err >= 0) {
                if (at_send_flex_message_local(fd, &config, vec, read_size) < 0) {
                    if (!loop_enabled)
                        goto error;

                    // In loop mode, continue on error
                    fprintf(stderr, "Failed to send message using local encoding, continuing...\n");
                } else {
                    printf("Sent %zu bytes using local encoding for capcode %" PRId64 "\n",
                        read_size, capcode);
                }
            }
            else {
                fprintf(stderr, "Error encoding message: %s\n", msg_errors[-err]);
                if (!loop_enabled)
                    goto error;
            }
        }
    } while (loop_enabled); /* Continue loop if enabled */

exit:
    ret = 0;
error:
    // Cleanup
    if (fd >= 0)
        close(fd);
    free(line);
    return ret;
}
