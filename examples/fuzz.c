
/* A tiny utility for fuzzing bzip3.
 * 
 * Instructions:
 * mkdir -p afl_in && mkdir -p afl_out
 * ./compress-file ../Makefile afl_in/a.bz3
 * afl-clang examples/fuzz.c -Iinclude src/libbz3.c -o examples/fuzz -g3 "-DVERSION=\"0.0.0\"" -O3 -march=native
 * AFL_SKIP_CPUFREQ=1 afl-fuzz -i afl_in -o afl_out -- ./decompress-fuzz @@
 * 
 * If you find a crash, consider also doing the following:
 * gcc examples/fuzz.c src/libbz3.c -g3 -O3 -march=native -o examples/fuzz_asan -Iinclude "-DVERSION=\"0.0.0\"" -fsanitize=undefined -fsanitize=address
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

    // Decompress the file:
    size_t orig_size = *(size_t *)buffer;
    if(orig_size >= 0x10000000) {
        // Sanity check: don't allocate more than 256MB.
        return 0;
    }
    uint8_t * outbuf = malloc(orig_size);
    int bzerr = bz3_decompress(buffer + sizeof(size_t), outbuf, size - sizeof(size_t), &orig_size);
    if (bzerr != BZ3_OK) {
        printf("bz3_decompress() failed with error code %d", bzerr);
        return 1;
    }

    printf("OK, %d => %d", size, orig_size);
    return 0;
}
