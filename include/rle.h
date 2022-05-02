
#ifndef _RLE_H
#define _RLE_H

#include <stddef.h>
#include <stdint.h>

int32_t mrlec(uint8_t * in, int32_t inlen, uint8_t * out);
int32_t mrled(uint8_t * in, uint8_t * out, int32_t outlen);

#endif
