
#ifndef _MTF_H
#define _MTF_H

#include <inttypes.h>
#include <stddef.h>

struct mtf_state {
    uint32_t prev[256], curr[256], symbols[256], ranks[256];
};

void mtf_encode(struct mtf_state * mtf, uint8_t *src, uint8_t *dst, uint32_t count);
void mtf_decode(struct mtf_state * mtf, uint8_t *src, uint8_t *dst, uint32_t count);

#endif
