#pragma once

#include "base/string.h"
#include <SDL3/SDL.h>

struct Mesh {
  SDL_GPUBuffer *vertex_buffer;
  SDL_GPUBuffer *index_buffer;
};

struct Model {
  Mesh *mesh;
  SDL_GPUTexture *texture;
};

SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, String8 filename);
