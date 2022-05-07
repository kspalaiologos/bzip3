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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "libbz3.h"

int main(int argc, char * argv[]) {
    // -1: decode, 0: unspecified, 1: encode, 2: test
    int mode = 0;

    // input and output file names
    char *input = NULL, *output = NULL;
    char *bz3_file = NULL, *regular_file = NULL;
    int no_bz3_suffix = 0;

    // command line arguments
    int force_stdstreams = 0, workers = 0;

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
            } else if (argv[i][1] == 'c') {
                force_stdstreams = 1;
            } else if (argv[i][1] == 'j') {
                workers = atoi(argv[i + 1]);
                i++;
            }
        } else {
            if(bz3_file != NULL && regular_file != NULL) {
                fprintf(stderr, "Error: too many files specified.\n");
                return 1;
            }

            int has_bz3_suffix = strlen(argv[i]) > 4 && !strcmp(argv[i] + strlen(argv[i]) - 4, ".bz3");

            if (has_bz3_suffix || regular_file != NULL) {
                bz3_file = argv[i];
                no_bz3_suffix = !has_bz3_suffix;
            } else {
                regular_file = argv[i];
            }
        }
    }

    if (mode == 0) {
        fprintf(stderr, "bzip3 - A better and stronger spiritual successor to bzip2.\n");
        fprintf(stderr, "Copyright (C) by Kamila Szewczyk, 2022. Licensed under the terms of GPLv3.\n");
        fprintf(stderr, "Usage: bzip3 [-e/-d/-t/-c] [-b block_size] input output\n");
        fprintf(stderr, "Operations:\n");
        fprintf(stderr, "  -e: encode\n");
        fprintf(stderr, "  -d: decode\n");
        fprintf(stderr, "  -t: test\n");
        fprintf(stderr, "Extra flags:\n");
        fprintf(stderr, "  -c: force reading/writing from standard streams\n");
        fprintf(stderr, "  -b N: set block size in MiB\n");
        fprintf(stderr, "  -j N: set the amount of parallel threads\n");
        return 1;
    }

    if(mode != 2) {
        if (no_bz3_suffix || mode == 1) {
            input = regular_file;
            output = bz3_file;
            if(!force_stdstreams && !no_bz3_suffix && output == NULL && input != NULL) {
                // add the bz3 extension
                output = malloc(strlen(input) + 5);
                strcpy(output, input);
                strcat(output, ".bz3");
            }
        } else {
            input = bz3_file;
            output = regular_file;
            if(!force_stdstreams && output == NULL && input != NULL) {
                // strip the bz3 extension
                output = malloc(strlen(input) - 4);
                strncpy(output, input, strlen(input) - 4);
                output[strlen(input) - 4] = '\0';
            }
        }
    } else {
        input = bz3_file != NULL ? bz3_file : regular_file;
    }

    FILE *input_des, *output_des;

    if (input != NULL) {
        input_des = fopen(input, "rb");
        if (input_des == NULL) {
            perror("fopen");
            return 1;
        }
    } else if (force_stdstreams) {
        input_des = stdin;
    }

    if (output != NULL && mode != 2) {
        output_des = fopen(output, "wb");
        if (output_des == NULL) {
            perror("open");
            return 1;
        }
    } else if (force_stdstreams) {
        output_des = stdout;
    }

    if (block_size < KiB(65) || block_size > MiB(2047)) {
        fprintf(stderr, "Block size must be between 65 KiB and 2047 MiB.\n");
        return 1;
    }

#if HAVE_ISATTY == 1 && HAVE_FILENO == 1
    if ((isatty(fileno(output_des)) && mode == 1) || (isatty(fileno(input_des)) && (mode == -1 || mode == 2))) {
        fprintf(stderr, "Refusing to read/write binary data from/to the terminal.\n");
        return 1;
    }
#endif

    u8 byteswap_buf[4];

    switch (mode) {
        case 1:
            fwrite("BZ3v1", 5, 1, output_des);

            write_neutral_s32(byteswap_buf, block_size);
            fwrite(byteswap_buf, 4, 1, output_des);
            break;
        case -1:
        case 2: {
            char signature[5];

            fread(signature, 5, 1, input_des);
            if (strncmp(signature, "BZ3v1", 5) != 0) {
                fprintf(stderr, "Invalid signature.\n");
                return 1;
            }

            fread(byteswap_buf, 4, 1, input_des);
            block_size = read_neutral_s32(byteswap_buf);

            if (block_size < KiB(65) || block_size > MiB(2047)) {
                fprintf(stderr,
                        "The input file is corrupted. Reason: Invalid block "
                        "size in the header.\n");
                return 1;
            }

            break;
        }
    }

    if(workers > 16 || workers < 0) {
        fprintf(stderr, "Number of workers must be between 2 and 16.\n");
        return 1;
    }

    if(workers <= 1) {
        struct bz3_state * state = bz3_new(block_size);

        if (state == NULL) {
            fprintf(stderr, "Failed to create a block encoder state.\n");
            return 1;
        }

        u8 * buffer = malloc(block_size + block_size / 50 + 16);

        if(!buffer) {
            fprintf(stderr, "Failed to allocate memory.\n");
            return 1;
        }

        if (mode == 1) {
            s32 read_count;
            while (!feof(input_des)) {
                read_count = fread(buffer, 1, block_size, input_des);

                s32 new_size = bz3_encode_block(state, buffer, read_count);
                if (new_size == -1) {
                    fprintf(stderr, "Failed to encode a block: %s\n", bz3_strerror(state));
                    return 1;
                }

                write_neutral_s32(byteswap_buf, new_size);
                fwrite(byteswap_buf, 4, 1, output_des);
                write_neutral_s32(byteswap_buf, read_count);
                fwrite(byteswap_buf, 4, 1, output_des);
                fwrite(buffer, new_size, 1, output_des);
            }
        } else if (mode == -1) {
            s32 new_size, old_size;
            while (!feof(input_des)) {
                if (fread(&byteswap_buf, 1, 4, input_des) != 4) {
                    // Assume that the file has no more data.
                    break;
                }
                new_size = read_neutral_s32(byteswap_buf);
                if (fread(&byteswap_buf, 1, 4, input_des) != 4) {
                    fprintf(stderr, "I/O error.\n");
                    return 1;
                }
                old_size = read_neutral_s32(byteswap_buf);
                fread(buffer, 1, new_size, input_des);
                if (bz3_decode_block(state, buffer, new_size, old_size) == -1) {
                    fprintf(stderr, "Failed to decode a block: %s\n", bz3_strerror(state));
                    return 1;
                }
                fwrite(buffer, old_size, 1, output_des);
            }
        } else if (mode == 2) {
            s32 new_size, old_size;
            while (!feof(input_des)) {
                if (fread(&byteswap_buf, 1, 4, input_des) != 4) {
                    // Assume that the file has no more data.
                    break;
                }
                new_size = read_neutral_s32(byteswap_buf);
                if (fread(&byteswap_buf, 1, 4, input_des) != 4) {
                    fprintf(stderr, "I/O error.\n");
                    return 1;
                }
                old_size = read_neutral_s32(byteswap_buf);
                fread(buffer, 1, new_size, input_des);
                if (bz3_decode_block(state, buffer, new_size, old_size) == -1) {
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
    } else {
        struct bz3_state * states[workers];
        u8 * buffers[workers];
        s32 sizes[workers];
        s32 old_sizes[workers];
        for(s32 i = 0; i < workers; i++) {
            states[i] = bz3_new(block_size);
            if (states[i] == NULL) {
                fprintf(stderr, "Failed to create a block encoder state.\n");
                return 1;
            }
            buffers[i] = malloc(block_size + block_size / 50 + 16);
            if(!buffers[i]) {
                fprintf(stderr, "Failed to allocate memory.\n");
                return 1;
            }
        }

        if (mode == 1) {
            while(!feof(input_des)) {
                s32 i = 0;
                for(; i < workers; i++) {
                    size_t read_count = fread(buffers[i], 1, block_size, input_des);
                    sizes[i] = old_sizes[i] = read_count;
                    if(read_count < block_size)
                        { i++; break; }
                }
                bz3_encode_blocks(states, buffers, sizes, i);
                for(s32 j = 0; j < i; j++) {
                    if (bz3_last_error(states[j]) != BZ3_OK) {
                        fprintf(stderr, "Failed to encode data: %s\n", bz3_strerror(states[j]));
                        return 1;
                    }
                }
                for(s32 j = 0; j < i; j++) {
                    write_neutral_s32(byteswap_buf, sizes[j]);
                    fwrite(byteswap_buf, 4, 1, output_des);
                    write_neutral_s32(byteswap_buf, old_sizes[j]);
                    fwrite(byteswap_buf, 4, 1, output_des);
                    fwrite(buffers[j], sizes[j], 1, output_des);
                }
            }
            end:;
        } else if(mode == -1) {
            while(!feof(input_des)) {
                s32 i = 0;
                for(; i < workers; i++) {
                    if(fread(&byteswap_buf, 1, 4, input_des) != 4)
                        break;
                    sizes[i] = read_neutral_s32(byteswap_buf);
                    if(fread(&byteswap_buf, 1, 4, input_des) != 4)
                        break;
                    old_sizes[i] = read_neutral_s32(byteswap_buf);
                    if(fread(buffers[i], 1, sizes[i], input_des) != sizes[i]) {
                        fprintf(stderr, "I/O error.\n");
                        return 1;
                    }
                }
                bz3_decode_blocks(states, buffers, sizes, old_sizes, i);
                for(s32 j = 0; j < i; j++) {
                    if (bz3_last_error(states[j]) != BZ3_OK) {
                        fprintf(stderr, "Failed to decode data: %s\n", bz3_strerror(states[j]));
                        return 1;
                    }
                }
                for(s32 j = 0; j < i; j++) {
                    fwrite(buffers[j], old_sizes[j], 1, output_des);
                }
            }
        } else if(mode == 2) {
            while(!feof(input_des)) {
                s32 i = 0;
                for(; i < workers; i++) {
                    if(fread(&byteswap_buf, 1, 4, input_des) != 4)
                        break;
                    sizes[i] = read_neutral_s32(byteswap_buf);
                    if(fread(&byteswap_buf, 1, 4, input_des) != 4)
                        break;
                    old_sizes[i] = read_neutral_s32(byteswap_buf);
                    if(fread(buffers[i], 1, sizes[i], input_des) != sizes[i]) {
                        fprintf(stderr, "I/O error.\n");
                        return 1;
                    }
                }
                bz3_decode_blocks(states, buffers, sizes, old_sizes, i);
                for(s32 j = 0; j < i; j++) {
                    if (bz3_last_error(states[j]) != BZ3_OK) {
                        fprintf(stderr, "Failed to decode data: %s\n", bz3_strerror(states[j]));
                        return 1;
                    }
                }
            }
        }

        for(s32 i = 0; i < workers; i++) {
            free(buffers[i]);
            bz3_free(states[i]);
        }
    }
}
