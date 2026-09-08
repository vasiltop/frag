#include "game.h"
#include "map.h"

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

priv frag::Model *LoadModel(State *state, Arena *arena, String8 path) {
  auto *model = Push<frag::Model>(arena);
  frag::LoadGlb(state->perm_arena, state->renderer->device, path, model);
  return model;
}

frag::Ref map_ref;

void Init(State *state) {
  state->cam_pos = glm::vec3(0.0f, 0.0f, 3.0f);
  state->cam_pitch = 0.0f;
  state->cam_yaw = 3.14159265f;

  auto scratch = Scratch();

  auto map_path = Str8Lit("./assets/maps/test_map.map");
  frag::Map parsed_map;
  LoadMap(scratch.arena, map_path, &parsed_map);

  map_ref = Add(state->things);
  auto &map = Get(state->things, map_ref);

  map.model = Push<frag::Model>(state->perm_arena);
  map.scale = glm::vec3(0.05f, 0.05f, 0.05f);
  BuildMapModel(state->perm_arena, state->renderer->device, &parsed_map,
                map.model);
}

void Update(State *state, f32 dt) { Movement(state, dt); }

void HandleEvent(State *state, SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    if (event->window.windowID == SDL_GetWindowID(state->renderer->window)) {
      u32 w = (u32)event->window.data1;
      u32 h = (u32)event->window.data2;
      if (frag::Resize(state->renderer, w, h)) {
        state->proj_mat = glm::perspectiveRH_ZO(glm::radians(45.0f),
                                                (f32)w / (f32)h, 0.1f, 100.f);
      }
    }
    break;

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

} // namespace game
