/* A tiny utility for fuzzing bzip3.
 *
 * Prerequisites:
 * 
 * - AFL https://github.com/AFLplusplus/AFLplusplus
 * - clang (part of LLVM)
 * 
 * On Arch this is `pacman -S afl++ clang`
 *
 * # Instructions:
 * 
 * 1. Prepare fuzzer directories
 * 
 * mkdir -p afl_in && mkdir -p afl_out
 * 
 * 2. Build binary (to compress test data).
 * 
 * afl-clang fuzz.c -I../include ../src/libbz3.c -o fuzz -g3 "-DVERSION=\"0.0.0\"" -O3 -march=native
 * 
 * 3. Make a fuzzer input file.
 * 
 * With `your_file` being an arbitrary input to test, use this utility
 * to generate a compressed test frame:
 * 
 * ./fuzz hl-api.c hl-api.c.bz3 8
 * mv hl-api.c.bz3 afl_in/
 * 
 * 4. Build binary (for fuzzing).
 * 
 * afl-clang-fast fuzz.c -I../include ../src/libbz3.c -o fuzz -g3 "-DVERSION=\"0.0.0\"" -O3 -march=native
 * 
 * 5. Run the fuzzer.
 * 
 * AFL_SKIP_CPUFREQ=1 afl-fuzz -i afl_in -o afl_out -- ./fuzz @@
 *
 * 6. Wanna go faster? Multithread.
 * 
 * alacritty -e bash -c "afl-fuzz -i afl_in -o afl_out -M fuzzer01 -- ./fuzz @@; exec bash" &
 * alacritty -e bash -c "afl-fuzz -i afl_in -o afl_out -S fuzzer02 -- ./fuzz @@; exec bash" &
 * alacritty -e bash -c "afl-fuzz -i afl_in -o afl_out -S fuzzer03 -- ./fuzz @@; exec bash" &
 * alacritty -e bash -c "afl-fuzz -i afl_in -o afl_out -S fuzzer04 -- ./fuzz @@; exec bash" &
 * 
 * etc. Replace `alacritty` with your terminal.
 * 
 * And check progress with `afl-whatsup afl_out`.
 * 
 * 7. Found a crash?
 * 
 * If you find a crash, consider also doing the following:
 * 
 * clang fuzz.c ../src/libbz3.c -g3 -O3 -march=native -o fuzz_asan -I../include "-DVERSION=\"0.0.0\"" -fsanitize=undefined -fsanitize=address
 *
 * And run fuzz_asan on the crashing test case. Attach the test case /and/ the output of fuzz_asan to the bug report.
 */

#include <libbz3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define KiB(x) ((x)*1024)

// Required for AFL++ persistent mode
#ifdef __AFL_HAVE_MANUAL_CONTROL
#include <unistd.h>
__AFL_FUZZ_INIT();
#endif

// Maximum allowed size to prevent excessive memory allocation
#define MAX_SIZE 0x10000000 // 256MB

// Returns 0 on success, negative on input validation errors, positive on bzip3 errors
static int try_decompress(const uint8_t *input_buf, size_t input_len) {
    if (input_len < 8) { // invalid, does not contain orig_size
        return -1;
    }

    size_t orig_size = *(const uint32_t *)input_buf;
    uint8_t *outbuf = malloc(orig_size);
    if (!outbuf) {
        return -3;
    }

    // We read orig_size from the input as we also want to fuzz it.
    int bzerr = bz3_decompress(
        input_buf + sizeof(uint32_t),
        outbuf,
        input_len - sizeof(uint32_t),
        &orig_size
    );

    if (bzerr != BZ3_OK) {
        printf("bz3_decompress() failed with error code %d\n", bzerr);
    } else {
        printf("OK, %d => %d\n", (int)input_len, (int)orig_size);
    }

    free(outbuf);
    return bzerr;
}

static int compress_file(const char *infile, const char *outfile, uint32_t block_size) {
    block_size = block_size <= KiB(65) ? KiB(65) : block_size;
    
    // Read the data into `inbuf`
    FILE *fp_in = fopen(infile, "rb");
    if (!fp_in) {
        perror("Failed to open input file");
        return 1;
    }

    fseek(fp_in, 0, SEEK_END);
    size_t insize = ftell(fp_in);
    fseek(fp_in, 0, SEEK_SET);

    uint8_t *inbuf = malloc(insize);
    if (!inbuf) {
        fclose(fp_in);
        return 1;
    }

    fread(inbuf, 1, insize, fp_in);
    fclose(fp_in);

    // Make buffer for output.
    size_t outsize = bz3_bound(insize);
    uint8_t *outbuf = malloc(outsize + sizeof(uint32_t));
    if (!outbuf) {
        free(inbuf);
        return 1;
    }

    // Store original size at the start
    // This is important, the `try_decompress` will read this field during fuzzing.
    // And pass it as a parameter to `bz3_decompress`. 
    *(uint32_t *)outbuf = insize;

    int bzerr = bz3_compress(block_size, inbuf, outbuf + sizeof(uint32_t), insize, &outsize);
    if (bzerr != BZ3_OK) {
        printf("bz3_compress() failed with error code %d\n", bzerr);
        free(inbuf);
        free(outbuf);
        return bzerr;
    }

    FILE *fp_out = fopen(outfile, "wb");
    if (!fp_out) {
        perror("Failed to open output file");
        free(inbuf);
        free(outbuf);
        return 1;
    }

    fwrite(outbuf, 1, outsize + sizeof(uint32_t), fp_out);
    fclose(fp_out);

    printf("Compressed %s (%zu bytes) to %s (%zu bytes)\n", 
           infile, insize, outfile, outsize + sizeof(uint32_t));

    free(inbuf);
    free(outbuf);
    return 0;
}

int main(int argc, char **argv) {
#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
    
    while (__AFL_LOOP(1000)) {
        try_decompress(__AFL_FUZZ_TESTCASE_BUF, __AFL_FUZZ_TESTCASE_LEN);
    }
#else
    if (argc == 4) {
        // Compression mode: input_file output_file block_size
        return compress_file(argv[1], argv[2], atoi(argv[3]));
    }
    
    if (argc != 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  Decompress: %s <input_file>\n", argv[0]);
        fprintf(stderr, "  Compress:   %s <input_file> <output_file> <block_size>\n", argv[0]);
        return 1;
    }

    // Decompression mode
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Failed to open input file");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size < 64) {
        fclose(fp);
        return 0;
    }

    uint8_t *buffer = malloc(size);
    if (!buffer) {
        fclose(fp);
        return 1;
    }

    fread(buffer, 1, size, fp);
    fclose(fp);

    int result = try_decompress(buffer, size);
    free(buffer);
    return result > 0 ? result : 0; // Return bzip3 errors but treat validation errors as success
#endif

    return 0;
}