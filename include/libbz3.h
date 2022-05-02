
#ifndef _LIBBZ3_H
#define _LIBBZ3_H

#include "common.h"

#define BZ3_OK 0
#define BZ3_ERR_OUT_OF_BOUNDS -1
#define BZ3_ERR_BWT -2
#define BZ3_ERR_CRC -3
#define BZ3_ERR_MALFORMED_HEADER -4
#define BZ3_ERR_TRUNCATED_DATA -5

struct block_encoder_state;

struct encoding_result {
    u8 * buffer;
    s32 size;
};

/**
 * @brief Get the last error number associated with a given state.
 */
s8 get_last_error(struct block_encoder_state * state);

/**
 * @brief Return a user-readable message explaining the cause of the error.
 */
const char * str_last_error(struct block_encoder_state * state);

/**
 * @brief Get the input buffer associated with given state. Fill it with data
 * of length not exceeding the block size and call commit_read() to commit
 * the read operation with the number of bytes read.
 */
u8 * get_buffer(struct block_encoder_state * state);

/**
 * @brief Commit the amount of bytes inserted into the buffer.
 */
s32 commit_read(struct block_encoder_state * state, s32 bytes_read);

/**
 * @brief Construct a new block encoder state.
 */
struct block_encoder_state * new_block_encoder_state(s32 block_size);

/**
 * @brief Free the memory occupied by a block encoder state.
 */
void delete_block_encoder_state(struct block_encoder_state * state);

/**
 * @brief Read a block of data from provided file descriptor, put it in
 * the input buffer and commit the read.
 *
 * @param filedes
 * @param state
 * @return s32
 */
s32 read_block(int filedes, struct block_encoder_state * state);

/**
 * @brief Encode a single block.
 */
struct encoding_result encode_block(struct block_encoder_state * state);

/**
 * @brief Decode a single block.
 */
struct encoding_result decode_block(struct block_encoder_state * state);

#endif
