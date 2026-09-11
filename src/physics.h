#pragma once

#include <glm/glm.hpp>
#include "base/base.h"

namespace frag {

struct AABB {
  glm::vec3 min{FLT_MAX, FLT_MAX, FLT_MAX};
  glm::vec3 max{-FLT_MAX, -FLT_MAX, -FLT_MAX};
};

b32 Collision(AABB a, AABB b);

};
