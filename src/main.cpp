#include "engine.h"
#define SDL_MAIN_USE_CALLBACKS
#include "base/mem.h"
#include <SDL3/SDL_main.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr auto width = 1280;
constexpr auto height = 720;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_Log("Init");

  auto perm_arena = mem::ArenaAlloc();
  auto state = mem::Push<State>(perm_arena, State{.perm_arena = perm_arena});

  *appstate = state;

  state->window = SDL_CreateWindow("frag", width, height, 0);

  if (!state->window) {
    SDL_Log("Failed to create window: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUShaderFormat formatFlags = SDL_GPU_SHADERFORMAT_SPIRV |
                                    SDL_GPU_SHADERFORMAT_DXIL |
                                    SDL_GPU_SHADERFORMAT_MSL;

  state->device = SDL_CreateGPUDevice(formatFlags, true, nullptr);
  if (!state->device) {
    SDL_Log("Couldn't create GPU device: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_ClaimWindowForGPUDevice(state->device, state->window)) {
    SDL_Log("Couldn't claim window for GPU device: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!CreatePipeline(state)) {
    return SDL_APP_FAILURE;
  }

  Vertex cube_vertices[] = {
      // Front face
      {-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f}, // 0
      {0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},  // 1
      {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f},   // 2
      {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f},  // 3
      // Back face
      {-0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f}, // 4
      {0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f},  // 5
      {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f},   // 6
      {-0.5f, 0.5f, -0.5f, 0.2f, 0.3f, 0.4f, 1.0f}   // 7
  };

  Uint32 cube_indices[] = {// Front
                           0, 1, 2, 2, 3, 0,
                           // Right
                           1, 5, 6, 6, 2, 1,
                           // Back
                           5, 4, 7, 7, 6, 5,
                           // Left
                           4, 0, 3, 3, 7, 4,
                           // Top
                           3, 2, 6, 6, 7, 3,
                           // Bottom
                           4, 5, 1, 1, 0, 4};

  if (!CreateVertexBuffer(state, cube_vertices)) {
    return SDL_APP_FAILURE;
  }

  if (!CreateIndexBuffer(state, cube_indices)) {
    return SDL_APP_FAILURE;
  }

  state->proj_mat = glm::perspective(glm::radians(45.0f),
                                     (f32)width / (f32)height, 0.1f, 100.f);
  state->view_mat =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS;
  default:
    return SDL_APP_CONTINUE;
  }
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto state = static_cast<State *>(appstate);

  auto *command_buffer = SDL_AcquireGPUCommandBuffer(state->device);

  if (!command_buffer) {
    SDL_Log("Couldn't acquire GPU command buffer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUTexture *swapchain_texture;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, state->window,
                                             &swapchain_texture, nullptr,
                                             nullptr)) {
    SDL_Log("Couldn't acquire swapchain texture: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUColorTargetInfo color_target_info = {
      .texture = swapchain_texture,
      .clear_color = SDL_FColor{0.4f, 0.6f, 0.9f, 1.0f},
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_STORE,
  };

  auto *render_pass =
      SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);

  SDL_BindGPUGraphicsPipeline(render_pass, state->pipeline);

  SDL_GPUBufferBinding vertex_buffers[] = {
      SDL_GPUBufferBinding{.buffer = state->vertex_buffer, .offset = 0}};

  SDL_BindGPUVertexBuffers(render_pass, 0, vertex_buffers,
                           ArrayCount(vertex_buffers));

  SDL_GPUBufferBinding index_buffers[] = {
      SDL_GPUBufferBinding{.buffer = state->index_buffer, .offset = 0}};

  SDL_BindGPUIndexBuffer(render_pass, index_buffers,
                         SDL_GPU_INDEXELEMENTSIZE_32BIT);

  state->angle += 0.01f;
  glm::mat4 model_mat =
      glm::rotate(glm::mat4(1.0f), state->angle, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 mvp = state->proj_mat * state->view_mat * model_mat;

  SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(glm::mat4));
  SDL_DrawGPUIndexedPrimitives(render_pass, 36, 1, 0, 0, 0);

  SDL_EndGPURenderPass(render_pass);
  SDL_SubmitGPUCommandBuffer(command_buffer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) { SDL_Log("Quit"); }
