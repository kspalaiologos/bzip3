
#include "rle.h"

/* Derived from Matt Mahoney's public domain RLE code. */

#define buffer_write(__ch, __out) *__out++ = (__ch)
#define buffer_read(in, in_) (in < in_ ? (*in++) : -1)

int32_t mrlec(uint8_t * in, int32_t inlen, uint8_t * out) {
    uint8_t *ip = in, *in_ = in + inlen, *op = out;
    int32_t i;
    int32_t c, pc = -1;
    int64_t t[256] = { 0 };
    int64_t run = 0;
    while ((c = buffer_read(ip, in_)) != -1) {
        if (c == pc)
            t[c] += (++run % 255) != 0;
        else
            --t[c], run = 0;
        pc = c;
    }
    for (i = 0; i < 32; ++i) {
        int32_t j;
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

int32_t mrled(uint8_t * in, uint8_t * out, int32_t outlen) {
    uint8_t *ip = in, *op = out;
    int32_t i;

    int32_t c, pc = -1;
    int64_t t[256] = { 0 };
    int64_t run = 0;

    for (i = 0; i < 32; ++i) {
        int32_t j;
        c = *ip++;
        for (j = 0; j < 8; ++j) t[i * 8 + j] = (c >> j) & 1;
    }

    while (op < out + outlen) {
        c = *ip++;
        if (t[c]) {
            for (run = 0; (pc = *ip++) == 255; run += 255)
                ;
            run += pc + 1;
            for (; run > 0; --run) buffer_write(c, op);
        } else
            buffer_write(c, op);
    }
    return ip - in;
}
