/*
 * FLEX Paging Message Transmitter - v2.5.1
 * Binary Protocol - Packet Implementation
 */

#include "binary_packet.h"
#include "crc16.h"
#include <string.h>

// =============================================================================
// PACKET BUILDING: COMMANDS
// =============================================================================

size_t build_cmd_send_flex(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id,
                           uint32_t capcode, float frequency, int8_t tx_power,
                           uint8_t mail_drop, const char *message, uint8_t msg_len) {
    if (pkt == nullptr || message == nullptr) {
        return 0;
    }

    if (msg_len > MAX_FLEX_MESSAGE_LENGTH) {
        msg_len = MAX_FLEX_MESSAGE_LENGTH;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t p = 0;
    size_t payload_len = 11 + msg_len;
    uint16_t total_len = PACKET_OVERHEAD + payload_len;

    memcpy(&buf[p], &total_len, 2);
    p += 2;

    buf[p++] = PKT_TYPE_CMD;
    buf[p++] = CMD_SEND_FLEX;
    buf[p++] = FLAG_ACK_REQUIRED;
    buf[p++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[p], &msg_id_be, 2);
    p += 2;

    memcpy(&buf[p], &capcode, 4);
    p += 4;

    memcpy(&buf[p], &frequency, 4);
    p += 4;

    buf[p++] = (uint8_t)tx_power;
    buf[p++] = mail_drop;
    buf[p++] = msg_len;

    memcpy(&buf[p], message, msg_len);
    p += msg_len;

    uint16_t crc = crc16_ccitt(buf, p);
    memcpy(&buf[p], &crc, 2);

    return total_len;
}

// =============================================================================
// PACKET BUILDING: RESPONSES
// =============================================================================

size_t build_rsp_ack(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t status) {
    if (pkt == nullptr) {
        return 0;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t pos = 0;
    uint16_t total_len = PACKET_OVERHEAD + 1;

    memcpy(&buf[pos], &total_len, 2);
    pos += 2;

    buf[pos++] = PKT_TYPE_RSP;
    buf[pos++] = RSP_ACK;
    buf[pos++] = 0x00;
    buf[pos++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[pos], &msg_id_be, 2);
    pos += 2;

    buf[pos++] = status;

    uint16_t crc = crc16_ccitt(buf, pos);
    memcpy(&buf[pos], &crc, 2);

    return total_len;
}

size_t build_rsp_nack(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t status) {
    if (pkt == nullptr) {
        return 0;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t pos = 0;
    uint16_t total_len = PACKET_OVERHEAD + 1;

    memcpy(&buf[pos], &total_len, 2);
    pos += 2;

    buf[pos++] = PKT_TYPE_RSP;
    buf[pos++] = RSP_NACK;
    buf[pos++] = FLAG_ERROR;
    buf[pos++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[pos], &msg_id_be, 2);
    pos += 2;

    buf[pos++] = status;

    uint16_t crc = crc16_ccitt(buf, pos);
    memcpy(&buf[pos], &crc, 2);

    return total_len;
}

size_t build_rsp_status(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id,
                        const rsp_status_payload_t *status) {
    if (pkt == nullptr || status == nullptr) {
        return 0;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t p = 0;
    uint16_t total_len = PACKET_OVERHEAD + sizeof(rsp_status_payload_t);

    memcpy(&buf[p], &total_len, 2);
    p += 2;

    buf[p++] = PKT_TYPE_RSP;
    buf[p++] = RSP_STATUS;
    buf[p++] = 0x00;
    buf[p++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[p], &msg_id_be, 2);
    p += 2;

    memcpy(&buf[p], status, sizeof(rsp_status_payload_t));
    p += sizeof(rsp_status_payload_t);

    uint16_t crc = crc16_ccitt(buf, p);
    memcpy(&buf[p], &crc, 2);

    return total_len;
}

size_t build_rsp_pong(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id) {
    if (pkt == nullptr) {
        return 0;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t pos = 0;
    uint16_t total_len = PACKET_OVERHEAD;

    memcpy(&buf[pos], &total_len, 2);
    pos += 2;

    buf[pos++] = PKT_TYPE_RSP;
    buf[pos++] = RSP_PONG;
    buf[pos++] = 0x00;
    buf[pos++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[pos], &msg_id_be, 2);
    pos += 2;

    uint16_t crc = crc16_ccitt(buf, pos);
    memcpy(&buf[pos], &crc, 2);

    return total_len;
}

// =============================================================================
// PACKET BUILDING: EVENTS
// =============================================================================

size_t build_evt_tx_queued(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t pos) {
    if (pkt == nullptr) {
        return 0;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t p = 0;
    uint16_t total_len = PACKET_OVERHEAD + 1;

    memcpy(&buf[p], &total_len, 2);
    p += 2;

    buf[p++] = PKT_TYPE_EVT;
    buf[p++] = EVT_TX_QUEUED;
    buf[p++] = 0x00;
    buf[p++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[p], &msg_id_be, 2);
    p += 2;

    buf[p++] = pos;

    uint16_t crc = crc16_ccitt(buf, p);
    memcpy(&buf[p], &crc, 2);

    return total_len;
}

size_t build_evt_tx_start(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id) {
    if (pkt == nullptr) {
        return 0;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t p = 0;
    uint16_t total_len = PACKET_OVERHEAD;

    memcpy(&buf[p], &total_len, 2);
    p += 2;

    buf[p++] = PKT_TYPE_EVT;
    buf[p++] = EVT_TX_START;
    buf[p++] = 0x00;
    buf[p++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[p], &msg_id_be, 2);
    p += 2;

    uint16_t crc = crc16_ccitt(buf, p);
    memcpy(&buf[p], &crc, 2);

    return total_len;
}

size_t build_evt_tx_done(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t result) {
    if (pkt == nullptr) {
        return 0;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t p = 0;
    uint16_t total_len = PACKET_OVERHEAD + 1;

    memcpy(&buf[p], &total_len, 2);
    p += 2;

    buf[p++] = PKT_TYPE_EVT;
    buf[p++] = EVT_TX_DONE;
    buf[p++] = 0x00;
    buf[p++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[p], &msg_id_be, 2);
    p += 2;

    buf[p++] = result;

    uint16_t crc = crc16_ccitt(buf, p);
    memcpy(&buf[p], &crc, 2);

    return total_len;
}

size_t build_evt_tx_failed(binary_packet_t *pkt, uint8_t seq, uint16_t msg_id, uint8_t error) {
    if (pkt == nullptr) {
        return 0;
    }

    uint8_t *buf = (uint8_t*)pkt;
    size_t p = 0;
    uint16_t total_len = PACKET_OVERHEAD + 1;

    memcpy(&buf[p], &total_len, 2);
    p += 2;

    buf[p++] = PKT_TYPE_EVT;
    buf[p++] = EVT_TX_FAILED;
    buf[p++] = FLAG_ERROR;
    buf[p++] = seq;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[p], &msg_id_be, 2);
    p += 2;

    buf[p++] = error;

    uint16_t crc = crc16_ccitt(buf, p);
    memcpy(&buf[p], &crc, 2);

    return total_len;
}

// =============================================================================
// PACKET PARSING
// =============================================================================

bool parse_packet(const uint8_t *data, size_t len, binary_packet_t *pkt) {
    if (data == nullptr || pkt == nullptr || len < PACKET_OVERHEAD) {
        return false;
    }

    uint16_t packet_len;
    memcpy(&packet_len, &data[0], 2);

    if (packet_len != len) {
        return false;
    }

    if (packet_len < PACKET_OVERHEAD || packet_len > PACKET_MAX_LEN) {
        return false;
    }

    memcpy(&pkt->len, &data[0], 2);
    pkt->type = data[2];
    pkt->opcode = data[3];
    pkt->flags = data[4];
    pkt->seq = data[5];

    pkt->msg_id = (data[6] << 8) | data[7];

    size_t payload_len = packet_len - PACKET_OVERHEAD;

    if (payload_len > 0) {
        memcpy(pkt->payload, &data[8], payload_len);
    }

    size_t crc_offset = packet_len - 2;
    pkt->crc16 = data[crc_offset] | (data[crc_offset + 1] << 8);

    return true;
}

// =============================================================================
// PACKET VALIDATION
// =============================================================================

bool validate_packet(const binary_packet_t *pkt) {
    if (pkt == nullptr) {
        return false;
    }

    // Validate length
    if (pkt->len < PACKET_OVERHEAD || pkt->len > PACKET_MAX_LEN) {
        return false;
    }

    // Validate type
    if (pkt->type < PKT_TYPE_CMD || pkt->type > PKT_TYPE_EVT) {
        return false;
    }

    // Calculate CRC (excluding CRC field itself)
    uint16_t calculated_crc = crc16_ccitt((uint8_t*)pkt, pkt->len - 2);

    // Compare with packet CRC
    return (calculated_crc == pkt->crc16);
}
