#include "./mem.h"
#include <SDL3/SDL_assert.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace mem {
u64 PageSize() {
#if defined(_WIN32)
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return (u64)info.dwPageSize;
#else
  static u64 cached = (u64)sysconf(_SC_PAGESIZE);
  return cached;
#endif
}

u8 *Reserve(u64 size) {
#if defined(_WIN32)
  return (u8 *)VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
#else
  void *p = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return (p == MAP_FAILED) ? nullptr : (u8 *)p;
#endif
}

bool Commit(u8 *base, u64 size) {
#if defined(_WIN32)
  return VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE) != nullptr;
#else
  return mprotect(base, size, PROT_READ | PROT_WRITE) == 0;
#endif
}

void Release(u8 *base, u64 size) {
#if defined(_WIN32)
  (void)size;
  VirtualFree(base, 0, MEM_RELEASE);
#else
  munmap(base, size);
#endif
}

Arena *ArenaAlloc(u64 reserve_size) {
  u64 page_size = PageSize();
  u64 reserve_amount =
      AlignPow2(Max(reserve_size, arena_commit_size), page_size);
  u8 *base = Reserve(reserve_amount);

  if (!base)
    return nullptr;

  u64 initial_commit =
      AlignPow2(Max(arena_header_size, arena_commit_size), page_size);
  if (initial_commit > reserve_size)
    initial_commit = reserve_size;

  if (!Commit(base, initial_commit)) {
    Release(base, reserve_size);
    return nullptr;
  }

  Arena *arena = (Arena *)base;
  arena->base = base;
  arena->capacity = reserve_size;
  arena->committed = initial_commit;
  arena->pos = arena_header_size;

  return arena;
}

void Release(Arena *arena) {
  if (!arena)
    return;
  Release(arena->base, arena->capacity);
}

void *Push(Arena *arena, u64 size, u64 align) {
  SDL_assert(arena);
  SDL_assert(align > 0 && (align & (align - 1)) == 0);

  u64 start = AlignPow2(arena->pos, align);
  u64 end = start + size;

  if (end > arena->capacity) {
    SDL_assert(!"arena out of reserved space");
    return nullptr;
  }

  if (end > arena->committed) {
    u64 page = PageSize();
    u64 target = AlignPow2(end, Max(arena_commit_size, page));
    if (target > arena->capacity)
      target = arena->capacity;
    if (!Commit(arena->base + arena->committed, target - arena->committed)) {
      SDL_assert(!"arena commit failed");
      return nullptr;
    }
    arena->committed = target;
  }

  arena->pos = end;
  return arena->base + start;
}

void *PushZero(Arena *arena, u64 size, u64 align) {
  void *p = Push(arena, size, align);
  if (p)
    memset(p, 0, size);
  return p;
}

void PopTo(Arena *arena, u64 pos) {
  SDL_assert(arena);
  u64 clamped = Max(pos, arena_header_size);
  if (clamped < arena->pos)
    arena->pos = clamped;
}

void Pop(Arena *arena, u64 amount) {
  SDL_assert(arena);
  u64 pos = (amount < arena->pos) ? arena->pos - amount : arena_header_size;
  PopTo(arena, pos);
}

void ArenaClear(Arena *arena) { PopTo(arena, arena_header_size); }

TempArena Scratch(Arena **conflicts, u64 conflict_count) {
  static thread_local Arena *pool[4] = {};

  for (u64 i = 0; i < ArrayCount(pool); i += 1) {
    if (!pool[i])
      pool[i] = ArenaAlloc(MB(256));

    bool conflicted = false;
    for (u64 j = 0; j < conflict_count; j += 1) {
      if (conflicts[j] == pool[i]) {
        conflicted = true;
        break;
      }
    }
    if (!conflicted)
      return TempArena{pool[i]};
  }

  SDL_assert(!"no non-conflicting scratch arena available");
  return TempArena{nullptr};
}

}; // namespace mem
