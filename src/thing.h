#pragma once

#include "./base/base.h"
#include "asset.h"
#include <glm/glm.hpp>

namespace frag {

constexpr auto max_things = 1024;

struct Ref {
  s32 idx;
  s32 gen;
};

enum class ThingKind : u8 { None, Map, Player };

struct Thing {
  ThingKind kind;
  glm::vec3 pos;
  glm::vec3 rot;
  glm::vec3 scale;
  Model *model;
};

struct Things {
  Thing slots[max_things];
  b32 used[max_things];
  s32 gen[max_things];

  s32 first_free;
  s32 next_free[max_things];
};

struct MapRefs {
	Ref map;
	Ref player;
};

void Init(Things *things);
Ref Add(Things *things);
Thing &Get(Things *things, Ref ref);
void Rem(Things *things, Ref ref);
MapRefs PopulateThingsFromMap(Arena *arena, SDL_GPUDevice *device, Things *things, Map *map);

} // namespace frag
