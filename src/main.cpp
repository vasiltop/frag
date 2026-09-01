#include "engine.h"
#define SDL_MAIN_USE_CALLBACKS
#include "base/mem.h"
#include <SDL3/SDL_main.h>

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_Log("Init");

  auto perm_arena = mem::ArenaAlloc();
  auto state = mem::Push<State>(perm_arena, nullptr, nullptr, nullptr, nullptr,
                                perm_arena);
  *appstate = state;

  state->window = SDL_CreateWindow("frag", 1280, 720, 0);

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

  Vertex vertices[]{
      Vertex{-1.0f, -1.0f, 0.0f},
      Vertex{1.0f, -1.0f, 0.0f},
      Vertex{0.0f, 1.0f, 0.0f},
  };

  if (!CreateVertexBuffer(state, vertices)) {
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  SDL_Log("Event");
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS;
  default:
    return SDL_APP_CONTINUE;
  }
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  SDL_Log("Iterate");
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

  SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

  SDL_EndGPURenderPass(render_pass);
  SDL_SubmitGPUCommandBuffer(command_buffer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) { SDL_Log("Quit"); }
