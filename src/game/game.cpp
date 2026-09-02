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

gpu::Vertex cube_vertices[] = {
    // Front face (Z = 0.5)
    {-0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // Bottom-Left
    {0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},  // Bottom-Right
    {0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},   // Top-Right
    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},  // Top-Left

    // Back face (Z = -0.5)
    {0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},  // Bottom-Left
    {-0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // Bottom-Right
    {-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},  // Top-Right
    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},   // Top-Left

    // Left face (X = -0.5)
    {-0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // Bottom-Left
    {-0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},  // Bottom-Right
    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},   // Top-Right
    {-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},  // Top-Left

    // Right face (X = 0.5)
    {0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},  // Bottom-Left
    {0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, // Bottom-Right
    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},  // Top-Right
    {0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f},   // Top-Left

    // Top face (Y = 0.5)
    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f},  // Bottom-Left
    {0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},   // Bottom-Right
    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},  // Top-Right
    {-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f}, // Top-Left

    // Bottom face (Y = -0.5)
    {-0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // Bottom-Left
    {0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},  // Bottom-Right
    {0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},   // Top-Right
    {-0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f}   // Top-Left
};

u32 cube_indices[] = { // Front face
    0, 1, 2, 2, 3, 0,
    // Back face
    4, 5, 6, 6, 7, 4,
    // Left face
    8, 9, 10, 10, 11, 8,
    // Right face
    12, 13, 14, 14, 15, 12,
    // Top face
    16, 17, 18, 18, 19, 16,
    // Bottom face
    20, 21, 22, 22, 23, 20};

thing::Ref cube_ref;
thing::Ref cube_ref2;

void Init(State *state) {
  auto texture =
      LoadTexture(state->renderer->device, Str8Lit("./assets/tex.png"));
  auto vb = CreateVertexBuffer(state->renderer->device, cube_vertices);
  auto ib = gpu::CreateIndexBuffer(state->renderer->device, cube_indices);

  cube_ref = thing::Add(state->things, thing::Kind::Cube);
  auto &cube = thing::Get(state->things, cube_ref);
  cube.model = mem::Push<Model>(state->perm_arena);
  cube.model->mesh = mem::Push<Mesh>(state->perm_arena);
  cube.model->texture = texture;
  cube.model->mesh->vertex_buffer = vb;
  cube.model->mesh->index_buffer = ib;
  cube.pos = glm::vec3(1.0, 1.0, 1.0);

  cube_ref2 = thing::Add(state->things, thing::Kind::Cube);
  auto &cube2 = thing::Get(state->things, cube_ref2);
  cube2.model = mem::Push<Model>(state->perm_arena);
  cube2.model->mesh = mem::Push<Mesh>(state->perm_arena);
  cube2.model->texture = texture;
  cube2.model->mesh->vertex_buffer = vb;
  cube2.model->mesh->index_buffer = ib;
  cube2.pos = glm::vec3(-1.0, -1.0, -1.0);
}

void Update(State *state, f32 dt) {
  Movement(state, dt);

  auto &cube = thing::Get(state->things, cube_ref);
  cube.rot.y += 1.0f * dt;

  auto &cube2 = thing::Get(state->things, cube_ref2);
  cube2.rot.y -= 0.5f * dt;
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
