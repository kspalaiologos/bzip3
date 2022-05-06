
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

static inline void write_out(state * s, u8 c) { s->out_queue[s->output_ptr++] = c; }

static inline u8 read_in(state * s) {
    if (s->input_ptr < s->input_max) return s->in_queue[s->input_ptr++];
    return -1;
}

// Encode a zero bit with given probability.
static inline void encodebit0(state * s, u32 p) {
    s->low += (((u64)(s->high - s->low) * p) >> 18) + 1;

    // Write identical bits.
    while ((s->low ^ s->high) < (1 << 24)) {
        write_out(s, s->low >> 24);  // Same as s->high >> 24
        s->low <<= 8;
        s->high = (s->high << 8) | 0xFF;
    }
}

// Encode an one bit with given probability.
static inline void encodebit1(state * s, u32 p) {
    s->high = s->low + (((u64)(s->high - s->low) * p) >> 18);
    while ((s->low ^ s->high) < (1 << 24)) {
        write_out(s, s->low >> 24);
        s->low <<= 8;
        s->high = (s->high << 8) + 255;
    }
}

static inline u8 decodebit(state * s, u32 p) {
    // Split the range.
    const u32 mid = s->low + (((u64)(s->high - s->low) * p) >> 18);
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
    return bit;
}

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
            for (int k = 0; k < 17; k++) s->C2[i][j][k] = (k << 12) - (k == 16);
}

void encode_byte(state * s, u8 c) {
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
        const int x1 = s->C2[f][ctx][j];
        const int x2 = s->C2[f][ctx][j + 1];
        const int ssep = x1 + (((x2 - x1) * (p & 4095)) >> 12);

        if (c & 128) {
            encodebit1(s, ssep * 3 + p);
            s->C0[ctx] = update1(s->C0[ctx], 2);
            s->C1[s->c1][ctx] = update1(s->C1[s->c1][ctx], 4);
            s->C2[f][ctx][j] = update1(s->C2[f][ctx][j], 6);
            s->C2[f][ctx][j + 1] = update1(s->C2[f][ctx][j + 1], 6);
            ctx += ctx + 1;
        } else {
            encodebit0(s, ssep * 3 + p);
            s->C0[ctx] = update0(s->C0[ctx], 2);
            s->C1[s->c1][ctx] = update0(s->C1[s->c1][ctx], 4);
            s->C2[f][ctx][j] = update0(s->C2[f][ctx][j], 6);
            s->C2[f][ctx][j + 1] = update0(s->C2[f][ctx][j + 1], 6);
            ctx += ctx;
        }

        c <<= 1;
    }

    s->c2 = s->c1;
    s->c1 = ctx & 255;
}

u8 decode_byte(state * s) {
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
        const int x1 = s->C2[f][ctx][j];
        const int x2 = s->C2[f][ctx][j + 1];
        const int ssep = x1 + (((x2 - x1) * (p & 4095)) >> 12);

        const int bit = decodebit(s, ssep * 3 + p);

        if (bit) {
            s->C0[ctx] = update1(s->C0[ctx], 2);
            s->C1[s->c1][ctx] = update1(s->C1[s->c1][ctx], 4);
            s->C2[f][ctx][j] = update1(s->C2[f][ctx][j], 6);
            s->C2[f][ctx][j + 1] = update1(s->C2[f][ctx][j + 1], 6);
            ctx += ctx + 1;
        } else {
            s->C0[ctx] = update0(s->C0[ctx], 2);
            s->C1[s->c1][ctx] = update0(s->C1[s->c1][ctx], 4);
            s->C2[f][ctx][j] = update0(s->C2[f][ctx][j], 6);
            s->C2[f][ctx][j + 1] = update0(s->C2[f][ctx][j + 1], 6);
            ctx += ctx;
        }
    }

    s->c2 = s->c1;
    return s->c1 = ctx & 255;
}
