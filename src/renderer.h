#pragma once

#include "./base/string.h"
#include "thing.h"
#include <SDL3/SDL.h>

namespace frag {

constexpr u32 default_window_width = 1280;
constexpr u32 default_window_height = 720;

struct Renderer {
  SDL_GPUDevice *device;
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_GPUSampler *sampler;
  SDL_GPUTexture *depth_texture;
  SDL_Window *window;
  u32 width;
  u32 height;
};

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, String8 filename);
b32 CreatePipeline(Renderer *renderer);
b32 Render(Renderer *renderer, Things *things, glm::mat4 proj_mat,
           glm::mat4 view_mat);

b32 Init(Renderer *renderer);
b32 Resize(Renderer *renderer, u32 width, u32 height);

} // namespace frag
