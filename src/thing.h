#pragma once

#include "./base/base.h"
#include "asset.h"
#include <glm/glm.hpp>

namespace thing {

constexpr auto max_things = 1024;

struct Ref {
  s32 idx;
  s32 gen;
};

struct Thing {
  glm::vec3 pos;
  glm::vec3 rot;
  glm::vec3 scale;
  asset::Model *model;
};

struct Things {
  Thing slots[max_things];
  b32 used[max_things];
  s32 gen[max_things];

  s32 first_free;
  s32 next_free[max_things];
};

void Init(Things *things);
Ref Add(Things *things);
Thing &Get(Things *things, Ref ref);
void Rem(Things *things, Ref ref);

}; // namespace thing
