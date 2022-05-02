
#include "rle.h"

/* Derived from Matt Mahoney's public domain RLE code. */

#define _putc(__ch, __out) *__out++ = (__ch)
#define _getc(in, in_) (in < in_ ? (*in++) : -1)
#define _rewind(in, _in) in = _in

int mrlec(unsigned char *in, int inlen, unsigned char *out) {
    unsigned char *ip = in, *in_ = in + inlen, *op = out;
    int i;
    int c, pc = -1;
    long t[256] = {0};
    long run = 0;
    while ((c = _getc(ip, in_)) != -1) {
        if (c == pc)
            t[c] += (++run % 255) != 0;
        else
            --t[c], run = 0;
        pc = c;
    }
    for (i = 0; i < 32; ++i) {
        int j;
        c = 0;
        for (j = 0; j < 8; ++j) c += (t[i * 8 + j] > 0) << j;
        _putc(c, op);
    }
    _rewind(ip, in);
    c = pc = -1;
    run = 0;
    do {
        c = _getc(ip, in_);
        if (c == pc)
            ++run;
        else if (run > 0 && t[pc] > 0) {
            _putc(pc, op);
            for (; run > 255; run -= 255) _putc(255, op);
            _putc(run - 1, op);
            run = 1;
        } else
            for (++run; run > 1; --run) _putc(pc, op);
        pc = c;
    } while (c != -1);

    return op - out;
}

int mrled(unsigned char *in, unsigned char *out, int outlen) {
    unsigned char *ip = in, *op = out;
    int i;

    int c, pc = -1;
    long t[256] = {0};
    long run = 0;

    for (i = 0; i < 32; ++i) {
        int j;
        c = *ip++;
        for (j = 0; j < 8; ++j) t[i * 8 + j] = (c >> j) & 1;
    }

    while (op < out + outlen) {
        c = *ip++;
        if (t[c]) {
            for (run = 0; (pc = *ip++) == 255; run += 255)
                ;
            run += pc + 1;
            for (; run > 0; --run) _putc(c, op);
        } else
            _putc(c, op);
    }
    return ip - in;
}
