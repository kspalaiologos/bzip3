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

void encode_block(int output_des, s32 bytes_read, u8 * buffer,
                  u8 * output, s32 * sais_array,
                  struct srt_state * srt_state, state * cm_state,
                  u32 block_size, struct mtf_state * mtf_state) {
    u32 crc32 = crc32sum(1, buffer, bytes_read);

    s32 new_size = mrlec(buffer, bytes_read, output);
    s32 bwt_index =
        libsais_bwt(output, output, sais_array, new_size, 16, NULL);
    s32 new_size2;

    if (new_size > MiB(3)) {
        new_size2 = srt_encode(srt_state, output, buffer, new_size);
    } else {
        new_size2 = -1;
        mtf_encode(mtf_state, output, buffer, new_size);
    }

    begin(cm_state);
    cm_state->out_queue = output;
    cm_state->output_ptr = 0;
    if (new_size2 != -1)
        for (s32 i = 0; i < new_size2; i++)
            encode_byte(cm_state, buffer[i]);
    else
        for (s32 i = 0; i < new_size; i++) encode_byte(cm_state, buffer[i]);
    flush(cm_state);
    s32 new_size3 = cm_state->output_ptr;

    write(output_des, &crc32, sizeof(u32));
    write(output_des, &bytes_read, sizeof(s32));
    write(output_des, &bwt_index, sizeof(s32));
    write(output_des, &new_size, sizeof(s32));
    write(output_des, &new_size2, sizeof(s32));
    write(output_des, &new_size3, sizeof(s32));
    write(output_des, output, new_size3);
}

int decode_block(int input_des, int output_des, u8 * buffer,
                 u8 * output, s32 * sais_array, s8 test,
                 struct srt_state * srt_state, state * cm_state,
                 struct mtf_state * mtf_state) {
#define safe_read(fd, buf, size) \
    if (read(fd, buf, size) != size) return 1;

    u32 crc32;
    s32 bytes_read, bwt_index, new_size, new_size2, new_size3;

    safe_read(input_des, &crc32, sizeof(u32));
    safe_read(input_des, &bytes_read, sizeof(s32));
    safe_read(input_des, &bwt_index, sizeof(s32));
    safe_read(input_des, &new_size, sizeof(s32));
    safe_read(input_des, &new_size2, sizeof(s32));
    safe_read(input_des, &new_size3, sizeof(s32));
    safe_read(input_des, buffer, new_size3);

    begin(cm_state);
    cm_state->in_queue = buffer;
    cm_state->input_ptr = 0;
    cm_state->input_max = new_size3;
    init(cm_state);
    if (new_size2 != -1) {
        for (s32 i = 0; i < new_size2; i++)
            output[i] = decode_byte(cm_state);
        srt_decode(srt_state, output, buffer, new_size2);
    } else {
        for (s32 i = 0; i < new_size; i++)
            output[i] = decode_byte(cm_state);
        mtf_decode(mtf_state, output, buffer, new_size);
    }
    libsais_unbwt(buffer, output, sais_array, new_size, NULL, bwt_index);
    mrled(output, buffer, bytes_read);
    if (crc32sum(1, buffer, bytes_read) != crc32) {
        fprintf(stderr, "CRC32 checksum mismatch.\n");
        return 1;
    }
    if(!test)
        write(output_des, buffer, bytes_read);
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

    struct srt_state srt_state;
    struct mtf_state mtf_state;

    if (mode == 1) {
        // Encode
        u8 * buffer = malloc(block_size + block_size / 3);
        u8 * output = malloc(block_size + block_size / 3);
        s32 * sais_array = malloc(block_size * sizeof(s32) + 16);
        s32 bytes_read;

        state s;

        write(output_des, "BZ3v1", 5);
        write(output_des, &block_size, sizeof(u32));

        while ((bytes_read = read(input_des, buffer, block_size)) > 0) {
            encode_block(output_des, bytes_read, buffer, output, sais_array,
                         &srt_state, &s, block_size, &mtf_state);
        }

        free(buffer);
        free(output);
        free(sais_array);
    } else if (mode == -1) {
        // Decode
        char signature[5];
        read(input_des, signature, 5);
        if (strncmp(signature, "BZ3v1", 5) != 0) {
            fprintf(stderr, "Invalid signature.\n");
            return 1;
        }
        read(input_des, &block_size, sizeof(u32));
        u8 * buffer = malloc(block_size + block_size / 2);
        u8 * output = malloc(block_size + block_size / 2);
        s32 * sais_array = malloc(block_size * sizeof(s32) + 16);

        state s;

        while (decode_block(input_des, output_des, buffer, output, sais_array, 0,
                            &srt_state, &s, &mtf_state) == 0)
            ;

        free(buffer);
        free(output);
        free(sais_array);
    } else if(mode == -2) {
        // Test
        char signature[5];
        read(input_des, signature, 5);
        if (strncmp(signature, "BZ3v1", 5) != 0) {
            fprintf(stderr, "Invalid signature.\n");
            return 1;
        }
        read(input_des, &block_size, sizeof(u32));
        u8 * buffer = malloc(block_size + block_size / 2);
        u8 * output = malloc(block_size + block_size / 2);
        s32 * sais_array = malloc(block_size * sizeof(s32) + 16);

        state s;

        while (decode_block(input_des, output_des, buffer, output, sais_array, 1,
                            &srt_state, &s, &mtf_state) == 0)
            ;

        free(buffer);
        free(output);
        free(sais_array);
    }

    close(input_des);
    close(output_des);
}
