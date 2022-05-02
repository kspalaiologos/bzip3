
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

uint32_t srt_encode(struct srt_state * mtf, uint8_t * src, uint8_t * dst,
                    uint32_t count);
uint32_t srt_decode(struct srt_state * mtf, uint8_t * src, uint8_t * dst,
                    uint32_t count);

#endif
