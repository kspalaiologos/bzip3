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

    if (block_size < KiB(65) || block_size > MiB(2047)) {
        fprintf(stderr, "Block size must be between 65 KiB and 2047 MiB.\n");
        return 1;
    }

    switch (mode) {
        case 1:
            write(output_des, "BZ3v1", 5);
            write(output_des, &block_size, sizeof(u32));
            break;
        case -1:
        case -2: {
            char signature[5];

            read(input_des, signature, 5);
            if (strncmp(signature, "BZ3v1", 5) != 0) {
                fprintf(stderr, "Invalid signature.\n");
                return 1;
            }

            read(input_des, &block_size, sizeof(u32));

            if (block_size < KiB(65) || block_size > MiB(2047)) {
                fprintf(stderr,
                        "The input file is corrupted. Reason: Invalid block "
                        "size in the header.\n");
                return 1;
            }

            break;
        }
    }

    struct block_encoder_state * block_encoder_state =
        new_block_encoder_state(block_size);

    if (block_encoder_state == NULL) {
        fprintf(stderr, "Failed to create a block encoder state.\n");
        return 1;
    }

    if (mode == 1)
        while (commit_read(block_encoder_state,
                           read(input_des, get_buffer(block_encoder_state),
                                block_size)) > 0) {
            if (get_last_error(block_encoder_state) != BZ3_OK) {
                fprintf(stderr, "Failed to read data: %s\n",
                        str_last_error(block_encoder_state));
                return 1;
            }
            struct encoding_result r = encode_block(block_encoder_state);
            if (get_last_error(block_encoder_state) != BZ3_OK) {
                fprintf(stderr, "Failed to encode the block: %s\n",
                        str_last_error(block_encoder_state));
                return 1;
            }
            write(output_des, r.buffer, r.size);
        }
    else if (mode == -1) {
        s32 read_size;
        while ((read_size = read_block(input_des, block_encoder_state)) > 0) {
            if (get_last_error(block_encoder_state) != BZ3_OK) {
                fprintf(stderr, "Failed to read data: %s\n",
                        str_last_error(block_encoder_state));
                return 1;
            }
            struct encoding_result r = decode_block(block_encoder_state);
            if (get_last_error(block_encoder_state) != BZ3_OK) {
                fprintf(stderr, "Failed to decode the block: %s\n",
                        str_last_error(block_encoder_state));
                return 1;
            }
            write(output_des, r.buffer, r.size);
        }
    } else if (mode == -2) {
        s32 read_size;
        while ((read_size = read_block(input_des, block_encoder_state)) > 0) {
            if (get_last_error(block_encoder_state) != BZ3_OK) {
                fprintf(stderr, "Failed to read data: %s\n",
                        str_last_error(block_encoder_state));
                return 1;
            }
            decode_block(block_encoder_state);
            if (get_last_error(block_encoder_state) != BZ3_OK) {
                fprintf(stderr, "Failed to decode data: %s\n",
                        str_last_error(block_encoder_state));
                return 1;
            }
        }
    }

    if (get_last_error(block_encoder_state) != BZ3_OK) {
        fprintf(stderr, "Failed to read data: %s\n",
                str_last_error(block_encoder_state));
        return 1;
    }

    delete_block_encoder_state(block_encoder_state);

    close(input_des);
    close(output_des);
}
