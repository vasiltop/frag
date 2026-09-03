#include "game.h"
#include "../gpu.h"

namespace game {

priv void Movement(State *state, f32 dt) {
  glm::vec3 front;
  front.x = cos(state->cam_pitch) * sin(state->cam_yaw);
  front.y = sin(state->cam_pitch);
  front.z = cos(state->cam_pitch) * cos(state->cam_yaw);
  front = glm::normalize(front);

  glm::vec3 right =
      glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

  auto *keys = SDL_GetKeyboardState(NULL);
  f32 velocity = 2.5f * dt;

  if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
    state->cam_pos += front * velocity;
  if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
    state->cam_pos -= front * velocity;
  if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])
    state->cam_pos -= right * velocity;
  if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
    state->cam_pos += right * velocity;

  state->view_mat = glm::lookAt(state->cam_pos, state->cam_pos + front,
                                glm::vec3(0.0f, 1.0f, 0.0f));
}

thing::Ref avocado_ref;

void Init(State *state) {
  auto *model = mem::Push<asset::Model>(state->perm_arena);
  asset::LoadGlb(state->perm_arena, state->renderer->device,
                 Str8Lit("./assets/Avocado.glb"), model);

  avocado_ref = thing::Add(state->things, thing::Kind::Cube);
  auto &avocado = thing::Get(state->things, avocado_ref);
  avocado.model = model;
  avocado.scale = glm::vec3(50.0f);
}

void Update(State *state, f32 dt) {
  Movement(state, dt);

  auto &av = thing::Get(state->things, avocado_ref);
  av.rot.y += 1.0f * dt;
}

void HandleEvent(State *state, SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      SDL_SetWindowRelativeMouseMode(state->renderer->window, true);
      state->mouse_captured = true;
    }
    break;

  case SDL_EVENT_KEY_DOWN:
    if (event->key.key == SDLK_ESCAPE) {
      SDL_SetWindowRelativeMouseMode(state->renderer->window, false);
      state->mouse_captured = false;
    }
    break;
  case SDL_EVENT_MOUSE_MOTION:
    if (state->mouse_captured) {
      float sensitivity = 0.003f;

      state->cam_yaw -= event->motion.xrel * sensitivity;
      state->cam_pitch -=
          event->motion.yrel * sensitivity; // Inverted Y so up looks up

      state->cam_pitch = glm::clamp(state->cam_pitch, -1.57f, 1.57f);
    }
    break;
  }
}

}; // namespace game
