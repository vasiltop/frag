#pragma once
#include "base/base.h"
#include <SDL3/SDL.h>
#include <span>

namespace gpu {
struct Vertex {
  f32 x, y, z;
  f32 r, g, b, a;
  f32 u, v;
  f32 nx, ny, nz;
};

struct Upload {
  SDL_GPUCopyPass *pass;
  SDL_GPUCommandBuffer *buf;
};

Upload BeginUpload(SDL_GPUDevice *device);
b32 CopyToBuffer(SDL_GPUDevice *device, void *data, u32 size,
                 SDL_GPUBuffer *buf);
SDL_GPUTransferBuffer *TransferData(SDL_GPUDevice *device, void *data,
                                    u32 size_bytes);
SDL_GPUBuffer *CreateVertexBuffer(SDL_GPUDevice *device,
                                  std::span<Vertex> vertices);
SDL_GPUBuffer *CreateIndexBuffer(SDL_GPUDevice *device, std::span<u32> indices);
}; // namespace gpu
