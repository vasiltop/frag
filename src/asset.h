#pragma once

#include "base/string.h"
#include "map.h"
#include <SDL3/SDL.h>

namespace frag {

struct SubMesh {
  SDL_GPUTexture *texture;
  u32 index_offset;
  u32 index_count;
};

struct Model {
  SDL_GPUBuffer *vertex_buffer;
  SDL_GPUBuffer *index_buffer;
  Array<SubMesh> sub_meshes;
};

constexpr auto vertices_per_face = 4;
constexpr auto indices_per_face = 6;
constexpr auto vertices_per_brush = vertices_per_face * 6;
constexpr auto indices_per_brush = indices_per_face * 6;

SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, String8 filename);
SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, void *data, s32 width,
                            s32 height);
b32 LoadGlb(Arena *arena, SDL_GPUDevice *device, String8 filename, Model *out);
b32 BuildMapModel(Arena *arena, SDL_GPUDevice *device, Map *map, Model *out);

} // namespace frag
