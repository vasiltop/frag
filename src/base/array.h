#pragma once

#include "./base.h"
#include "./mem.h"
#include <SDL3/SDL.h>

// These two Arrays are non-owning, an arena will own data
template <typename T> struct Array {
  T *data;
  s32 size;
};

template <typename T> struct DynArray {
  T *data;
  s32 size;
  s32 capacity;
};

template <typename T> Array<T> Cat(Arena *arena, Array<T> a, Array<T> b) {
  auto *dst = PushCount<u8>(arena, a.size + b.size + 1);
  if (a.size)
    SDL_memcpy(dst, a.data, a.size);
  if (b.size)
    SDL_memcpy(dst + a.size, b.data, b.size);

  dst[a.size + b.size] = 0;
  return Array{dst, a.size + b.size};
}

template <typename T> T At(Array<T> a, s32 idx) {
  Assert(idx >= 0 && idx < a.size);
  return a.data[idx];
}

template <typename T> Array<T> NewArray(Arena *arena, s32 size) {
  return {
      .data = PushCount<T>(arena, size),
      .size = size,
  };
}

template <typename T>
DynArray<T> NewDynArray(Arena *arena, s32 size = 0, s32 capacity = 0) {
  if (capacity < size)
    capacity = size;

  return {
      .data = PushCount<T>(arena, capacity),
      .size = size,
      .capacity = capacity,
  };
}
