
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

#include "libbz3.h"

#include <stdlib.h>
#include <string.h>

#include "cm.h"
#include "common.h"
#include "crc32.h"
#include "libsais.h"
#include "lzp.h"
#include "rle.h"

#define LZP_DICTIONARY 18
#define LZP_MIN_MATCH 40

struct bz3_state {
    u8 * swap_buffer;
    s32 block_size;
    s32 *sais_array, *lzp_lut;
    state * cm_state;
    s8 last_error;
};

PUBLIC_API s8 bz3_last_error(struct bz3_state * state) { return state->last_error; }

PUBLIC_API const char * bz3_strerror(struct bz3_state * state) {
    switch (state->last_error) {
        case BZ3_OK:
            return "No error";
        case BZ3_ERR_OUT_OF_BOUNDS:
            return "Data index out of bounds";
        case BZ3_ERR_BWT:
            return "Burrows-Wheeler transform failed";
        case BZ3_ERR_CRC:
            return "CRC32 check failed";
        case BZ3_ERR_MALFORMED_HEADER:
            return "Malformed header";
        case BZ3_ERR_TRUNCATED_DATA:
            return "Truncated data";
        case BZ3_ERR_DATA_TOO_BIG:
            return "Too much data";
        default:
            return "Unknown error";
    }
}

PUBLIC_API struct bz3_state * bz3_new(s32 block_size) {
    struct bz3_state * bz3_state = malloc(sizeof(struct bz3_state));

    if (!bz3_state) {
        return NULL;
    }

    bz3_state->cm_state = malloc(sizeof(state));

    bz3_state->swap_buffer = malloc(block_size + block_size / 50 + 16);
    bz3_state->sais_array = malloc(block_size * sizeof(s32));

    bz3_state->lzp_lut = calloc(1 << LZP_DICTIONARY, sizeof(s32));

    if (!bz3_state->cm_state || !bz3_state->swap_buffer || !bz3_state->sais_array || !bz3_state->lzp_lut) {
        if(bz3_state->cm_state)
            free(bz3_state->cm_state);
        if(bz3_state->swap_buffer)
            free(bz3_state->swap_buffer);
        if(bz3_state->sais_array)
            free(bz3_state->sais_array);
        if(bz3_state->lzp_lut)
            free(bz3_state->lzp_lut);

        return NULL;
    }

    bz3_state->block_size = block_size;

    bz3_state->last_error = BZ3_OK;

    return bz3_state;
}

PUBLIC_API void bz3_free(struct bz3_state * state) {
    free(state->swap_buffer);
    free(state->sais_array);
    free(state->cm_state);
    free(state->lzp_lut);
    free(state);
}

#define swap(x, y)    \
    {                 \
        u8 * tmp = x; \
        x = y;        \
        y = tmp;      \
    }

PUBLIC_API s32 bz3_encode_block(struct bz3_state * state, u8 * buffer, s32 data_size) {
    u8 *b1 = buffer, *b2 = state->swap_buffer;

    if (data_size > state->block_size) {
        state->last_error = BZ3_ERR_DATA_TOO_BIG;
        return -1;
    }

    u32 crc32 = crc32sum(1, b1, data_size);

    // Ignore small blocks. They won't benefit from the entropy coding step.
    if (data_size < 64) {
        write_neutral_s32(b1, crc32);
        write_neutral_s32(b1 + 4, -1);
        memmove(b1 + 8, b1, data_size);
        return data_size + 8;
    }

    // Back to front:
    // bit 1: lzp | no lzp
    // bit 2: srt | no srt
    s8 model = 0;
    s32 lzp_size, rle_size;

    rle_size = mrlec(b1, data_size, b2);
    if (rle_size < data_size + 64) {
        swap(b1, b2);
        data_size = rle_size;
        model |= 4;
    }

    lzp_size = lzp_compress(b1, b2, data_size, LZP_DICTIONARY, LZP_MIN_MATCH, state->lzp_lut);
    if (lzp_size > 0) {
        swap(b1, b2);
        data_size = lzp_size;
        model |= 2;
    }

    s32 bwt_idx = libsais_bwt(b1, b2, state->sais_array, data_size, 0, NULL);
    if (bwt_idx < 0) {
        state->last_error = BZ3_ERR_BWT;
        return -1;
    }

    // Compute the amount of overhead dwords.
    s32 overhead = 2;           // CRC32 + BWT index
    if (model & 2) overhead++;  // LZP
    if (model & 4) overhead++;  // RLE

    begin(state->cm_state);
    state->cm_state->out_queue = b1 + overhead * 4 + 1;
    state->cm_state->output_ptr = 0;
    for (s32 i = 0; i < data_size; i++) encode_byte(state->cm_state, b2[i]);
    flush(state->cm_state);
    data_size = state->cm_state->output_ptr;

    // Write the header. Starting with common entries.
    write_neutral_s32(b1, crc32);
    write_neutral_s32(b1 + 4, bwt_idx);
    b1[8] = model;

    s32 p = 0;
    if (model & 2) write_neutral_s32(b1 + 9 + 4 * p++, lzp_size);
    if (model & 4) write_neutral_s32(b1 + 9 + 4 * p++, rle_size);

    state->last_error = BZ3_OK;

    // XXX: Better solution
    if (b1 != buffer) memcpy(buffer, b1, data_size + overhead * 4 + 1);

    return data_size + overhead * 4 + 1;
}

PUBLIC_API s32 bz3_decode_block(struct bz3_state * state, u8 * buffer, s32 data_size, s32 orig_size) {
    // Read the header.
    u32 crc32 = read_neutral_s32(buffer);
    s32 bwt_idx = read_neutral_s32(buffer + 4);

    if (bwt_idx == -1) {
        memmove(buffer, buffer + 8, data_size - 8);
        return data_size - 8;
    }

    if (orig_size > state->block_size) {
        state->last_error = BZ3_ERR_DATA_TOO_BIG;
        return -1;
    }

    s8 model = buffer[8];
    s32 lzp_size = -1, rle_size, p = 0;

    if (model & 2) lzp_size = read_neutral_s32(buffer + 9 + 4 * p++);
    if (model & 4) rle_size = read_neutral_s32(buffer + 9 + 4 * p++);

    p += 2;

    data_size -= p * 4 + 1;

    // Decode the data.
    u8 *b1 = buffer, *b2 = state->swap_buffer;

    begin(state->cm_state);
    state->cm_state->in_queue = b1 + p * 4 + 1;
    state->cm_state->input_ptr = 0;
    state->cm_state->input_max = data_size;
    init(state->cm_state);

    s32 size_src;

    if (model & 2)
        size_src = lzp_size;
    else if (model & 4)
        size_src = rle_size;
    else
        size_src = orig_size;

    for (s32 i = 0; i < size_src; i++) b2[i] = decode_byte(state->cm_state);
    swap(b1, b2);

    // Undo BWT
    if (libsais_unbwt(b1, b2, state->sais_array, size_src, NULL, bwt_idx) < 0) {
        state->last_error = BZ3_ERR_BWT;
        return -1;
    }
    swap(b1, b2);

    // Undo LZP
    if (model & 2) {
        size_src = lzp_decompress(b1, b2, lzp_size, LZP_DICTIONARY, LZP_MIN_MATCH, state->lzp_lut);
        swap(b1, b2);
    }

    if (model & 4) {
        mrled(b1, b2, orig_size);
        size_src = orig_size;
        swap(b1, b2);
    }

    state->last_error = BZ3_OK;

    // XXX: Better solution
    if (b1 != buffer) memcpy(buffer, b1, size_src);

    if (crc32 != crc32sum(1, buffer, size_src)) {
        state->last_error = BZ3_ERR_CRC;
        return -1;
    }

    return size_src;
}

#undef swap

#include <pthread.h>

typedef struct {
    struct bz3_state * state;
    uint8_t * buffer;
    int32_t size;
} encode_thread_msg;

typedef struct {
    struct bz3_state * state;
    uint8_t * buffer;
    int32_t size;
    int32_t orig_size;
} decode_thread_msg;

static void bz3_init_encode_thread(encode_thread_msg * msg) {
    msg->size = bz3_encode_block(msg->state, msg->buffer, msg->size);
    pthread_exit(NULL);
}

static void bz3_init_decode_thread(decode_thread_msg * msg) {
    bz3_decode_block(msg->state, msg->buffer, msg->size, msg->orig_size);
    pthread_exit(NULL);
}

PUBLIC_API void bz3_encode_blocks(struct bz3_state * states[], uint8_t * buffers[], int32_t sizes[], int32_t n) {
    encode_thread_msg messages[n];
    pthread_t threads[n];
    for(int32_t i = 0; i < n; i++) {
        messages[i].state = states[i];
        messages[i].buffer = buffers[i];
        messages[i].size = sizes[i];
        pthread_create(&threads[i], NULL, (void *(*)(void *)) bz3_init_encode_thread, &messages[i]);
    }
    for(int32_t i = 0; i < n; i++)
        pthread_join(threads[i], NULL);
    for(int32_t i = 0; i < n; i++)
        sizes[i] = messages[i].size;
}

PUBLIC_API void bz3_decode_blocks(struct bz3_state * states[], uint8_t * buffers[], int32_t sizes[], int32_t orig_sizes[], int32_t n) {
    decode_thread_msg messages[n];
    pthread_t threads[n];
    for(int32_t i = 0; i < n; i++) {
        messages[i].state = states[i];
        messages[i].buffer = buffers[i];
        messages[i].size = sizes[i];
        messages[i].orig_size = orig_sizes[i];
        pthread_create(&threads[i], NULL, (void *(*)(void *)) bz3_init_decode_thread, &messages[i]);
    }
    for(int32_t i = 0; i < n; i++)
        pthread_join(threads[i], NULL);
}
