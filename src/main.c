
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

#ifdef HAVE_GETOPT_LONG
    #include <getopt.h>
#else
    #include "getopt-shim.h"
#endif

#if defined __MSVCRT__
    #include <fcntl.h>
    #include <io.h>
#endif

#include "common.h"
#include "libbz3.h"

#define MODE_DECODE 0
#define MODE_ENCODE 1
#define MODE_TEST 2

static void version() {
    fprintf(stdout, "bzip3 " VERSION
                    "\n"
                    "Copyright (C) by Kamila Szewczyk, 2022.\n"
                    "License: GNU Lesser GPL version 3 <https://www.gnu.org/licenses/lgpl-3.0.en.html>\n");
}

static void help() {
    fprintf(stdout,
            "bzip3 - better and stronger spiritual successor to bzip2.\n"
            "Usage: bzip3 [-e/-d/-t/-c/-h/-V] [-b block_size] [-j jobs] files...\n"
            "Operations:\n"
            "  -e, --encode\tcompress data (default)\n"
            "  -d, --decode\tdecompress data\n"
            "  -t, --test\tverify validity of compressed data\n"
            "  -h, --help\tdisplay an usage overview\n"
            "  -f, --force\tforce overwriting output if it already exists\n"
            "  -V, --version\tdisplay version information\n"
            "Extra flags:\n"
            "  -c, --stdout: force writing to standard output\n"
            "  -b N, --block=N: set block size in MiB {16}\n"
#ifdef PTHREAD
            "  -j N, --jobs=N: set the amount of parallel threads\n"
#endif
            "\n"
            "Report bugs to: https://github.com/kspalaiologos/bzip3\n"
            "\n");
}

static int is_dir(const char * path) {
    struct stat sb;
    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode)) return 1;
    return 0;
}

static int is_numeric(const char * str) {
    for (; *str; str++)
        if (!isdigit(*str)) return 0;
    return 1;
}

int main(int argc, char * argv[]) {
    int mode = MODE_ENCODE;

    // input and output file names
    char *input = NULL, *output = NULL;
    char *f1 = NULL, *f2 = NULL;
    int force = 0;

    // command line arguments
    int force_stdstreams = 0, workers = 0;

    // the block size
    u32 block_size = MiB(16);

#ifdef PTHREAD
    const char * short_options = "b:cdefhj:tV";
#else
    const char * short_options = "b:cdefhtV";
#endif

    static struct option long_options[] = { { "encode", no_argument, 0, 'e' },
                                            { "decode", no_argument, 0, 'd' },
                                            { "test", no_argument, 0, 't' },
                                            { "stdout", no_argument, 0, 'c' },
                                            { "force", no_argument, 0, 'f' },
                                            { "help", no_argument, 0, 'h' },
                                            { "version", no_argument, 0, 'V' },
                                            { "block", required_argument, 0, 'b' },
#ifdef PTHREAD
                                            { "jobs", required_argument, 0, 'j' },
#endif
                                            { 0, 0, 0, 0 } };

    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (c == -1) break;

        switch (c) {
            case '?':
                fprintf(stderr, "Try 'bzip3 --help' for more information.\n");
                return 1;
            case 'e':
                mode = MODE_ENCODE;
                break;
            case 'd':
                mode = MODE_DECODE;
                break;
            case 't':
                mode = MODE_TEST;
                break;
            case 'c':
                force_stdstreams = 1;
                break;
            case 'f':
                force = 1;
                break;
            case 'h':
                help();
                return 0;
            case 'V':
                version();
                return 0;
            case 'b':
                if (!is_numeric(optarg)) {
                    fprintf(stderr, "bzip3: invalid block size: %s", optarg);
                    return 1;
                }
                block_size = MiB(atoi(optarg));
                break;
#ifdef PTHREAD
            case 'j':
                if (!is_numeric(optarg)) {
                    fprintf(stderr, "bzip3: invalid amount of jobs: %s", optarg);
                    return 1;
                }
                workers = atoi(optarg);
                break;
#endif
        }
    }

    while (optind < argc) {
        // Positional argument. Likely a file name.
        char * arg = argv[optind++];

        if (f1 != NULL && f2 != NULL) {
            fprintf(stderr, "Error: too many files specified.\n");
            return 1;
        }

        if (f1 == NULL)
            f1 = arg;
        else
            f2 = arg;
    }

#ifndef O_BINARY
    #define O_BINARY 0
#endif
#if defined(__MSVCRT__)
    setmode(STDIN_FILENO, O_BINARY);
    setmode(STDOUT_FILENO, O_BINARY);
#endif

    if (f1 == NULL && f2 == NULL)
        input = NULL, output = NULL;
    else if (mode == MODE_TEST)
        input = f1;
    else {
        if (mode == MODE_ENCODE) {
            if (f2 == NULL) {
                // encode from f1?
                input = f1;
                if (force_stdstreams)
                    output = NULL;
                else {
                    output = (char *)malloc(strlen(f1) + 5);
                    strcpy(output, f1);
                    strcat(output, ".bz3");
                }
            } else {
                // encode from f1 to f2.
                input = f1;
                output = f2;
            }
        } else if (mode == MODE_DECODE) {
            if (f2 == NULL) {
                // decode from f1 to stdout.
                input = f1;
                if (force_stdstreams)
                    output = NULL;
                else {
                    output = (char *)malloc(strlen(f1) + 1);
                    strcpy(output, f1);
                    if (strlen(output) > 4 && !strcmp(output + strlen(output) - 4, ".bz3"))
                        output[strlen(output) - 4] = 0;
                    else {
                        fprintf(stderr, "Warning: file %s has an unknown extension, skipping.\n", f1);
                        return 1;
                    }
                }
            } else {
                // decode from f1 to f2.
                input = f1;
                output = f2;
            }
        }
    }

    FILE *input_des = NULL, *output_des = NULL;

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
    } else {
        input_des = stdin;
    }

    if (output != NULL && mode != MODE_TEST) {
        if (is_dir(output)) {
            fprintf(stderr, "Error: output is a directory.\n");
            return 1;
        }

        if (access(output, F_OK) == 0) {
            if (!force) {
                fprintf(stderr, "Error: output file already exists. Use -f to force overwrite.\n");
                return 1;
            }
        }

        output_des = fopen(output, "wb");
        if (output_des == NULL) {
            perror("open");
            return 1;
        }
    } else {
        output_des = stdout;
    }

    if (block_size < KiB(65) || block_size > MiB(511)) {
        fprintf(stderr, "Block size must be between 65 KiB and 511 MiB.\n");
        return 1;
    }

    if ((mode == MODE_ENCODE && isatty(fileno(output_des))) ||
        ((mode == MODE_DECODE || mode == MODE_TEST) && isatty(fileno(input_des)))) {
        fprintf(stderr, "Refusing to read/write binary data from/to the terminal.\n");
        return 1;
    }

    u8 byteswap_buf[4];

    switch (mode) {
        case MODE_ENCODE:
            fwrite("BZ3v1", 5, 1, output_des);

            write_neutral_s32(byteswap_buf, block_size);
            fwrite(byteswap_buf, 4, 1, output_des);
            break;
        case MODE_DECODE:
        case MODE_TEST: {
            char signature[5];

            fread(signature, 5, 1, input_des);
            if (strncmp(signature, "BZ3v1", 5) != 0) {
                fprintf(stderr, "Invalid signature.\n");
                return 1;
            }

            if (fread(byteswap_buf, 4, 1, input_des) != 1) {
                fprintf(stderr, "I/O error.\n");
                return 1;
            }

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
    if (workers > 64 || workers < 0) {
        fprintf(stderr, "Number of workers must be between 0 and 64.\n");
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

        if (mode == MODE_ENCODE) {
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
            fflush(output_des);
        } else if (mode == MODE_DECODE) {
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
            fflush(output_des);
        } else if (mode == MODE_TEST) {
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

        if (mode == MODE_ENCODE) {
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
            fflush(output_des);
        } else if (mode == MODE_DECODE) {
            while (!feof(input_des)) {
                s32 i = 0;
                for (; i < workers; i++) {
                    if (fread(&byteswap_buf, 1, 4, input_des) != 4) break;
                    sizes[i] = read_neutral_s32(byteswap_buf);
                    if (fread(&byteswap_buf, 1, 4, input_des) != 4) {
                        fprintf(stderr, "I/O error.\n");
                        return 1;
                    }
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
            fflush(output_des);
        } else if (mode == MODE_TEST) {
            while (!feof(input_des)) {
                s32 i = 0;
                for (; i < workers; i++) {
                    if (fread(&byteswap_buf, 1, 4, input_des) != 4) break;
                    sizes[i] = read_neutral_s32(byteswap_buf);
                    if (fread(&byteswap_buf, 1, 4, input_des) != 4) {
                        fprintf(stderr, "I/O error.\n");
                        return 1;
                    }
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
