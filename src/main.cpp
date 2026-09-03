#define SDL_MAIN_USE_CALLBACKS
#include "game/game.h"
#include <SDL3/SDL_main.h>

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_Log("Init");

  auto perm_arena = mem::ArenaAlloc();

  auto things = mem::Push<thing::Things>(perm_arena);
  thing::Init(things);

  auto renderer = mem::Push<renderer::Renderer>(perm_arena);
  renderer::Init(renderer);

  auto state = mem::Push<State>(
      perm_arena,
      State{
          .perm_arena = perm_arena,
          .renderer = renderer,
          .things = things,
          .proj_mat = glm::perspectiveRH_ZO(
              glm::radians(45.0f), (f32)width / (f32)height, 0.1f, 100.f),
      });

  game::Init(state);

  *appstate = state;
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  auto *state = static_cast<State *>(appstate);
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS;
  }

  game::HandleEvent(state, event);

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

  if (!Render(state->renderer, state->things, state->proj_mat,
              state->view_mat)) {
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) { SDL_Log("Quit"); }
