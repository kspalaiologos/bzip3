
#ifndef _LZP_H
#define _LZP_H

#include "common.h"

s32 lzp_compress(const u8 * input, u8 * output, s32 n, s32 hash, s32 min);

s32 lzp_decompress(const u8 * input, u8 * output, s32 n, s32 hash, s32 min);

#endif
