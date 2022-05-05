
#ifndef _LIBBZ3_H
#define _LIBBZ3_H

#include "common.h"

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
s8 bz3_last_error(struct bz3_state * state);

/**
 * @brief Return a user-readable message explaining the cause of the last error.
 */
const char * bz3_strerror(struct bz3_state * state);

/**
 * @brief Construct a new block encoder state, which will encode blocks as big as the given block size.
 * The decoder will be able to decode blocks at most as big as the given block size.
 */
struct bz3_state * bz3_new(s32 block_size);

/**
 * @brief Free the memory occupied by a block encoder state.
 */
void bz3_free(struct bz3_state * state);

/**
 * @brief Encode a single block. Returns the amount of bytes written to `buffer'.
 * `buffer' must be able to hold at least `size + size / 4' bytes. The size must not
 * exceed the block size associated with the state.
 */
s32 bz3_encode_block(struct bz3_state * state, u8 * buffer, s32 size);

/**
 * @brief Decode a single block.
 * `buffer' must be able to hold at least `size + size / 4' bytes. The size must not exceed
 * the block size associated with the state.
 * @param size The size of the compressed data in `buffer'
 * @param orig_size The original size of the data before compression.
 */
s32 bz3_decode_block(struct bz3_state * state, u8 * buffer, s32 size, s32 orig_size);

#endif
