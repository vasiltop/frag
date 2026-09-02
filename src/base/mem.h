#pragma once

#include "./base.h"
#include <new>
#include <utility>

namespace mem {

struct Arena {
  u8 *base;
  u64 capacity;
  u64 committed;
  u64 pos;
};

constexpr u64 arena_header_size = 128;
constexpr u64 arena_commit_size = KB(64);

Arena *ArenaAlloc(u64 reserve_size = GB(1));
void Release(Arena *arena);
void *Push(Arena *arena, u64 size, u64 align = 16);
void *PushZero(Arena *arena, u64 size, u64 align = 16);
void PopTo(Arena *arena, u64 pos);
void Pop(Arena *arena, u64 amount);
void Clear(Arena *arena);

template <typename T> T *PushCount(Arena *arena, u64 count) {
  T *res = (T *)Push(arena, sizeof(T) * count);
  return res;
}

template <typename T, typename... Args> T *Push(Arena *arena, Args &&...args) {
  void *res = Push(arena, sizeof(T), alignof(T));
  return new (res) T{std::forward<Args>(args)...};
}

struct TempArena {
  Arena *arena;
  u64 pos;

  TempArena(Arena *a) : arena(a) {
    if (a) {
      pos = a->pos;
    }
  }

  ~TempArena() { PopTo(arena, pos); }
};

TempArena Scratch(Arena **conflicts = nullptr, u64 conflict_count = 0);

}; // namespace mem
