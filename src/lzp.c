
// Lempel Ziv Prediction code.
// A heavily modified version of libbcm's LZP predictor. This one has single thread performance.

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define MATCH 0xf2

static s32 lzp_encode_block(const u8 * restrict in, const u8 * in_end, u8 * restrict out, u8 * out_end,
                            s32 * restrict lut, s32 mask, s32 m_len) {
    const u8 *ins = in, *outs = out;
    const u8 * out_eob = out_end - 8;
    const u8 * heur = in;

    u32 ctx;

    for (s32 i = 0; i < 4; ++i) *out++ = *in++;

    ctx = ((u32) in[-1]) | (((u32) in[-2]) << 8) | (((u32) in[-3]) << 16) | (((u32) in[-4]) << 24);

    while (in < in_end - m_len - 32 && out < out_eob) {
        u32 idx = (ctx >> 15 ^ ctx ^ ctx >> 3) & mask;
        s32 val = lut[idx];
        lut[idx] = in - ins;
        if (val > 0) {
            const u8 * restrict ref = ins + val;
            if (memcmp(in + m_len - 4, ref + m_len - 4, sizeof(u32)) == 0 && memcmp(in, ref, sizeof(u32)) == 0) {
                if (heur > in && *(u32 *)heur != *(u32 *)(ref + (heur - in))) goto not_found;

                s32 len = 4;
                for (; in + len < in_end - m_len - 32; len += sizeof(u32)) {
                    if (*(u32 *)(in + len) != *(u32 *)(ref + len)) break;
                }

                if (len < m_len) {
                    if (heur < in + len) heur = in + len;
                    goto not_found;
                }

                len += in[len] == ref[len];
                len += in[len] == ref[len];
                len += in[len] == ref[len];

                in += len;
                ctx = ((u32) in[-1]) | (((u32) in[-2]) << 8) | (((u32) in[-3]) << 16) | (((u32) in[-4]) << 24);

                *out++ = MATCH;

                len -= m_len;
                while (len >= 254) {
                    len -= 254;
                    *out++ = 254;
                    if (out >= out_eob) break;
                }

                *out++ = len;
            } else {
            not_found:;
                u8 next = *out++ = *in++;
                ctx = ctx << 8 | next;
                if (next == MATCH) *out++ = 255;
            }
        } else {
            ctx = (ctx << 8) | (*out++ = *in++);
        }
    }

    ctx = ((u32) in[-1]) | (((u32) in[-2]) << 8) | (((u32) in[-3]) << 16) | (((u32) in[-4]) << 24);

    while (in < in_end && out < out_eob) {
        u32 idx = (ctx >> 15 ^ ctx ^ ctx >> 3) & mask;
        s32 val = lut[idx];
        lut[idx] = (s32)(in - ins);

        u8 next = *out++ = *in++;
        ctx = ctx << 8 | next;
        if (next == MATCH && val > 0) *out++ = 255;
    }

    return out >= out_eob ? -1 : (s32)(out - outs);
}

static s32 lzp_decode_block(const u8 * restrict in, const u8 * in_end, s32 * restrict lut, u8 * restrict out, s32 hash,
                            s32 m_len) {
    if (in_end - in < 4) return -1;

    memset(lut, 0, sizeof(s32) * (1 << hash));

    u32 mask = (s32)(1 << hash) - 1;
    const u8 * outs = out;

    for (s32 i = 0; i < 4; ++i) *out++ = *in++;

    u32 ctx = ((u32) out[-1]) | (((u32) out[-2]) << 8) | (((u32) out[-3]) << 16) | (((u32) out[-4]) << 24);

    while (in < in_end) {
        u32 idx = (ctx >> 15 ^ ctx ^ ctx >> 3) & mask;
        s32 val = lut[idx];
        lut[idx] = (s32)(out - outs);
        if (*in == MATCH && val > 0) {
            in++;
            if (*in != 255) {
                s32 len = m_len;
                while (1) {
                    len += *in;
                    if (*in++ != 254) break;
                }

                const u8 * ref = outs + val;
                u8 * out_end = out + len;

                while (out < out_end) *out++ = *ref++;

                ctx = ((u32) out[-1]) | (((u32) out[-2]) << 8) | (((u32) out[-3]) << 16) | (((u32) out[-4]) << 24);
            } else {
                in++;
                ctx = (ctx << 8) | (*out++ = MATCH);
            }
        } else {
            ctx = (ctx << 8) | (*out++ = *in++);
        }
    }

    return out - outs;
}

s32 lzp_compress(const u8 * restrict in, u8 * restrict out, s32 n, s32 hash, s32 m_len, s32 * restrict lut) {
    if (n - m_len < 32) return -1;

    memset(lut, 0, sizeof(s32) * (1 << hash));

    return lzp_encode_block(in, in + n, out, out + n, lut, (s32)(1 << hash) - 1, m_len);
}

s32 lzp_decompress(const u8 * restrict in, u8 * restrict out, s32 n, s32 hash, s32 m_len, s32 * restrict lut) {
    return lzp_decode_block(in, in + n, lut, out, hash, m_len);
}
