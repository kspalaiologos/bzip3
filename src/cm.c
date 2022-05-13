
#include "cm.h"

#if defined(__has_builtin)
    #if __has_builtin(__builtin_prefetch)
        #define HAS_BUILTIN_PREFECTCH
    #endif
#elif defined(__GNUC__) && (((__GNUC__ == 3) && (__GNUC_MINOR__ >= 2)) || (__GNUC__ >= 4))
    #define HAS_BUILTIN_PREFECTCH
#endif

#if defined(HAS_BUILTIN_PREFECTCH)
    #define prefetch(address) __builtin_prefetch((const void *)(address), 0, 0)
#elif defined(_M_IX86) || defined(_M_AMD64)
    #include <intrin.h>
    #define prefetch(address) _mm_prefetch((const void *)(address), _MM_HINT_NTA)
#elif defined(_M_ARM)
    #include <intrin.h>
    #define prefetch(address) __prefetch((const void *)(address))
#elif defined(_M_ARM64)
    #include <intrin.h>
    #define prefetch(address) __prefetch2((const void *)(address), 1)
#else
    #define prefetch(address)
#endif

// Uses an arithmetic coder implementation outlined in:
// http://mattmahoney.net/dc/dce.html#Section_31

#define write_out(s, c) (s)->out_queue[(s)->output_ptr++] = (c)
#define read_in(s) ((s)->input_ptr < (s)->input_max ? (s)->in_queue[(s)->input_ptr++] : -1)

void flush(state * s) {
    write_out(s, s->low >> 24);
    s->low <<= 8;
    write_out(s, s->low >> 24);
    s->low <<= 8;
    write_out(s, s->low >> 24);
    s->low <<= 8;
    write_out(s, s->low >> 24);
    s->low <<= 8;
}

void init(state * s) {
    s->code = (s->code << 8) + read_in(s);
    s->code = (s->code << 8) + read_in(s);
    s->code = (s->code << 8) + read_in(s);
    s->code = (s->code << 8) + read_in(s);
}

#define update0(p, x) ((p) - ((p) >> x))
#define update1(p, x) ((p) + (((p) ^ 65535) >> x))

void begin(state * s) {
    prefetch(s);
    s->c1 = s->c2 = 0;
    s->run = 0;
    s->low = 0;
    s->high = 0xFFFFFFFF;
    s->code = 0;
    for (int i = 0; i < 256; i++) s->C0[i] = 1 << 15;
    for (int i = 0; i < 256; i++)
        for (int j = 0; j < 256; j++) s->C1[i][j] = 1 << 15;
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 256; j++)
            for (int k = 0; k < 17; k++) s->C2[2 * j + i][k] = (k << 12) - (k == 16);
}

void encode_bytes(state * s, u8 * buf, s32 size) {
    for(s32 i = 0; i < size; i++) {
        u8 c = buf[i];

        if (s->c1 == s->c2)
            ++s->run;
        else
            s->run = 0;

        const int f = s->run > 2;

        int ctx = 1;

        while (ctx < 256) {
            const int p0 = s->C0[ctx];
            const int p1 = s->C1[s->c1][ctx];
            const int p2 = s->C1[s->c2][ctx];
            const int p = ((p0 + p1) * 7 + p2 + p2) >> 4;

            const int j = p >> 12;
            const int x1 = s->C2[2 * ctx + f][j];
            const int x2 = s->C2[2 * ctx + f][j + 1];
            const int ssep = x1 + (((x2 - x1) * (p & 4095)) >> 12);

            if (c & 128) {
                s->high = s->low + (((u64)(s->high - s->low) * (ssep * 3 + p)) >> 18);

                while ((s->low ^ s->high) < (1 << 24)) {
                    write_out(s, s->low >> 24);
                    s->low <<= 8;
                    s->high = (s->high << 8) + 0xFF;
                }

                s->C0[ctx] = update1(s->C0[ctx], 2);
                s->C1[s->c1][ctx] = update1(s->C1[s->c1][ctx], 4);
                s->C2[2 * ctx + f][j] = update1(s->C2[2 * ctx + f][j], 6);
                s->C2[2 * ctx + f][j + 1] = update1(s->C2[2 * ctx + f][j + 1], 6);
                ctx += ctx + 1;
            } else {
                s->low += (((u64)(s->high - s->low) * (ssep * 3 + p)) >> 18) + 1;

                // Write identical bits.
                while ((s->low ^ s->high) < (1 << 24)) {
                    write_out(s, s->low >> 24);  // Same as s->high >> 24
                    s->low <<= 8;
                    s->high = (s->high << 8) + 0xFF;
                }

                s->C0[ctx] = update0(s->C0[ctx], 2);
                s->C1[s->c1][ctx] = update0(s->C1[s->c1][ctx], 4);
                s->C2[2 * ctx + f][j] = update0(s->C2[2 * ctx + f][j], 6);
                s->C2[2 * ctx + f][j + 1] = update0(s->C2[2 * ctx + f][j + 1], 6);
                ctx += ctx;
            }

            c <<= 1;
        }

        s->c2 = s->c1;
        s->c1 = ctx & 255;
    }
}

void decode_bytes(state * s, u8 * c, s32 size) {
    for(s32 i = 0; i < size; i++) {
        if (s->c1 == s->c2)
            ++s->run;
        else
            s->run = 0;

        const int f = s->run > 2;

        int ctx = 1;

        while (ctx < 256) {
            const int p0 = s->C0[ctx];
            const int p1 = s->C1[s->c1][ctx];
            const int p2 = s->C1[s->c2][ctx];
            const int p = ((p0 + p1) * 7 + p2 + p2) >> 4;

            const int j = p >> 12;
            const int x1 = s->C2[2 * ctx + f][j];
            const int x2 = s->C2[2 * ctx + f][j + 1];
            const int ssep = x1 + (((x2 - x1) * (p & 4095)) >> 12);

            const u32 mid = s->low + (((u64)(s->high - s->low) * (ssep * 3 + p)) >> 18);
            const u8 bit = s->code <= mid;
            if (bit)
                s->high = mid;
            else
                s->low = mid + 1;
            while ((s->low ^ s->high) < (1 << 24)) {
                s->low <<= 8;
                s->high = (s->high << 8) + 255;
                s->code = (s->code << 8) + read_in(s);
            }

            if (bit) {
                s->C0[ctx] = update1(s->C0[ctx], 2);
                s->C1[s->c1][ctx] = update1(s->C1[s->c1][ctx], 4);
                s->C2[2 * ctx + f][j] = update1(s->C2[2 * ctx + f][j], 6);
                s->C2[2 * ctx + f][j + 1] = update1(s->C2[2 * ctx + f][j + 1], 6);
                ctx += ctx + 1;
            } else {
                s->C0[ctx] = update0(s->C0[ctx], 2);
                s->C1[s->c1][ctx] = update0(s->C1[s->c1][ctx], 4);
                s->C2[2 * ctx + f][j] = update0(s->C2[2 * ctx + f][j], 6);
                s->C2[2 * ctx + f][j + 1] = update0(s->C2[2 * ctx + f][j + 1], 6);
                ctx += ctx;
            }
        }

        s->c2 = s->c1;
        c[i] = s->c1 = ctx & 255;
    }
}
