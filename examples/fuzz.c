
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
 * 1. Build the Repository (per example in README.md)
 * 
 * This will get you a working binary of `bzip3` (in repo root).
 * Then cd into this (examples) folder.
 * 
 * 2. Prepare fuzzer directories
 * 
 * mkdir -p afl_in && mkdir -p afl_out
 * 
 * 3. Make a fuzzer input file.
 * 
 * With `your_file` being an arbitrary input to test.
 * 
 * ../bzip3 -e your_file
 * mv your_file.bz3 afl_in/
 * 
 * 4. Build instrumented binary.
 * 
 * afl-clang fuzz.c -I../include ../src/libbz3.c -o fuzz -g3 "-DVERSION=\"0.0.0\"" -O3 -march=native
 * 
 * 5. Run the fuzzer.
 * 
 * AFL_SKIP_CPUFREQ=1 afl-fuzz -i afl_in -o afl_out -- ./fuzz @@
 *
 * 6. Found a crash?
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

int main(int argc, char ** argv) {
    // Read the entire input file to memory:
    FILE * fp = fopen(argv[1], "rb");
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    volatile uint8_t * buffer = malloc(size);
    fread(buffer, 1, size, fp);
    fclose(fp);

    if (size < 64) {
        // Too small.
        free(buffer);
        return 0;
    }

    // Decompress the file:
    size_t orig_size = *(size_t *)buffer;
    if (orig_size >= 0x10000000) {
        // Sanity check: don't allocate more than 256MB.
        free(buffer);
        return 0;
    }
    uint8_t * outbuf = malloc(orig_size);
    int bzerr = bz3_decompress(buffer + sizeof(size_t), outbuf, size - sizeof(size_t), &orig_size);
    if (bzerr != BZ3_OK) {
        printf("bz3_decompress() failed with error code %d", bzerr);
        free(outbuf);
        free(buffer);
        return 1;
    }

    printf("OK, %d => %d", size, orig_size);
    free(outbuf);
    free(buffer);
    return 0;
}
