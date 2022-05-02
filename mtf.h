
#ifndef _MTF_H
#define _MTF_H

struct mtf_state {
    uint32_t prev[256], curr[256], symbols[256], ranks[256];
};

void mtf_encode(struct mtf_state * mtf, uint8_t *src, uint8_t *dst, uint32_t count) {
    for (uint32_t i = 0; i < 256; i++) {
        mtf->prev[i] = mtf->curr[i] = 0;
        mtf->symbols[i] = mtf->ranks[i] = i;
    }

    for (uint32_t i = 0; i < count; i++) {
        uint32_t r = mtf->symbols[src[i]];
        dst[i] = r;

        mtf->prev[src[i]] = mtf->curr[src[i]] = i;

        for (; r > 0 && mtf->curr[mtf->ranks[r - 1]] <= i; r--) {
            mtf->ranks[r] = mtf->ranks[r - 1];
            mtf->symbols[mtf->ranks[r]] = r;
        }

        mtf->ranks[r] = src[i];
        mtf->symbols[src[i]] = r;
    }
}

void mtf_decode(struct mtf_state * mtf, uint8_t *src, uint8_t *dst, uint32_t count) {
    for (uint32_t i = 0; i < 256; i++) {
        mtf->prev[i] = mtf->curr[i] = 0;
        mtf->ranks[i] = i;
    }

    for (uint32_t i = 0; i < count; i++) {
        uint32_t r = src[i] & 0xFF;

        const uint32_t c = mtf->ranks[r];
        dst[i] = (int8_t)c;

        mtf->prev[c] = mtf->curr[c] = i;

        for (; r > 0 && mtf->curr[mtf->ranks[r - 1]] <= i; r--)
            mtf->ranks[r] = mtf->ranks[r - 1];

        mtf->ranks[r] = c;
    }
}

#endif
