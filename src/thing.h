#pragma once

#include "./base/base.h"
#include "asset.h"
#include <glm/glm.hpp>

namespace frag {

constexpr auto MAX_THINGS = 1024;

struct Ref {
  s32 idx;
  s32 gen;
};

enum class ThingKind : u8 { Nil, Map, Player, Enemy, Projectile };

struct Thing {
  ThingKind kind;
  ThingKind owner;
  glm::vec3 pos;
  glm::vec3 rot;
  glm::vec3 vel;
  glm::vec3 scale;
  Array<AABB> colliders;
  Model *model;
  f32 timer;
  b32 alerted;
};

struct Things {
  Thing slots[MAX_THINGS];
  b32 used[MAX_THINGS];
  s32 gen[MAX_THINGS];

  s32 first_free;
  s32 next_free[MAX_THINGS];

  s32 first_used;
  s32 next_used[MAX_THINGS];

  struct Iter {
    Things *things;
    s32 idx;

    Thing &operator*() const { return things->slots[idx]; }
    Iter &operator++() {
      idx = things->next_used[idx];
      return *this;
    }
    bool operator!=(Iter other) const { return idx != other.idx; }
  };

  Iter begin() { return {this, first_used}; }
  Iter end() { return {this, 0}; }
};

struct MapRefs {
	Ref map;
	Ref player;
};

void Init(Things *things);
void Clear(Things *things);
Ref Add(Things *things);
Thing &Get(Things *things, Ref ref);
Ref MakeRef(Things *things, s32 idx);
void Rem(Things *things, Ref ref);
MapRefs PopulateThingsFromMap(Arena *arena, SDL_GPUDevice *device,
                              Things *things, Map *map, Model *enemy_model);
b32 Collision(Thing &a, Thing &b);

} // namespace frag
