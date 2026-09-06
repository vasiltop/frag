#include "string.h"
#include <string.h>

String8 Str8C(const char *cstr) {
  if (!cstr)
    return String8{nullptr, 0};

  return String8{(u8 *)cstr, (s32)strlen(cstr)};
}

String8 Copy(Arena *arena, String8 src) {
  if (src.size == 0)
    return {};

  u8 *data = PushCount<u8>(arena, src.size);
  SDL_memcpy(data, src.data, src.size);
  return {data, src.size};
}

b32 Eq(String8 a, String8 b) {
  return a.size == b.size && SDL_memcmp(a.data, b.data, a.size) == 0;
}

u64 FindFirstChar(String8 s, u8 c, u64 start) {
  for (u64 i = start; i < s.size; i += 1) {
    if (s.data[i] == c)
      return i;
  }

  return s.size;
}

b32 Done(String8Cursor *c) { return c->pos >= c->content.size; }

u8 Current(String8Cursor *c) {
  if (Done(c))
    return '\0';
  return At(c->content, c->pos);
}

b32 IsWhitespace(u8 c) {
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

void SkipWhitespace(String8Cursor *c) {
  while (!Done(c) && IsWhitespace(Current(c)))
    c->pos++;
}

b32 Expect(String8Cursor *c, u8 ch) {
  SkipWhitespace(c);
  if (Done(c) || Current(c) != ch)
    return false;

  c->pos++;
  return true;
}

String8 ParseQuoted(Arena *arena, String8Cursor *c) {
  if (!Expect(c, '"'))
    return {};

  s32 start = c->pos;
  while (!Done(c) && Current(c) != '"')
    c->pos++;

  String8 slice = {c->content.data + start, c->pos - start};
  if (!Expect(c, '"'))
    return {};

  return Copy(arena, slice);
}

String8 ParseToken(Arena *arena, String8Cursor *c) {
  SkipWhitespace(c);
  s32 start = c->pos;

  while (!Done(c) && !IsWhitespace(Current(c)))
    c->pos++;

  return Copy(arena, {c->content.data + start, c->pos - start});
}
