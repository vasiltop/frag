#pragma once

#include "./base.h"
#include "./mem.h"

struct String8 {
  u8 *str;
  s32 size;
};

#define Str8Lit(s) String8{(u8 *)(s), sizeof(s) - 1}
String8 Str8C(const char *cstr);
String8 Str8Cat(mem::Arena *arena, String8 a, String8 b);
u64 FindFirstChar(String8 s, u8 c, u64 start);
