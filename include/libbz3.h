
/*
 * BZip3 - A spiritual successor to BZip2.
 * Copyright (C) 2022 Kamila Szewczyk
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LIBBZ3_H
#define _LIBBZ3_H

#include <stdint.h>

#define BZ3_OK 0
#define BZ3_ERR_OUT_OF_BOUNDS -1
#define BZ3_ERR_BWT -2
#define BZ3_ERR_CRC -3
#define BZ3_ERR_MALFORMED_HEADER -4
#define BZ3_ERR_TRUNCATED_DATA -5
#define BZ3_ERR_DATA_TOO_BIG -6

struct bz3_state;

/**
 * @brief Get the last error number associated with a given state.
 */
int8_t bz3_last_error(struct bz3_state * state);

/**
 * @brief Return a user-readable message explaining the cause of the last error.
 */
const char * bz3_strerror(struct bz3_state * state);

/**
 * @brief Construct a new block encoder state, which will encode blocks as big as the given block size.
 * The decoder will be able to decode blocks at most as big as the given block size.
 */
struct bz3_state * bz3_new(int32_t block_size);

/**
 * @brief Free the memory occupied by a block encoder state.
 */
void bz3_free(struct bz3_state * state);

/**
 * @brief Encode a single block. Returns the amount of bytes written to `buffer'.
 * `buffer' must be able to hold at least `size + size / 4' bytes. The size must not
 * exceed the block size associated with the state.
 */
int32_t bz3_encode_block(struct bz3_state * state, uint8_t * buffer, int32_t size);

/**
 * @brief Decode a single block.
 * `buffer' must be able to hold at least `size + size / 4' bytes. The size must not exceed
 * the block size associated with the state.
 * @param size The size of the compressed data in `buffer'
 * @param orig_size The original size of the data before compression.
 */
int32_t bz3_decode_block(struct bz3_state * state, uint8_t * buffer, int32_t size, int32_t orig_size);

#endif
