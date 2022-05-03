
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

static const s32 MAX_HDR_SIZE = 4 * 256;

static s32 preprocess(const u32 * freqs, u8 * symbols) {
    s32 nb_symbols = 0;
    for (s32 i = 0; i < 256; i++)
        if (freqs[i] > 0) symbols[nb_symbols++] = i;
    u32 h = 4;
    while (h < nb_symbols) h = h * 3 + 1;
    while (1) {
        h /= 3;
        for (u32 i = h; i < nb_symbols; i++) {
            const s32 t = symbols[i];
            s32 b = i - h;
            while ((b >= 0) && (freqs[symbols[b]] < freqs[t] ||
                                (freqs[t] == freqs[symbols[b]] && t < symbols[b]))) {
                symbols[b + h] = symbols[b];
                b -= h;
            }
            symbols[b + h] = t;
        }
        if (h == 1) break;
    }
    return nb_symbols;
}

static s32 encode_header(u32 * freqs, u8 * dst) {
    u32 idx = 0;
    for (s32 i = 0; i < 256; i++) {
        u32 f = freqs[i];
        while (f >= 128) {
            dst[idx++] = (u8)(f | 0x80);
            f >>= 7;
        }
        dst[idx++] = (u8)f;
    }
    return idx;
}

static s32 decode_header(u8 * src, u32 * freqs) {
    u32 idx = 0;
    for (s32 i = 0; i < 256; i++) {
        s32 val = src[idx++] & 0xFF;
        s32 res = val & 0x7F;
        s32 shift = 7;
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

u32 srt_encode(struct srt_state * mtf, u8 * src, u8 * dst, u32 count) {
    // Find first symbols and build a histogram.
    for (s32 i = 0; i < 256; i++) mtf->freqs[i] = 0;
    for (u32 i = 0, b = 0; i < count;) {
        if (mtf->freqs[src[i]] == 0) {
            mtf->r2s[b] = src[i];
            mtf->s2r[src[i]] = b;
            b++;
        }
        u32 j = i + 1;
        while (j < count && src[j] == src[i]) j++;
        mtf->freqs[src[i]] += j - i;
        i = j;
    }

    s32 n_symbols = preprocess(mtf->freqs, mtf->symbols);
    for (u32 i = 0, bucket_pos = 0; i < n_symbols; i++) {
        mtf->buckets[mtf->symbols[i]] = bucket_pos;
        bucket_pos += mtf->freqs[mtf->symbols[i]];
    }

    const u32 header_size = encode_header(mtf->freqs, dst);
    const s32 dst_idx = header_size;
    for (u32 i = 0; i < count;) {
        const s32 c = src[i] & 0xFF;
        s32 r = mtf->s2r[c] & 0xFF;
        u32 p = mtf->buckets[c];
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

u32 srt_decode(struct srt_state * mtf, u8 * src, u8 * dst, u32 count) {
    const u32 header_size = decode_header(src, mtf->freqs);
    const u32 src_idx = header_size;
    s32 nb_symbols = preprocess(mtf->freqs, mtf->symbols);
    for (u32 i = 0, bucket_pos = 0; i < nb_symbols; i++) {
        const s32 c = mtf->symbols[i] & 0xFF;
        mtf->r2s[src[src_idx + bucket_pos] & 0xFF] = c;
        mtf->buckets[c] = bucket_pos + 1;
        bucket_pos += mtf->freqs[c];
        mtf->bucket_ends[c] = bucket_pos;
    }
    u32 c = mtf->r2s[0];
    for (u32 i = 0; i < count; i++) {
        dst[i] = c;
        if (mtf->buckets[c] < mtf->bucket_ends[c]) {
            const s32 r = src[src_idx + mtf->buckets[c]] & 0xFF;
            mtf->buckets[c]++;
            if (r == 0) continue;
            for (s32 s = 0; s < r; s++) mtf->r2s[s] = mtf->r2s[s + 1];
            mtf->r2s[r] = c;
            c = mtf->r2s[0];
        } else {
            if (nb_symbols == 1) continue;
            nb_symbols--;
            for (s32 s = 0; s < nb_symbols; s++) mtf->r2s[s] = mtf->r2s[s + 1];
            c = mtf->r2s[0];
        }
    }
    return count - header_size;
}
