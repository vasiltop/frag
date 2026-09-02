#define SDL_MAIN_USE_CALLBACKS
#include "game/game.h"
#include <SDL3/SDL_main.h>

constexpr auto width = 1280;
constexpr auto height = 720;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_Log("Init");

  auto perm_arena = mem::ArenaAlloc();

  auto things = mem::Push<thing::Things>(perm_arena);
  thing::Init(things);

  auto state = mem::Push<State>(
      perm_arena, State{.perm_arena = perm_arena, .things = things});

  state->renderer = mem::Push<Renderer>(perm_arena);
  state->renderer->window = SDL_CreateWindow("frag", width, height, 0);

  if (!state->renderer->window) {
    SDL_Log("Failed to create window: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUShaderFormat formatFlags = SDL_GPU_SHADERFORMAT_SPIRV |
                                    SDL_GPU_SHADERFORMAT_DXIL |
                                    SDL_GPU_SHADERFORMAT_MSL;

  state->renderer->device = SDL_CreateGPUDevice(formatFlags, true, nullptr);
  if (!state->renderer->device) {
    SDL_Log("Couldn't create GPU device: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_ClaimWindowForGPUDevice(state->renderer->device,
                                   state->renderer->window)) {
    SDL_Log("Couldn't claim window for GPU device: %s", SDL_GetError());
    return SDL_APP_FAILURE;
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

  state->renderer->depth_texture =
      SDL_CreateGPUTexture(state->renderer->device, &depth_info);

  if (!CreatePipeline(state->renderer)) {
    return SDL_APP_FAILURE;
  }

  state->proj_mat = glm::perspective(glm::radians(45.0f),
                                     (f32)width / (f32)height, 0.1f, 100.f);
  state->view_mat =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));

  SDL_GPUSamplerCreateInfo sampler_info{
      .min_filter = SDL_GPU_FILTER_LINEAR,
      .mag_filter = SDL_GPU_FILTER_LINEAR,
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
      .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
      .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
  };

  state->renderer->sampler =
      SDL_CreateGPUSampler(state->renderer->device, &sampler_info);

  *appstate = state;
  game::Init(state);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  auto state = static_cast<State *>(appstate);
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
      SDL_SetWindowRelativeMouseMode(state->renderer->window, true);
      state->mouse_captured = true;
    }
    break;

  case SDL_EVENT_KEY_DOWN:
    if (event->key.key == SDLK_ESCAPE) {
      SDL_SetWindowRelativeMouseMode(state->renderer->window, false);
      state->mouse_captured = false;
    }
    break;
  case SDL_EVENT_MOUSE_MOTION:
    if (state->mouse_captured) {
      float sensitivity = 0.003f;

      state->cam_yaw -= event->motion.xrel * sensitivity;
      state->cam_pitch -=
          event->motion.yrel * sensitivity; // Inverted Y so up looks up

      state->cam_pitch = glm::clamp(state->cam_pitch, -1.57f, 1.57f);
    }
    break;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  auto state = static_cast<State *>(appstate);

  u64 current_tick = SDL_GetTicks();
  f32 delta_time{};

  if (state->last_tick > 0) {
    delta_time = (f32)(current_tick - state->last_tick) / 1000.f;
  }
  state->last_tick = current_tick;

  game::Update(state, delta_time);

  auto *command_buffer = SDL_AcquireGPUCommandBuffer(state->renderer->device);

  if (!command_buffer) {
    SDL_Log("Couldn't acquire GPU command buffer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUTexture *swapchain_texture;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(
          command_buffer, state->renderer->window, &swapchain_texture, nullptr,
          nullptr)) {
    SDL_Log("Couldn't acquire swapchain texture: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_GPUColorTargetInfo color_target_info{
      .texture = swapchain_texture,
      .clear_color = SDL_FColor{0.4f, 0.6f, 0.9f, 1.0f},
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_STORE,
  };

  SDL_GPUDepthStencilTargetInfo depth_target_info{
      .texture = state->renderer->depth_texture,
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

  SDL_BindGPUGraphicsPipeline(render_pass, state->renderer->pipeline);

  for (s32 i = 1; i < thing::max_things; i++) {
    auto things = state->things;
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

    glm::mat4 mvp = state->proj_mat * state->view_mat * model_mat;

    SDL_GPUTextureSamplerBinding binding{.texture = thing.model->texture,
                                         .sampler = state->renderer->sampler};

    SDL_BindGPUFragmentSamplers(render_pass, 0, &binding, 1);

    SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(glm::mat4));
    SDL_DrawGPUIndexedPrimitives(render_pass, 36, 1, 0, 0, 0);
  }

  SDL_EndGPURenderPass(render_pass);
  SDL_SubmitGPUCommandBuffer(command_buffer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) { SDL_Log("Quit"); }
