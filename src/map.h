#pragma once

#include "base/array.h"
#include "physics.h"
#include "base/string.h"
#include <SDL3/SDL.h>
#include <cfloat>
#include <glm/glm.hpp>

namespace frag {

constexpr auto MAX_BRUSHES = 10000;
constexpr auto MAX_ENTITIES = 100;
constexpr auto MAX_ENTITY_KEYS = 32;
constexpr auto MAX_FACES = 64;
constexpr f32 MAP_SCALE = 0.01f;

using MapParser = String8Cursor;

struct Face {
  glm::vec3 a, b, c;
  String8 tex_name;
  s32 u, v;
  s32 tex_rot;
  f32 u_scale;
  f32 v_scale;
};

struct Brush {
  Array<Face> faces;
};

struct EntityKeyValue {
  String8 key;
  String8 value;
};

struct Entity {
  String8 classname;
  Array<EntityKeyValue> properties;
  Array<Brush> brushes;
};

struct Map {
  Array<Entity> entities;
};

glm::vec3 FaceNormal(Face face);
AABB GetBrushAABB(const Brush &brush);
glm::vec3 ParseMapVec3(String8 s);
glm::vec3 ParseMapAngles(String8 s);
glm::vec3 QuakeToEngine(glm::vec3 quake, f32 scale);
AABB QuakeToEngine(AABB quake, f32 scale);
b32 LoadMap(Arena *arena, String8 filename, Map *result);
}; // namespace frag
