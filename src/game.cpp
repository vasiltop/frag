#include "game.h"
#include "map.h"
#include "thing.h"
#include <cmath>

namespace game {

priv b32 Grounded(frag::Thing &player, frag::Thing &map) {
  frag::Thing test = player;
  test.pos.y -= GROUNDED_HEIGHT;
  return frag::Collision(test, map);
}

priv f32 ApplyGravity(f32 dt, f32 vel_y, b32 grounded) {
  if (grounded)
    return vel_y;
  return vel_y - GRAVITY * dt;
}

priv glm::vec2 ApplyFriction(f32 dt, glm::vec2 vel_planar, glm::vec2 wish_dir,
                             b32 grounded, b32 is_jumping) {
  if (!grounded || is_jumping)
    return vel_planar;

  glm::vec2 v = vel_planar;
  if (glm::dot(vel_planar, vel_planar) > 0.f)
    v = vel_planar -
        glm::normalize(vel_planar) * dt * MAX_GROUND_ACCEL / FRICTION;

  if (glm::dot(v, v) < 1.f && glm::dot(wish_dir, wish_dir) < 0.01f)
    return glm::vec2(0.f);

  return v;
}

priv glm::vec2 UpdateVelocity(f32 dt, glm::vec2 vel_planar, glm::vec2 wish_dir,
                              b32 grounded) {
  f32 current_speed = glm::dot(vel_planar, wish_dir);
  f32 max_speed = grounded ? MAX_GROUND_SPEED : MAX_AIR_SPEED;
  f32 max_accel = grounded ? MAX_GROUND_ACCEL : MAX_AIR_ACCEL;
  f32 add_speed = Clamp(0.f, max_speed - current_speed, max_accel * dt);
  return vel_planar + wish_dir * add_speed;
}

priv f32 CheckForJump(f32 y_vel, b32 is_jumping, b32 grounded) {
  if (is_jumping && grounded)
    return JUMP_FORCE;
  return y_vel;
}

priv void MoveAndSlide(frag::Thing &player, frag::Thing &map, glm::vec3 &velocity,
                       f32 dt) {
  glm::vec3 motion = velocity * dt;
  const s32 axes[] = {0, 2, 1};

  for (s32 axis : axes) {
    if (motion[axis] == 0.f)
      continue;

    player.pos[axis] += motion[axis];
    if (frag::Collision(player, map)) {
      player.pos[axis] -= motion[axis];
      velocity[axis] = 0.f;
    }
  }
}

priv void FreecamMovement(State *state, f32 dt) {
  glm::vec3 front;
  front.x = cos(state->cam_pitch) * sin(state->cam_yaw);
  front.y = sin(state->cam_pitch);
  front.z = cos(state->cam_pitch) * cos(state->cam_yaw);
  front = glm::normalize(front);

  glm::vec3 right =
      glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

  auto *keys = SDL_GetKeyboardState(NULL);
  f32 velocity = 2.5f * dt;

  if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
    state->cam_pos += front * velocity;
  if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
    state->cam_pos -= front * velocity;
  if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])
    state->cam_pos -= right * velocity;
  if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
    state->cam_pos += right * velocity;

  state->view_mat = glm::lookAt(state->cam_pos, state->cam_pos + front,
                                glm::vec3(0.0f, 1.0f, 0.0f));

  auto &player = Get(state->things, state->map_refs.player);
  player.pos = state->cam_pos;
  for (auto &thing : *state->things) {
    if (thing.kind != frag::ThingKind::Player && Collision(player, thing)) {
			SDL_Log("Collision");
    }
  }
}

priv void Movement(State *state, f32 dt) {
  auto &player = frag::Get(state->things, state->map_refs.player);
  auto &map = frag::Get(state->things, state->map_refs.map);
  auto *keys = SDL_GetKeyboardState(NULL);

  glm::vec2 wasd{};
  if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
    wasd.y += 1.f;
  if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
    wasd.y -= 1.f;
  if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])
    wasd.x += 1.f;
  if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
    wasd.x -= 1.f;

  b32 is_jumping = keys[SDL_SCANCODE_SPACE];

  glm::vec2 wish_dir = wasd;
  f32 yaw = -state->cam_yaw;
  f32 c = cosf(yaw);
  f32 s = sinf(yaw);
  wish_dir = glm::vec2(wish_dir.x * c - wish_dir.y * s,
                       wish_dir.x * s + wish_dir.y * c);

  b32 grounded = Grounded(player, map);

  glm::vec2 vel_planar(player.vel.x, player.vel.z);
  f32 vel_vertical = ApplyGravity(dt, player.vel.y, grounded);
  vel_planar = ApplyFriction(dt, vel_planar, wish_dir, grounded, is_jumping);
  vel_planar = UpdateVelocity(dt, vel_planar, wish_dir, grounded);
  vel_vertical = CheckForJump(vel_vertical, is_jumping, grounded);

  player.vel = glm::vec3(vel_planar.x, vel_vertical, vel_planar.y);
  MoveAndSlide(player, map, player.vel, dt);

  player.rot.y = state->cam_yaw;
  state->cam_pos = player.pos + glm::vec3(0.f, VIEW_HEIGHT, 0.f);

  glm::vec3 front;
  front.x = cosf(state->cam_pitch) * sinf(state->cam_yaw);
  front.y = sinf(state->cam_pitch);
  front.z = cosf(state->cam_pitch) * cosf(state->cam_yaw);
  front = glm::normalize(front);

  state->view_mat = glm::lookAt(state->cam_pos, state->cam_pos + front,
                                glm::vec3(0.f, 1.f, 0.f));
}

priv void SetMap(State *state, String8 path) {
  auto scratch = Scratch();
  frag::Map map{};
  if (!LoadMap(scratch.arena, path, &map)) {
    SDL_Log("Failed to load map: %s", path.data);
    return;
  }
  state->map_refs = PopulateThingsFromMap(
      state->perm_arena, state->renderer->device, state->things, &map);
}

void Init(State *state) {
  state->cam_pitch = 0.0f;
  state->cam_yaw = 3.14159265f;

  SetMap(state, Str8Lit("assets/maps/test_map.map"));

  auto &player = frag::Get(state->things, state->map_refs.player);
  state->cam_pos = player.pos + glm::vec3(0.f, VIEW_HEIGHT, 0.f);
  state->cam_yaw = player.rot.y;
}

void Update(State *state, f32 dt) { Movement(state, dt); }

void HandleEvent(State *state, SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    if (event->window.windowID == SDL_GetWindowID(state->renderer->window)) {
      u32 w = (u32)event->window.data1;
      u32 h = (u32)event->window.data2;
      if (frag::Resize(state->renderer, w, h)) {
        state->proj_mat = glm::perspectiveRH_ZO(glm::radians(45.0f),
                                                (f32)w / (f32)h, 0.1f, 100.f);
      }
    }
    break;

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
      float sensitivity = 0.0006f;

      state->cam_yaw -= event->motion.xrel * sensitivity;
      state->cam_pitch -=
          event->motion.yrel * sensitivity; // Inverted Y so up looks up

      state->cam_pitch = glm::clamp(state->cam_pitch, -1.57f, 1.57f);
    }
    break;
  }
}

} // namespace game
