#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include "base/mem.h"
#include <SDL3/SDL_main.h>

struct State {
  SDL_Window *window;
  SDL_GPUDevice *device;
  mem::Arena *perm_arena;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_Log("Init");

  auto perm_arena = mem::ArenaAlloc();
  auto state = mem::Push<State>(perm_arena, nullptr, nullptr, perm_arena);
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
  SDL_EndGPURenderPass(render_pass);
  SDL_SubmitGPUCommandBuffer(command_buffer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) { SDL_Log("Quit"); }
