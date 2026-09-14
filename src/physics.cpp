#include "physics.h"
#include <cmath>

namespace frag {

b32 Collision(AABB a, AABB b) {
  return a.min.x <= b.max.x && a.max.x >= b.min.x &&
         a.min.y <= b.max.y && a.max.y >= b.min.y &&
         a.min.z <= b.max.z && a.max.z >= b.min.z;
}

b32 RayHitAABB(glm::vec3 origin, glm::vec3 dir, AABB box, f32 *t_hit) {
  f32 tmin = 0.f;
  f32 tmax = FLT_MAX;

  for (s32 i = 0; i < 3; i++) {
    if (fabsf(dir[i]) < 1e-8f) {
      if (origin[i] < box.min[i] || origin[i] > box.max[i])
        return false;
      continue;
    }

    f32 inv = 1.f / dir[i];
    f32 t0 = (box.min[i] - origin[i]) * inv;
    f32 t1 = (box.max[i] - origin[i]) * inv;
    if (t0 > t1) {
      f32 tmp = t0;
      t0 = t1;
      t1 = tmp;
    }
    tmin = Max(tmin, t0);
    tmax = Min(tmax, t1);
    if (tmin > tmax)
      return false;
  }

  if (t_hit)
    *t_hit = tmin;
  return true;
}

b32 SegmentBlocked(glm::vec3 a, glm::vec3 b, Array<AABB> colliders,
                   glm::vec3 offset) {
  glm::vec3 delta = b - a;
  f32 len2 = glm::dot(delta, delta);
  if (len2 < 1e-12f) {
    for (s32 i = 0; i < colliders.size; i++) {
      AABB box{
          .min = colliders.data[i].min + offset,
          .max = colliders.data[i].max + offset,
      };
      if (a.x >= box.min.x && a.x <= box.max.x && a.y >= box.min.y &&
          a.y <= box.max.y && a.z >= box.min.z && a.z <= box.max.z)
        return true;
    }
    return false;
  }

  f32 len = sqrtf(len2);
  glm::vec3 dir = delta / len;
  for (s32 i = 0; i < colliders.size; i++) {
    AABB box{
        .min = colliders.data[i].min + offset,
        .max = colliders.data[i].max + offset,
    };
    f32 t = 0.f;
    if (RayHitAABB(a, dir, box, &t) && t <= len)
      return true;
  }
  return false;
}

} // namespace frag
