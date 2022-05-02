
#ifndef _SRT_H
#define _SRT_H

#include <inttypes.h>
#include <stddef.h>

struct srt_state {
    uint32_t freqs[256];
    uint8_t symbols[256];
    uint32_t r2s[256];
    uint32_t s2r[256];
    uint32_t buckets[256];
    uint32_t bucket_ends[256];
};

uint32_t srt_encode(struct srt_state * mtf, uint8_t *src, uint8_t *dst, uint32_t count);
uint32_t srt_decode(struct srt_state * mtf, uint8_t *src, uint8_t *dst, uint32_t count);

#endif
