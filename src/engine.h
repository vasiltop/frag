#pragma once

#include "./base/base.h"
#include "./base/string.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <span>

struct State {
  SDL_Window *window;
  SDL_GPUDevice *device;
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_GPUBuffer *vertex_buffer;
  SDL_GPUBuffer *index_buffer;
  SDL_GPUTexture *depth_texture;
  SDL_GPUSampler *sampler;
  mem::Arena *perm_arena;
  glm::mat4 proj_mat;
  glm::mat4 view_mat;
  f32 angle;
  b32 mouse_captured;
  glm::vec3 cam_pos;
  f32 cam_pitch;
  f32 cam_yaw;
  u64 last_tick;
  SDL_GPUTexture *texture;
};

struct Vertex {
  f32 x, y, z;
  f32 r, g, b, a;
  f32 u, v;
};

struct Upload {
  SDL_GPUCopyPass *pass;
  SDL_GPUCommandBuffer *buf;
};

SDL_GPUTransferBuffer *TransferData(State *state, void *data, u32 size_bytes);
Upload BeginUpload(State *state);
SDL_GPUTexture *LoadTexture(State *state, String8 filename);
b32 CopyToBuffer(State *state, void *data, u32 size, SDL_GPUBuffer *buf);
SDL_GPUShader *LoadShader(SDL_GPUDevice *device, String8 filename);
b32 CreatePipeline(State *state);
b32 CreateVertexBuffer(State *state, std::span<Vertex> vertices);
b32 CreateIndexBuffer(State *state, std::span<u32> indices);
