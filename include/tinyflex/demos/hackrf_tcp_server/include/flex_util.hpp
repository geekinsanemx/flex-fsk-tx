#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include "../../../tinyflex.h"

inline bool encode_flex_message(const std::string& message, uint64_t capcode, uint8_t* flex_buffer, size_t flex_buffer_size, size_t& flex_len, int& error) {
    memset(flex_buffer, 0, flex_buffer_size);
    error = 0;
    flex_len = tf_encode_flex_message(message.c_str(), capcode, flex_buffer, flex_buffer_size, &error);
    return (flex_len != 0 && error == 0);
}
