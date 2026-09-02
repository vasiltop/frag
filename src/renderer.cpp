#include "renderer.h"
#include "base/mem.h"
#include "gpu.h"

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

b32 CreatePipeline(Renderer *renderer) {
  auto *vertex_shader = LoadShader(renderer->device, Str8Lit("v"));
  if (!vertex_shader) {
    SDL_Log("Could not create vertex shader!");
    return false;
  }

  auto *fragment_shader = LoadShader(renderer->device, Str8Lit("f"));
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
                                        renderer->device, renderer->window)}};

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

  renderer->pipeline =
      SDL_CreateGPUGraphicsPipeline(renderer->device, &pipeline_info);

  if (!renderer->pipeline) {
    SDL_Log("Could not create graphics pipeline! %s", SDL_GetError());
    return false;
  }

  SDL_ReleaseGPUShader(renderer->device, vertex_shader);
  SDL_ReleaseGPUShader(renderer->device, fragment_shader);

  return true;
}
