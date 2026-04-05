/*
 * FLEX Paging Message Transmitter - v2.5.1
 * Binary Protocol - Event Senders Implementation
 */

#include "binary_events.h"
#include "binary_packet.h"
#include "cobs.h"
#include "crc16.h"
#include "logging.h"

// =============================================================================
// GLOBAL STATE
// =============================================================================

bool binary_protocol_active = false;
uint8_t binary_event_seq = 0;

// =============================================================================
// INTERNAL HELPERS
// =============================================================================

static void send_packet_via_serial(const binary_packet_t *pkt) {
    if (pkt == nullptr) {
        return;
    }

    uint8_t *buf = (uint8_t*)pkt;
    uint16_t packet_len;
    memcpy(&packet_len, buf, 2);

    uint8_t cobs_buffer[PACKET_MAX_LEN + 10];
    size_t cobs_len = cobs_encode(buf, packet_len, cobs_buffer);

    if (cobs_len == 0) {
        logMessage("BINARY: COBS encode failed");
        return;
    }

    Serial.write(cobs_buffer, cobs_len);
    Serial.flush();
}

// =============================================================================
// GENERIC EVENT SENDERS
// =============================================================================

void send_binary_event(uint8_t opcode, uint16_t msg_id) {
    if (!binary_protocol_active) {
        return;  // Binary protocol not active
    }

    binary_packet_t pkt;

    switch (opcode) {
        case EVT_TX_START:
            build_evt_tx_start(&pkt, binary_event_seq++, msg_id);
            break;

        default:
            logMessagef("BINARY: Unknown event opcode 0x%02X", opcode);
            return;
    }

    send_packet_via_serial(&pkt);

    logMessagef("BINARY: Sent event opcode=0x%02X msg_id=0x%04X", opcode, msg_id);
}

void send_binary_event_with_payload(uint8_t opcode, uint16_t msg_id,
                                    const uint8_t *payload, size_t len) {
    if (!binary_protocol_active) {
        return;
    }

    if (len > PACKET_MAX_PAYLOAD) {
        logMessage("BINARY: Payload too large");
        return;
    }

    binary_packet_t pkt;
    uint8_t *buf = (uint8_t*)&pkt;
    size_t p = 0;
    uint16_t total_len = PACKET_OVERHEAD + len;

    memcpy(&buf[p], &total_len, 2);
    p += 2;

    buf[p++] = PKT_TYPE_EVT;
    buf[p++] = opcode;
    buf[p++] = 0x00;
    buf[p++] = binary_event_seq++;

    uint16_t msg_id_be = htons_custom(msg_id);
    memcpy(&buf[p], &msg_id_be, 2);
    p += 2;

    if (len > 0 && payload != nullptr) {
        memcpy(&buf[p], payload, len);
        p += len;
    }

    uint16_t crc = crc16_ccitt(buf, p);
    memcpy(&buf[p], &crc, 2);

    send_packet_via_serial(&pkt);

    logMessagef("BINARY: Sent event opcode=0x%02X msg_id=0x%04X len=%d",
                opcode, msg_id, len);
}

// =============================================================================
// SPECIFIC EVENT SENDERS
// =============================================================================

void send_evt_tx_queued(uint16_t msg_id, uint8_t pos) {
    if (!binary_protocol_active) {
        return;
    }

    binary_packet_t pkt;
    build_evt_tx_queued(&pkt, binary_event_seq++, msg_id, pos);
    send_packet_via_serial(&pkt);

    logMessagef("BINARY: EVT_TX_QUEUED msg_id=0x%04X pos=%d", msg_id, pos);
}

void send_evt_tx_start(uint16_t msg_id) {
    if (!binary_protocol_active) {
        return;
    }

    binary_packet_t pkt;
    build_evt_tx_start(&pkt, binary_event_seq++, msg_id);
    send_packet_via_serial(&pkt);

    logMessagef("BINARY: EVT_TX_START msg_id=0x%04X", msg_id);
}

void send_evt_tx_done(uint16_t msg_id, uint8_t result) {
    if (!binary_protocol_active) {
        return;
    }

    binary_packet_t pkt;
    build_evt_tx_done(&pkt, binary_event_seq++, msg_id, result);
    send_packet_via_serial(&pkt);

    logMessagef("BINARY: EVT_TX_DONE msg_id=0x%04X result=%d", msg_id, result);
}

void send_evt_tx_failed(uint16_t msg_id, uint8_t error) {
    if (!binary_protocol_active) {
        return;
    }

    binary_packet_t pkt;
    build_evt_tx_failed(&pkt, binary_event_seq++, msg_id, error);
    send_packet_via_serial(&pkt);

    logMessagef("BINARY: EVT_TX_FAILED msg_id=0x%04X error=%d", msg_id, error);
}

void send_evt_boot() {
    if (!binary_protocol_active) {
        return;
    }

    binary_packet_t pkt;
    uint8_t *buf = (uint8_t*)&pkt;
    size_t p = 0;
    uint16_t total_len = PACKET_OVERHEAD;

    memcpy(&buf[p], &total_len, 2);
    p += 2;

    buf[p++] = PKT_TYPE_EVT;
    buf[p++] = EVT_BOOT;
    buf[p++] = 0x00;
    buf[p++] = binary_event_seq++;

    uint16_t msg_id_be = 0x0000;
    memcpy(&buf[p], &msg_id_be, 2);
    p += 2;

    uint16_t crc = crc16_ccitt(buf, p);
    memcpy(&buf[p], &crc, 2);

    send_packet_via_serial(&pkt);

    logMessage("BINARY: EVT_BOOT sent");
}

void send_evt_battery_low(uint8_t battery_pct) {
    if (!binary_protocol_active) {
        return;
    }

    uint8_t payload[1] = {battery_pct};
    send_binary_event_with_payload(EVT_BATTERY_LOW, 0x0000, payload, 1);

    logMessagef("BINARY: EVT_BATTERY_LOW pct=%d", battery_pct);
}

void send_evt_power_disconnected() {
    if (!binary_protocol_active) {
        return;
    }

    binary_packet_t pkt;
    uint8_t *buf = (uint8_t*)&pkt;
    size_t p = 0;
    uint16_t total_len = PACKET_OVERHEAD;

    memcpy(&buf[p], &total_len, 2);
    p += 2;

    buf[p++] = PKT_TYPE_EVT;
    buf[p++] = EVT_POWER_DISCONNECTED;
    buf[p++] = 0x00;
    buf[p++] = binary_event_seq++;

    uint16_t msg_id_be = 0x0000;
    memcpy(&buf[p], &msg_id_be, 2);
    p += 2;

    uint16_t crc = crc16_ccitt(buf, p);
    memcpy(&buf[p], &crc, 2);

    send_packet_via_serial(&pkt);

    logMessage("BINARY: EVT_POWER_DISCONNECTED sent");
}

// =============================================================================
// RESPONSE SENDERS
// =============================================================================

void send_binary_response_ack(uint8_t seq, uint16_t msg_id, uint8_t status) {
    if (!binary_protocol_active) {
        return;
    }

    binary_packet_t pkt;
    build_rsp_ack(&pkt, seq, msg_id, status);
    send_packet_via_serial(&pkt);

    logMessagef("BINARY: RSP_ACK seq=%d msg_id=0x%04X status=%d", seq, msg_id, status);
}

void send_binary_response_nack(uint8_t seq, uint16_t msg_id, uint8_t status) {
    if (!binary_protocol_active) {
        return;
    }

    binary_packet_t pkt;
    build_rsp_nack(&pkt, seq, msg_id, status);
    send_packet_via_serial(&pkt);

    logMessagef("BINARY: RSP_NACK seq=%d msg_id=0x%04X status=%d", seq, msg_id, status);
}

void send_binary_response_pong(uint8_t seq, uint16_t msg_id) {
    if (!binary_protocol_active) {
        return;
    }

    binary_packet_t pkt;
    build_rsp_pong(&pkt, seq, msg_id);
    send_packet_via_serial(&pkt);

    logMessagef("BINARY: RSP_PONG seq=%d msg_id=0x%04X", seq, msg_id);
}

void send_binary_response_status(uint8_t seq, uint16_t msg_id,
                                 uint8_t device_state, uint8_t queue_count,
                                 uint8_t battery_pct, uint16_t battery_mv,
                                 float frequency, int8_t power) {
    if (!binary_protocol_active) {
        return;
    }

    rsp_status_payload_t status;
    status.device_state = device_state;
    status.queue_count = queue_count;
    status.battery_pct = battery_pct;
    status.battery_mv = battery_mv;
    status.frequency = frequency;
    status.power = power;

    binary_packet_t pkt;
    build_rsp_status(&pkt, seq, msg_id, &status);
    send_packet_via_serial(&pkt);

    logMessagef("BINARY: RSP_STATUS seq=%d msg_id=0x%04X state=%d queue=%d",
                seq, msg_id, device_state, queue_count);
}
