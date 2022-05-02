
#ifndef _CRC32_H
#define _CRC32_H

#include <inttypes.h>
#include <stddef.h>

uint32_t crc32sum(uint32_t crc, uint8_t *buf, size_t size);

#endif
