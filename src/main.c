
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libsais.h"
#include "rle.h"
#include "mtf.h"
#include "srt.h"
#include "crc32.h"
#include "cm.h"

#define KiB(x) ((x) * 1024)
#define MiB(x) ((x) * 1024 * 1024)

void encode_block(int output_des, int32_t bytes_read, uint8_t * buffer, uint8_t * output, int32_t * sais_array, struct srt_state * srt_state, state * cm_state, uint32_t block_size, struct mtf_state * mtf_state) {
    uint32_t crc32 = crc32sum(1, buffer, bytes_read);

    int32_t new_size = mrlec(buffer, bytes_read, output);
    int32_t bwt_index =
        libsais_bwt(output, output, sais_array, new_size, 16, NULL);
    int32_t new_size2;

    if(new_size > MiB(3)) {
        new_size2 = srt_encode(srt_state, output, buffer, new_size);
    } else {
        new_size2 = -1;
        mtf_encode(mtf_state, output, buffer, new_size);
    }

    begin(cm_state);
    cm_state->out_queue = output;
    cm_state->output_ptr = 0;
    if(new_size2 != -1)
        for (int32_t i = 0; i < new_size2; i++) encode_bit(cm_state, buffer[i]);
    else 
        for (int32_t i = 0; i < new_size; i++) encode_bit(cm_state, buffer[i]);
    flush(cm_state);
    int32_t new_size3 = cm_state->output_ptr;

    write(output_des, &crc32, sizeof(uint32_t));
    write(output_des, &bytes_read, sizeof(int32_t));
    write(output_des, &bwt_index, sizeof(int32_t));
    write(output_des, &new_size, sizeof(int32_t));
    write(output_des, &new_size2, sizeof(int32_t));
    write(output_des, &new_size3, sizeof(int32_t));
    write(output_des, output, new_size3);
}

int decode_block(int input_des, int output_des, uint8_t * buffer, uint8_t * output, int32_t * sais_array, struct srt_state * srt_state, state * cm_state, struct mtf_state * mtf_state) {
    #define safe_read(fd, buf, size) \
        if (read(fd, buf, size) != size) return 1;

    uint32_t crc32;
    int32_t bytes_read, bwt_index, new_size, new_size2, new_size3;

    safe_read(input_des, &crc32, sizeof(uint32_t));
    safe_read(input_des, &bytes_read, sizeof(int32_t));
    safe_read(input_des, &bwt_index, sizeof(int32_t));
    safe_read(input_des, &new_size, sizeof(int32_t));
    safe_read(input_des, &new_size2, sizeof(int32_t));
    safe_read(input_des, &new_size3, sizeof(int32_t));
    safe_read(input_des, buffer, new_size3);

    begin(cm_state);
    cm_state->in_queue = buffer;
    cm_state->input_ptr = 0;
    cm_state->input_max = new_size3;
    init(cm_state);
    if(new_size2 != -1) {
        for (int32_t i = 0; i < new_size2; i++) output[i] = decode_bit(cm_state);
        srt_decode(srt_state, output, buffer, new_size2);
    } else {
        for (int32_t i = 0; i < new_size; i++) output[i] = decode_bit(cm_state);
        mtf_decode(mtf_state, output, buffer, new_size);
    }
    libsais_unbwt(buffer, output, sais_array, new_size, NULL,
                    bwt_index);
    mrled(output, buffer, bytes_read);
    if (crc32sum(1, buffer, bytes_read) != crc32) {
        fprintf(stderr, "CRC32 checksum mismatch.\n");
        return 1;
    }
    write(output_des, buffer, bytes_read);
    return 0;
}

int main(int argc, char *argv[]) {
    int mode = 0;  // -1: encode, 0: unspecified, 1: encode
    char *input = NULL, *output = NULL;  // input and output file names
    uint32_t block_size = 8 * 1024 * 1024; // the block size

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'e') {
                mode = 1;
            } else if (argv[i][1] == 'd') {
                mode = -1;
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
        fprintf(stderr, "Usage: %s [-e/-d] [-b block_size] input output\n",
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

    struct srt_state srt_state;
    struct mtf_state mtf_state;

    if (mode == 1) {
        // Encode
        uint8_t *buffer = malloc(block_size + block_size / 2);
        uint8_t *output = malloc(block_size + block_size / 2);
        int32_t *sais_array = malloc(block_size * sizeof(int32_t) + 16);
        int32_t bytes_read;

        state s;

        write(output_des, "BZ3v1", 5);
        write(output_des, &block_size, sizeof(uint32_t));

        while ((bytes_read = read(input_des, buffer, block_size)) > 0) {
            encode_block(output_des, bytes_read, buffer, output, sais_array, &srt_state, &s, block_size, &mtf_state);
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
        read(input_des, &block_size, sizeof(uint32_t));
        uint8_t *buffer = malloc(block_size + block_size / 2);
        uint8_t *output = malloc(block_size + block_size / 2);
        int32_t *sais_array = malloc(block_size * sizeof(int32_t) + 16);

        state s;

        while(decode_block(input_des, output_des, buffer, output, sais_array, &srt_state, &s, &mtf_state) == 0);

        free(buffer);
        free(output);
        free(sais_array);
    }

    close(input_des);
    close(output_des);
}
