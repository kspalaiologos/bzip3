
/*
 * BZip3 - A spiritual successor to BZip2.
 * Copyright (C) 2022 Kamila Szewczyk
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "e8e9.h"

/* Loosely based on Shelwien's E8E9 filter. Doesn't blindly transform data. */
struct e8e9 {
    uint8_t cs;
	uint32_t x0, x1, i, k;
};

static struct e8e9 e8e9_init() {
    struct e8e9 s;
    s.x0 = 0;
    s.x1 = 0;
    s.i = 0;
    s.k = 5;
    s.cs = 0xFF;
    return s;
}

static int32_t e8e9_cache_byte(struct e8e9 * s, int32_t c) {
    int32_t d = s->cs & 0x80 ? -1 : (uint8_t)(s->x1);
    s->x1 >>= 8;
    s->x1 |= (s->x0 << 24);
    s->x0 >>= 8;
    s->x0 |= (c << 24);
    s->cs <<= 1;
    s->i++;
    return d;
}

static uint32_t e8e9_x_swap(uint32_t x) {
    x <<= 7;
    return (x >> 24) | ((uint8_t)(x >> 16) << 8) | ((uint8_t)(x >> 8) << 16) | ((uint8_t)(x) << (24 - 7));
}

static uint32_t e8e9_y_swap(uint32_t x) {
    x = ((uint8_t)(x >> 24) << 7) | ((uint8_t)(x >> 16) << 8) | ((uint8_t)(x >> 8) << 16) | (x << 24);
    return x >> 7;
}

static int32_t e8e9_fb(struct e8e9 * s, int32_t c) {
    uint32_t x;
    if (s->i >= s->k) {
        if ((s->x1 & 0xFE000000) == 0xE8000000) {
            s->k = s->i + 4;
            x = s->x0 - 0xFF000000;
            if (x < 0x02000000) {
                x = (x + s->i) & 0x01FFFFFF;
                x = x_swap(x);
                s->x0 = x + 0xFF000000;
            }
        }
    }
    return cache_byte(s, c);
}

static int32_t e8e9_bb(struct e8e9 * s, int32_t c) {
    uint32_t x;
    if (s->i >= s->k) {
        if ((s->x1 & 0xFE000000) == 0xE8000000) {
            s->k = s->i + 4;
            x = s->x0 - 0xFF000000;
            if (x < 0x02000000) {
                x = y_swap(x);
                x = (x - s->i) & 0x01FFFFFF;
                s->x0 = x + 0xFF000000;
            }
        }
    }
    return cache_byte(s, c);
}

static int32_t e8e9_flush(struct e8e9 * s) {
    int32_t d;
    if (s->cs != 0xFF) {
        while (s->cs & 0x80) cache_byte(s, 0), ++s->cs;
        d = cache_byte(s, 0);
        ++s->cs;
        return d;
    } else {
        s->x0 = 0;
        s->x1 = 0;
        s->i = 0;
        s->k = 5;
        s->cs = 0xFF;
        return -1;
    }
}

s32 e8e9_forward(u8 * restrict in, s32 inlen, u8 * restrict out) {
    s32 out_ptr = 0;

    s32 oct = 0;
    for(s32 i = 0; i < inlen; i++)
        if(in[i] == 0xE8 || in[i] == 0xE9)
            oct++;

    struct e8e9 s = e8e9_init();

    /* All of the octets should be less than 2% of the data. */
    s32 p = oct * 1000 / inlen;
    if(p < 20) {
        for(s32 i = 0; i < inlen; i++) {
            int c = e8e9_fb(&s, in[i]);
            if (c >= 0) out[out_ptr++] = c;
        }
        int c;
        while ((c = flush()) >= 0)
            out[out_ptr++] = c;
        return out_ptr;
    } else {
        return -1;
    }
}

s32 e8e9_backward(u8 * restrict in, s32 inlen, u8 * restrict out) {
    s32 out_ptr = 0;
    struct e8e9 s = e8e9_init();
    for(s32 i = 0; i < inlen; i++) {
        int c = e8e9_bb(&s, in[i]);
        if (c >= 0) out[out_ptr++] = c;
    }
    int c;
    while ((c = flush()) >= 0)
        out[out_ptr++] = c;
    return out_ptr;
}
