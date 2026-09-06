#pragma once

#include "./array.h"
#include "./base.h"

using String8 = Array<u8>;

struct String8Cursor {
  String8 content;
  s32 pos = 0;
};

#define Str8Lit(s) String8{(u8 *)(s), sizeof(s) - 1}
String8 Str8C(const char *cstr);
String8 Copy(Arena *arena, String8 src);
b32 Eq(String8 a, String8 b);
u64 FindFirstChar(String8 s, u8 c, u64 start);

b32 Done(String8Cursor *c);
u8 Current(String8Cursor *c);
b32 IsWhitespace(u8 c);
void SkipWhitespace(String8Cursor *c);
b32 Expect(String8Cursor *c, u8 ch);
String8 ParseQuoted(Arena *arena, String8Cursor *c);
String8 ParseToken(Arena *arena, String8Cursor *c);
