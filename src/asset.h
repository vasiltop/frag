#pragma once

#include "base/string.h"
#include <SDL3/SDL.h>

namespace asset {
struct Mesh {
  SDL_GPUBuffer *vertex_buffer;
  SDL_GPUBuffer *index_buffer;
  u32 index_count;
};

struct Model {
  Mesh *mesh;
  SDL_GPUTexture *texture;
};

SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, String8 filename);
SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, void *data, s32 width,
                            s32 height);
b32 LoadGlb(mem::Arena *arena, SDL_GPUDevice *device, String8 filename,
            Model *model);
}; // namespace asset
