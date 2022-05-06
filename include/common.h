
/*
 * BZip3 - A spiritual successor to BZip2.
 * Copyright (C) 2022 Kamila Szewczyk
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _COMMON_H
#define _COMMON_H

#include "../config.h"

#define KiB(x) ((x)*1024)
#define MiB(x) ((x)*1024 * 1024)

#include <inttypes.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

// Supply ntohl and htonl.
#if HAVE_ARPA_INET_H == 1 && HAVE_HTONL == 1 && HAVE_NTOHL == 1
    #include <arpa/inet.h>
#else
    #ifndef WORDS_BIGENDIAN
        #include <string.h>
    #endif

    static u32 ntohl(u32 value) {
        #ifdef WORDS_BIGENDIAN
            return value;
        #else
            u8 data[4];
            memcpy(&data, &value, sizeof(data));

            return ((u32) data[3])
                 | ((u32) data[2] << 8)
                 | ((u32) data[1] << 16)
                 | ((u32) data[0] << 24);
        #endif
    }

    static u32 htonl(u32 value) {
        #ifdef WORDS_BIGENDIAN
            return value;
        #else
            u8 data[4];
            memcpy(&data, &value, sizeof(data));

            return ((u32) data[3])
                 | ((u32) data[2] << 8)
                 | ((u32) data[1] << 16)
                 | ((u32) data[0] << 24);
        #endif
    }
#endif

#endif
