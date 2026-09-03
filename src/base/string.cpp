#include "string.h"
#include <SDL3/SDL.h>
#include <string.h>

String8 Str8C(const char *cstr) {
  if (!cstr)
    return String8{nullptr, 0};

  return String8{(u8 *)cstr, (s32)strlen(cstr)};
}

String8 Str8Cat(Arena *arena, String8 a, String8 b) {
  auto *dst = PushCount<u8>(arena, a.size + b.size + 1);
  if (a.size)
    SDL_memcpy(dst, a.str, a.size);
  if (b.size)
    SDL_memcpy(dst + a.size, b.str, b.size);

  dst[a.size + b.size] = 0;
  return String8{dst, a.size + b.size};
}

u64 FindFirstChar(String8 s, u8 c, u64 start) {
  for (u64 i = start; i < s.size; i += 1) {
    if (s.str[i] == c)
      return i;
  }

  return s.size;
}
