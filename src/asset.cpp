#include "asset.h"
#include "base/base.h"
#define STB_IMAGE_IMPLEMENTATION
#include "gpu.h"
#include "stb_image.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

namespace asset {

SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, void *data, s32 width,
                            s32 height) {
  SDL_GPUTextureCreateInfo texture_info{
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = (u32)width,
      .height = (u32)height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
  };

  auto *texture = SDL_CreateGPUTexture(device, &texture_info);
  u32 size_bytes = width * height * 4;

  auto transfer = gpu::TransferData(device, data, size_bytes);

  if (!transfer)
    return nullptr;

  defer { SDL_ReleaseGPUTransferBuffer(device, transfer); };

  SDL_GPUTextureTransferInfo source_info = {
      .transfer_buffer = transfer,
      .offset = 0,
  };

  SDL_GPUTextureRegion destination_region = {
      .texture = texture,
      .w = (u32)width,
      .h = (u32)height,
      .d = 1,
  };

  auto upload = gpu::BeginUpload(device);
  if (!upload.pass)
    return nullptr;

  SDL_UploadToGPUTexture(upload.pass, &source_info, &destination_region, false);
  SDL_EndGPUCopyPass(upload.pass);

  if (!SDL_SubmitGPUCommandBuffer(upload.buf)) {
    SDL_Log("Could not submit GPU command buffer: %s", SDL_GetError());
    return nullptr;
  }

  return texture;
}

SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, String8 filename) {
  s32 tex_width;
  s32 tex_height;
  s32 channels;

  u8 *tex =
      stbi_load((char *)filename.str, &tex_width, &tex_height, &channels, 4);

  if (!tex) {
    SDL_Log("Failed to load texture: %s", stbi_failure_reason());
    return nullptr;
  }

  defer { stbi_image_free(tex); };

  return LoadTexture(device, tex, tex_width, tex_height);
}

b32 LoadGlb(mem::Arena *arena, SDL_GPUDevice *device, String8 filename,
            Model *model) {
  cgltf_options options{};
  cgltf_data *data{};

  cgltf_result result = cgltf_parse_file(&options, (char *)filename.str, &data);
  if (result != cgltf_result_success)
    return false;

  defer { cgltf_free(data); };

  result = cgltf_load_buffers(&options, data, (char *)filename.str);
  if (result != cgltf_result_success)
    return false;

  if (data->images_count > 0) {
    cgltf_image *cgltf_img = &data->images[0];
    u8 *img_data{};
    s32 img_size{};

    if (cgltf_img->buffer_view) {
      auto *view = cgltf_img->buffer_view;
      img_data = (u8 *)view->buffer->data + view->offset;
      img_size = view->size;
    }

    int width, height, channels;
    u8 *tex = stbi_load_from_memory((const stbi_uc *)img_data, (s32)img_size,
                                    &width, &height, &channels, 4);
    if (tex) {
      model->texture = LoadTexture(device, tex, width, height);
      stbi_image_free(tex);
    }
  }

  if (data->meshes_count == 0)
    return false;

  cgltf_mesh *cg_mesh = &data->meshes[0];
  cgltf_primitive *prim = &cg_mesh->primitives[0];

  u64 vertex_count = 0;
  u64 index_count = 0;

  for (size_t i = 0; i < prim->attributes_count; ++i) {
    if (prim->attributes[i].type == cgltf_attribute_type_position) {
      vertex_count = prim->attributes[i].data->count;
      break;
    }
  }

  if (prim->indices) {
    index_count = prim->indices->count;
  }

  if (vertex_count == 0)
    return false;

  mem::TempArena scratch = mem::Scratch();
  gpu::Vertex *vertices =
      mem::PushCount<gpu::Vertex>(scratch.arena, vertex_count);
  u32 *indices = index_count > 0
                     ? mem::PushCount<u32>(scratch.arena, index_count)
                     : nullptr;

  for (u64 j = 0; j < vertex_count; ++j) {
    vertices[j] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  }

  for (size_t i = 0; i < prim->attributes_count; ++i) {
    cgltf_attribute *attr = &prim->attributes[i];
    cgltf_accessor *acc = attr->data;

    for (size_t j = 0; j < acc->count; ++j) {
      if (attr->type == cgltf_attribute_type_position) {
        f32 val[3];
        cgltf_accessor_read_float(acc, j, val, 3);
        vertices[j].x = val[0];
        vertices[j].y = val[1];
        vertices[j].z = val[2];
      } else if (attr->type == cgltf_attribute_type_color) {
        f32 val[4];
        cgltf_accessor_read_float(acc, j, val, 4);
        vertices[j].r = val[0];
        vertices[j].g = val[1];
        vertices[j].b = val[2];
        vertices[j].a = val[3];
      } else if (attr->type == cgltf_attribute_type_texcoord) {
        f32 val[2];
        cgltf_accessor_read_float(acc, j, val, 2);
        vertices[j].u = val[0];
        vertices[j].v = val[1];
      } else if (attr->type == cgltf_attribute_type_normal) {
        f32 val[3];
        cgltf_accessor_read_float(acc, j, val, 3);
        vertices[j].nx = val[0];
        vertices[j].ny = val[1];
        vertices[j].nz = val[2];
      }
    }
  }

  if (prim->indices && indices) {
    cgltf_accessor *acc = prim->indices;
    for (size_t j = 0; j < acc->count; ++j) {
      cgltf_uint val;
      cgltf_accessor_read_uint(acc, j, &val, 1);
      indices[j] = static_cast<u32>(val);
    }
  }

  auto vb = gpu::CreateVertexBuffer(device, {vertices, vertex_count});
  auto ib = index_count > 0
                ? gpu::CreateIndexBuffer(device, {indices, index_count})
                : nullptr;

  model->mesh = mem::Push<Mesh>(arena, vb, ib, (u32)index_count);

  return true;
}
}; // namespace asset
