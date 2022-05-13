
#ifndef _CM_H
#define _CM_H

#include <inttypes.h>
#include <stdint.h>

#include "common.h"

typedef struct {
    u32 low, high, code;
    s32 c1, c2, run;
    u8 *in_queue, *out_queue;
    s32 input_ptr, output_ptr, input_max;

    u16 C0[256], C1[256][256], C2[512][17];
} state;

void begin(state * s);
void encode_bytes(state * s, u8 * c, s32 size);
void decode_bytes(state * s, u8 * c, s32 size);

#endif
