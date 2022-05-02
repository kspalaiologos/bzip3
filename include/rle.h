
#ifndef _RLE_H
#define _RLE_H

#include <stddef.h>
#include <stdint.h>

#include "common.h"

s32 mrlec(u8 * in, s32 inlen, u8 * out);
s32 mrled(u8 * in, u8 * out, s32 outlen);

#endif
