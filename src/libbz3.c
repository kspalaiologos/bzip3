
#include "libbz3.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "cm.h"
#include "common.h"
#include "crc32.h"
#include "libsais.h"
#include "mtf.h"
#include "txt.h"
#include "rle.h"
#include "srt.h"

struct block_encoder_state {
    u8 *buf1, *buf2;
    s32 bytes_read, block_size;
    s32 * sais_array;
    struct srt_state * srt_state;
    struct mtf_state * mtf_state;
    state * cm_state;
    s8 last_error;
};

s8 get_last_error(struct block_encoder_state * state) {
    return state->last_error;
}

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
    struct block_encoder_state * block_encoder_state =
        malloc(sizeof(struct block_encoder_state));

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

struct encoding_result encode_block(struct block_encoder_state * state) {
    u32 crc32 = crc32sum(1, state->buf1, state->bytes_read);

    int txt = is_text(state->buf1, state->bytes_read);

    if(txt) {
        s32 bwt_index = libsais_bwt(state->buf1, state->buf1, state->sais_array,
                                    state->bytes_read, 16, NULL);
        if(bwt_index < 0) {
            state->last_error = BZ3_ERR_BWT;
            return (struct encoding_result) { NULL, -1 };
        }
        begin(state->cm_state);
        state->cm_state->out_queue = state->buf2 + 24;
        state->cm_state->output_ptr = 0;
        for(s32 i = 0; i < state->bytes_read; i++)
            encode_byte(state->cm_state, state->buf1[i]);
        flush(state->cm_state);
        s32 new_size = state->cm_state->output_ptr;

        ((uint32_t *)state->buf2)[0] = htonl(crc32);
        ((uint32_t *)state->buf2)[1] = htonl(state->bytes_read);
        ((uint32_t *)state->buf2)[2] = htonl(bwt_index);
        ((uint32_t *)state->buf2)[3] = 0xFFFFFFFF;
        ((uint32_t *)state->buf2)[4] = 0xFFFFFFFF;
        ((uint32_t *)state->buf2)[5] = htonl(new_size);
        
        state->last_error = BZ3_OK;
        return (struct encoding_result) { state->buf2, 24 + new_size };
    } else {
        s32 new_size = mrlec(state->buf1, state->bytes_read, state->buf2);
        s32 bwt_index = libsais_bwt(state->buf2, state->buf2, state->sais_array,
                                    new_size, 16, NULL);
        if (bwt_index < 0) {
            state->last_error = BZ3_ERR_BWT;
            return (struct encoding_result){ NULL, -1 };
        }
        s32 new_size2;

        if (new_size > MiB(3)) {
            new_size2 =
                srt_encode(state->srt_state, state->buf2, state->buf1, new_size);
        } else {
            new_size2 = -1;
            mtf_encode(state->mtf_state, state->buf2, state->buf1, new_size);
        }

        begin(state->cm_state);
        state->cm_state->out_queue = state->buf2 + 24;
        state->cm_state->output_ptr = 0;
        if (new_size2 != -1)
            for (s32 i = 0; i < new_size2; i++)
                encode_byte(state->cm_state, state->buf1[i]);
        else
            for (s32 i = 0; i < new_size; i++)
                encode_byte(state->cm_state, state->buf1[i]);
        flush(state->cm_state);
        s32 new_size3 = state->cm_state->output_ptr;

        ((uint32_t *)state->buf2)[0] = htonl(crc32);
        ((uint32_t *)state->buf2)[1] = htonl(state->bytes_read);
        ((uint32_t *)state->buf2)[2] = htonl(bwt_index);
        ((uint32_t *)state->buf2)[3] = htonl(new_size);
        ((uint32_t *)state->buf2)[4] = htonl(new_size2);
        ((uint32_t *)state->buf2)[5] = htonl(new_size3);
        state->last_error = BZ3_OK;
        return (struct encoding_result){ .buffer = state->buf2,
                                        .size = 24 + new_size3 };
    }
}

struct encoding_result decode_block(struct block_encoder_state * state) {
    u32 crc32;
    s32 bwt_index, new_size, new_size2, new_size3;

    crc32 = ntohl(((uint32_t *)state->buf1)[0]);
    state->bytes_read = ntohl(((uint32_t *)state->buf1)[1]);
    bwt_index = ntohl(((uint32_t *)state->buf1)[2]);
    new_size = ntohl(((uint32_t *)state->buf1)[3]);
    new_size2 = ntohl(((uint32_t *)state->buf1)[4]);
    new_size3 = ntohl(((uint32_t *)state->buf1)[5]);

    if(new_size2 != 0xFFFFFFFF || new_size != 0xFFFFFFFF) {
        begin(state->cm_state);
        state->cm_state->in_queue = state->buf1 + 24;
        state->cm_state->input_ptr = 0;
        state->cm_state->input_max = new_size3;
        init(state->cm_state);
        if (new_size2 != -1) {
            for (s32 i = 0; i < new_size2; i++)
                state->buf2[i] = decode_byte(state->cm_state);
            srt_decode(state->srt_state, state->buf2, state->buf1, new_size2);
        } else {
            for (s32 i = 0; i < new_size; i++)
                state->buf2[i] = decode_byte(state->cm_state);
            mtf_decode(state->mtf_state, state->buf2, state->buf1, new_size);
        }
        if (libsais_unbwt(state->buf1, state->buf2, state->sais_array, new_size,
                        NULL, bwt_index) < 0) {
            state->last_error = BZ3_ERR_BWT;
            return (struct encoding_result){ NULL, -1 };
        }
        mrled(state->buf2, state->buf1, state->bytes_read);
        if (crc32sum(1, state->buf1, state->bytes_read) != crc32) {
            state->last_error = BZ3_ERR_CRC;
            return (struct encoding_result){ .buffer = NULL, .size = -1 };
        }
        state->last_error = BZ3_OK;
        return (struct encoding_result){ .buffer = state->buf1,
                                        .size = state->bytes_read };
    } else {
        begin(state->cm_state);
        state->cm_state->in_queue = state->buf1 + 24;
        state->cm_state->input_ptr = 0;
        state->cm_state->input_max = new_size3;
        init(state->cm_state);
        for (s32 i = 0; i < state->bytes_read; i++)
            state->buf2[i] = decode_byte(state->cm_state);
        if (libsais_unbwt(state->buf2, state->buf1, state->sais_array, state->bytes_read,
                        NULL, bwt_index) < 0) {
            state->last_error = BZ3_ERR_BWT;
            return (struct encoding_result){ NULL, -1 };
        }
        if (crc32sum(1, state->buf1, state->bytes_read) != crc32) {
            state->last_error = BZ3_ERR_CRC;
            return (struct encoding_result){ .buffer = NULL, .size = -1 };
        }
        state->last_error = BZ3_OK;
        return (struct encoding_result){ .buffer = state->buf1,
                                         .size = state->bytes_read };
    }
}

s32 read_block(int filedes, struct block_encoder_state * state) {
    s32 metadata[6];
    s32 bytes_read = read(filedes, state->buf1, 24);
    if (bytes_read == 0) return 0;
    if (bytes_read != 24) {
        state->last_error = BZ3_ERR_MALFORMED_HEADER;
        return -1;
    }
    s32 data_size = ntohl(((uint32_t *)state->buf1)[5]);
    if (data_size > state->block_size) {
        state->last_error = BZ3_ERR_MALFORMED_HEADER;
        return -1;
    }
    bytes_read = read(filedes, state->buf1 + 24, data_size);
    if (bytes_read != data_size) {
        state->last_error = BZ3_ERR_TRUNCATED_DATA;
        return -1;
    }
    state->last_error = BZ3_OK;
    return state->bytes_read = 24 + data_size;
}
