#pragma once

#include "./base/string.h"
#include <SDL3/SDL.h>
#include <span>

struct Renderer {
  SDL_GPUDevice *device;
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_GPUSampler *sampler;
  SDL_GPUTexture *depth_texture;
  SDL_Window *window;
};

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, String8 filename);
b32 CreatePipeline(Renderer *state);
