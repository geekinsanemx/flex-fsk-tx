/*
 * FLEX-FSK-TX Binary Protocol Client
 *
 * Command-line tool for testing binary protocol over UART
 *
 * Usage:
 *   flex-binary-client -d /dev/ttyUSB0 -f 931.9375 -p 10 1234567 "Message"
 *
 * Compile:
 *   gcc -o flex-binary-client flex-binary-client.c -O2 -Wall
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>
#include <sys/select.h>

// =============================================================================
// PROTOCOL CONSTANTS
// =============================================================================

#define PACKET_MAX_LEN 512
#define PACKET_HEADER_LEN 8
#define PACKET_CRC_LEN 2
#define PACKET_OVERHEAD (PACKET_HEADER_LEN + PACKET_CRC_LEN)
#define MAX_FLEX_MESSAGE_LENGTH 248

// Packet types
#define PKT_TYPE_CMD 0x01
#define PKT_TYPE_RSP 0x02
#define PKT_TYPE_EVT 0x03

// Opcodes
#define CMD_SEND_FLEX     0x01
#define CMD_GET_STATUS    0x02
#define CMD_PING          0x06

#define RSP_ACK     0x01
#define RSP_NACK    0x02
#define RSP_STATUS  0x03
#define RSP_PONG    0x05

#define EVT_TX_QUEUED          0x01
#define EVT_TX_START           0x02
#define EVT_TX_DONE            0x03
#define EVT_TX_FAILED          0x04

// Flags
#define FLAG_ACK_REQUIRED  0x01

// Status codes
#define STATUS_ACCEPTED      0x00
#define STATUS_REJECTED      0x01
#define STATUS_QUEUE_FULL    0x02
#define STATUS_INVALID_PARAM 0x03

// Result codes
#define RESULT_SUCCESS         0x00
#define RESULT_RADIO_ERROR     0x01
#define RESULT_ENCODING_ERROR  0x02

// =============================================================================
// DATA STRUCTURES
// =============================================================================

typedef struct __attribute__((packed)) {
    uint16_t len;
    uint8_t type;
    uint8_t opcode;
    uint8_t flags;
    uint8_t seq;
    uint16_t msg_id;
    uint8_t payload[PACKET_MAX_LEN - PACKET_OVERHEAD];
    uint16_t crc16;
} binary_packet_t;

// =============================================================================
// CRC16-CCITT IMPLEMENTATION
// =============================================================================

static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

uint16_t crc16_ccitt(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = ((crc >> 8) ^ data[i]) & 0xFF;
        crc = (crc << 8) ^ crc16_table[index];
    }
    return crc;
}

// =============================================================================
// COBS IMPLEMENTATION
// =============================================================================

size_t cobs_encode(const uint8_t *input, size_t length, uint8_t *output) {
    const uint8_t *src = input;
    uint8_t *dst = output;
    uint8_t *code_ptr = dst++;
    uint8_t code = 0x01;

    for (size_t i = 0; i < length; i++) {
        if (*src == 0x00) {
            *code_ptr = code;
            code_ptr = dst++;
            code = 0x01;
        } else {
            *dst++ = *src;
            code++;
            if (code == 0xFF) {
                *code_ptr = code;
                code_ptr = dst++;
                code = 0x01;
            }
        }
        src++;
    }

    *code_ptr = code;
    *dst++ = 0x00;  // Frame delimiter

    return dst - output;
}

size_t cobs_decode(const uint8_t *input, size_t length, uint8_t *output) {
    if (length == 0 || input[length - 1] != 0x00) {
        return 0;
    }

    const uint8_t *src = input;
    uint8_t *dst = output;
    length--;  // Remove delimiter

    while (length > 0) {
        uint8_t code = *src++;
        length--;

        if (code == 0x00) {
            return 0;  // Invalid
        }

        for (uint8_t i = 1; i < code && length > 0; i++) {
            *dst++ = *src++;
            length--;
        }

        if (code < 0xFF && length > 0) {
            *dst++ = 0x00;
        }
    }

    return dst - output;
}

// =============================================================================
// ENDIANNESS HELPERS
// =============================================================================

uint16_t htons_custom(uint16_t hostshort) {
    return (hostshort >> 8) | (hostshort << 8);
}

uint16_t ntohs_custom(uint16_t netshort) {
    return (netshort >> 8) | (netshort << 8);
}

uint32_t htonl_custom(uint32_t hostlong) {
    return ((hostlong & 0xFF000000) >> 24) |
           ((hostlong & 0x00FF0000) >> 8)  |
           ((hostlong & 0x0000FF00) << 8)  |
           ((hostlong & 0x000000FF) << 24);
}

uint32_t ntohl_custom(uint32_t netlong) {
    return htonl_custom(netlong);
}

// =============================================================================
// MESSAGE ID GENERATION
// =============================================================================

uint16_t generate_msg_id() {
    static uint16_t counter = 0;
    return ++counter;
}

// =============================================================================
// PACKET BUILDING
// =============================================================================

size_t build_cmd_send_flex(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id,
                           uint32_t capcode, float frequency, int8_t tx_power,
                           uint8_t mail_drop, const char *message, uint8_t msg_len) {
    if (msg_len > MAX_FLEX_MESSAGE_LENGTH) {
        msg_len = MAX_FLEX_MESSAGE_LENGTH;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t pos = 0;
    size_t payload_len = 11 + msg_len;
    uint16_t total_len = PACKET_OVERHEAD + payload_len;

    memcpy(&buf[pos], &total_len, 2);
    pos += 2;

    buf[pos++] = PKT_TYPE_CMD;
    buf[pos++] = CMD_SEND_FLEX;
    buf[pos++] = FLAG_ACK_REQUIRED;
    buf[pos++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[pos], &msg_id_be, 2);
    pos += 2;

    memcpy(&buf[pos], &capcode, 4);
    pos += 4;

    memcpy(&buf[pos], &frequency, 4);
    pos += 4;

    buf[pos++] = (uint8_t)tx_power;
    buf[pos++] = mail_drop;
    buf[pos++] = msg_len;

    memcpy(&buf[pos], message, msg_len);
    pos += msg_len;

    uint16_t crc = crc16_ccitt(buf, pos);
    memcpy(&buf[pos], &crc, 2);
    pos += 2;

    return total_len;
}

// =============================================================================
// GLOBAL VERBOSE FLAG
// =============================================================================
static int g_verbose = 0;

// =============================================================================
// SERIAL PORT FUNCTIONS
// =============================================================================

int serial_open(const char *device, int baudrate) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", device, strerror(errno));
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Error getting serial attributes: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    // Set baudrate
    speed_t speed;
    switch (baudrate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        case 460800: speed = B460800; break;
        case 921600: speed = B921600; break;
        default:
            fprintf(stderr, "Unsupported baudrate: %d\n", baudrate);
            close(fd);
            return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    // 8N1
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    // Raw mode
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;

    // Non-blocking read
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error setting serial attributes: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    // Flush
    tcflush(fd, TCIOFLUSH);

    return fd;
}

int serial_send_packet(int fd, const binary_packet_t *pkt) {
    uint8_t cobs_buffer[PACKET_MAX_LEN + 10];
    size_t cobs_len = cobs_encode((uint8_t*)pkt, pkt->len, cobs_buffer);

    if (cobs_len == 0) {
        fprintf(stderr, "COBS encoding failed\n");
        return -1;
    }

    if (g_verbose) {
        printf("TX [%zu bytes COBS]: ", cobs_len);
        for (size_t i = 0; i < cobs_len && i < 32; i++) {
            printf("%02X ", cobs_buffer[i]);
        }
        if (cobs_len > 32) printf("...");
        printf("\n");
    }

    ssize_t written = write(fd, cobs_buffer, cobs_len);
    if (written < 0) {
        fprintf(stderr, "Write error: %s\n", strerror(errno));
        return -1;
    }

    if ((size_t)written != cobs_len) {
        fprintf(stderr, "Incomplete write: %zd/%zu bytes\n", written, cobs_len);
        return -1;
    }

    fsync(fd);
    return 0;
}

int serial_read_frame(int fd, uint8_t *buffer, size_t max_len, int timeout_ms) {
    uint8_t byte;
    size_t pos = 0;
    struct timeval start, now;
    gettimeofday(&start, NULL);

    char ascii_line[512];
    size_t ascii_pos = 0;
    int in_cobs_frame = 0;

    while (1) {
        gettimeofday(&now, NULL);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                         (now.tv_usec - start.tv_usec) / 1000;
        if (elapsed_ms > timeout_ms) {
            return -1;
        }

        ssize_t n = read(fd, &byte, 1);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);
                continue;
            }
            fprintf(stderr, "Read error: %s\n", strerror(errno));
            return -1;
        }

        if (n == 0) {
            usleep(1000);
            continue;
        }

        if (byte == 0x02 && !in_cobs_frame) {
            in_cobs_frame = 1;
            pos = 0;
            buffer[pos++] = byte;
            continue;
        }

        if (in_cobs_frame) {
            if (pos < max_len) {
                buffer[pos++] = byte;
            }

            if (byte == 0x00) {
                if (g_verbose) {
                    printf("RX [%zu bytes COBS]: ", pos);
                    for (size_t i = 0; i < pos && i < 32; i++) {
                        printf("%02X ", buffer[i]);
                    }
                    if (pos > 32) printf("...");
                    printf("\n");
                }
                return pos;
            }
            continue;
        }

        if (byte >= 0x20 && byte < 0x7F) {
            if (ascii_pos < sizeof(ascii_line) - 1) {
                ascii_line[ascii_pos++] = byte;
            }
            continue;
        }

        if ((byte == '\r' || byte == '\n') && ascii_pos > 0) {
            if (byte == '\n') {
                ascii_line[ascii_pos] = '\0';
                printf("DEVICE: %s\n", ascii_line);
                ascii_pos = 0;
            }
            continue;
        }

        if (byte == '\r' || byte == '\n') {
            continue;
        }
    }
}

// =============================================================================
// PACKET PARSING
// =============================================================================

int parse_response(const uint8_t *data, size_t len, binary_packet_t *pkt) {
    if (len < PACKET_OVERHEAD) {
        return -1;
    }

    uint8_t decoded[PACKET_MAX_LEN];
    size_t decoded_len = cobs_decode(data, len, decoded);
    if (decoded_len == 0) {
        fprintf(stderr, "COBS decode failed\n");
        return -1;
    }

    if (decoded_len < PACKET_OVERHEAD) {
        fprintf(stderr, "Packet too short: %zu bytes\n", decoded_len);
        return -1;
    }

    memcpy(pkt, decoded, decoded_len);

    uint16_t calculated_crc = crc16_ccitt(decoded, decoded_len - 2);

    uint16_t recv_crc;
    memcpy(&recv_crc, &decoded[decoded_len - 2], 2);

    if (calculated_crc != recv_crc) {
        fprintf(stderr, "CRC mismatch: expected 0x%04X, got 0x%04X\n",
                calculated_crc, recv_crc);
        return -1;
    }

    if (g_verbose) {
        printf("PARSED: len=%d type=0x%02X opcode=0x%02X flags=0x%02X seq=%d msg_id=0x%04X crc=0x%04X\n",
               pkt->len, pkt->type, pkt->opcode, pkt->flags, pkt->seq,
               ntohs_custom(pkt->msg_id), recv_crc);
    }

    return 0;
}

// =============================================================================
// MAIN
// =============================================================================

void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS] CAPCODE MESSAGE\n\n", prog);
    printf("Binary Protocol Client for FLEX-FSK-TX v2.5.1\n\n");
    printf("Options:\n");
    printf("  -d DEVICE    Serial device (default: /dev/ttyUSB0)\n");
    printf("  -b BAUD      Baudrate (default: 115200)\n");
    printf("  -f FREQ      Frequency in MHz (default: 929.6625)\n");
    printf("  -p POWER     Power in dBm (default: 10)\n");
    printf("  -m           Enable mail drop flag\n");
    printf("  -v           Verbose output\n");
    printf("  -w           Wait for TX_DONE event\n");
    printf("  -h           Show this help\n\n");
    printf("Examples:\n");
    printf("  %s 1234567 \"Hello World\"\n", prog);
    printf("  %s -d /dev/ttyUSB0 -f 931.9375 -p 15 -w 1234567 \"Test message\"\n", prog);
    printf("  %s -v -w -m 37137 \"Long message...\"\n\n", prog);
}

int main(int argc, char *argv[]) {
    const char *device = "/dev/ttyUSB0";
    int baudrate = 115200;
    float frequency = 929.6625;
    int power = 10;
    uint8_t mail_drop = 0;
    int wait_for_done = 0;
    int opt;

    // Parse options
    while ((opt = getopt(argc, argv, "d:b:f:p:mvwh")) != -1) {
        switch (opt) {
            case 'd':
                device = optarg;
                break;
            case 'b':
                baudrate = atoi(optarg);
                break;
            case 'f':
                frequency = atof(optarg);
                break;
            case 'p':
                power = atoi(optarg);
                break;
            case 'm':
                mail_drop = 1;
                break;
            case 'v':
                g_verbose = 1;
                break;
            case 'w':
                wait_for_done = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Check arguments
    if (optind + 2 > argc) {
        fprintf(stderr, "Error: Missing capcode and/or message\n\n");
        print_usage(argv[0]);
        return 1;
    }

    // Parse capcode and message
    uint32_t capcode = (uint32_t)atoll(argv[optind]);
    const char *message = argv[optind + 1];
    uint8_t msg_len = strlen(message);

    if (msg_len > MAX_FLEX_MESSAGE_LENGTH) {
        fprintf(stderr, "Warning: Message truncated to %d chars\n", MAX_FLEX_MESSAGE_LENGTH);
        msg_len = MAX_FLEX_MESSAGE_LENGTH;
    }

    if (g_verbose) {
        printf("Device: %s\n", device);
        printf("Baudrate: %d\n", baudrate);
        printf("Frequency: %.4f MHz\n", frequency);
        printf("Power: %d dBm\n", power);
        printf("Mail drop: %s\n", mail_drop ? "enabled" : "disabled");
        printf("Capcode: %u\n", capcode);
        printf("Message: %s\n", message);
        printf("Length: %d bytes\n", msg_len);
        printf("\n");
    }

    // Open serial port
    int fd = serial_open(device, baudrate);
    if (fd < 0) {
        return 1;
    }

    printf("Connected to %s @ %d baud\n", device, baudrate);

    // Build CMD_SEND_FLEX packet
    binary_packet_t pkt;
    static uint8_t seq = 1;
    uint16_t msg_id = generate_msg_id();

    size_t pkt_len = build_cmd_send_flex(&pkt, seq++, msg_id, capcode, frequency,
                                         (int8_t)power, mail_drop, message, msg_len);

    if (g_verbose) {
        printf("Sending CMD_SEND_FLEX (msg_id=0x%04X, seq=%d, len=%zu)\n",
               msg_id, seq - 1, pkt_len);
    }

    // Send packet
    if (serial_send_packet(fd, &pkt) < 0) {
        fprintf(stderr, "Failed to send packet\n");
        close(fd);
        return 1;
    }

    printf("Sent message (msg_id=0x%04X, capcode=%u)\n", msg_id, capcode);

    // Wait for RSP_ACK
    uint8_t frame_buffer[PACKET_MAX_LEN + 10];
    int frame_len = serial_read_frame(fd, frame_buffer, sizeof(frame_buffer), 5000);

    if (frame_len < 0) {
        fprintf(stderr, "Timeout waiting for ACK\n");
        close(fd);
        return 1;
    }

    binary_packet_t rsp;
    if (parse_response(frame_buffer, frame_len, &rsp) < 0) {
        fprintf(stderr, "Failed to parse response\n");
        close(fd);
        return 1;
    }

    // Check response type
    if (rsp.type == PKT_TYPE_RSP && rsp.opcode == RSP_ACK) {
        uint8_t status = rsp.payload[0];
        uint16_t response_msg_id = ntohs_custom(rsp.msg_id);

        if (response_msg_id != msg_id) {
            fprintf(stderr, "Warning: msg_id mismatch (expected 0x%04X, got 0x%04X)\n",
                    msg_id, response_msg_id);
        }

        switch (status) {
            case STATUS_ACCEPTED:
                printf("ACK: Message accepted\n");
                break;
            case STATUS_QUEUE_FULL:
                fprintf(stderr, "ACK: Queue full\n");
                close(fd);
                return 1;
            case STATUS_INVALID_PARAM:
                fprintf(stderr, "ACK: Invalid parameters\n");
                close(fd);
                return 1;
            default:
                fprintf(stderr, "ACK: Unknown status 0x%02X\n", status);
                close(fd);
                return 1;
        }
    } else if (rsp.type == PKT_TYPE_RSP && rsp.opcode == RSP_NACK) {
        uint8_t error = rsp.payload[0];
        fprintf(stderr, "NACK: Error 0x%02X\n", error);
        close(fd);
        return 1;
    } else {
        fprintf(stderr, "Unexpected response: type=0x%02X opcode=0x%02X\n",
                rsp.type, rsp.opcode);
        close(fd);
        return 1;
    }

    // Wait for events if requested
    if (wait_for_done) {
        printf("Waiting for TX completion...\n");

        int timeout_sec = 30;
        time_t start_time = time(NULL);

        while (1) {
            // Check timeout
            if (time(NULL) - start_time > timeout_sec) {
                fprintf(stderr, "Timeout waiting for TX_DONE\n");
                close(fd);
                return 1;
            }

            // Read event
            frame_len = serial_read_frame(fd, frame_buffer, sizeof(frame_buffer), 1000);
            if (frame_len < 0) {
                continue;  // Timeout, keep waiting
            }

            binary_packet_t evt;
            if (parse_response(frame_buffer, frame_len, &evt) < 0) {
                fprintf(stderr, "Failed to parse event\n");
                continue;
            }

            if (evt.type != PKT_TYPE_EVT) {
                if (g_verbose) {
                    printf("Ignoring non-event packet: type=0x%02X\n", evt.type);
                }
                continue;
            }

            uint16_t evt_msg_id = ntohs_custom(evt.msg_id);

            switch (evt.opcode) {
                case EVT_TX_QUEUED:
                    if (g_verbose) {
                        uint8_t queue_pos = evt.payload[0];
                        printf("EVENT: TX_QUEUED (msg_id=0x%04X, pos=%d)\n",
                               evt_msg_id, queue_pos);
                    }
                    break;

                case EVT_TX_START:
                    printf("EVENT: TX_START (msg_id=0x%04X) - Transmitting...\n",
                           evt_msg_id);
                    break;

                case EVT_TX_DONE:
                    if (evt_msg_id == msg_id) {
                        uint8_t result = evt.payload[0];
                        if (result == RESULT_SUCCESS) {
                            printf("EVENT: TX_DONE (msg_id=0x%04X) - SUCCESS\n", evt_msg_id);
                            close(fd);
                            return 0;
                        } else {
                            fprintf(stderr, "EVENT: TX_DONE (msg_id=0x%04X) - FAILED (result=0x%02X)\n",
                                    evt_msg_id, result);
                            close(fd);
                            return 1;
                        }
                    } else {
                        if (g_verbose) {
                            printf("EVENT: TX_DONE for different message (0x%04X)\n", evt_msg_id);
                        }
                    }
                    break;

                case EVT_TX_FAILED:
                    if (evt_msg_id == msg_id) {
                        uint8_t result = evt.payload[0];
                        fprintf(stderr, "EVENT: TX_FAILED (msg_id=0x%04X, result=0x%02X)\n",
                                evt_msg_id, result);
                        close(fd);
                        return 1;
                    } else {
                        if (g_verbose) {
                            printf("EVENT: TX_FAILED for different message (0x%04X)\n", evt_msg_id);
                        }
                    }
                    break;

                default:
                    if (g_verbose) {
                        printf("EVENT: Unknown opcode 0x%02X (msg_id=0x%04X)\n",
                               evt.opcode, evt_msg_id);
                    }
                    break;
            }
        }
    }

    close(fd);
    printf("Done.\n");
    return 0;
}