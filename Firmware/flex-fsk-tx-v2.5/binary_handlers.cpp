/*
 * FLEX Paging Message Transmitter - v2.5.1
 * Binary Protocol - Command Handlers Implementation
 */

#include "binary_handlers.h"
#include "binary_events.h"
#include "flex_protocol.h"
#include "hardware.h"
#include "display.h"
#include "utils.h"
#include "logging.h"
#include "config.h"
#include "boards/boards.h"

// =============================================================================
// EXTERNAL VARIABLES
// =============================================================================

extern volatile int queue_count;
extern float current_tx_frequency;
extern float current_tx_power;

// =============================================================================
// CMD_SEND_FLEX HANDLER
// =============================================================================

void handle_cmd_send_flex(binary_packet_t *pkt) {
    if (pkt == nullptr) {
        return;
    }

    uint8_t *payload = pkt->payload;
    size_t p = 0;

    uint32_t capcode;
    memcpy(&capcode, &payload[p], 4);
    p += 4;

    float frequency;
    memcpy(&frequency, &payload[p], 4);
    p += 4;

    int8_t tx_power = (int8_t)payload[p++];
    uint8_t mail_drop = payload[p++];
    uint8_t msg_len = payload[p++];

    char *message = (char*)&payload[p];

    uint16_t msg_id = ntohs_custom(pkt->msg_id);

    logMessagef("BINARY: CMD_SEND_FLEX msg_id=0x%04X capcode=%lu freq=%.4f power=%d mail=%d len=%d",
                msg_id, (unsigned long)capcode, frequency, tx_power, mail_drop, msg_len);

    if (!validate_flex_capcode(capcode)) {
        logMessage("BINARY: Invalid capcode");
        send_binary_response_nack(pkt->seq, msg_id, STATUS_INVALID_PARAM);
        return;
    }

    if (msg_len < 1 || msg_len > MAX_FLEX_MESSAGE_LENGTH) {
        logMessage("BINARY: Invalid message length");
        send_binary_response_nack(pkt->seq, msg_id, STATUS_INVALID_PARAM);
        return;
    }

    if (frequency < 400.0 || frequency > 1000.0) {
        logMessage("BINARY: Invalid frequency");
        send_binary_response_nack(pkt->seq, msg_id, STATUS_INVALID_PARAM);
        return;
    }

    if (tx_power < -1 || tx_power > 20) {
        logMessage("BINARY: Invalid TX power");
        send_binary_response_nack(pkt->seq, msg_id, STATUS_INVALID_PARAM);
        return;
    }

    char safe_message[MAX_FLEX_MESSAGE_LENGTH + 1];
    memcpy(safe_message, message, msg_len);
    safe_message[msg_len] = '\0';

    bool mail_drop_flag = (mail_drop != 0);

    if (!queue_add_message_with_id(msg_id, capcode, frequency,
                                    tx_power, mail_drop_flag, safe_message)) {
        logMessage("BINARY: Queue full");
        send_binary_response_nack(pkt->seq, msg_id, STATUS_QUEUE_FULL);
        return;
    }

    send_binary_response_ack(pkt->seq, msg_id, STATUS_ACCEPTED);
    send_evt_tx_queued(msg_id, queue_count);
}

// =============================================================================
// CMD_GET_STATUS HANDLER
// =============================================================================

void handle_cmd_get_status(binary_packet_t *pkt) {
    if (pkt == nullptr) {
        return;
    }

    uint16_t msg_id = ntohs_custom(pkt->msg_id);

    logMessagef("BINARY: CMD_GET_STATUS msg_id=0x%04X", msg_id);

    // Get battery info
    uint16_t battery_voltage_mv;
    int battery_percentage;
    getBatteryInfo(&battery_voltage_mv, &battery_percentage);

    // Send status response
    send_binary_response_status(
        pkt->seq,
        msg_id,
        device_state,
        queue_count,
        (uint8_t)battery_percentage,
        battery_voltage_mv,
        current_tx_frequency,
        (int8_t)current_tx_power
    );
}

// =============================================================================
// CMD_ABORT HANDLER
// =============================================================================

void handle_cmd_abort(binary_packet_t *pkt) {
    if (pkt == nullptr) {
        return;
    }

    uint16_t msg_id = ntohs_custom(pkt->msg_id);

    logMessagef("BINARY: CMD_ABORT msg_id=0x%04X", msg_id);

    // Abort current operation
    radio_standby();
    digitalWrite(LED_PIN, LOW);

    // Send ACK
    send_binary_response_ack(pkt->seq, msg_id, STATUS_ACCEPTED);
}

// =============================================================================
// CMD_PING HANDLER
// =============================================================================

void handle_cmd_ping(binary_packet_t *pkt) {
    if (pkt == nullptr) {
        return;
    }

    uint16_t msg_id = ntohs_custom(pkt->msg_id);

    logMessagef("BINARY: CMD_PING msg_id=0x%04X", msg_id);

    // Send PONG response
    send_binary_response_pong(pkt->seq, msg_id);
}

// =============================================================================
// CMD_SET_CONFIG HANDLER (Placeholder)
// =============================================================================

void handle_cmd_set_config(binary_packet_t *pkt) {
    if (pkt == nullptr) {
        return;
    }

    uint16_t msg_id = ntohs_custom(pkt->msg_id);

    logMessagef("BINARY: CMD_SET_CONFIG msg_id=0x%04X (not implemented)", msg_id);

    // Not implemented yet
    send_binary_response_nack(pkt->seq, msg_id, STATUS_REJECTED);
}

// =============================================================================
// CMD_GET_CONFIG HANDLER (Placeholder)
// =============================================================================

void handle_cmd_get_config(binary_packet_t *pkt) {
    if (pkt == nullptr) {
        return;
    }

    uint16_t msg_id = ntohs_custom(pkt->msg_id);

    logMessagef("BINARY: CMD_GET_CONFIG msg_id=0x%04X (not implemented)", msg_id);

    // Not implemented yet
    send_binary_response_nack(pkt->seq, msg_id, STATUS_REJECTED);
}

// =============================================================================
// COMMAND DISPATCHER
// =============================================================================

void dispatch_binary_command(binary_packet_t *pkt) {
    if (pkt == nullptr) {
        logMessage("BINARY: NULL packet");
        return;
    }

    // Verify it's a command packet
    if (pkt->type != PKT_TYPE_CMD) {
        logMessagef("BINARY: Not a command (type=0x%02X)", pkt->type);
        return;
    }

    // Dispatch based on opcode
    switch (pkt->opcode) {
        case CMD_SEND_FLEX:
            handle_cmd_send_flex(pkt);
            break;

        case CMD_GET_STATUS:
            handle_cmd_get_status(pkt);
            break;

        case CMD_ABORT:
            handle_cmd_abort(pkt);
            break;

        case CMD_PING:
            handle_cmd_ping(pkt);
            break;

        case CMD_SET_CONFIG:
            handle_cmd_set_config(pkt);
            break;

        case CMD_GET_CONFIG:
            handle_cmd_get_config(pkt);
            break;

        default:
            logMessagef("BINARY: Unknown command opcode 0x%02X", pkt->opcode);
            // Send NACK for unknown commands
            uint16_t msg_id = ntohs_custom(pkt->msg_id);
            send_binary_response_nack(pkt->seq, msg_id, STATUS_INVALID_PARAM);
            break;
    }
}
