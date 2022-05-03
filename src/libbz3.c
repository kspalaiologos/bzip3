
#include "libbz3.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "cm.h"
#include "common.h"
#include "crc32.h"
#include "libsais.h"
#include "lzp.h"
#include "mtf.h"
#include "rle.h"
#include "srt.h"
#include "txt.h"

#define LZP_DICTIONARY 18
#define LZP_MIN_MATCH 100

struct block_encoder_state {
    u8 *buf1, *buf2;
    s32 bytes_read, block_size;
    s32 * sais_array;
    struct srt_state * srt_state;
    struct mtf_state * mtf_state;
    state * cm_state;
    s8 last_error;
};

s8 get_last_error(struct block_encoder_state * state) { return state->last_error; }

const char * str_last_error(struct block_encoder_state * state) {
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
        default:
            return "Unknown error";
    }
}

u8 * get_buffer(struct block_encoder_state * state) { return state->buf1; }

s32 commit_read(struct block_encoder_state * state, s32 bytes_read) {
    if (bytes_read > state->block_size) {
        state->last_error = BZ3_ERR_OUT_OF_BOUNDS;
        return -1;
    }
    state->last_error = BZ3_OK;
    return state->bytes_read = bytes_read;
}

struct block_encoder_state * new_block_encoder_state(s32 block_size) {
    struct block_encoder_state * block_encoder_state = malloc(sizeof(struct block_encoder_state));

    if (!block_encoder_state) {
        return NULL;
    }

    block_encoder_state->cm_state = malloc(sizeof(state));
    block_encoder_state->srt_state = malloc(sizeof(struct srt_state));
    block_encoder_state->mtf_state = malloc(sizeof(struct mtf_state));

    block_encoder_state->buf1 = malloc(block_size + block_size / 3);
    block_encoder_state->buf2 = malloc(block_size + block_size / 3);
    block_encoder_state->sais_array = malloc(block_size * sizeof(s32) + 16);

    block_encoder_state->block_size = block_size;

    block_encoder_state->last_error = BZ3_OK;

    return block_encoder_state;
}

void delete_block_encoder_state(struct block_encoder_state * state) {
    free(state->buf1);
    free(state->buf2);
    free(state->sais_array);
    free(state->srt_state);
    free(state->mtf_state);
    free(state->cm_state);
    free(state);
}

// TODO: Wire up RLE with lzp percentage checking.

#define swap(x, y) { u8 * tmp = x; x = y; y = tmp; }

struct encoding_result encode_block(struct block_encoder_state * state) {
    u8 * b1 = state->buf1, * b2 = state->buf2;
    s32 data_size = state->bytes_read;
    
    u32 crc32 = crc32sum(1, b1, data_size);

    // Back to front:
    // bit 0: text | binary
    // bit 1: lzp | no lzp
    // bit 2: srt | no srt
    s8 model = is_text(b1, data_size);

    s32 lzp_size;
    if(model)
        lzp_size = lzp_compress(b1, b2, data_size, LZP_DICTIONARY, LZP_MIN_MATCH);
    else
        lzp_size = lzp_compress(b1, b2, data_size, LZP_DICTIONARY, 2 * LZP_MIN_MATCH);
    if(lzp_size > 0) {
        swap(b1, b2);
        data_size = lzp_size;
        model |= 2;
    }

    s32 bwt_idx = libsais_bwt(b1, b2, state->sais_array, data_size, 16, NULL);
    if(bwt_idx < 0) {
        state->last_error = BZ3_ERR_BWT;
        return (struct encoding_result) { .buffer = NULL, .size = -1 };
    }
    swap(b1, b2);
    
    s32 srt_size;
    if((model & 1) == 0) {
        if(data_size > MiB(3)) {
            srt_size = srt_encode(state->srt_state, b1, b2, data_size);
            swap(b1, b2);
            data_size = srt_size;
            model |= 4;
        } else {
            mtf_encode(state->mtf_state, b1, b2, data_size);
            swap(b1, b2);
            model |= 8;
        }
    }

    // Compute the amount of overhead dwords.
    s32 overhead = 4; // CRC32 + BWT index + original size + new size
    if(model & 2) overhead++; // LZP
    if(model & 4) overhead++; // sorted rank transform

    begin(state->cm_state);
    state->cm_state->out_queue = b2 + overhead * 4 + 1;
    state->cm_state->output_ptr = 0;
    for (s32 i = 0; i < data_size; i++) encode_byte(state->cm_state, b1[i]);
    flush(state->cm_state);
    data_size = state->cm_state->output_ptr;

    // Write the header. Starting with common entries:
    ((s32 *) (b2))[0] = htonl(data_size + overhead * 4 - 3);
    ((u32 *) (b2))[1] = htonl(crc32);
    ((s32 *) (b2))[2] = htonl(bwt_idx);
    ((s32 *) (b2))[3] = htonl(state->bytes_read);
    b2[16] = model;

    s32 p = 0;
    if(model & 2) ((s32 *)(b2 + 17))[p++] = htonl(lzp_size);
    if(model & 4) ((s32 *)(b2 + 17))[p++] = htonl(srt_size);

    return (struct encoding_result) { .buffer = b2, .size = data_size + overhead * 4 + 1 };
}

struct encoding_result decode_block(struct block_encoder_state * state) {
    // Read the header.
    s32 data_len = ntohl(((s32 *) state->buf1)[0]) - 1;
    u32 crc32 = ntohl(((u32 *) state->buf1)[1]);
    s32 bwt_idx = ntohl(((s32 *) state->buf1)[2]);
    s32 orig_size = ntohl(((s32 *) state->buf1)[3]);
    s8 model = state->buf1[16];
    s32 lzp_size = -1, srt_size = -1, p = 0;

    if(model & 2) lzp_size = ntohl(((s32 *) (state->buf1 + 17))[p++]);
    if(model & 4) srt_size = ntohl(((s32 *) (state->buf1 + 17))[p++]);

    data_len -= p * 4;

    // Decode the data.
    u8 * b1 = state->buf1, * b2 = state->buf2;

    begin(state->cm_state);
    state->cm_state->in_queue = b1 + 17 + p * 4;
    state->cm_state->input_ptr = 0;
    state->cm_state->input_max = data_len;
    init(state->cm_state);

    s32 size_src;

    if(model & 4)
        size_src = srt_size;
    else if(model & 2)
        size_src = lzp_size;
    else
        size_src = orig_size;

    for (s32 i = 0; i < size_src; i++)
        b2[i] = decode_byte(state->cm_state);
    swap(b1, b2);

    // Undo SRT
    if(model & 4) {
        size_src = srt_decode(state->srt_state, b1, b2, srt_size);
        swap(b1, b2);
    } else if(model & 8) {
        mtf_decode(state->mtf_state, b1, b2, size_src);
        swap(b1, b2);
    }

    // Undo BWT
    if (libsais_unbwt(b1, b2, state->sais_array, size_src, NULL, bwt_idx) < 0) {
        state->last_error = BZ3_ERR_BWT;
        return (struct encoding_result) { .buffer = NULL, .size = -1 };
    }
    swap(b1, b2);

    // Undo LZP
    if(model & 2) {
        size_src = lzp_decompress(b1, b2, lzp_size, LZP_DICTIONARY, (model & 1) ? LZP_MIN_MATCH : 2 * LZP_MIN_MATCH);
        swap(b1, b2);
    }

    return (struct encoding_result) { .buffer = b1, .size = size_src };
}

#undef swap

s32 read_block(int filedes, struct block_encoder_state * state) {
    s32 bytes_read = read(filedes, state->buf1, 4);
    if (bytes_read == 0) return 0;
    if (bytes_read != 4) {
        state->last_error = BZ3_ERR_MALFORMED_HEADER;
        return -1;
    }
    s32 data_size = ntohl(((uint32_t *)state->buf1)[0]);
    if (data_size > state->block_size) {
        state->last_error = BZ3_ERR_MALFORMED_HEADER;
        return -1;
    }
    bytes_read = read(filedes, state->buf1 + 4, data_size);
    if (bytes_read != data_size) {
        state->last_error = BZ3_ERR_TRUNCATED_DATA;
        return -1;
    }
    state->last_error = BZ3_OK;
    return state->bytes_read = 4 + data_size;
}
