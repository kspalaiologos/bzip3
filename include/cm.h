
#ifndef _CM_H
#define _CM_H

#include <inttypes.h>
#include <stdint.h>

typedef struct {
    uint32_t low, high, code;
    uint16_t C0[256], C1[256][256], C2[2][256][17];
    int32_t c1, c2, run;

    uint8_t *in_queue, *out_queue;
    int64_t input_ptr, output_ptr, input_max;
} state;

void flush(state * s);
void init(state * s);
void begin(state * s);
void encode_byte(state * s, uint8_t c);
uint8_t decode_byte(state * s);

#endif
