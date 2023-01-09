
/*
 * BZip3 - A spiritual successor to BZip2.
 * Copyright (C) 2022 Kamila Szewczyk
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LIBBZ3_H
#define _LIBBZ3_H

#include <stddef.h>
#include <stdint.h>

/* Symbol visibility control. */
#ifndef BZIP3_VISIBLE
    #if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__MINGW32__)
        #define BZIP3_VISIBLE __attribute__((visibility("default")))
    #else
        #define BZIP3_VISIBLE
    #endif
#endif

#if defined(BZIP3_DLL_EXPORT) && (BZIP3_DLL_EXPORT == 1)
    #define BZIP3_API __declspec(dllexport) BZIP3_VISIBLE
#elif defined(BZIP3_DLL_IMPORT) && (BZIP3_DLL_IMPORT == 1)
    #define BZIP3_API __declspec(dllimport) BZIP3_VISIBLE
#else
    #define BZIP3_API BZIP3_VISIBLE
#endif

#define BZ3_OK 0
#define BZ3_ERR_OUT_OF_BOUNDS -1
#define BZ3_ERR_BWT -2
#define BZ3_ERR_CRC -3
#define BZ3_ERR_MALFORMED_HEADER -4
#define BZ3_ERR_TRUNCATED_DATA -5
#define BZ3_ERR_DATA_TOO_BIG -6
#define BZ3_ERR_INIT -7

struct bz3_state;

/**
 * @brief Get bzip3 version.
 */
BZIP3_API const char * bz3_version(void);

/**
 * @brief Get the last error number associated with a given state.
 */
BZIP3_API int8_t bz3_last_error(struct bz3_state * state);

/**
 * @brief Return a user-readable message explaining the cause of the last error.
 */
BZIP3_API const char * bz3_strerror(struct bz3_state * state);

/**
 * @brief Construct a new block encoder state, which will encode blocks as big as the given block size.
 * The decoder will be able to decode blocks at most as big as the given block size.
 * Returns NULL in case allocation fails or the block size is not between 65K and 511M
 */
BZIP3_API struct bz3_state * bz3_new(int32_t block_size);

/**
 * @brief Free the memory occupied by a block encoder state.
 */
BZIP3_API void bz3_free(struct bz3_state * state);

/**
 * @brief Return the recommended size of the output buffer for the compression functions.
 */
BZIP3_API size_t bz3_bound(size_t input_size);

/* ** HIGH LEVEL APIs ** */

/**
 * @brief Compress a block of data. This function does not support parallelism
 * by itself, consider using the low level `bz3_encode_blocks()` function instead.
 * Using the low level API might provide better performance.
 * Returns a bzip3 error code; BZ3_OK when the operation is successful.
 * Make sure to set out_size to the size of the output buffer before the operation;
 * out_size must be at least equal to `bz3_bound(in_size)'.
 */
BZIP3_API int bz3_compress(uint32_t block_size, const uint8_t * in, uint8_t * out, size_t in_size, size_t * out_size);

/**
 * @brief Decompress a block of data. This function does not support parallelism
 * by itself, consider using the low level `bz3_decode_blocks()` function instead.
 * Using the low level API might provide better performance.
 * Returns a bzip3 error code; BZ3_OK when the operation is successful.
 * Make sure to set out_size to the size of the output buffer before the operation.
 */
BZIP3_API int bz3_decompress(const uint8_t * in, uint8_t * out, size_t in_size, size_t * out_size);

/* ** LOW LEVEL APIs ** */

/**
 * @brief Encode a single block. Returns the amount of bytes written to `buffer'.
 * `buffer' must be able to hold at least `bz3_bound(size)' bytes. The size must not
 * exceed the block size associated with the state.
 */
BZIP3_API int32_t bz3_encode_block(struct bz3_state * state, uint8_t * buffer, int32_t size);

/**
 * @brief Decode a single block.
 * `buffer' must be able to hold at least `orig_size' bytes. The size must not exceed the block size
 * associated with the state.
 * @param size The size of the compressed data in `buffer'
 * @param orig_size The original size of the data before compression.
 */
BZIP3_API int32_t bz3_decode_block(struct bz3_state * state, uint8_t * buffer, int32_t size, int32_t orig_size);

/**
 * @brief Encode `n' blocks, all in parallel.
 * All specifics of the `bz3_encode_block' still hold. The function will launch a thread for each block.
 * The compressed sizes are written to the `sizes' array. Every buffer is overwritten and none of them can overlap.
 * Precisely `n' states, buffers and sizes must be supplied.
 *
 * Expects `n' between 2 and 16.
 *
 * Present in the shared library only if -lpthread was present during building.
 */
BZIP3_API void bz3_encode_blocks(struct bz3_state * states[], uint8_t * buffers[], int32_t sizes[], int32_t n);

/**
 * @brief Decode `n' blocks, all in parallel.
 * Same specifics as `bz3_encode_blocks', but doesn't overwrite `sizes'.
 */
BZIP3_API void bz3_decode_blocks(struct bz3_state * states[], uint8_t * buffers[], int32_t sizes[],
                                 int32_t orig_sizes[], int32_t n);

#endif
