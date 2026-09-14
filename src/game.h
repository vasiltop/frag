#pragma once

#include "base/mem.h"
#include "renderer.h"
#include "thing.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace game {

priv constexpr f32 MAX_GROUND_SPEED = 3.2;
priv constexpr f32 MAX_GROUND_ACCEL = MAX_GROUND_SPEED * 8.0;
priv constexpr f32 MAX_AIR_SPEED = 0.4;
priv constexpr f32 MAX_AIR_ACCEL = 200.0;
priv constexpr f32 MAX_SLOPE = 1.0;
priv constexpr f32 JUMP_FORCE = 2.7;
priv constexpr f32 GRAVITY = 8.0;
priv constexpr f32 GROUNDED_HEIGHT = 0.01;
priv constexpr f32 FRICTION = 2.0;
priv constexpr f32 VIEW_HEIGHT = 0.22;
priv constexpr f32 FOV = 90.0f;
priv constexpr f32 BULLET_SPEED = 8.0f;
priv constexpr f32 BULLET_LIFETIME = 4.0f;
priv constexpr f32 BULLET_LENGTH = 0.12f;
priv constexpr f32 BULLET_SPAWN_OFFSET = 0.4f;
priv constexpr f32 PLAYER_FIRE_INTERVAL = 0.35f;
priv constexpr f32 ENEMY_FIRE_INTERVAL = 1.2f;

struct State {
  Arena *perm_arena;
  frag::Renderer *renderer;
  frag::Things *things;

  u64 last_tick;
  glm::mat4 proj_mat;
  glm::mat4 view_mat;

  b32 mouse_captured;
  glm::vec3 cam_pos;
  f32 cam_pitch;
  f32 cam_yaw;
  frag::MapRefs map_refs;

  frag::Model *enemy_model;
  frag::Model *bullet_model;
  glm::vec3 bullet_scale;
  Array<frag::AABB> bullet_colliders;
  f32 player_fire_cd;
};

void Init(State *state);
void Update(State *state, f32 dt);
void HandleEvent(State *state, SDL_Event *event);

} // namespace game
