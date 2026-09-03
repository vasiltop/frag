#include "gpu.h"

namespace frag {

SDL_GPUTransferBuffer *TransferData(SDL_GPUDevice *device, void *data,
                                    u32 size_bytes) {
  SDL_GPUTransferBufferCreateInfo transfer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = size_bytes,
  };

  SDL_GPUTransferBuffer *transfer_buffer =
      SDL_CreateGPUTransferBuffer(device, &transfer_info);

  void *tb_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

  if (!tb_data) {
    SDL_Log("Couldn't map transfer buffer: %s", SDL_GetError());
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    return nullptr;
  }

  SDL_memcpy(tb_data, data, size_bytes);
  SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

  return transfer_buffer;
}

Upload BeginUpload(SDL_GPUDevice *device) {
  auto *upload_buf = SDL_AcquireGPUCommandBuffer(device);
  if (!upload_buf) {
    SDL_Log("Could not acquire GPU command buffer: %s", SDL_GetError());
    return {};
  }

  auto *copy_pass = SDL_BeginGPUCopyPass(upload_buf);

  return {copy_pass, upload_buf};
};

b32 CopyToBuffer(SDL_GPUDevice *device, void *data, u32 size,
                 SDL_GPUBuffer *buf) {
  auto transfer = TransferData(device, data, size);
  defer { SDL_ReleaseGPUTransferBuffer(device, transfer); };
  auto upload = BeginUpload(device);

  if (!transfer || !upload.pass)
    return false;

  SDL_GPUTransferBufferLocation buf_loc{.transfer_buffer = transfer,
                                        .offset = 0};

  SDL_GPUBufferRegion buf_reg{.buffer = buf, .offset = 0, .size = size};

  SDL_UploadToGPUBuffer(upload.pass, &buf_loc, &buf_reg, false);
  SDL_EndGPUCopyPass(upload.pass);

  if (!SDL_SubmitGPUCommandBuffer(upload.buf)) {
    SDL_Log("Could not submit GPU command buffer: %s", SDL_GetError());
    return false;
  }

  return true;
}

SDL_GPUBuffer *CreateVertexBuffer(SDL_GPUDevice *device,
                                  std::span<Vertex> vertices) {
  u32 size = vertices.size() * sizeof(Vertex);
  SDL_GPUBufferCreateInfo vb_info{.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                  .size = size};

  auto vertex_buffer = SDL_CreateGPUBuffer(device, &vb_info);
  if (!vertex_buffer) {
    SDL_Log("Could not create vertex buffer: %s", SDL_GetError());
    return nullptr;
  }

  CopyToBuffer(device, vertices.data(), size, vertex_buffer);

  return vertex_buffer;
}

SDL_GPUBuffer *CreateIndexBuffer(SDL_GPUDevice *device,
                                 std::span<u32> indices) {
  u32 size = indices.size() * sizeof(u32);
  SDL_GPUBufferCreateInfo ib_info{.usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                  .size = size};

  auto index_buffer = SDL_CreateGPUBuffer(device, &ib_info);

  if (!index_buffer) {
    SDL_Log("Could not create index buffer: %s", SDL_GetError());
    return nullptr;
  }

  CopyToBuffer(device, indices.data(), size, index_buffer);

  return index_buffer;
}

} // namespace frag
