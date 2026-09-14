#pragma once

#include "base/array.h"
#include "base/base.h"
#include <cfloat>
#include <glm/glm.hpp>

namespace frag {

struct AABB {
  glm::vec3 min{FLT_MAX, FLT_MAX, FLT_MAX};
  glm::vec3 max{-FLT_MAX, -FLT_MAX, -FLT_MAX};
};

b32 Collision(AABB a, AABB b);
b32 RayHitAABB(glm::vec3 origin, glm::vec3 dir, AABB box, f32 *t_hit);
b32 SegmentBlocked(glm::vec3 a, glm::vec3 b, Array<AABB> colliders,
                   glm::vec3 offset = {});

} // namespace frag
