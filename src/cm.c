
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

#define update0(p, x) (p) = ((p) - ((p) >> x))
#define update1(p, x) (p) = ((p) + (((p) ^ 65535) >> x))

void begin(state * s) {
    prefetch(s);
    for (int i = 0; i < 256; i++) s->C0[i] = 1 << 15;
    for (int i = 0; i < 256; i++)
        for (int j = 0; j < 256; j++) s->C1[i][j] = 1 << 15;
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 256; j++)
            for (int k = 0; k < 17; k++) s->C2[2 * j + i][k] = (k << 12) - (k == 16);  // Firm difference from stdpack.
}

void encode_bytes(state * s, u8 * buf, s32 size) {
    u32 low = 0, high = 0xFFFFFFFF, code = 0;
    s32 c1 = 0, c2 = 0, run = 0;
    
    for (s32 i = 0; i < size; i++) {
        u8 c = buf[i];

        if (c1 == c2)
            ++run;
        else
            run = 0;

        const int f = run > 2;

        int ctx = 1;

        while (ctx < 256) {
            const int p0 = s->C0[ctx];
            const int p1 = s->C1[c1][ctx];
            const int p2 = s->C1[c2][ctx];
            const int p = ((p0 + p1) * 7 + p2 + p2) >> 4;

            const int j = p >> 12;
            const int x1 = s->C2[2 * ctx + f][j];
            const int x2 = s->C2[2 * ctx + f][j + 1];
            const int ssep = x1 + (((x2 - x1) * (p & 4095)) >> 12);

            if (c & 128) {
                high = low + (((u64)(high - low) * (ssep * 3 + p)) >> 18);

                while ((low ^ high) < (1 << 24)) {
                    write_out(s, low >> 24);
                    low <<= 8;
                    high = (high << 8) + 0xFF;
                }

                update1(s->C0[ctx], 2);
                update1(s->C1[c1][ctx], 4);
                update1(s->C2[2 * ctx + f][j], 6);
                update1(s->C2[2 * ctx + f][j + 1], 6);
                ctx += ctx + 1;
            } else {
                low += (((u64)(high - low) * (ssep * 3 + p)) >> 18) + 1;

                // Write identical bits.
                while ((low ^ high) < (1 << 24)) {
                    write_out(s, low >> 24);  // Same as high >> 24
                    low <<= 8;
                    high = (high << 8) + 0xFF;
                }

                update0(s->C0[ctx], 2);
                update0(s->C1[c1][ctx], 4);
                update0(s->C2[2 * ctx + f][j], 6);
                update0(s->C2[2 * ctx + f][j + 1], 6);
                ctx += ctx;
            }

            c <<= 1;
        }

        c2 = c1;
        c1 = ctx & 255;
    }

    write_out(s, low >> 24);
    low <<= 8;
    write_out(s, low >> 24);
    low <<= 8;
    write_out(s, low >> 24);
    low <<= 8;
    write_out(s, low >> 24);
    low <<= 8;
}

void decode_bytes(state * s, u8 * c, s32 size) {
    u32 low = 0, high = 0xFFFFFFFF, code = 0;
    s32 c1 = 0, c2 = 0, run = 0;

    code = (code << 8) + read_in(s);
    code = (code << 8) + read_in(s);
    code = (code << 8) + read_in(s);
    code = (code << 8) + read_in(s);

    for (s32 i = 0; i < size; i++) {
        if (c1 == c2)
            ++run;
        else
            run = 0;

        const int f = run > 2;

        int ctx = 1;

        while (ctx < 256) {
            const int p0 = s->C0[ctx];
            const int p1 = s->C1[c1][ctx];
            const int p2 = s->C1[c2][ctx];
            const int p = ((p0 + p1) * 7 + p2 + p2) >> 4;

            const int j = p >> 12;
            const int x1 = s->C2[2 * ctx + f][j];
            const int x2 = s->C2[2 * ctx + f][j + 1];
            const int ssep = x1 + (((x2 - x1) * (p & 4095)) >> 12);

            const u32 mid = low + (((u64)(high - low) * (ssep * 3 + p)) >> 18);
            const u8 bit = code <= mid;
            if (bit)
                high = mid;
            else
                low = mid + 1;
            while ((low ^ high) < (1 << 24)) {
                low <<= 8;
                high = (high << 8) + 255;
                code = (code << 8) + read_in(s);
            }

            if (bit) {
                update1(s->C0[ctx], 2);
                update1(s->C1[c1][ctx], 4);
                update1(s->C2[2 * ctx + f][j], 6);
                update1(s->C2[2 * ctx + f][j + 1], 6);
                ctx += ctx + 1;
            } else {
                update0(s->C0[ctx], 2);
                update0(s->C1[c1][ctx], 4);
                update0(s->C2[2 * ctx + f][j], 6);
                update0(s->C2[2 * ctx + f][j + 1], 6);
                ctx += ctx;
            }
        }

        c2 = c1;
        c[i] = c1 = ctx & 255;
    }
}
