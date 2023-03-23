
/* Compress a file SEQUENTIALLY (i.e. *not* in parallel) using bzip3 high level API with block size of 6 MB. */
/* This is just a demonstration of bzip3 library usage, it does not contain all the necessary error checks and will not
 * support cross-endian encoding/decoding. */

#include <libbz3.h>
#include <stdio.h>
#include <stdlib.h>

#define MB (1024 * 1024)

int main(int argc, char ** argv) {
    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
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

    // Compress the file:
    size_t out_size = bz3_bound(size);
    uint8_t * outbuf = malloc(out_size);
    int bzerr = bz3_compress(6 * MB, buffer, outbuf, size, &out_size);
    if (bzerr != BZ3_OK) {
        printf("bz3_compress() failed with error code %d", bzerr);
        return 1;
    }

    FILE * outfp = fopen(argv[2], "wb");
    /* XXX: Doesn't preserve endianess. We should write the `size_t` value manually with known endianess. */
    fwrite(&size, 1, sizeof(size_t), outfp);
    fwrite(outbuf, 1, out_size, outfp);
    fclose(outfp);

    printf("OK, %d => %d\n", size, out_size);
    return 0;
}
