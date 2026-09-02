#pragma once

#include "./base/string.h"
#include <SDL3/SDL.h>
#include <span>

struct Vertex {
  f32 x, y, z;
  f32 r, g, b, a;
  f32 u, v;
};

struct Renderer {
  SDL_GPUDevice *device;
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_GPUSampler *sampler;
  SDL_GPUTexture *depth_texture;
  SDL_Window *window;

  SDL_GPUBuffer *vertex_buffer;
  SDL_GPUBuffer *index_buffer;
};

struct Upload {
  SDL_GPUCopyPass *pass;
  SDL_GPUCommandBuffer *buf;
};

SDL_GPUTransferBuffer *TransferData(Renderer *state, void *data,
                                    u32 size_bytes);
Upload BeginUpload(Renderer *state);
SDL_GPUTexture *LoadTexture(Renderer *state, String8 filename);
b32 CopyToBuffer(Renderer *state, void *data, u32 size, SDL_GPUBuffer *buf);
SDL_GPUShader *LoadShader(SDL_GPUDevice *device, String8 filename);
b32 CreatePipeline(Renderer *state);
b32 CreateVertexBuffer(Renderer *state, std::span<Vertex> vertices);
b32 CreateIndexBuffer(Renderer *state, std::span<u32> indices);
