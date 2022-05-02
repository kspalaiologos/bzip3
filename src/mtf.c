
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

#include "mtf.h"

void mtf_encode(struct mtf_state * mtf, u8 * src, u8 * dst, u32 count) {
    for (u32 i = 0; i < 256; i++) {
        mtf->prev[i] = mtf->curr[i] = 0;
        mtf->symbols[i] = mtf->ranks[i] = i;
    }

    for (u32 i = 0; i < count; i++) {
        u32 r = mtf->symbols[src[i]];
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

void mtf_decode(struct mtf_state * mtf, u8 * src, u8 * dst, u32 count) {
    for (u32 i = 0; i < 256; i++) {
        mtf->prev[i] = mtf->curr[i] = 0;
        mtf->ranks[i] = i;
    }

    for (u32 i = 0; i < count; i++) {
        u32 r = src[i] & 0xFF;

        const u32 c = mtf->ranks[r];
        dst[i] = (s8)c;

        mtf->prev[c] = mtf->curr[c] = i;

        for (; r > 0 && mtf->curr[mtf->ranks[r - 1]] <= i; r--)
            mtf->ranks[r] = mtf->ranks[r - 1];

        mtf->ranks[r] = c;
    }
}
