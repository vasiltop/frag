#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using f32 = float;
using f64 = double;
using b32 = s32;

#define priv static

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Clamp(lo, x, hi) Min(Max(x, lo), hi)

#define KB(n) ((u64)(n) << 10)
#define MB(n) ((u64)(n) << 20)
#define GB(n) ((u64)(n) << 30)

#define AlignPow2(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

template <typename F> struct ScopeGuard {
  F cleanup;

  ScopeGuard(F f) : cleanup(f) {}
  ~ScopeGuard() { cleanup(); }
};

#define DEFER_CONCAT_IMPL(x, y) x##y
#define DEFER_CONCAT(x, y) DEFER_CONCAT_IMPL(x, y)

#define defer const ScopeGuard DEFER_CONCAT(_defer_var_, __LINE__) = [&]()

#if defined(NDEBUG)
#define Assert(condition) ((void)0)
#else
#define Assert(condition)                                                        \
  do {                                                                           \
    if (!(condition)) {                                                          \
      fprintf(stderr, "Assertion failed: %s\n  at %s:%d\n", #condition,         \
              __FILE__, __LINE__);                                               \
      abort();                                                                   \
    }                                                                            \
  } while (0)
#endif
