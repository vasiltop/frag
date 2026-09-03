#include "test.h"
#include "thing.h"

TEST(thing_init_sets_up_free_list) {
  frag::Things things{};
  frag::Init(&things);

  EXPECT(things.first_free == 1);
  EXPECT(things.used[1] == false);
  EXPECT(things.next_free[1] == 2);
  EXPECT(things.next_free[frag::max_things - 1] == 0);
}

TEST(thing_add_returns_valid_ref) {
  frag::Things things{};
  frag::Init(&things);

  frag::Ref ref = frag::Add(&things);
  EXPECT(ref.idx == 1);
  EXPECT(ref.gen == things.gen[1]);
  EXPECT(things.used[1] == true);
}

TEST(thing_add_defaults_scale) {
  frag::Things things{};
  frag::Init(&things);

  frag::Ref ref = frag::Add(&things);
  frag::Thing &thing = frag::Get(&things, ref);

  EXPECT(thing.scale.x == 1.0f);
  EXPECT(thing.scale.y == 1.0f);
  EXPECT(thing.scale.z == 1.0f);
  EXPECT(thing.model == nullptr);
}

TEST(thing_add_assigns_unique_slots) {
  frag::Things things{};
  frag::Init(&things);

  frag::Ref a = frag::Add(&things);
  frag::Ref b = frag::Add(&things);

  EXPECT(a.idx != b.idx);
  EXPECT(a.idx == 1);
  EXPECT(b.idx == 2);
}

TEST(thing_get_returns_added_thing) {
  frag::Things things{};
  frag::Init(&things);

  frag::Ref ref = frag::Add(&things);
  frag::Get(&things, ref).pos = glm::vec3(1.0f, 2.0f, 3.0f);

  EXPECT(frag::Get(&things, ref).pos.x == 1.0f);
  EXPECT(frag::Get(&things, ref).pos.y == 2.0f);
  EXPECT(frag::Get(&things, ref).pos.z == 3.0f);
}

TEST(thing_rem_frees_slot) {
  frag::Things things{};
  frag::Init(&things);

  frag::Ref ref = frag::Add(&things);
  frag::Rem(&things, ref);

  EXPECT(things.used[ref.idx] == false);
  EXPECT(things.first_free == ref.idx);
}

TEST(thing_rem_invalidates_ref) {
  frag::Things things{};
  frag::Init(&things);

  frag::Ref ref = frag::Add(&things);
  s32 idx = ref.idx;
  s32 gen = ref.gen;

  frag::Rem(&things, ref);

  EXPECT(things.used[idx] == false);
  EXPECT(things.gen[idx] == gen);
}

TEST(thing_reuse_bumps_generation) {
  frag::Things things{};
  frag::Init(&things);

  frag::Ref first = frag::Add(&things);
  s32 idx = first.idx;
  s32 first_gen = first.gen;

  frag::Rem(&things, first);

  frag::Ref second = frag::Add(&things);
  EXPECT(second.idx == idx);
  EXPECT(second.gen == first_gen + 1);
  EXPECT(things.used[idx] == true);
}

void run_thing_tests() {
  printf("thing tests\n");

  RUN_TEST(thing_init_sets_up_free_list);
  RUN_TEST(thing_add_returns_valid_ref);
  RUN_TEST(thing_add_defaults_scale);
  RUN_TEST(thing_add_assigns_unique_slots);
  RUN_TEST(thing_get_returns_added_thing);
  RUN_TEST(thing_rem_frees_slot);
  RUN_TEST(thing_rem_invalidates_ref);
  RUN_TEST(thing_reuse_bumps_generation);
}
