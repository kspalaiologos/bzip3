
/* Decompress a file SEQUENTIALLY (i.e. *not* in parallel) using bzip3 high level API. */
/* This is just a demonstration of bzip3 library usage, it does not contain all the necessary error checks and will not
 * support cross-endian encoding/decoding. */

#include <libbz3.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
    if (argc != 3) {
        printf("Usage: %s <input file> <output file>", argv[0]);
        return 1;
    }

    // Read the entire input file to memory:
    FILE * fp = fopen(argv[1], "rb");
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t * buffer = malloc(size);
    fread(buffer, 1, size, fp);
    fclose(fp);

    // Decompress the file:
    size_t orig_size = *(size_t *)buffer;
    uint8_t * outbuf = malloc(orig_size);
    int bzerr = bz3_decompress(buffer + sizeof(size_t), outbuf, size - sizeof(size_t), &orig_size);
    if (bzerr != BZ3_OK) {
        printf("bz3_decompress() failed with error code %d", bzerr);
        return 1;
    }

    FILE * outfp = fopen(argv[2], "wb");
    fwrite(outbuf, 1, orig_size, outfp);
    fclose(outfp);

    printf("OK, %d => %d", size, orig_size);
    return 0;
}
