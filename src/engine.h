#pragma once

#include "./base/string.h"
#include "./base/types.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <span>

struct State {
  SDL_Window *window;
  SDL_GPUDevice *device;
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_GPUBuffer *vertex_buffer;
  SDL_GPUBuffer *index_buffer;
  mem::Arena *perm_arena;
  glm::mat4 proj_mat;
  glm::mat4 view_mat;
  f32 angle;
};

struct Vertex {
  f32 x, y, z;
  f32 r, g, b, a;
};

b32 CopyToBuffer(State *state, void *data, u32 size, SDL_GPUBuffer *buf);
SDL_GPUShader *LoadShader(SDL_GPUDevice *device, String8 filename);
b32 CreatePipeline(State *state);
b32 CreateVertexBuffer(State *state, std::span<Vertex> vertices);
b32 CreateIndexBuffer(State *state, std::span<u32> indices);
