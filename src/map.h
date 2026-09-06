#pragma once

#include "base/array.h"
#include "base/string.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace frag {

constexpr auto MAX_BRUSHES = 10000;
constexpr auto MAX_ENTITIES = 100;
constexpr auto MAX_FACES = 64;

using MapParser = String8Cursor;

struct Face {
  glm::vec3 a, b, c;
  String8 tex_name;
  s32 u, v;
  s32 tex_rot;
  s32 u_scale;
  s32 v_scale;
};

struct Brush {
  Array<Face> faces;
};

struct Entity {
  String8 classname;
  Array<Brush> brushes;
};

struct Map {
  Array<Entity> entities;
};

b32 LoadMap(Arena *arena, String8 filename, Map *result);
}; // namespace frag
