#pragma once

#include "./base/string.h"
#include "./base/types.h"
#include <SDL3/SDL.h>
#include <span>

struct State {
  SDL_Window *window;
  SDL_GPUDevice *device;
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_GPUBuffer *vertex_buffer;
  mem::Arena *perm_arena;
};

struct Vertex {
  f32 x, y, z;
  f32 r, g, b, a;
};

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, String8 filename);
b32 CreatePipeline(State *state);
b32 CreateVertexBuffer(State *state, std::span<Vertex> vertices);
