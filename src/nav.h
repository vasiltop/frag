#pragma once

#include "physics.h"

namespace frag {

constexpr f32 NAV_CELL = 0.2f;
constexpr f32 NAV_PROBE_MIN_Y = 0.2f;
constexpr f32 NAV_PROBE_MAX_Y = 0.5f;
constexpr f32 NAV_PROBE_RADIUS = 0.16f;
constexpr s32 NAV_MAX_DIM = 256;

struct NavGrid {
  glm::vec3 origin;
  f32 cell;
  s32 w;
  s32 h;
  u8 *blocked;
};

NavGrid BuildNavGrid(Arena *arena, Array<AABB> colliders,
                     glm::vec3 offset = {});
s32 FindPath(NavGrid *grid, glm::vec3 from, glm::vec3 to, glm::vec3 *out,
             s32 out_cap);

} // namespace frag
