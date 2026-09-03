#include "base/mem.h"
#include "test.h"

TEST(arena_alloc_initializes) {
  Arena *arena = ArenaAlloc(MB(1));
  EXPECT(arena != nullptr);
  EXPECT(arena->base != nullptr);
  EXPECT(arena->capacity >= MB(1));
  EXPECT(arena->committed >= arena_header_size);
  EXPECT(arena->pos == arena_header_size);
  Release(arena);
}

TEST(push_advances_pos) {
  Arena *arena = ArenaAlloc(MB(1));
  u64 pos_before = arena->pos;

  void *mem = Push(arena, 64);
  EXPECT(mem != nullptr);
  EXPECT(arena->pos > pos_before);
  EXPECT((u8 *)mem >= arena->base + pos_before);

  Release(arena);
}

TEST(push_is_aligned) {
  Arena *arena = ArenaAlloc(MB(1));

  void *a = Push(arena, 1, 64);
  void *b = Push(arena, 1, 64);
  EXPECT(((uintptr_t)a & 63) == 0);
  EXPECT(((uintptr_t)b & 63) == 0);
  EXPECT(a != b);

  Release(arena);
}

TEST(push_zero_clears_memory) {
  Arena *arena = ArenaAlloc(MB(1));

  u8 *mem = (u8 *)PushZero(arena, 32);
  for (u32 i = 0; i < 32; ++i) {
    EXPECT(mem[i] == 0);
  }

  Release(arena);
}

TEST(pop_to_restores_pos) {
  Arena *arena = ArenaAlloc(MB(1));
  u64 mark = arena->pos;

  Push(arena, 128);
  EXPECT(arena->pos > mark);

  PopTo(arena, mark);
  EXPECT(arena->pos == mark);

  Release(arena);
}

TEST(pop_reverts_by_amount) {
  Arena *arena = ArenaAlloc(MB(1));
  Push(arena, 128);
  u64 pos = arena->pos;

  Pop(arena, 64);
  EXPECT(arena->pos == pos - 64);

  Release(arena);
}

TEST(clear_resets_to_header) {
  Arena *arena = ArenaAlloc(MB(1));
  Push(arena, 256);
  EXPECT(arena->pos > arena_header_size);

  Clear(arena);
  EXPECT(arena->pos == arena_header_size);

  Release(arena);
}

TEST(push_count_allocates_elements) {
  Arena *arena = ArenaAlloc(MB(1));

  u32 *values = PushCount<u32>(arena, 4);
  values[0] = 1;
  values[3] = 4;
  EXPECT(values[0] == 1);
  EXPECT(values[3] == 4);

  Release(arena);
}

TEST(push_constructs_object) {
  Arena *arena = ArenaAlloc(MB(1));

  struct Payload {
    s32 x;
    s32 y;
  };

  Payload *payload = Push<Payload>(arena, Payload{.x = 3, .y = 7});
  EXPECT(payload->x == 3);
  EXPECT(payload->y == 7);

  Release(arena);
}

TEST(temp_arena_restores_on_scope_exit) {
  Arena *arena = ArenaAlloc(MB(1));
  Push(arena, 32);
  u64 mark = arena->pos;

  {
    TempArena temp(arena);
    Push(arena, 64);
    EXPECT(arena->pos > mark);
  }

  EXPECT(arena->pos == mark);
  Release(arena);
}

TEST(scratch_skips_conflicting_arena) {
  Arena *arena = ArenaAlloc(MB(1));
  Push(arena, 32);
  u64 mark = arena->pos;

  {
    TempArena temp = Scratch(&arena, 1);
    EXPECT(temp.arena != nullptr);
    EXPECT(temp.arena != arena);
    Push(temp.arena, 64);
  }

  EXPECT(arena->pos == mark);
  Release(arena);
}

void run_arena_tests() {
  printf("arena tests\n");

  RUN_TEST(arena_alloc_initializes);
  RUN_TEST(push_advances_pos);
  RUN_TEST(push_is_aligned);
  RUN_TEST(push_zero_clears_memory);
  RUN_TEST(pop_to_restores_pos);
  RUN_TEST(pop_reverts_by_amount);
  RUN_TEST(clear_resets_to_header);
  RUN_TEST(push_count_allocates_elements);
  RUN_TEST(push_constructs_object);
  RUN_TEST(temp_arena_restores_on_scope_exit);
  RUN_TEST(scratch_skips_conflicting_arena);
}
