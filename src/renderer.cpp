#include "renderer.h"
#include "base/mem.h"
#include "gpu.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace renderer {
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
      .num_uniform_buffers = 2,
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
          .pitch = sizeof(gpu::Vertex),
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
      },
      SDL_GPUVertexAttribute{
          .location = 3,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
          .offset = sizeof(f32) * 9,
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

b32 Render(Renderer *renderer, thing::Things *things, glm::mat4 proj_mat,
           glm::mat4 view_mat) {
  auto *command_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);

  if (!command_buffer) {
    SDL_Log("Couldn't acquire GPU command buffer: %s", SDL_GetError());
    return false;
  }

  SDL_GPUTexture *swapchain_texture;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, renderer->window,
                                             &swapchain_texture, nullptr,
                                             nullptr)) {
    SDL_Log("Couldn't acquire swapchain texture: %s", SDL_GetError());
    return false;
  }

  SDL_GPUColorTargetInfo color_target_info{
      .texture = swapchain_texture,
      .clear_color = SDL_FColor{0.4f, 0.6f, 0.9f, 1.0f},
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_STORE,
  };

  SDL_GPUDepthStencilTargetInfo depth_target_info{
      .texture = renderer->depth_texture,
      .clear_depth = 1.0f,
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_DONT_CARE,
      .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
      .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
      .cycle = true,
      .clear_stencil = 0,
  };

  auto *render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info,
                                             1, &depth_target_info);

  SDL_BindGPUGraphicsPipeline(render_pass, renderer->pipeline);

  for (s32 i = 1; i < thing::max_things; i++) {
    if (!things->used[i])
      continue;

    auto &thing = things->slots[i];
    if (!thing.model)
      continue;

    SDL_GPUBufferBinding vertex_buffers[] = {SDL_GPUBufferBinding{
        .buffer = thing.model->mesh->vertex_buffer, .offset = 0}};

    SDL_BindGPUVertexBuffers(render_pass, 0, vertex_buffers,
                             ArrayCount(vertex_buffers));

    SDL_GPUBufferBinding index_buffers[] = {SDL_GPUBufferBinding{
        .buffer = thing.model->mesh->index_buffer, .offset = 0}};

    SDL_BindGPUIndexBuffer(render_pass, index_buffers,
                           SDL_GPU_INDEXELEMENTSIZE_32BIT);

    glm::mat4 model_mat = glm::mat4(1.0f);
    model_mat = glm::translate(model_mat, thing.pos);
    model_mat =
        glm::rotate(model_mat, thing.rot.y, glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
    model_mat = glm::rotate(model_mat, thing.rot.x,
                            glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
    model_mat = glm::rotate(model_mat, thing.rot.z,
                            glm::vec3(0.0f, 0.0f, 1.0f)); // Roll
    model_mat = glm::scale(model_mat, thing.scale);

    glm::mat4 mvp = proj_mat * view_mat * model_mat;

    SDL_GPUTextureSamplerBinding binding{.texture = thing.model->texture,
                                         .sampler = renderer->sampler};

    SDL_BindGPUFragmentSamplers(render_pass, 0, &binding, 1);
    struct VertexUniforms {
      glm::mat4 mvp;
      glm::mat4 model;
    };
    VertexUniforms uniforms{mvp, model_mat};
    SDL_PushGPUVertexUniformData(command_buffer, 0, &uniforms,
                                 sizeof(uniforms));

    SDL_DrawGPUIndexedPrimitives(render_pass, thing.model->mesh->index_count, 1,
                                 0, 0, 0);
  }

  SDL_EndGPURenderPass(render_pass);
  SDL_SubmitGPUCommandBuffer(command_buffer);

  return true;
}

priv b32 CreateDepthTexture(Renderer *renderer, u32 width, u32 height) {
  if (renderer->depth_texture) {
    SDL_ReleaseGPUTexture(renderer->device, renderer->depth_texture);
    renderer->depth_texture = nullptr;
  }

  SDL_GPUTextureCreateInfo depth_info{
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
      .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
      .width = width,
      .height = height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
  };

  renderer->depth_texture = SDL_CreateGPUTexture(renderer->device, &depth_info);
  if (!renderer->depth_texture) {
    SDL_Log("Could not create depth texture: %s", SDL_GetError());
    return false;
  }

  renderer->width = width;
  renderer->height = height;
  return true;
}

b32 Resize(Renderer *renderer, u32 width, u32 height) {
  if (width == 0 || height == 0)
    return true;

  if (width == renderer->width && height == renderer->height)
    return true;

  return CreateDepthTexture(renderer, width, height);
}

b32 Init(Renderer *renderer) {
  renderer->window =
      SDL_CreateWindow("frag", width, height, SDL_WINDOW_RESIZABLE);

  if (!renderer->window) {
    SDL_Log("Failed to create window: %s", SDL_GetError());
    return false;
  }

  SDL_GPUShaderFormat formatFlags = SDL_GPU_SHADERFORMAT_SPIRV |
                                    SDL_GPU_SHADERFORMAT_DXIL |
                                    SDL_GPU_SHADERFORMAT_MSL;

  renderer->device = SDL_CreateGPUDevice(formatFlags, true, nullptr);
  if (!renderer->device) {
    SDL_Log("Couldn't create GPU device: %s", SDL_GetError());
    return false;
  }

  if (!SDL_ClaimWindowForGPUDevice(renderer->device, renderer->window)) {
    SDL_Log("Couldn't claim window for GPU device: %s", SDL_GetError());
    return false;
  }

  s32 window_width{};
  s32 window_height{};
  SDL_GetWindowSizeInPixels(renderer->window, &window_width, &window_height);

  if (!CreateDepthTexture(renderer, (u32)window_width, (u32)window_height)) {
    return false;
  }

  if (!CreatePipeline(renderer)) {
    return false;
  }

  SDL_GPUSamplerCreateInfo sampler_info{
      .min_filter = SDL_GPU_FILTER_LINEAR,
      .mag_filter = SDL_GPU_FILTER_LINEAR,
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
      .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
      .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
  };

  renderer->sampler = SDL_CreateGPUSampler(renderer->device, &sampler_info);

  return true;
}
}; // namespace renderer
