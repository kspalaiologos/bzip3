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

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "cm.h"
#include "crc32.h"
#include "libsais.h"
#include "mtf.h"
#include "rle.h"
#include "srt.h"

struct block_encoder_state {
    s32 input_des, output_des;
    u8 * buf1, * buf2; s32 bytes_read;
    s32 * sais_array;
    struct srt_state * srt_state;
    struct mtf_state * mtf_state;
    state * cm_state;
};

void encode_block(struct block_encoder_state * state) {
    u32 crc32 = crc32sum(1, state->buf1, state->bytes_read);

    s32 new_size = mrlec(state->buf1, state->bytes_read, state->buf2);
    s32 bwt_index = libsais_bwt(state->buf2, state->buf2, state->sais_array, new_size, 16, NULL);
    s32 new_size2;

    if (new_size > MiB(3)) {
        new_size2 = srt_encode(state->srt_state, state->buf2, state->buf1, new_size);
    } else {
        new_size2 = -1;
        mtf_encode(state->mtf_state, state->buf2, state->buf1, new_size);
    }

    begin(state->cm_state);
    state->cm_state->out_queue = state->buf2;
    state->cm_state->output_ptr = 0;
    if (new_size2 != -1)
        for (s32 i = 0; i < new_size2; i++)
            encode_byte(state->cm_state, state->buf1[i]);
    else
        for (s32 i = 0; i < new_size; i++)
            encode_byte(state->cm_state, state->buf1[i]);
    flush(state->cm_state);
    s32 new_size3 = state->cm_state->output_ptr;

    write(state->output_des, &crc32, sizeof(u32));
    write(state->output_des, &state->bytes_read, sizeof(s32));
    write(state->output_des, &bwt_index, sizeof(s32));
    write(state->output_des, &new_size, sizeof(s32));
    write(state->output_des, &new_size2, sizeof(s32));
    write(state->output_des, &new_size3, sizeof(s32));
    write(state->output_des, state->buf2, new_size3);
}

int decode_block(struct block_encoder_state * state, s8 test) {
#define safe_read(fd, buf, size) \
    if (read(fd, buf, size) != size) return 1;

    u32 crc32;
    s32 bwt_index, new_size, new_size2, new_size3;

    safe_read(state->input_des, &crc32, sizeof(u32));
    safe_read(state->input_des, &state->bytes_read, sizeof(s32));
    safe_read(state->input_des, &bwt_index, sizeof(s32));
    safe_read(state->input_des, &new_size, sizeof(s32));
    safe_read(state->input_des, &new_size2, sizeof(s32));
    safe_read(state->input_des, &new_size3, sizeof(s32));
    safe_read(state->input_des, state->buf1, new_size3);

    begin(state->cm_state);
    state->cm_state->in_queue = state->buf1;
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
    libsais_unbwt(state->buf1, state->buf2, state->sais_array, new_size, NULL, bwt_index);
    mrled(state->buf2, state->buf1, state->bytes_read);
    if (crc32sum(1, state->buf1, state->bytes_read) != crc32) {
        fprintf(stderr, "CRC32 checksum mismatch.\n");
        return 1;
    }
    if(!test)
        write(state->output_des, state->buf1, state->bytes_read);
    return 0;
}

int main(int argc, char * argv[]) {
    int mode = 0;  // -1: encode, 0: unspecified, 1: encode, 2: test
    char *input = NULL, *output = NULL;     // input and output file names
    u32 block_size = 8 * 1024 * 1024;  // the block size

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'e') {
                mode = 1;
            } else if (argv[i][1] == 'd') {
                mode = -1;
            } else if (argv[i][1] == 't') {
                mode = 2;
            } else if (argv[i][1] == 'b') {
                block_size = 1024 * 1024 * atoi(argv[i + 1]);
                i++;
            }
        } else {
            if (input == NULL) {
                input = argv[i];
            } else if (output == NULL) {
                output = argv[i];
            }
        }
    }

    if (mode == 0) {
        fprintf(stderr, "Usage: %s [-e/-d/-t] [-b block_size] input output\n",
                argv[0]);
        fprintf(stderr,
                "If input or output are not specified, they default to stdin "
                "and stdout.\n");
        return 1;
    }

    int input_des, output_des;

    if (input != NULL) {
        input_des = open(input, O_RDONLY);
        if (input_des == -1) {
            perror("open");
            return 1;
        }
    } else {
        input_des = STDIN_FILENO;
    }

    if (output != NULL) {
        output_des = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_des == -1) {
            perror("open");
            return 1;
        }
    } else {
        output_des = STDOUT_FILENO;
    }

    if (block_size < KiB(65)) {
        fprintf(stderr, "Block size must be at least 65 KiB.\n");
        return 1;
    }

    if (block_size > MiB(2047)) {
        fprintf(stderr, "Block size must be at most 2047 MiB.\n");
        return 1;
    }

    struct block_encoder_state block_encoder_state;
    struct srt_state srt_state;
    struct mtf_state mtf_state;
    state cm_state;

    block_encoder_state.cm_state = &cm_state;
    block_encoder_state.srt_state = &srt_state;
    block_encoder_state.mtf_state = &mtf_state;

    if (mode == 1) {
        // Encode
        write(output_des, "BZ3v1", 5);
        write(output_des, &block_size, sizeof(u32));

        block_encoder_state.buf1 = malloc(block_size + block_size / 3);
        block_encoder_state.buf2 = malloc(block_size + block_size / 3);
        block_encoder_state.sais_array = malloc(block_size * sizeof(s32) + 16);
        block_encoder_state.input_des = input_des;
        block_encoder_state.output_des = output_des;

        while ((block_encoder_state.bytes_read = read(input_des, block_encoder_state.buf1, block_size)) > 0) {
            encode_block(&block_encoder_state);
        }

        free(block_encoder_state.buf1);
        free(block_encoder_state.buf2);
        free(block_encoder_state.sais_array);
    } else if (mode == -1) {
        // Decode
        char signature[5];
        read(input_des, signature, 5);
        if (strncmp(signature, "BZ3v1", 5) != 0) {
            fprintf(stderr, "Invalid signature.\n");
            return 1;
        }

        read(input_des, &block_size, sizeof(u32));
        
        block_encoder_state.buf1 = malloc(block_size + block_size / 3);
        block_encoder_state.buf2 = malloc(block_size + block_size / 3);
        block_encoder_state.sais_array = malloc(block_size * sizeof(s32) + 16);
        block_encoder_state.input_des = input_des;
        block_encoder_state.output_des = output_des;

        while (decode_block(&block_encoder_state, 0) == 0)
            ;

        free(block_encoder_state.buf1);
        free(block_encoder_state.buf2);
        free(block_encoder_state.sais_array);
    } else if(mode == -2) {
        // Test
        char signature[5];
        read(input_des, signature, 5);
        if (strncmp(signature, "BZ3v1", 5) != 0) {
            fprintf(stderr, "Invalid signature.\n");
            return 1;
        }
        read(input_des, &block_size, sizeof(u32));
        
        block_encoder_state.buf1 = malloc(block_size + block_size / 3);
        block_encoder_state.buf2 = malloc(block_size + block_size / 3);
        block_encoder_state.sais_array = malloc(block_size * sizeof(s32) + 16);
        block_encoder_state.input_des = input_des;
        block_encoder_state.output_des = output_des;

        while (decode_block(&block_encoder_state, 1) == 0)
            ;

        free(block_encoder_state.buf1);
        free(block_encoder_state.buf2);
        free(block_encoder_state.sais_array);
    }

    close(input_des);
    close(output_des);
}
