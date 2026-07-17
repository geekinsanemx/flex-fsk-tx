/*
 * tinyflex: A minimal, dependency-free, single-header library, FLEX encoder.
 * Written by Davidson Francis (aka Theldus) - 2025.
 *
 * This is free and unencumbered software released into the public domain.
 */

/* WebAssembly wrapper for tinyflex encoding functionality. */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "tinyflex.h"

/* Global buffer to store encoded data for JS access. */
static uint8_t global_buffer[FLEX_BUFFER_SIZE];
static size_t global_size = 0;
static int last_error = 0;

/* Global config struct, initialized once just like CLI tool. */
static struct tf_message_config global_config = {0};

/*
 * @brief Safe string-to-uint64_t routine.
 * Handles overflow, invalid characters, and rejects negative input.
 *
 * @param out Pointer to uint64_t.
 * @param s String to be converted.
 *
 * @return Returns 0 on success, -1 on error.
 */
static int str2uint64(uint64_t *out, const char *s)
{
    char *end;
    unsigned long long ull;
    const char *p = s;

    /* Check for empty. */
    if (p[0] == '\0')
        return -1;

    errno = 0;
    ull = strtoull(p, &end, 10);

    /* Check if:
     * - No digits were found
     * - Overflow
     * - Extra chars at the end
     * - If fits into uint64_t
     */
    if (end == p || errno == ERANGE || *end != '\0' || ull > UINT64_MAX)
        return -1;

    *out = (uint64_t)ull;
    return 0;
}

/**
 * @brief Encodes a FLEX message with string capcode.
 *
 * @param msg         Message to be encoded.
 * @param capcode_str Capcode as string to avoid JS precision issues.
 * @param mail_drop   Mail drop flag (0 or 1).
 * 
 * @return Returns the size of encoded data on success, 0 on error.
 */
size_t encode_flex_message_str(const char *msg, const char *capcode_str, int mail_drop)
{
	int error = 0;
	size_t msg_len;
	uint64_t capcode;
	
	/* Parse capcode string to uint64_t */
	if (str2uint64(&capcode, capcode_str) != 0) {
		last_error = TF_INVALID_CAPCODE;
		return 0;
	}
	
	/* Clear previous data. */
	memset(global_buffer, 0, sizeof(global_buffer));
	global_size = 0;
	last_error = 0;
	
	/* Validate input parameters. */
	if (!msg) {
		last_error = -100; /* NULL message */
		return 0;
	}
	
	msg_len = strlen(msg);
	if (msg_len == 0) {
		last_error = -101; /* Empty message */
		return 0;
	}
	
	if (msg_len >= MAX_CHARS_ALPHA) {
		last_error = -102; /* Message too long */
		return 0;
	}
	
	/* Set mail drop flag in global config, just like CLI tool. */
	global_config.mail_drop = (mail_drop != 0) ? 1 : 0;
	
	global_size = tf_encode_flex_message_ex(msg, capcode, global_buffer,
		sizeof(global_buffer), &error, &global_config);
	
	/* Store the error for debugging. */
	last_error = error;
	
	/* Return 0 on encoding error. */
	if (error < 0) {
		global_size = 0;
		return 0;
	}
	
	return global_size;
}

/**
 * @brief Returns pointer to the encoded data buffer.
 * @return Pointer to global buffer containing encoded data.
 */
uint8_t* get_encoded_data(void)
{
	return global_buffer;
}

/**
 * @brief Returns the size of the encoded data.
 * @return Size of encoded data in bytes.
 */
size_t get_encoded_size(void)
{
	return global_size;
}

/**
 * @brief Returns the last error code.
 * @return Last error code from encoding operation.
 */
int get_last_error(void)
{
	return last_error;
}