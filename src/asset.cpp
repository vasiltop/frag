#include "asset.h"
#include "base/base.h"
#define STB_IMAGE_IMPLEMENTATION
#include "gpu.h"
#include "stb_image.h"

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

  SDL_GPUTextureCreateInfo texture_info{
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = (u32)tex_width,
      .height = (u32)tex_height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
  };

  auto *texture = SDL_CreateGPUTexture(device, &texture_info);
  u32 size_bytes = tex_width * tex_height * 4;

  auto transfer = TransferData(device, tex, size_bytes);

  if (!transfer)
    return nullptr;

  defer { SDL_ReleaseGPUTransferBuffer(device, transfer); };

  SDL_GPUTextureTransferInfo source_info = {
      .transfer_buffer = transfer,
      .offset = 0,
  };

  SDL_GPUTextureRegion destination_region = {
      .texture = texture,
      .w = (u32)tex_width,
      .h = (u32)tex_height,
      .d = 1,
  };

  auto upload = BeginUpload(device);
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
