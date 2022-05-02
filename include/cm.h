
#ifndef _CM_H
#define _CM_H

typedef struct {
    uint32_t low, high, code;
    uint16_t C0[256], C1[256][256], C2[2][256][17];
    int32_t c1, c2, run;

    uint8_t *in_queue, *out_queue;
    int64_t input_ptr, output_ptr, input_max;
} state;

static void write_out(state *s, uint8_t c) {
    s->out_queue[s->output_ptr++] = c;
}

static uint8_t read_in(state *s) {
    if (s->input_ptr < s->input_max) return s->in_queue[s->input_ptr++];
    return -1;
}

static void encodebit0(state *s, uint32_t p) {
    s->low += (((uint64_t)(s->high - s->low) * p) >> 18) + 1;
    while ((s->low ^ s->high) < (1 << 24)) {
        write_out(s, s->low >> 24);
        s->low <<= 8;
        s->high = (s->high << 8) | 0xFF;
    }
}

static void encodebit1(state *s, uint32_t p) {
    s->high = s->low + (((uint64_t)(s->high - s->low) * p) >> 18);
    while ((s->low ^ s->high) < (1 << 24)) {
        write_out(s, s->low >> 24);
        s->low <<= 8;
        s->high = (s->high << 8) + 255;
    }
}

static uint8_t decodebit(state *s, uint32_t p) {
    const uint32_t mid = s->low + (((uint64_t)(s->high - s->low) * p) >> 18);
    const uint8_t bit = s->code <= mid;
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

static void flush(state *s) {
    write_out(s, s->low >> 24); s->low <<= 8;
    write_out(s, s->low >> 24); s->low <<= 8;
    write_out(s, s->low >> 24); s->low <<= 8;
    write_out(s, s->low >> 24); s->low <<= 8;
}

static void init(state *s) {
    s->code = (s->code << 8) + read_in(s);
    s->code = (s->code << 8) + read_in(s);
    s->code = (s->code << 8) + read_in(s);
    s->code = (s->code << 8) + read_in(s);
}

#define update0(p, x) ((p) - ((p) >> x))
#define update1(p, x) ((p) + (((p) ^ 65535) >> x))

static void begin(state * s) {
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

static void encode_bit(state *s, uint8_t c) {
    if (s->c1 == s->c2)
        ++s->run;
    else
        s->run = 0;

    const int f = s->run > 1;

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

static uint8_t decode_bit(state *s) {
    if (s->c1 == s->c2)
        ++s->run;
    else
        s->run = 0;

    const int f = s->run > 1;

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

#endif
