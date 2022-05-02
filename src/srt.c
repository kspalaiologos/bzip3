
/*
 * BZip3 - A spiritual successor to BZip2.
 * Copyright (C) 2022 Kamila Szewczyk
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "srt.h"

static const int32_t MAX_HDR_SIZE = 4 * 256;

static int32_t preprocess(const uint32_t * freqs, uint8_t * symbols) {
    int32_t nb_symbols = 0;
    for (int32_t i = 0; i < 256; i++)
        if (freqs[i] > 0) symbols[nb_symbols++] = i;
    uint32_t h = 4;
    while (h < nb_symbols) h = h * 3 + 1;
    while (1) {
        h /= 3;
        for (uint32_t i = h; i < nb_symbols; i++) {
            const int32_t t = symbols[i] & 0xFF;
            int32_t b = i - h;
            while ((b >= 0) && freqs[symbols[b]] < freqs[t] ||
                   (freqs[t] == freqs[symbols[b]]) && t < symbols[b]) {
                symbols[b + h] = symbols[b];
                b -= h;
            }
            symbols[b + h] = t;
        }
        if (h == 1) break;
    }
    return nb_symbols;
}

static int32_t encode_header(uint32_t * freqs, uint8_t * dst) {
    uint32_t idx = 0;
    for (int32_t i = 0; i < 256; i++) {
        uint32_t f = freqs[i];
        while (f >= 128) {
            dst[idx++] = (uint8_t)(f | 0x80);
            f >>= 7;
        }
        dst[idx++] = (uint8_t)f;
    }
    return idx;
}

static int32_t decode_header(uint8_t * src, uint32_t * freqs) {
    uint32_t idx = 0;
    for (int32_t i = 0; i < 256; i++) {
        int32_t val = src[idx++] & 0xFF;
        int32_t res = val & 0x7F;
        int32_t shift = 7;
        while (val >= 128) {
            val = src[idx++] & 0xFF;
            res |= (val & 0x7F) << shift;
            if (shift > 21) break;
            shift += 7;
        }
        freqs[i] = res;
    }
    return idx;
}

uint32_t srt_encode(struct srt_state * mtf, uint8_t * src, uint8_t * dst,
                    uint32_t count) {
    // Find first symbols and build a histogram.
    for (int32_t i = 0; i < 256; i++) mtf->freqs[i] = 0;
    for (uint32_t i = 0, b = 0; i < count;) {
        if (mtf->freqs[src[i]] == 0) {
            mtf->r2s[b] = src[i];
            mtf->s2r[src[i]] = b;
            b++;
        }
        uint32_t j = i + 1;
        while (j < count && src[j] == src[i]) j++;
        mtf->freqs[src[i]] += j - i;
        i = j;
    }

    int32_t n_symbols = preprocess(mtf->freqs, mtf->symbols);
    for (uint32_t i = 0, bucket_pos = 0; i < n_symbols; i++) {
        mtf->buckets[mtf->symbols[i]] = bucket_pos;
        bucket_pos += mtf->freqs[mtf->symbols[i]];
    }

    const uint32_t header_size = encode_header(mtf->freqs, dst);
    const int32_t dst_idx = header_size;
    for (uint32_t i = 0; i < count;) {
        const int32_t c = src[i] & 0xFF;
        int32_t r = mtf->s2r[c] & 0xFF;
        uint32_t p = mtf->buckets[c];
        dst[dst_idx + p++] = r;
        if (r != 0) {
            do {
                mtf->r2s[r] = mtf->r2s[r - 1];
                mtf->s2r[mtf->r2s[r]] = r;
                r--;
            } while (r != 0);
            mtf->r2s[0] = c;
            mtf->s2r[c] = 0;
        }
        i++;
        while (i < count && src[i] == c) {
            dst[dst_idx + p++] = 0;
            i++;
        }
        mtf->buckets[c] = p;
    }
    return count + header_size;
}

uint32_t srt_decode(struct srt_state * mtf, uint8_t * src, uint8_t * dst,
                    uint32_t count) {
    const uint32_t header_size = decode_header(src, mtf->freqs);
    const uint32_t src_idx = header_size;
    int32_t nb_symbols = preprocess(mtf->freqs, mtf->symbols);
    for (uint32_t i = 0, bucket_pos = 0; i < nb_symbols; i++) {
        const int32_t c = mtf->symbols[i] & 0xFF;
        mtf->r2s[src[src_idx + bucket_pos] & 0xFF] = c;
        mtf->buckets[c] = bucket_pos + 1;
        bucket_pos += mtf->freqs[c];
        mtf->bucket_ends[c] = bucket_pos;
    }
    uint32_t c = mtf->r2s[0];
    for (uint32_t i = 0; i < count; i++) {
        dst[i] = c;
        if (mtf->buckets[c] < mtf->bucket_ends[c]) {
            const int32_t r = src[src_idx + mtf->buckets[c]] & 0xFF;
            mtf->buckets[c]++;
            if (r == 0) continue;
            for (int32_t s = 0; s < r; s++) mtf->r2s[s] = mtf->r2s[s + 1];
            mtf->r2s[r] = c;
            c = mtf->r2s[0];
        } else {
            if (nb_symbols == 1) continue;
            nb_symbols--;
            for (int32_t s = 0; s < nb_symbols; s++)
                mtf->r2s[s] = mtf->r2s[s + 1];
            c = mtf->r2s[0];
        }
    }
    return count - header_size;
}
