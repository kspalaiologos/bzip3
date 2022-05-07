
#include "rle.h"

/* Derived from Matt Mahoney's public domain RLE code. */

#define buffer_write(__ch, __out) *__out++ = (__ch)
#define buffer_read(in, in_) (in < in_ ? (*in++) : -1)

s32 mrlec(u8 * in, s32 inlen, u8 * out) {
    u8 *ip = in, *in_ = in + inlen, *op = out;
    s32 i;
    s32 c, pc = -1;
    s32 t[256] = { 0 };
    s32 run = 0;
    while ((c = buffer_read(ip, in_)) != -1) {
        if (c == pc)
            t[c] += (++run % 255) != 0;
        else
            --t[c], run = 0;
        pc = c;
    }
    for (i = 0; i < 32; ++i) {
        s32 j;
        c = 0;
        for (j = 0; j < 8; ++j) c += (t[i * 8 + j] > 0) << j;
        buffer_write(c, op);
    }
    ip = in;
    c = pc = -1;
    run = 0;
    do {
        c = buffer_read(ip, in_);
        if (c == pc)
            ++run;
        else if (run > 0 && t[pc] > 0) {
            buffer_write(pc, op);
            for (; run > 255; run -= 255) buffer_write(255, op);
            buffer_write(run - 1, op);
            run = 1;
        } else
            for (++run; run > 1; --run) buffer_write(pc, op);
        pc = c;
    } while (c != -1);

    return op - out;
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
