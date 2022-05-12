
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <libbz3.h>

uint8_t * new_buf;
uint32_t orig_size, new_size;
FILE * dest;
uint32_t n = 0;

void try_flip(struct bz3_state * s) {
    uint32_t idx = rand() % new_size, bit = rand() % 8;
    new_buf[idx] ^= (1 << bit);
    fseek(dest, idx, SEEK_SET); fputc(new_buf[idx], dest); fflush(dest);

    // The operation will fail. It's just important so that it doesn't segfault/access wrong memory.
    bz3_decode_block(s, new_buf, new_size, orig_size);

    printf("\r%d", n++); fflush(stdout);
}

int main(void) {
    FILE * f = fopen("shakespeare.txt", "r");
    if (!f) { perror("fopen"); return 1; }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t * buf = malloc(size);
    if (!buf) { perror("malloc"); return 1; }
    if (fread(buf, 1, size, f) != size) { perror("fread"); return 1; }
    fclose(f);
    struct bz3_state * s = bz3_new(6 * 1024 * 1024);
    if (!s) { printf("error in bz3_new.\n"); return 1; }
    int32_t len = bz3_encode_block(s, buf, size);
    if (len < 0) { printf("error in bz3_compress. %s.\n", bz3_strerror(s)); return 1; }
    new_buf = malloc(size + size / 50 + 32);
    if (!new_buf) { perror("malloc"); return 1; }
    orig_size = size;
    new_size = len;
    dest = fopen("shakespeare.bz3", "w");
    if (!dest) { perror("fopen"); return 1; }
    srand(time(NULL));
    fwrite(buf, 1, len, dest);
    while(1) {
        memcpy(new_buf, buf, len);
        try_flip(s);
    }
}
