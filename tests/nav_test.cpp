#include "test.h"
#include "base/mem.h"
#include "nav.h"
#include "physics.h"
#include <cmath>

priv b32 Near(f32 a, f32 b, f32 eps = 0.05f) {
  return std::abs(a - b) < eps;
}

priv Array<frag::AABB> MakeColliders(Arena *arena, frag::AABB *boxes, s32 n) {
  auto colliders = NewArray<frag::AABB>(arena, n);
  for (s32 i = 0; i < n; i++)
    colliders.data[i] = boxes[i];
  return colliders;
}

TEST(nav_path_goes_around_u_wall) {
  auto scratch = Scratch();
  frag::AABB boxes[] = {
      {.min = {-1.f, 0.2f, -1.f}, .max = {-0.6f, 1.f, 1.f}},
      {.min = {0.6f, 0.2f, -1.f}, .max = {1.f, 1.f, 1.f}},
      {.min = {-1.f, 0.2f, -1.f}, .max = {1.f, 1.f, -0.6f}},
  };
  auto colliders = MakeColliders(scratch.arena, boxes, 3);
  frag::NavGrid grid = frag::BuildNavGrid(scratch.arena, colliders);

  glm::vec3 waypoints[128]{};
  s32 n = frag::FindPath(&grid, {0.f, 0.3f, 0.f}, {0.f, 0.3f, -1.5f},
                         waypoints, 128);
  EXPECT(n > 2);

  b32 went_through_wall = false;
  b32 went_out_opening = false;
  for (s32 i = 0; i < n; i++) {
    if (waypoints[i].z < -0.55f && waypoints[i].z > -1.05f &&
        std::abs(waypoints[i].x) < 0.5f)
      went_through_wall = true;
    if (waypoints[i].z > 0.8f)
      went_out_opening = true;
  }
  EXPECT(!went_through_wall);
  EXPECT(went_out_opening);
}

TEST(nav_no_path_through_closed_box) {
  auto scratch = Scratch();
  frag::AABB boxes[] = {
      {.min = {-0.5f, 0.2f, -0.5f}, .max = {-0.3f, 1.f, 0.5f}},
      {.min = {0.3f, 0.2f, -0.5f}, .max = {0.5f, 1.f, 0.5f}},
      {.min = {-0.5f, 0.2f, -0.5f}, .max = {0.5f, 1.f, -0.3f}},
      {.min = {-0.5f, 0.2f, 0.3f}, .max = {0.5f, 1.f, 0.5f}},
  };
  auto colliders = MakeColliders(scratch.arena, boxes, 4);
  frag::NavGrid grid = frag::BuildNavGrid(scratch.arena, colliders);

  glm::vec3 waypoints[64]{};
  s32 n = frag::FindPath(&grid, {0.f, 0.3f, 0.f}, {2.f, 0.3f, 0.f}, waypoints,
                         64);
  EXPECT(n == 0);
}

TEST(nav_straight_path_when_clear) {
  auto scratch = Scratch();
  frag::AABB boxes[] = {
      {.min = {-2.f, -0.1f, -2.f}, .max = {2.f, 0.1f, 2.f}},
  };
  auto colliders = MakeColliders(scratch.arena, boxes, 1);
  frag::NavGrid grid = frag::BuildNavGrid(scratch.arena, colliders);

  glm::vec3 waypoints[64]{};
  s32 n =
      frag::FindPath(&grid, {-1.f, 0.3f, 0.f}, {1.f, 0.3f, 0.f}, waypoints, 64);
  EXPECT(n > 0);
  EXPECT(Near(waypoints[n - 1].x, 1.f, 0.3f));
}

void run_nav_tests() {
  printf("nav tests\n");

  RUN_TEST(nav_path_goes_around_u_wall);
  RUN_TEST(nav_no_path_through_closed_box);
  RUN_TEST(nav_straight_path_when_clear);
}
