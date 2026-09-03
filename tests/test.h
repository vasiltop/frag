#pragma once

#include <stdio.h>

inline int g_test_failures = 0;
inline int g_test_count = 0;
inline int g_failed_tests = 0;

#define TEST(name) void name()

#define RUN_TEST(name)                                                         \
  do {                                                                         \
    g_test_count++;                                                            \
    int failures_before = g_test_failures;                                     \
    name();                                                                    \
    if (g_test_failures == failures_before) {                                  \
      printf("  %s\n", #name);                                                 \
    } else {                                                                   \
      g_failed_tests++;                                                        \
      printf("  %s FAILED\n", #name);                                          \
    }                                                                          \
  } while (0)

#define EXPECT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      g_test_failures++;                                                       \
    }                                                                          \
  } while (0)
