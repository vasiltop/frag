#include "engine.h"
#include "base/mem.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_GPUTransferBuffer *TransferData(State *state, void *data, u32 size_bytes) {
  SDL_GPUTransferBufferCreateInfo transfer_info{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = size_bytes,
  };

  SDL_GPUTransferBuffer *transfer_buffer =
      SDL_CreateGPUTransferBuffer(state->device, &transfer_info);

  void *tb_data =
      SDL_MapGPUTransferBuffer(state->device, transfer_buffer, false);

  if (!tb_data) {
    SDL_Log("Couldn't map transfer buffer: %s", SDL_GetError());
    SDL_ReleaseGPUTransferBuffer(state->device, transfer_buffer);
    return nullptr;
  }

  SDL_memcpy(tb_data, data, size_bytes);
  SDL_UnmapGPUTransferBuffer(state->device, transfer_buffer);

  return transfer_buffer;
}

Upload BeginUpload(State *state) {
  auto *upload_buf = SDL_AcquireGPUCommandBuffer(state->device);
  if (!upload_buf) {
    SDL_Log("Could not acquire GPU command buffer: %s", SDL_GetError());
    return {};
  }

  auto *copy_pass = SDL_BeginGPUCopyPass(upload_buf);

  return {copy_pass, upload_buf};
};

SDL_GPUTexture *LoadTexture(State *state, String8 filename) {
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

  auto *texture = SDL_CreateGPUTexture(state->device, &texture_info);
  u32 size_bytes = tex_width * tex_height * 4;

  auto transfer = TransferData(state, tex, size_bytes);

  if (!transfer)
    return nullptr;

  defer { SDL_ReleaseGPUTransferBuffer(state->device, transfer); };

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

  auto upload = BeginUpload(state);
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

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, String8 filename) {
  SDL_GPUShaderStage stage;

  if (filename.str[0] == 'v') {
    stage = SDL_GPU_SHADERSTAGE_VERTEX;
  } else if (filename.str[0] == 'f') {
    stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  } else {
    SDL_Log("Could not deduce shader stage: %s", filename.str);
    return nullptr;
  }

  auto scratch = mem::Scratch();
  auto base_path = Str8C(SDL_GetBasePath());
  auto full_path = Str8Cat(scratch.arena, base_path, Str8Lit("/shaders"));

  SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
  auto backend_formats = SDL_GetGPUShaderFormats(device);

  auto entrypoint = Str8Lit("main");
  full_path = Str8Cat(scratch.arena, full_path, Str8Lit("/"));

  if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
    filename = Str8Cat(scratch.arena, filename, Str8Lit(".spv"));
    format = SDL_GPU_SHADERFORMAT_SPIRV;
  } else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
    filename = Str8Cat(scratch.arena, filename, Str8Lit(".msl"));
    format = SDL_GPU_SHADERFORMAT_MSL;
    entrypoint = Str8Lit("main0");
  } else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
    filename = Str8Cat(scratch.arena, filename, Str8Lit(".dxil"));
    format = SDL_GPU_SHADERFORMAT_DXIL;
  } else {
    SDL_Log("Could not find a supported shader format for backend %s",
            SDL_GetGPUDeviceDriver(device));
    return nullptr;
  }

  full_path = Str8Cat(scratch.arena, full_path, filename);
  size_t file_size;
  void *code = SDL_LoadFile((char *)full_path.str, &file_size);
  if (!code) {
    SDL_Log("Could not load shader from disk\n\t%s", SDL_GetError());
    return nullptr;
  }

  u32 num_samplers = 0;
  if (stage == SDL_GPU_SHADERSTAGE_FRAGMENT) {
    num_samplers = 1;
  }

  SDL_GPUShaderCreateInfo shader_info{
      .code_size = file_size,
      .code = static_cast<u8 *>(code),
      .entrypoint = (char *)entrypoint.str,
      .format = format,
      .stage = stage,
      .num_samplers = num_samplers,
      .num_uniform_buffers = 1,
  };

  auto *shader = SDL_CreateGPUShader(device, &shader_info);
  if (!shader) {
    SDL_Log("Could not create shader from file %s: %s", full_path.str,
            SDL_GetError());
    SDL_free(code);
    return nullptr;
  }

  return shader;
}

b32 CreatePipeline(State *state) {
  auto *vertex_shader = LoadShader(state->device, Str8Lit("v"));
  if (!vertex_shader) {
    SDL_Log("Could not create vertex shader!");
    return false;
  }

  auto *fragment_shader = LoadShader(state->device, Str8Lit("f"));
  if (!vertex_shader) {
    SDL_Log("Could not create vertex shader!");
    return false;
  }

  SDL_GPUVertexBufferDescription vb_desc[] = {
      SDL_GPUVertexBufferDescription{
          .slot = 0,
          .pitch = sizeof(Vertex),
          .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
          .instance_step_rate = 0,
      },
  };

  SDL_GPUVertexAttribute v_attribs[] = {
      SDL_GPUVertexAttribute{
          .location = 0,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
          .offset = 0,
      },
      SDL_GPUVertexAttribute{
          .location = 1,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
          .offset = sizeof(f32) * 3,
      },
      SDL_GPUVertexAttribute{
          .location = 2,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
          .offset = sizeof(f32) * 7,
      }};

  SDL_GPUColorTargetDescription gpu_color_descs[] = {
      SDL_GPUColorTargetDescription{.format = SDL_GetGPUSwapchainTextureFormat(
                                        state->device, state->window)}};

  SDL_GPUGraphicsPipelineCreateInfo pipeline_info{
      .vertex_shader = vertex_shader,
      .fragment_shader = fragment_shader,
      .vertex_input_state =
          SDL_GPUVertexInputState{.vertex_buffer_descriptions = vb_desc,
                                  .num_vertex_buffers = ArrayCount(vb_desc),
                                  .vertex_attributes = v_attribs,
                                  .num_vertex_attributes =
                                      ArrayCount(v_attribs)},
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .rasterizer_state =
          SDL_GPURasterizerState{
              .fill_mode = SDL_GPU_FILLMODE_FILL,
          },
      .depth_stencil_state =
          {
              .compare_op = SDL_GPU_COMPAREOP_LESS,
              .enable_depth_test = true,
              .enable_depth_write = true,
          },
      .target_info =
          SDL_GPUGraphicsPipelineTargetInfo{
              .color_target_descriptions = gpu_color_descs,
              .num_color_targets = ArrayCount(gpu_color_descs),
              .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
              .has_depth_stencil_target = true,
          },
  };

  state->pipeline =
      SDL_CreateGPUGraphicsPipeline(state->device, &pipeline_info);

  if (!state->pipeline) {
    SDL_Log("Could not create graphics pipeline! %s", SDL_GetError());
    return false;
  }

  SDL_ReleaseGPUShader(state->device, vertex_shader);
  SDL_ReleaseGPUShader(state->device, fragment_shader);

  return true;
}

b32 CopyToBuffer(State *state, void *data, u32 size, SDL_GPUBuffer *buf) {
  auto transfer = TransferData(state, data, size);
  defer { SDL_ReleaseGPUTransferBuffer(state->device, transfer); };
  auto upload = BeginUpload(state);

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

b32 CreateVertexBuffer(State *state, std::span<Vertex> vertices) {
  u32 size = vertices.size() * sizeof(Vertex);
  SDL_GPUBufferCreateInfo vb_info{.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                  .size = size};

  state->vertex_buffer = SDL_CreateGPUBuffer(state->device, &vb_info);
  if (!state->vertex_buffer) {
    SDL_Log("Could not create vertex buffer: %s", SDL_GetError());
    return false;
  }

  CopyToBuffer(state, vertices.data(), size, state->vertex_buffer);

  return true;
}

b32 CreateIndexBuffer(State *state, std::span<u32> indices) {
  u32 size = indices.size() * sizeof(u32);
  SDL_GPUBufferCreateInfo ib_info{.usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                  .size = size};

  state->index_buffer = SDL_CreateGPUBuffer(state->device, &ib_info);

  if (!state->index_buffer) {
    SDL_Log("Could not create index buffer: %s", SDL_GetError());
    return false;
  }

  CopyToBuffer(state, indices.data(), size, state->index_buffer);

  return true;
}
