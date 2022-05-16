
/*
 * BZip3 - A spiritual successor to BZip2.
 * Copyright (C) 2022 Kamila Szewczyk
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined __MSVCRT__
  #include <fcntl.h>
  #include <io.h>
#endif

#include "common.h"
#include "libbz3.h"

int is_dir(const char * path) {
    struct stat sb;
    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode)) return 1;
    return 0;
}

int is_numeric(const char * str) {
    for (; *str; str++)
        if (!isdigit(*str)) return 0;
    return 1;
}

int main(int argc, char * argv[]) {
    // -1: decode, 0: unspecified, 1: encode, 2: test
    int mode = 0;
    int args_status = 1;

    // input and output file names
    char *input = NULL, *output = NULL;
    char *bz3_file = NULL, *regular_file = NULL;
    int no_bz3_suffix = 0;

    // command line arguments
    int force_stdstreams = 0, workers = 0;
    int double_dash = 0;

    // the block size
    u32 block_size = MiB(8);

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && !double_dash) {
            if (strlen(argv[i]) < 2) {
                fprintf(stderr, "Invalid flag '%s'.\n", argv[i]);
                return 1;
            } else if (argv[i][1] == 'e') {
                mode = 1;
            } else if (argv[i][1] == 'd') {
                mode = -1;
            } else if (argv[i][1] == 't') {
                mode = 2;
            } else if (argv[i][1] == 'b') {
                if (i + 1 >= argc) {
                    fprintf(stderr, "Error: -b requires an argument.\n");
                    return 1;
                }

                if (!is_numeric(argv[i + 1])) {
                    fprintf(stderr, "Error: -b requires an integer argument.\n");
                    return 1;
                }

                block_size = MiB(atoi(argv[i + 1]));
                i++;
            } else if (argv[i][1] == 'c') {
                force_stdstreams = 1;
#ifdef PTHREAD
            } else if (argv[i][1] == 'j') {
                if (i + 1 >= argc) {
                    fprintf(stderr, "Error: -j requires an argument.\n");
                    return 1;
                }

                if (!is_numeric(argv[i + 1])) {
                    fprintf(stderr, "Error: -j requires an integer argument.\n");
                    return 1;
                }

                workers = atoi(argv[i + 1]);
                i++;
#endif
            } else if (argv[i][1] == 'h') {
                mode = 0;
                args_status = 0;
            } else if (argv[i][1] == 'v') {
                fprintf(stderr, "bzip3 %s", VERSION);
                return 0;
            } else if (argv[i][1] == '-') {
                double_dash = 1;
            } else {
                fprintf(stderr, "Invalid flag '%s'.\n", argv[i]);
                return 1;
            }
        } else {
            if (bz3_file != NULL && regular_file != NULL) {
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
        fprintf(stderr, "bzip3 version %s\n", VERSION);
        fprintf(stderr, "- A better and stronger spiritual successor to bzip2.\n");
        fprintf(stderr, "Copyright (C) by Kamila Szewczyk, 2022. Licensed under the terms of LGPLv3.\n");
        fprintf(stderr, "Usage: bzip3 [-e/-d/-t/-c/-h/-v] [-b block_size] input output\n");
        fprintf(stderr, "Operations:\n");
        fprintf(stderr, "  -e: encode\n");
        fprintf(stderr, "  -d: decode\n");
        fprintf(stderr, "  -t: test\n");
        fprintf(stderr, "  -h: help\n");
        fprintf(stderr, "  -v: version\n");
        fprintf(stderr, "Extra flags:\n");
        fprintf(stderr, "  -c: force reading/writing from standard streams\n");
        fprintf(stderr, "  -b N: set block size in MiB\n");
#ifdef PTHREAD
        fprintf(stderr, "  -j N: set the amount of parallel threads\n");
#endif
        return args_status;
    }
    #ifndef O_BINARY
		#define O_BINARY 0
    #endif
    #if defined(__MSVCRT__)
		setmode(STDIN_FILENO, O_BINARY);
		setmode(STDOUT_FILENO, O_BINARY);
    #endif

    if (mode != 2) {
        if (no_bz3_suffix || mode == 1) {
            input = regular_file;
            output = bz3_file;
            if (!force_stdstreams && !no_bz3_suffix && output == NULL && input != NULL) {
                // add the bz3 extension
                output = malloc(strlen(input) + 5);
                strcpy(output, input);
                strcat(output, ".bz3");
            }
        } else {
            input = bz3_file;
            output = regular_file;
            if (!force_stdstreams && output == NULL && input != NULL) {
                // strip the bz3 extension
                if (strlen(input) > 4 && !strcmp(input + strlen(input) - 4, ".bz3")) {
                    output = malloc(strlen(input));
                    strncpy(output, input, strlen(input) - 4);
                    output[strlen(input) - 4] = '\0';
                }
            }
        }
    } else {
        input = bz3_file != NULL ? bz3_file : regular_file;
    }

    if (input == NULL && output == NULL) force_stdstreams = 1;

    FILE *input_des, *output_des;

    if (input != NULL) {
        if (is_dir(input)) {
            fprintf(stderr, "Error: input is a directory.\n");
            return 1;
        }

        input_des = fopen(input, "rb");
        if (input_des == NULL) {
            perror("fopen");
            return 1;
        }
    } else if (force_stdstreams) {
        input_des = stdin;
    }

    if (output != NULL && mode != 2) {
        if (is_dir(output)) {
            fprintf(stderr, "Error: output is a directory.\n");
            return 1;
        }

        output_des = fopen(output, "wb");
        if (output_des == NULL) {
            perror("open");
            return 1;
        }
    } else if (force_stdstreams) {
        output_des = stdout;
    }

    if (block_size < KiB(65) || block_size > MiB(511)) {
        fprintf(stderr, "Block size must be between 65 KiB and 511 MiB.\n");
        return 1;
    }

    if ((isatty(fileno(output_des)) && mode == 1) || (isatty(fileno(input_des)) && (mode == -1 || mode == 2))) {
        fprintf(stderr, "Refusing to read/write binary data from/to the terminal.\n");
        return 1;
    }

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

            if (block_size < KiB(65) || block_size > MiB(511)) {
                fprintf(stderr,
                        "The input file is corrupted. Reason: Invalid block "
                        "size in the header.\n");
                return 1;
            }

            break;
        }
    }

#ifdef PTHREAD
    if (workers > 16 || workers < 0) {
        fprintf(stderr, "Number of workers must be between 0 and 16.\n");
        return 1;
    }

    if (workers <= 1) {
#endif
        struct bz3_state * state = bz3_new(block_size);

        if (state == NULL) {
            fprintf(stderr, "Failed to create a block encoder state.\n");
            return 1;
        }

        u8 * buffer = malloc(block_size + block_size / 50 + 32);

        if (!buffer) {
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
                if (fread(buffer, 1, new_size, input_des) != new_size) {
                    fprintf(stderr, "I/O error.\n");
                    return 1;
                }
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
                if (fread(buffer, 1, new_size, input_des) != new_size) {
                    fprintf(stderr, "I/O error.\n");
                    return 1;
                }
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
#ifdef PTHREAD
    } else {
        struct bz3_state * states[workers];
        u8 * buffers[workers];
        s32 sizes[workers];
        s32 old_sizes[workers];
        for (s32 i = 0; i < workers; i++) {
            states[i] = bz3_new(block_size);
            if (states[i] == NULL) {
                fprintf(stderr, "Failed to create a block encoder state.\n");
                return 1;
            }
            buffers[i] = malloc(block_size + block_size / 50 + 32);
            if (!buffers[i]) {
                fprintf(stderr, "Failed to allocate memory.\n");
                return 1;
            }
        }

        if (mode == 1) {
            while (!feof(input_des)) {
                s32 i = 0;
                for (; i < workers; i++) {
                    size_t read_count = fread(buffers[i], 1, block_size, input_des);
                    sizes[i] = old_sizes[i] = read_count;
                    if (read_count < block_size) {
                        i++;
                        break;
                    }
                }
                bz3_encode_blocks(states, buffers, sizes, i);
                for (s32 j = 0; j < i; j++) {
                    if (bz3_last_error(states[j]) != BZ3_OK) {
                        fprintf(stderr, "Failed to encode data: %s\n", bz3_strerror(states[j]));
                        return 1;
                    }
                }
                for (s32 j = 0; j < i; j++) {
                    write_neutral_s32(byteswap_buf, sizes[j]);
                    fwrite(byteswap_buf, 4, 1, output_des);
                    write_neutral_s32(byteswap_buf, old_sizes[j]);
                    fwrite(byteswap_buf, 4, 1, output_des);
                    fwrite(buffers[j], sizes[j], 1, output_des);
                }
            }
        } else if (mode == -1) {
            while (!feof(input_des)) {
                s32 i = 0;
                for (; i < workers; i++) {
                    if (fread(&byteswap_buf, 1, 4, input_des) != 4) break;
                    sizes[i] = read_neutral_s32(byteswap_buf);
                    if (fread(&byteswap_buf, 1, 4, input_des) != 4) break;
                    old_sizes[i] = read_neutral_s32(byteswap_buf);
                    if (fread(buffers[i], 1, sizes[i], input_des) != sizes[i]) {
                        fprintf(stderr, "I/O error.\n");
                        return 1;
                    }
                }
                bz3_decode_blocks(states, buffers, sizes, old_sizes, i);
                for (s32 j = 0; j < i; j++) {
                    if (bz3_last_error(states[j]) != BZ3_OK) {
                        fprintf(stderr, "Failed to decode data: %s\n", bz3_strerror(states[j]));
                        return 1;
                    }
                }
                for (s32 j = 0; j < i; j++) {
                    fwrite(buffers[j], old_sizes[j], 1, output_des);
                }
            }
        } else if (mode == 2) {
            while (!feof(input_des)) {
                s32 i = 0;
                for (; i < workers; i++) {
                    if (fread(&byteswap_buf, 1, 4, input_des) != 4) break;
                    sizes[i] = read_neutral_s32(byteswap_buf);
                    if (fread(&byteswap_buf, 1, 4, input_des) != 4) break;
                    old_sizes[i] = read_neutral_s32(byteswap_buf);
                    if (fread(buffers[i], 1, sizes[i], input_des) != sizes[i]) {
                        fprintf(stderr, "I/O error.\n");
                        return 1;
                    }
                }
                bz3_decode_blocks(states, buffers, sizes, old_sizes, i);
                for (s32 j = 0; j < i; j++) {
                    if (bz3_last_error(states[j]) != BZ3_OK) {
                        fprintf(stderr, "Failed to decode data: %s\n", bz3_strerror(states[j]));
                        return 1;
                    }
                }
            }
        }

        for (s32 i = 0; i < workers; i++) {
            free(buffers[i]);
            bz3_free(states[i]);
        }
    }
#endif
}
