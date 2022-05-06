
// Lempel Ziv Prediction code.

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define MATCH 0xf2

static inline s32 num_blocks(s32 n) {
    if (n < MiB(4)) return 1;
    if (n < MiB(16)) return 2;
    return 4;
}

static s32 lzp_encode_block(const u8 * restrict in, const u8 * in_end, u8 * restrict out, u8 * out_end,
                            s32 * restrict lut, s32 mask, s32 m_len) {
    const u8 *ins = in, *outs = out;
    const u8 * out_eob = out_end - 8;
    const u8 * heur = in;

    u32 ctx;

    for (s32 i = 0; i < 4; ++i) *out++ = *in++;

    ctx = in[-1] | (in[-2] << 8) | (in[-3] << 16) | (in[-4] << 24);

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
                ctx = in[-1] | (in[-2] << 8) | (in[-3] << 16) | (in[-4] << 24);

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

    ctx = in[-1] | (in[-2] << 8) | (in[-3] << 16) | (in[-4] << 24);

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

    u32 ctx = out[-1] | (out[-2] << 8) | (out[-3] << 16) | (out[-4] << 24);

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

                ctx = out[-1] | out[-2] << 8 | out[-3] << 16 | out[-4] << 24;
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

s32 lzp_compress(const u8 * in, u8 * out, s32 n, s32 hash, s32 m_len, s32 * lut) {
    s32 nblk = num_blocks(n);

    if (nblk == 1) {
        if (n - m_len < 32) return -1;

        memset(lut, 0, sizeof(s32) * (1 << hash));

        s32 r = lzp_encode_block(in, in + n, out + 1, out + n - 1, lut, (s32)(1 << hash) - 1, m_len);

        if (r >= 0) {
            out[0] = 1;
            r++;
        }

        return r;
    }

    s32 out_ptr = 1 + 8 * nblk;

    out[0] = nblk;
    for (s32 b_id = 0; b_id < nblk; ++b_id) {
        s32 ins = b_id * (n / nblk);
        s32 insz = b_id != nblk - 1 ? n / nblk : n - ins;
        s32 outsz = insz;
        if (outsz > n - out_ptr) outsz = n - out_ptr;

        s32 r;

        if (insz - m_len < 32)
            r = -1;
        else {
            memset(lut, 0, sizeof(s32) * (1 << hash));

            r = lzp_encode_block(in + ins, in + ins + insz, out + out_ptr, out + out_ptr + outsz, lut,
                                 (s32)(1 << hash) - 1, m_len);
        }

        if (r < 0) {
            if (out_ptr + insz >= n) return -1;
            r = insz;
            memcpy(out + out_ptr, in + ins, insz);
        }
        memcpy(out + 1 + 8 * b_id + 0, &insz, sizeof(s32));
        memcpy(out + 1 + 8 * b_id + 4, &r, sizeof(s32));

        out_ptr += r;
    }

    return out_ptr;
}

s32 lzp_decompress(const u8 * in, u8 * out, s32 n, s32 hash, s32 m_len, s32 * lut) {
    s32 nblk = in[0];

    if (nblk == 1) return lzp_decode_block(in + 1, in + n, lut, out, hash, m_len);

    s32 dec[256];

    for (s32 b_id = 0; b_id < nblk; ++b_id) {
        s32 in_ptr = 0, out_ptr = 0;
        for (s32 p = 0; p < b_id; ++p) {
            in_ptr += *(s32 *)(in + 1 + 8 * p + 4);
            out_ptr += *(s32 *)(in + 1 + 8 * p + 0);
        }

        in_ptr += 1 + 8 * nblk;

        s32 insz = *(s32 *)(in + 1 + 8 * b_id + 4);
        s32 outsz = *(s32 *)(in + 1 + 8 * b_id + 0);

        if (insz != outsz) {
            dec[b_id] = lzp_decode_block(in + in_ptr, in + in_ptr + insz, lut, out + out_ptr, hash, m_len);
        } else {
            dec[b_id] = insz;
            memcpy(out + out_ptr, in + in_ptr, insz);
        }
    }

    s32 dataSize = 0, r = 0;
    for (s32 b_id = 0; b_id < nblk; ++b_id) {
        if (dec[b_id] < 0) r = dec[b_id];
        dataSize += dec[b_id];
    }

    return (r == 0) ? dataSize : r;
}
