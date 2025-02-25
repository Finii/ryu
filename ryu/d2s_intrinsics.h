// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
#ifndef RYU_D2S_INTRINSICS_H
#define RYU_D2S_INTRINSICS_H

#include <assert.h>
#include <stdint.h>

// Defines RYU_32_BIT_PLATFORM if applicable.
#include "ryu/common.h"

// ABSL avoids uint128_t on Win32 even if __SIZEOF_INT128__ is defined.
// Let's do the same for now.
#if defined(__SIZEOF_INT128__) && !defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS)
#define HAS_UINT128
#elif defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS) && defined(_M_X64)
#define HAS_64_BIT_INTRINSICS
#endif

#if defined(HAS_UINT128)
typedef __uint128_t uint128_t;
#endif

#if defined(HAS_64_BIT_INTRINSICS)

#include <intrin.h>

static inline uint64_t umul128(const uint64_t a, const uint64_t b, uint64_t* const productHi) {
  return _umul128(a, b, productHi);
}

static inline uint64_t shiftright128(const uint64_t lo, const uint64_t hi, const uint32_t dist) {
  // For the __shiftright128 intrinsic, the shift value is always
  // modulo 64.
  // In the current implementation of the double-precision version
  // of Ryu, the shift value is always < 64. (In the case
  // RYU_OPTIMIZE_SIZE == 0, the shift value is in the range [49, 58].
  // Otherwise in the range [2, 59].)
  // Check this here in case a future change requires larger shift
  // values. In this case this function needs to be adjusted.
  assert(dist < 64);
  return __shiftright128(lo, hi, (unsigned char) dist);
}

#else // defined(HAS_64_BIT_INTRINSICS)

static inline uint64_t umul128(const uint64_t a, const uint64_t b, uint64_t* const productHi) {
  // The casts here help MSVC to avoid calls to the __allmul library function.
  const uint32_t aLo = (uint32_t)a;
  const uint32_t aHi = (uint32_t)(a >> 32);
  const uint32_t bLo = (uint32_t)b;
  const uint32_t bHi = (uint32_t)(b >> 32);

  const uint64_t b00 = (uint64_t)aLo * bLo;
  const uint64_t b01 = (uint64_t)aLo * bHi;
  const uint64_t b10 = (uint64_t)aHi * bLo;
  const uint64_t b11 = (uint64_t)aHi * bHi;

  const uint32_t b00Lo = (uint32_t)b00;
  const uint32_t b00Hi = (uint32_t)(b00 >> 32);

  const uint64_t mid1 = b10 + b00Hi;
  const uint32_t mid1Lo = (uint32_t)(mid1);
  const uint32_t mid1Hi = (uint32_t)(mid1 >> 32);

  const uint64_t mid2 = b01 + mid1Lo;
  const uint32_t mid2Lo = (uint32_t)(mid2);
  const uint32_t mid2Hi = (uint32_t)(mid2 >> 32);

  const uint64_t pHi = b11 + mid1Hi + mid2Hi;
  const uint64_t pLo = ((uint64_t)mid2Lo << 32) | b00Lo;

  *productHi = pHi;
  return pLo;
}

static inline uint64_t shiftright128(const uint64_t lo, const uint64_t hi, const uint32_t dist) {
  // We don't need to handle the case dist >= 64 here (see above).
  assert(dist < 64);
#if defined(RYU_OPTIMIZE_SIZE) || !defined(RYU_32_BIT_PLATFORM)
  assert(dist > 0);
  return (hi << (64 - dist)) | (lo >> dist);
#else
  // Avoid a 64-bit shift by taking advantage of the range of shift values.
  assert(dist >= 32);
  return (hi << (64 - dist)) | ((uint32_t)(lo >> 32) >> (dist - 32));
#endif
}

#endif // defined(HAS_64_BIT_INTRINSICS)

#if defined(RYU_32_BIT_PLATFORM)

// Returns the high 64 bits of the 128-bit product of a and b.
static inline uint64_t umulh(const uint64_t a, const uint64_t b) {
  // Reuse the umul128 implementation.
  // Optimizers will likely eliminate the instructions used to compute the
  // low part of the product.
  uint64_t hi;
  umul128(a, b, &hi);
  return hi;
}

// On 32-bit platforms, compilers typically generate calls to library
// functions for 64-bit divisions, even if the divisor is a constant.
//
// E.g.:
// https://bugs.llvm.org/show_bug.cgi?id=37932
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=17958
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37443
//
// The functions here perform division-by-constant using multiplications
// in the same way as 64-bit compilers would do.
//
// NB:
// The multipliers and shift values are the ones generated by clang x64
// for expressions like x/5, x/10, etc.

static inline uint64_t div5(const uint64_t x) {
  return umulh(x, 0xCCCCCCCCCCCCCCCDu) >> 2;
}

static inline uint64_t div10(const uint64_t x) {
  return umulh(x, 0xCCCCCCCCCCCCCCCDu) >> 3;
}

static inline uint64_t div100(const uint64_t x) {
  return umulh(x >> 2, 0x28F5C28F5C28F5C3u) >> 2;
}

static inline uint64_t div1e8(const uint64_t x) {
  return umulh(x, 0xABCC77118461CEFDu) >> 26;
}

static inline uint64_t div1e9(const uint64_t x) {
  return umulh(x >> 9, 0x44B82FA09B5A53u) >> 11;
}

static inline uint32_t mod1e9(const uint64_t x) {
  // Avoid 64-bit math as much as possible.
  // Returning (uint32_t) (x - 1000000000 * div1e9(x)) would
  // perform 32x64-bit multiplication and 64-bit subtraction.
  // x and 1000000000 * div1e9(x) are guaranteed to differ by
  // less than 10^9, so their highest 32 bits must be identical,
  // so we can truncate both sides to uint32_t before subtracting.
  // We can also simplify (uint32_t) (1000000000 * div1e9(x)).
  // We can truncate before multiplying instead of after, as multiplying
  // the highest 32 bits of div1e9(x) can't affect the lowest 32 bits.
  return ((uint32_t) x) - 1000000000 * ((uint32_t) div1e9(x));
}

#else // defined(RYU_32_BIT_PLATFORM)

static inline uint64_t div5(const uint64_t x) {
  return x / 5;
}

static inline uint64_t div10(const uint64_t x) {
  return x / 10;
}

static inline uint64_t div100(const uint64_t x) {
  return x / 100;
}

static inline uint64_t div1e8(const uint64_t x) {
  return x / 100000000;
}

static inline uint64_t div1e9(const uint64_t x) {
  return x / 1000000000;
}

static inline uint32_t mod1e9(const uint64_t x) {
  return (uint32_t) (x - 1000000000 * div1e9(x));
}

#endif // defined(RYU_32_BIT_PLATFORM)

static inline uint32_t pow5Factor(uint64_t value) {
  uint32_t count = 0;
  for (;;) {
    assert(value != 0);
    const uint64_t q = div5(value);
    const uint32_t r = ((uint32_t) value) - 5 * ((uint32_t) q);
    if (r != 0) {
      break;
    }
    value = q;
    ++count;
  }
  return count;
}

// Returns true if value is divisible by 5^p.
static inline bool multipleOfPowerOf5(const uint64_t value, const uint32_t p) {
  // I tried a case distinction on p, but there was no performance difference.
  return pow5Factor(value) >= p;
}

// Returns true if value is divisible by 2^p.
static inline bool multipleOfPowerOf2(const uint64_t value, const uint32_t p) {
  assert(value != 0);
  // __builtin_ctzll doesn't appear to be faster here.
  return (value & ((1ull << p) - 1)) == 0;
}

#endif // RYU_D2S_INTRINSICS_H
