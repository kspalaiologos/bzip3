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
#include <arpa/inet.h>

#include "common.h"
#include "libbz3.h"

int main(int argc, char * argv[]) {
    // -1: encode, 0: unspecified, 1: encode, 2: test
    int mode = 0;

    // input and output file names
    char *input = NULL, *output = NULL;

    // the block size
    u32 block_size = MiB(8);

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'e') {
                mode = 1;
            } else if (argv[i][1] == 'd') {
                mode = -1;
            } else if (argv[i][1] == 't') {
                mode = 2;
            } else if (argv[i][1] == 'b') {
                block_size = MiB(atoi(argv[i + 1]));
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
        fprintf(stderr, "Usage: %s [-e/-d/-t] [-b block_size] input output\n", argv[0]);
        fprintf(stderr,
                "If input or output are not specified, they default to stdin "
                "and stdout.\n");
        return 1;
    }

    FILE * input_des, * output_des;

    if (input != NULL) {
        input_des = fopen(input, "rb");
        if (input_des == NULL) {
            perror("fopen");
            return 1;
        }
    } else {
        input_des = stdin;
    }

    if (output != NULL) {
        output_des = fopen(output, "wb");
        if (output_des == NULL) {
            perror("open");
            return 1;
        }
    } else {
        output_des = stdout;
    }

    if (block_size < KiB(65) || block_size > MiB(2047)) {
        fprintf(stderr, "Block size must be between 65 KiB and 2047 MiB.\n");
        return 1;
    }

    switch (mode) {
        case 1:
            fwrite("BZ3v1", 5, 1, output_des);

            block_size = htonl(block_size);
            fwrite(&block_size, sizeof(u32), 1, output_des);
            block_size = ntohl(block_size);
            break;
        case -1:
        case -2: {
            char signature[5];

            fread(signature, 5, 1, input_des);
            if (strncmp(signature, "BZ3v1", 5) != 0) {
                fprintf(stderr, "Invalid signature.\n");
                return 1;
            }

            fread(&block_size, sizeof(u32), 1, input_des);

            block_size = ntohl(block_size);

            if (block_size < KiB(65) || block_size > MiB(2047)) {
                fprintf(stderr,
                        "The input file is corrupted. Reason: Invalid block "
                        "size in the header.\n");
                return 1;
            }

            break;
        }
    }

    struct bz3_state * state = bz3_new(block_size);

    if (state == NULL) {
        fprintf(stderr, "Failed to create a block encoder state.\n");
        return 1;
    }

    u8 * buffer = malloc(block_size + block_size / 4);

    if (mode == 1) {
        s32 read_count;
        while (!feof(input_des)) {
            read_count = fread(buffer, 1, block_size, input_des);

            s32 new_size = bz3_encode_block(state, buffer, read_count);
            if (new_size == -1) {
                fprintf(stderr, "Failed to encode a block: %s\n", bz3_strerror(state));
                return 1;
            }

            read_count = htonl(read_count); new_size = ntohl(new_size);
            fwrite(&new_size, 4, 1, output_des);
            fwrite(&read_count, 4, 1, output_des);
            fwrite(buffer, ntohl(new_size), 1, output_des);
        }
    } else if (mode == -1) {
        s32 new_size, old_size;
        while (!feof(input_des)) {
            if(fread(&new_size, 1, 4, input_des) != 4) {
                // Assume that the file has no more data.
                break;
            }
            if(fread(&old_size, 1, 4, input_des) != 4) {
                fprintf(stderr, "I/O error.\n");
                return 1;
            }
            new_size = ntohl(new_size); old_size = ntohl(old_size);
            fread(buffer, 1, new_size, input_des);
            if(bz3_decode_block(state, buffer, new_size, old_size) == -1) {
                fprintf(stderr, "Failed to decode a block: %s\n", bz3_strerror(state));
                return 1;
            }
            fwrite(buffer, old_size, 1, output_des);
        }
    } else if (mode == -2) {
        s32 new_size, old_size;
        while (!feof(input_des)) {
            if(fread(&new_size, 4, 1, input_des) != 4) {
                fprintf(stderr, "I/O error.\n");
            }
            if(fread(&old_size, 4, 1, input_des) != 4) {
                fprintf(stderr, "I/O error.\n");
            }
            new_size = ntohl(new_size); old_size = ntohl(old_size);
            fread(buffer, 1, new_size, input_des);
            if(bz3_decode_block(state, buffer, new_size, old_size) == -1) {
                fprintf(stderr, "Failed to decode a block: %s\n", bz3_strerror(state));
                return 1;
            }
        }
    }

    if (bz3_last_error(state) != BZ3_OK) {
        fprintf(stderr, "Failed to read data: %s\n", bz3_strerror(state));
        return 1;
    }

    free(buffer);

    bz3_free(state);

    fclose(input_des);
    fclose(output_des);
}
