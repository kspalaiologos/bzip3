/*--

This file is a part of libsais, a library for linear time suffix array,
longest common prefix array and burrows wheeler transform construction.

   Copyright (c) 2021-2022 Ilya Grebnov <ilya.grebnov@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

Please see the file LICENSE for full copyright information.

--*/

#ifndef LIBSAIS_H
#define LIBSAIS_H

#include "common.h"

/**
 * Creates the libsais context that allows reusing allocated memory with each
 * libsais operation. In multi-threaded environments, use one context per thread
 * for parallel executions.
 * @return the libsais context, NULL otherwise.
 */
void * libsais_create_ctx(void);

/**
 * Destroys the libsass context and free previusly allocated memory.
 * @param ctx The libsais context (can be NULL).
 */
void libsais_free_ctx(void * ctx);

/**
 * Constructs the suffix array of a given string.
 * @param T [0..n-1] The input string.
 * @param SA [0..n-1+fs] The output array of suffixes.
 * @param n The length of the given string.
 * @param fs The extra space available at the end of SA array (0 should be
 * enough for most cases).
 * @param freq [0..255] The output symbol frequency table (can be NULL).
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
s32 libsais(const u8 * T, s32 * SA, s32 n, s32 fs, s32 * freq);

/**
 * Constructs the suffix array of a given integer array.
 * Note, during construction input array will be modified, but restored at the
 * end if no errors occurred.
 * @param T [0..n-1] The input integer array.
 * @param SA [0..n-1+fs] The output array of suffixes.
 * @param n The length of the integer array.
 * @param k The alphabet size of the input integer array.
 * @param fs Extra space available at the end of SA array (can be 0, but 4k or
 * better 6k is recommended for optimal performance).
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_int(s32 * T, s32 * SA, s32 n, s32 k, s32 fs);

/**
 * Constructs the suffix array of a given string using libsais context.
 * @param ctx The libsais context.
 * @param T [0..n-1] The input string.
 * @param SA [0..n-1+fs] The output array of suffixes.
 * @param n The length of the given string.
 * @param fs The extra space available at the end of SA array (0 should be
 * enough for most cases).
 * @param freq [0..255] The output symbol frequency table (can be NULL).
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_ctx(const void * ctx, const u8 * T, s32 * SA, s32 n, s32 fs,
                s32 * freq);

/**
 * Constructs the burrows-wheeler transformed string (BWT) of a given string.
 * @param T [0..n-1] The input string.
 * @param U [0..n-1] The output string (can be T).
 * @param A [0..n-1+fs] The temporary array.
 * @param n The length of the given string.
 * @param fs The extra space available at the end of A array (0 should be enough
 * for most cases).
 * @param freq [0..255] The output symbol frequency table (can be NULL).
 * @return The primary index if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_bwt(const u8 * T, u8 * U, s32 * A, s32 n, s32 fs, s32 * freq);

/**
 * Constructs the burrows-wheeler transformed string (BWT) of a given string
 * with auxiliary indexes.
 * @param T [0..n-1] The input string.
 * @param U [0..n-1] The output string (can be T).
 * @param A [0..n-1+fs] The temporary array.
 * @param n The length of the given string.
 * @param fs The extra space available at the end of A array (0 should be enough
 * for most cases).
 * @param freq [0..255] The output symbol frequency table (can be NULL).
 * @param r The sampling rate for auxiliary indexes (must be power of 2).
 * @param I [0..(n-1)/r] The output auxiliary indexes.
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_bwt_aux(const u8 * T, u8 * U, s32 * A, s32 n, s32 fs, s32 * freq,
                    s32 r, s32 * I);

/**
 * Constructs the burrows-wheeler transformed string (BWT) of a given string
 * using libsais context.
 * @param ctx The libsais context.
 * @param T [0..n-1] The input string.
 * @param U [0..n-1] The output string (can be T).
 * @param A [0..n-1+fs] The temporary array.
 * @param n The length of the given string.
 * @param fs The extra space available at the end of A array (0 should be enough
 * for most cases).
 * @param freq [0..255] The output symbol frequency table (can be NULL).
 * @return The primary index if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_bwt_ctx(const void * ctx, const u8 * T, u8 * U, s32 * A, s32 n,
                    s32 fs, s32 * freq);

/**
 * Constructs the burrows-wheeler transformed string (BWT) of a given string
 * with auxiliary indexes using libsais context.
 * @param ctx The libsais context.
 * @param T [0..n-1] The input string.
 * @param U [0..n-1] The output string (can be T).
 * @param A [0..n-1+fs] The temporary array.
 * @param n The length of the given string.
 * @param fs The extra space available at the end of A array (0 should be enough
 * for most cases).
 * @param freq [0..255] The output symbol frequency table (can be NULL).
 * @param r The sampling rate for auxiliary indexes (must be power of 2).
 * @param I [0..(n-1)/r] The output auxiliary indexes.
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_bwt_aux_ctx(const void * ctx, const u8 * T, u8 * U, s32 * A, s32 n,
                        s32 fs, s32 * freq, s32 r, s32 * I);

/**
 * Creates the libsais reverse BWT context that allows reusing allocated memory
 * with each libsais_unbwt_* operation. In multi-threaded environments, use one
 * context per thread for parallel executions.
 * @return the libsais context, NULL otherwise.
 */
void * libsais_unbwt_create_ctx(void);

/**
 * Destroys the libsass reverse BWT context and free previusly allocated memory.
 * @param ctx The libsais context (can be NULL).
 */
void libsais_unbwt_free_ctx(void * ctx);

/**
 * Constructs the original string from a given burrows-wheeler transformed
 * string (BWT) with primary index.
 * @param T [0..n-1] The input string.
 * @param U [0..n-1] The output string (can be T).
 * @param A [0..n] The temporary array (NOTE, temporary array must be n + 1
 * size).
 * @param n The length of the given string.
 * @param freq [0..255] The input symbol frequency table (can be NULL).
 * @param i The primary index.
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_unbwt(const u8 * T, u8 * U, s32 * A, s32 n, const s32 * freq,
                  s32 i);

/**
 * Constructs the original string from a given burrows-wheeler transformed
 * string (BWT) with primary index using libsais reverse BWT context.
 * @param ctx The libsais reverse BWT context.
 * @param T [0..n-1] The input string.
 * @param U [0..n-1] The output string (can be T).
 * @param A [0..n] The temporary array (NOTE, temporary array must be n + 1
 * size).
 * @param n The length of the given string.
 * @param freq [0..255] The input symbol frequency table (can be NULL).
 * @param i The primary index.
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_unbwt_ctx(const void * ctx, const u8 * T, u8 * U, s32 * A, s32 n,
                      const s32 * freq, s32 i);

/**
 * Constructs the original string from a given burrows-wheeler transformed
 * string (BWT) with auxiliary indexes.
 * @param T [0..n-1] The input string.
 * @param U [0..n-1] The output string (can be T).
 * @param A [0..n] The temporary array (NOTE, temporary array must be n + 1
 * size).
 * @param n The length of the given string.
 * @param freq [0..255] The input symbol frequency table (can be NULL).
 * @param r The sampling rate for auxiliary indexes (must be power of 2).
 * @param I [0..(n-1)/r] The input auxiliary indexes.
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_unbwt_aux(const u8 * T, u8 * U, s32 * A, s32 n, const s32 * freq,
                      s32 r, const s32 * I);

/**
 * Constructs the original string from a given burrows-wheeler transformed
 * string (BWT) with auxiliary indexes using libsais reverse BWT context.
 * @param ctx The libsais reverse BWT context.
 * @param T [0..n-1] The input string.
 * @param U [0..n-1] The output string (can be T).
 * @param A [0..n] The temporary array (NOTE, temporary array must be n + 1
 * size).
 * @param n The length of the given string.
 * @param freq [0..255] The input symbol frequency table (can be NULL).
 * @param r The sampling rate for auxiliary indexes (must be power of 2).
 * @param I [0..(n-1)/r] The input auxiliary indexes.
 * @return 0 if no error occurred, -1 or -2 otherwise.
 */
s32 libsais_unbwt_aux_ctx(const void * ctx, const u8 * T, u8 * U, s32 * A,
                          s32 n, const s32 * freq, s32 r, const s32 * I);

/**
 * Constructs the permuted longest common prefix array (PLCP) of a given string
 * and a suffix array.
 * @param T [0..n-1] The input string.
 * @param SA [0..n-1] The input suffix array.
 * @param PLCP [0..n-1] The output permuted longest common prefix array.
 * @param n The length of the string and the suffix array.
 * @return 0 if no error occurred, -1 otherwise.
 */
s32 libsais_plcp(const u8 * T, const s32 * SA, s32 * PLCP, s32 n);

/**
 * Constructs the longest common prefix array (LCP) of a given permuted longest
 * common prefix array (PLCP) and a suffix array.
 * @param PLCP [0..n-1] The input permuted longest common prefix array.
 * @param SA [0..n-1] The input suffix array.
 * @param LCP [0..n-1] The output longest common prefix array (can be SA).
 * @param n The length of the permuted longest common prefix array and the
 * suffix array.
 * @return 0 if no error occurred, -1 otherwise.
 */
s32 libsais_lcp(const s32 * PLCP, const s32 * SA, s32 * LCP, s32 n);

#endif
