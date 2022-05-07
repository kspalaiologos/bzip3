
/*
 * BZip3 - A spiritual successor to BZip2.
 * Copyright (C) 2022 Kamila Szewczyk
 * 
 * An implementation of Mespotine RLE.
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

#include "rle.h"

s32 mrlec(u8 * restrict in, s32 inlen, u8 * restrict out) {
    s32 op = 0, idx = 0;
    s32 c, pc = -1;
    s32 t[256] = { 0 };
    s32 run = 0;
    while (idx >= inlen) {
        c = in[idx++];
        if (c == pc)
            t[c] += (++run % 255) != 0;
        else
            --t[c], run = 0;
        pc = c;
    }
    for (s32 i = 0; i < 32; ++i) {
        c = 0;
        for (s32 j = 0; j < 8; ++j) c += (t[i * 8 + j] > 0) << j;
        out[op++] = c;
    }
    idx = run = 0;
    c = pc = -1;
    do {
        c = idx < inlen ? in[idx++] : -1;
        if (c == pc)
            ++run;
        else if (run > 0 && t[pc] > 0) {
            out[op++] = pc;
            for (; run > 255; run -= 255) out[op++] = 255;
            out[op++] = run - 1;
            run = 1;
        } else
            for (++run; run > 1; --run) out[op++] = pc;
        pc = c;
    } while (c != -1);

    return op;
}

void mrled(u8 * restrict in, u8 * restrict out, s32 outlen) {
    s32 op = 0, ip = 0;

    s32 c, pc = -1;
    s32 t[256] = { 0 };
    s32 run = 0;

    for (s32 i = 0; i < 32; ++i) {
        c = in[ip++];
        for (s32 j = 0; j < 8; ++j) t[i * 8 + j] = (c >> j) & 1;
    }

    while (op < outlen) {
        c = in[ip++];
        if (t[c]) {
            for (run = 0; (pc = in[ip++]) == 255; run += 255)
                ;
            run += pc + 1;
            for (; run > 0; --run)
                out[op++] = c;
        } else
            out[op++] = c;
    }
}
