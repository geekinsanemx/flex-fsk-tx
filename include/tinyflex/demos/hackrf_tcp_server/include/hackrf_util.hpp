#pragma once
#include <libhackrf/hackrf.h>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>

inline hackrf_device* setup_hackrf(uint64_t frequency, uint32_t sample_rate, int tx_gain) {
    hackrf_device* device = nullptr;
   
    int result = hackrf_init();
    if (result != HACKRF_SUCCESS) {
        printf("hackrf_init() failed: %s\n", hackrf_error_name((hackrf_error)result));
        return nullptr;
    }
   
    result = hackrf_open(&device);
    if (result != HACKRF_SUCCESS) {
        printf("hackrf_open() failed: %s\n", hackrf_error_name((hackrf_error)result));
        hackrf_exit();
        return nullptr;
    }
   
    result = hackrf_set_sample_rate(device, sample_rate);
    if (result != HACKRF_SUCCESS) {
        printf("hackrf_set_sample_rate() failed\n");
    }
   
    result = hackrf_set_freq(device, frequency);
    if (result != HACKRF_SUCCESS) {
        printf("hackrf_set_freq() failed\n");
    }
   
    result = hackrf_set_txvga_gain(device, tx_gain); // 0-47 dB
    if (result != HACKRF_SUCCESS) {
        printf("hackrf_set_txvga_gain() failed\n");
    }

    return device;
}

inline void close_hackrf(hackrf_device* device) {
    if (device) {
        hackrf_close(device);
    }

    hackrf_exit();
}

struct TxState {
    const int8_t* data;
    size_t total;
    size_t sent;
};

inline bool transmit_hackrf(hackrf_device* device, const std::vector<int8_t>& iq_samples) {
    TxState tx_state = { iq_samples.data(), iq_samples.size(), 0 };
    auto tx_callback = [](hackrf_transfer* transfer) -> int {
        TxState* state = reinterpret_cast<TxState*>(transfer->tx_ctx);
        size_t to_copy = transfer->buffer_length;
        if (state->sent + to_copy > state->total) {
            to_copy = state->total - state->sent;
        }
        if (to_copy > 0) {
            memcpy(transfer->buffer, state->data + state->sent, to_copy);
            state->sent += to_copy;
            if (to_copy < (size_t)transfer->buffer_length) {
                memset(transfer->buffer + to_copy, 0, transfer->buffer_length - to_copy);
            }
        } else {
            memset(transfer->buffer, 0, transfer->buffer_length);
        }

        return (state->sent >= state->total) ? 1 : 0;
    };

    int result = hackrf_start_tx(device, tx_callback, &tx_state);
    if (result != HACKRF_SUCCESS) {
        printf("hackrf_start_tx() failed\n");
        return false;
    }

    while (tx_state.sent < tx_state.total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    hackrf_stop_tx(device);
    printf("Transmission complete.\n");

    return true;
}
