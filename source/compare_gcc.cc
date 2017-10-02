/*
 *  Copyright 2012 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/basic_types.h"

#include "libyuv/compare_row.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// This module is for GCC x86 and x64.
#if !defined(LIBYUV_DISABLE_X86) && \
    (defined(__x86_64__) || (defined(__i386__) && !defined(_MSC_VER)))

uint32 HammingDistance_X86(const uint8* src_a, const uint8* src_b, int count) {
  uint32 diff = 0u;

  int i;
  for (i = 0; i < count - 7; i += 8) {
    uint64 x = *((uint64*)src_a) ^ *((uint64*)src_b);
    src_a += 8;
    src_b += 8;
    diff += __builtin_popcountll(x);
  }
  return diff;
}

#ifdef HAS_HAMMINGDISTANCE_AVX2
static uint32 kNibbleMask = 0x0f0f0f0fu;
static vec8 kBitCount = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

uint32 HammingDistance_AVX2(const uint8* src_a, const uint8* src_b, int count) {
  uint32 diff = 0u;

  asm volatile(
      "vbroadcastss  %4,%%ymm2                   \n"
      "vbroadcastf128 %5,%%ymm3                  \n"
      "vpxor      %%ymm0,%%ymm0,%%ymm0           \n"
      "vpxor      %%ymm1,%%ymm1,%%ymm1           \n"
      "sub        %0,%1                          \n"

      LABELALIGN
      "1:                                        \n"
      "vmovdqa    (%0),%%ymm4                    \n"
      "vmovdqa    0x20(%0), %%ymm5               \n"
      "vpxor      (%0,%1), %%ymm4, %%ymm4        \n"
      "vpxor      0x20(%0,%1),%%ymm5,%%ymm5      \n"
      "add        $0x40,%0                       \n"
      "vpsrlw     $0x4,%%ymm4,%%ymm6             \n"
      "vpand      %%ymm2,%%ymm4,%%ymm4           \n"
      "vpand      %%ymm2,%%ymm6,%%ymm6           \n"
      "vpshufb    %%ymm4,%%ymm3,%%ymm4           \n"
      "vpshufb    %%ymm6,%%ymm3,%%ymm6           \n"
      "vpaddb     %%ymm4,%%ymm6,%%ymm6           \n"
      "vpsrlw     $0x4,%%ymm5,%%ymm4             \n"
      "vpand      %%ymm2,%%ymm4,%%ymm4           \n"
      "vpand      %%ymm2,%%ymm5,%%ymm5           \n"
      "vpshufb    %%ymm4,%%ymm3,%%ymm4           \n"
      "vpshufb    %%ymm5,%%ymm3,%%ymm5           \n"
      "vpaddb     %%ymm5,%%ymm4,%%ymm4           \n"
      "vpaddb     %%ymm6,%%ymm4,%%ymm4           \n"
      "vpsadbw    %%ymm1,%%ymm4,%%ymm4           \n"
      "vpaddd     %%ymm0,%%ymm4,%%ymm0           \n"
      "sub        $0x40,%2                       \n"
      "jg         1b                             \n"

      "vpermq     $0xb1,%%ymm0,%%ymm1            \n"
      "vpaddd     %%ymm1,%%ymm0,%%ymm0           \n"
      "vpermq     $0xaa,%%ymm0,%%ymm1            \n"
      "vpaddd     %%ymm1,%%ymm0,%%ymm0           \n"
      "vmovd      %%xmm0, %3                     \n"
      "vzeroupper                                \n"
      : "+r"(src_a),       // %0
        "+r"(src_b),       // %1
        "+r"(count),       // %2
        "=g"(diff)         // %3
      : "m"(kNibbleMask),  // %4
        "m"(kBitCount)     // %5
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6");

  return diff;
}
#endif  // HAS_HAMMINGDISTANCE_AVX2

uint32 SumSquareError_SSE2(const uint8* src_a, const uint8* src_b, int count) {
  uint32 sse;
  asm volatile (
    "pxor      %%xmm0,%%xmm0                   \n"
    "pxor      %%xmm5,%%xmm5                   \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm1         \n"
    "lea       " MEMLEA(0x10, 0) ",%0          \n"
    "movdqu    " MEMACCESS(1) ",%%xmm2         \n"
    "lea       " MEMLEA(0x10, 1) ",%1          \n"
    "movdqa    %%xmm1,%%xmm3                   \n"
    "psubusb   %%xmm2,%%xmm1                   \n"
    "psubusb   %%xmm3,%%xmm2                   \n"
    "por       %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm1                   \n"
    "punpckhbw %%xmm5,%%xmm2                   \n"
    "pmaddwd   %%xmm1,%%xmm1                   \n"
    "pmaddwd   %%xmm2,%%xmm2                   \n"
    "paddd     %%xmm1,%%xmm0                   \n"
    "paddd     %%xmm2,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"

    "pshufd    $0xee,%%xmm0,%%xmm1             \n"
    "paddd     %%xmm1,%%xmm0                   \n"
    "pshufd    $0x1,%%xmm0,%%xmm1              \n"
    "paddd     %%xmm1,%%xmm0                   \n"
    "movd      %%xmm0,%3                       \n"

  : "+r"(src_a),      // %0
    "+r"(src_b),      // %1
    "+r"(count),      // %2
    "=g"(sse)         // %3
  :: "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
  );
  return sse;
}

static uvec32 kHash16x33 = {0x92d9e201, 0, 0, 0};  // 33 ^ 16
static uvec32 kHashMul0 = {
    0x0c3525e1,  // 33 ^ 15
    0xa3476dc1,  // 33 ^ 14
    0x3b4039a1,  // 33 ^ 13
    0x4f5f0981,  // 33 ^ 12
};
static uvec32 kHashMul1 = {
    0x30f35d61,  // 33 ^ 11
    0x855cb541,  // 33 ^ 10
    0x040a9121,  // 33 ^ 9
    0x747c7101,  // 33 ^ 8
};
static uvec32 kHashMul2 = {
    0xec41d4e1,  // 33 ^ 7
    0x4cfa3cc1,  // 33 ^ 6
    0x025528a1,  // 33 ^ 5
    0x00121881,  // 33 ^ 4
};
static uvec32 kHashMul3 = {
    0x00008c61,  // 33 ^ 3
    0x00000441,  // 33 ^ 2
    0x00000021,  // 33 ^ 1
    0x00000001,  // 33 ^ 0
};

uint32 HashDjb2_SSE41(const uint8* src, int count, uint32 seed) {
  uint32 hash;
  asm volatile (
    "movd      %2,%%xmm0                       \n"
    "pxor      %%xmm7,%%xmm7                   \n"
    "movdqa    %4,%%xmm6                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm1         \n"
    "lea       " MEMLEA(0x10, 0) ",%0          \n"
    "pmulld    %%xmm6,%%xmm0                   \n"
    "movdqa    %5,%%xmm5                       \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm2,%%xmm3                   \n"
    "punpcklwd %%xmm7,%%xmm3                   \n"
    "pmulld    %%xmm5,%%xmm3                   \n"
    "movdqa    %6,%%xmm5                       \n"
    "movdqa    %%xmm2,%%xmm4                   \n"
    "punpckhwd %%xmm7,%%xmm4                   \n"
    "pmulld    %%xmm5,%%xmm4                   \n"
    "movdqa    %7,%%xmm5                       \n"
    "punpckhbw %%xmm7,%%xmm1                   \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklwd %%xmm7,%%xmm2                   \n"
    "pmulld    %%xmm5,%%xmm2                   \n"
    "movdqa    %8,%%xmm5                       \n"
    "punpckhwd %%xmm7,%%xmm1                   \n"
    "pmulld    %%xmm5,%%xmm1                   \n"
    "paddd     %%xmm4,%%xmm3                   \n"
    "paddd     %%xmm2,%%xmm1                   \n"
    "paddd     %%xmm3,%%xmm1                   \n"
    "pshufd    $0xe,%%xmm1,%%xmm2              \n"
    "paddd     %%xmm2,%%xmm1                   \n"
    "pshufd    $0x1,%%xmm1,%%xmm2              \n"
    "paddd     %%xmm2,%%xmm1                   \n"
    "paddd     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%1                        \n"
    "jg        1b                              \n"
    "movd      %%xmm0,%3                       \n"
  : "+r"(src),        // %0
    "+r"(count),      // %1
    "+rm"(seed),      // %2
    "=g"(hash)        // %3
  : "m"(kHash16x33),  // %4
    "m"(kHashMul0),   // %5
    "m"(kHashMul1),   // %6
    "m"(kHashMul2),   // %7
    "m"(kHashMul3)    // %8
  : "memory", "cc"
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
  );
  return hash;
}
#endif  // defined(__x86_64__) || (defined(__i386__) && !defined(__pic__)))

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif
