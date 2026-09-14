#include "test.h"
#include "base/mem.h"
#include "physics.h"
#include <cmath>

priv b32 Near(f32 a, f32 b, f32 eps = 0.001f) {
  return std::abs(a - b) < eps;
}

TEST(ray_hit_aabb_misses_beside_box) {
  frag::AABB box{.min = {0.f, 0.f, 0.f}, .max = {1.f, 1.f, 1.f}};
  f32 t = 0.f;
  EXPECT(!frag::RayHitAABB({-1.f, 0.5f, 0.5f}, {0.f, 0.f, 1.f}, box, &t));
}

TEST(ray_hit_aabb_hits_front_face) {
  frag::AABB box{.min = {0.f, 0.f, 0.f}, .max = {1.f, 1.f, 1.f}};
  f32 t = 0.f;
  EXPECT(frag::RayHitAABB({-1.f, 0.5f, 0.5f}, {1.f, 0.f, 0.f}, box, &t));
  EXPECT(Near(t, 1.f));
}

TEST(ray_hit_aabb_origin_inside) {
  frag::AABB box{.min = {0.f, 0.f, 0.f}, .max = {1.f, 1.f, 1.f}};
  f32 t = 1.f;
  EXPECT(frag::RayHitAABB({0.5f, 0.5f, 0.5f}, {1.f, 0.f, 0.f}, box, &t));
  EXPECT(Near(t, 0.f));
}

TEST(segment_blocked_by_wall_box) {
  auto scratch = Scratch();
  auto colliders = NewArray<frag::AABB>(scratch.arena, 1);
  colliders.data[0] = {.min = {-0.5f, -0.5f, -0.5f}, .max = {0.5f, 0.5f, 0.5f}};

  EXPECT(frag::SegmentBlocked({-2.f, 0.f, 0.f}, {2.f, 0.f, 0.f}, colliders));
}

TEST(segment_clear_when_going_around_wall) {
  auto scratch = Scratch();
  auto colliders = NewArray<frag::AABB>(scratch.arena, 1);
  colliders.data[0] = {.min = {-0.5f, -0.5f, -0.5f}, .max = {0.5f, 0.5f, 0.5f}};

  EXPECT(!frag::SegmentBlocked({-2.f, 0.f, 2.f}, {2.f, 0.f, 2.f}, colliders));
}

TEST(segment_blocked_uses_offset) {
  auto scratch = Scratch();
  auto colliders = NewArray<frag::AABB>(scratch.arena, 1);
  colliders.data[0] = {.min = {-0.5f, -0.5f, -0.5f}, .max = {0.5f, 0.5f, 0.5f}};

  EXPECT(!frag::SegmentBlocked({-2.f, 0.f, 0.f}, {2.f, 0.f, 0.f}, colliders,
                               {0.f, 0.f, 4.f}));
  EXPECT(frag::SegmentBlocked({-2.f, 0.f, 4.f}, {2.f, 0.f, 4.f}, colliders,
                              {0.f, 0.f, 4.f}));
}

void run_physics_tests() {
  printf("physics tests\n");

  RUN_TEST(ray_hit_aabb_misses_beside_box);
  RUN_TEST(ray_hit_aabb_hits_front_face);
  RUN_TEST(ray_hit_aabb_origin_inside);
  RUN_TEST(segment_blocked_by_wall_box);
  RUN_TEST(segment_clear_when_going_around_wall);
  RUN_TEST(segment_blocked_uses_offset);
}
