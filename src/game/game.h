#pragma once

#include "../base/mem.h"
#include "../renderer.h"
#include "../thing.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct State {
  mem::Arena *perm_arena;
  renderer::Renderer *renderer;
  thing::Things *things;

  u64 last_tick;
  glm::mat4 proj_mat;
  glm::mat4 view_mat;

  b32 mouse_captured;
  glm::vec3 cam_pos;
  f32 cam_pitch;
  f32 cam_yaw;
};

namespace game {

void Init(State *state);
void Update(State *state, f32 dt);
void HandleEvent(State *state, SDL_Event *event);

}; // namespace game
