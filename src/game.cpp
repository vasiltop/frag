#include "game.h"
#include "asset.h"
#include "map.h"
#include "nav.h"
#include "physics.h"
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
                             b32 grounded, b32 is_jumping, f32 max_accel) {
  if (!grounded || is_jumping)
    return vel_planar;

  glm::vec2 v = vel_planar;
  if (glm::dot(vel_planar, vel_planar) > 0.f)
    v = vel_planar - glm::normalize(vel_planar) * dt * max_accel / FRICTION;

  if (glm::dot(v, v) < 1.f && glm::dot(wish_dir, wish_dir) < 0.01f)
    return glm::vec2(0.f);

  return v;
}

priv glm::vec2 UpdateVelocity(f32 dt, glm::vec2 vel_planar, glm::vec2 wish_dir,
                              b32 grounded, f32 max_ground_speed,
                              f32 max_ground_accel) {
  f32 current_speed = glm::dot(vel_planar, wish_dir);
  f32 max_speed = grounded ? max_ground_speed : MAX_AIR_SPEED;
  f32 max_accel = grounded ? max_ground_accel : MAX_AIR_ACCEL;
  f32 add_speed = Clamp(0.f, max_speed - current_speed, max_accel * dt);
  return vel_planar + wish_dir * add_speed;
}

priv f32 CheckForJump(f32 y_vel, b32 is_jumping, b32 grounded) {
  if (is_jumping && grounded)
    return JUMP_FORCE;
  return y_vel;
}

priv void MoveAndSlide(frag::Thing &thing, frag::Thing &map, glm::vec3 &velocity,
                       f32 dt) {
  glm::vec3 motion = velocity * dt;
  const s32 axes[] = {0, 2, 1};

  for (s32 axis : axes) {
    if (motion[axis] == 0.f)
      continue;

    thing.pos[axis] += motion[axis];
    if (frag::Collision(thing, map)) {
      thing.pos[axis] -= motion[axis];
      velocity[axis] = 0.f;
    }
  }
}

priv void MoveWalk(frag::Thing &thing, frag::Thing &map, glm::vec2 wish_dir,
                   f32 dt, f32 max_speed, f32 max_accel) {
  if (glm::dot(wish_dir, wish_dir) > 0.0001f)
    wish_dir = glm::normalize(wish_dir);

  b32 grounded = Grounded(thing, map);
  glm::vec2 vel_planar(thing.vel.x, thing.vel.z);
  f32 vel_vertical = ApplyGravity(dt, thing.vel.y, grounded);
  vel_planar =
      ApplyFriction(dt, vel_planar, wish_dir, grounded, false, max_accel);
  vel_planar = UpdateVelocity(dt, vel_planar, wish_dir, grounded, max_speed,
                              max_accel);
  thing.vel = glm::vec3(vel_planar.x, vel_vertical, vel_planar.y);
  MoveAndSlide(thing, map, thing.vel, dt);
}

priv b32 HasLineOfSight(frag::Thing &map, glm::vec3 from, glm::vec3 to) {
  return !frag::SegmentBlocked(from, to, map.colliders, map.pos);
}

priv f32 XzLen(glm::vec3 v) { return sqrtf(v.x * v.x + v.z * v.z); }

priv glm::vec2 Xz(glm::vec3 v) { return {v.x, v.z}; }

priv glm::vec3 CamForward(State *state) {
  glm::vec3 front;
  front.x = cosf(state->cam_pitch) * sinf(state->cam_yaw);
  front.y = sinf(state->cam_pitch);
  front.z = cosf(state->cam_pitch) * cosf(state->cam_yaw);
  return glm::normalize(front);
}

priv glm::vec3 ThingCenter(frag::Thing &thing) {
  if (thing.colliders.size == 0)
    return thing.pos;
  auto &col = thing.colliders.data[0];
  return thing.pos + 0.5f * (col.min + col.max);
}

priv void SweepVoices(State *state) {
  for (s32 i = 0; i < MAX_SOUND_VOICES; i++) {
    SDL_AudioStream *stream = state->voices[i];
    if (!stream)
      continue;
    if (SDL_GetAudioStreamQueued(stream) > 0 ||
        SDL_GetAudioStreamAvailable(stream) > 0)
      continue;
    SDL_DestroyAudioStream(stream);
    state->voices[i] = nullptr;
  }
}

priv void PlaySound(State *state, Sound *sound, f32 pitch) {
  if (!state->audio_device || !sound || !sound->buf || !sound->len)
    return;

  SweepVoices(state);

  s32 slot = -1;
  for (s32 i = 0; i < MAX_SOUND_VOICES; i++) {
    if (!state->voices[i]) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    SDL_DestroyAudioStream(state->voices[0]);
    state->voices[0] = nullptr;
    slot = 0;
  }

  SDL_AudioStream *stream = SDL_CreateAudioStream(&sound->spec, nullptr);
  if (!stream)
    return;

  SDL_SetAudioStreamFrequencyRatio(stream, pitch);
  if (!SDL_BindAudioStream(state->audio_device, stream)) {
    SDL_DestroyAudioStream(stream);
    return;
  }
  SDL_PutAudioStreamData(stream, sound->buf, (int)sound->len);
  SDL_FlushAudioStream(stream);
  state->voices[slot] = stream;
}

priv glm::vec3 DirToRot(glm::vec3 dir) {
  dir = glm::normalize(dir);
  // Renderer applies yaw (Y) then pitch (X). Local +Z is forward, matching
  // CamForward; pitch is negated because RotX maps +Z toward -Y.
  return glm::vec3(-asinf(glm::clamp(dir.y, -1.f, 1.f)), atan2f(dir.x, dir.z),
                   0.f);
}

priv void SpawnProjectile(State *state, glm::vec3 origin, glm::vec3 dir,
                          frag::ThingKind owner) {
  if (!state->bullet_model || glm::dot(dir, dir) < 0.0001f)
    return;

  dir = glm::normalize(dir);
  auto ref = frag::Add(state->things);
  auto &thing = frag::Get(state->things, ref);
  thing.kind = frag::ThingKind::Projectile;
  thing.owner = owner;
  thing.model = state->bullet_model;
  thing.scale = state->bullet_scale;
  thing.colliders = state->bullet_colliders;
  thing.pos = origin + dir * BULLET_SPAWN_OFFSET;
  thing.vel = dir * BULLET_SPEED;
  thing.rot = DirToRot(dir);
  thing.timer = BULLET_LIFETIME;

  f32 pitch = owner == frag::ThingKind::Enemy ? ENEMY_SHOOT_PITCH
                                              : PLAYER_SHOOT_PITCH;
  PlaySound(state, &state->shoot, pitch);
}

priv void PlayerShoot(State *state, f32 dt) {
  if (state->player_fire_cd > 0.f)
    state->player_fire_cd -= dt;

  if (!state->mouse_captured || state->player_fire_cd > 0.f)
    return;

  f32 mx, my;
  SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
  if (!(buttons & SDL_BUTTON_LMASK))
    return;

  SpawnProjectile(state, state->cam_pos, CamForward(state),
                  frag::ThingKind::Player);
  state->player_fire_cd = PLAYER_FIRE_INTERVAL;
}

priv glm::vec2 EnemyDodge(State *state, glm::vec3 origin) {
  glm::vec2 dodge{};
  auto *things = state->things;
  for (s32 idx = things->first_used; idx; idx = things->next_used[idx]) {
    auto &bullet = things->slots[idx];
    if (bullet.kind != frag::ThingKind::Projectile ||
        bullet.owner != frag::ThingKind::Player)
      continue;

    glm::vec3 rel = origin - bullet.pos;
    f32 v2 = glm::dot(bullet.vel, bullet.vel);
    if (v2 < 0.0001f)
      continue;
    if (glm::dot(bullet.vel, rel) <= 0.f)
      continue;

    f32 t = Clamp(0.f, glm::dot(rel, bullet.vel) / v2, bullet.timer);
    glm::vec3 closest = bullet.pos + bullet.vel * t;
    glm::vec3 delta = origin - closest;
    delta.y = 0.f;
    if (glm::dot(delta, delta) > ENEMY_DODGE_RADIUS * ENEMY_DODGE_RADIUS)
      continue;

    glm::vec3 perp = glm::cross(bullet.vel, glm::vec3(0.f, 1.f, 0.f));
    if (glm::dot(perp, perp) < 0.0001f)
      continue;
    perp = glm::normalize(perp);
    glm::vec3 side = rel;
    side.y = 0.f;
    if (glm::dot(side, perp) < 0.f)
      perp = -perp;
    dodge += glm::vec2(perp.x, perp.z);
  }
  return dodge;
}

priv glm::vec2 EnemyPathWish(State *state, frag::Thing &enemy, glm::vec3 goal,
                             b32 can_see) {
  glm::vec2 to_player = Xz(goal - enemy.pos);
  f32 dist = glm::length(to_player);
  if (can_see && dist < ENEMY_STOP_RANGE)
    return {};

  glm::vec3 waypoints[64]{};
  s32 n = frag::FindPath(&state->nav, enemy.pos, goal, waypoints, 64);
  if (n <= 0)
    return dist > 0.0001f ? glm::normalize(to_player) : glm::vec2{};

  glm::vec3 target = waypoints[n - 1];
  for (s32 i = 0; i < n; i++) {
    if (XzLen(waypoints[i] - enemy.pos) > ENEMY_WAYPOINT_REACH) {
      target = waypoints[i];
      break;
    }
  }
  glm::vec2 wish = Xz(target - enemy.pos);
  if (glm::dot(wish, wish) < 0.0001f)
    return {};
  return glm::normalize(wish);
}

priv b32 EnemyCanSpot(frag::Thing &enemy, glm::vec3 origin, glm::vec3 target,
                      b32 los) {
  if (!los)
    return false;
  glm::vec3 to = target - origin;
  if (glm::dot(to, to) > ENEMY_SIGHT_RANGE * ENEMY_SIGHT_RANGE)
    return false;
  to.y = 0.f;
  if (glm::dot(to, to) < 0.0001f)
    return true;
  to = glm::normalize(to);
  glm::vec3 fwd(sinf(enemy.rot.y), 0.f, cosf(enemy.rot.y));
  f32 min_dot = cosf(glm::radians(ENEMY_SIGHT_FOV * 0.5f));
  return glm::dot(fwd, to) >= min_dot;
}

priv void UpdateEnemies(State *state, f32 dt) {
  auto *things = state->things;
  auto &map = frag::Get(things, state->map_refs.map);
  auto &player = frag::Get(things, state->map_refs.player);

  for (s32 idx = things->first_used; idx; idx = things->next_used[idx]) {
    auto &enemy = things->slots[idx];
    if (enemy.kind != frag::ThingKind::Enemy)
      continue;

    glm::vec3 origin = ThingCenter(enemy);
    glm::vec3 to_cam = state->cam_pos - origin;
    b32 los = HasLineOfSight(map, origin, state->cam_pos);
    if (!enemy.alerted && EnemyCanSpot(enemy, origin, state->cam_pos, los))
      enemy.alerted = true;

    glm::vec2 wish{};
    if (enemy.alerted) {
      glm::vec2 path = EnemyPathWish(state, enemy, player.pos, los);
      glm::vec2 dodge = EnemyDodge(state, origin);
      wish = path;
      if (glm::dot(dodge, dodge) > 0.0001f) {
        dodge = glm::normalize(dodge);
        if (glm::dot(path, path) > 0.0001f)
          wish = glm::normalize(path * 0.4f + dodge * 0.8f);
        else
          wish = dodge;
      }

      if (los && glm::dot(to_cam, to_cam) > 0.0001f)
        enemy.rot.y = atan2f(to_cam.x, to_cam.z);
      else if (glm::dot(wish, wish) > 0.0001f)
        enemy.rot.y = atan2f(wish.x, wish.y);
    }
    MoveWalk(enemy, map, wish, dt, ENEMY_GROUND_SPEED, ENEMY_GROUND_ACCEL);

    enemy.timer -= dt;
    if (!los || !enemy.alerted || enemy.timer > 0.f)
      continue;
    if (glm::dot(to_cam, to_cam) < 0.0001f)
      continue;

    SpawnProjectile(state, origin, glm::normalize(to_cam),
                    frag::ThingKind::Enemy);
    enemy.timer = ENEMY_FIRE_INTERVAL;
  }
}

priv void UpdateProjectiles(State *state, f32 dt) {
  auto *things = state->things;
  for (s32 idx = things->first_used; idx;) {
    s32 next = things->next_used[idx];
    auto &bullet = things->slots[idx];
    if (!things->used[idx] || bullet.kind != frag::ThingKind::Projectile) {
      idx = next;
      continue;
    }

    bullet.pos += bullet.vel * dt;
    bullet.rot = DirToRot(bullet.vel);
    bullet.timer -= dt;

    b32 remove = bullet.timer <= 0.f;
    frag::Ref hit_enemy{};

    if (!remove) {
      for (s32 other_idx = things->first_used; other_idx;
           other_idx = things->next_used[other_idx]) {
        if (!things->used[other_idx] || other_idx == idx)
          continue;

        auto &other = things->slots[other_idx];
        if (!frag::Collision(bullet, other))
          continue;

        if (other.kind == frag::ThingKind::Map) {
          remove = true;
          break;
        }
        if (bullet.owner == frag::ThingKind::Player &&
            other.kind == frag::ThingKind::Enemy) {
          hit_enemy = frag::MakeRef(things, other_idx);
          remove = true;
          break;
        }
        if (bullet.owner == frag::ThingKind::Enemy &&
            other.kind == frag::ThingKind::Player) {
          SDL_Log("Player hit by a bullet");
          remove = true;
          break;
        }
      }
    }

    if (hit_enemy.idx)
      frag::Rem(things, hit_enemy);
    if (remove)
      frag::Rem(things, frag::MakeRef(things, idx));

    idx = next;
  }
}

priv void FreecamMovement(State *state, f32 dt) {
  glm::vec3 front = CamForward(state);

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
  vel_planar = ApplyFriction(dt, vel_planar, wish_dir, grounded, is_jumping,
                             MAX_GROUND_ACCEL);
  vel_planar = UpdateVelocity(dt, vel_planar, wish_dir, grounded,
                              MAX_GROUND_SPEED, MAX_GROUND_ACCEL);
  vel_vertical = CheckForJump(vel_vertical, is_jumping, grounded);
  if (is_jumping && grounded)
    PlaySound(state, &state->jump, 1.f);

  player.vel = glm::vec3(vel_planar.x, vel_vertical, vel_planar.y);
  MoveAndSlide(player, map, player.vel, dt);

  player.rot.y = state->cam_yaw;
  state->cam_pos = player.pos + glm::vec3(0.f, VIEW_HEIGHT, 0.f);

  glm::vec3 front = CamForward(state);
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
  state->map_refs = PopulateThingsFromMap(state->perm_arena,
                                          state->renderer->device, state->things,
                                          &map, state->enemy_model);

  s32 enemy_i = 0;
  for (auto &thing : *state->things) {
    if (thing.kind == frag::ThingKind::Enemy)
      thing.timer = 0.4f + enemy_i++ * 0.35f;
  }

  auto &map_thing = frag::Get(state->things, state->map_refs.map);
  state->nav = frag::BuildNavGrid(state->perm_arena, map_thing.colliders,
                                  map_thing.pos);
}

priv void LoadSharedModels(State *state) {
  auto scratch = Scratch();
  auto *device = state->renderer->device;

  auto *enemy_model = Push<frag::Model>(state->perm_arena);
  auto enemy_path =
      WithBasePath(scratch.arena, Str8Lit("assets/models/cube.glb"));
  if (LoadGlb(state->perm_arena, device, enemy_path, enemy_model))
    state->enemy_model = enemy_model;
  else
    SDL_Log("Failed to load model: %s", enemy_path.data);

  auto *bullet_model = Push<frag::Model>(state->perm_arena);
  auto bullet_path =
      WithBasePath(scratch.arena, Str8Lit("assets/models/bullet.glb"));
  if (!LoadGlb(state->perm_arena, device, bullet_path, bullet_model)) {
    SDL_Log("Failed to load model: %s", bullet_path.data);
    return;
  }

  state->bullet_model = bullet_model;
  glm::vec3 extent = bullet_model->bounds.max - bullet_model->bounds.min;
  f32 longest = glm::max(extent.x, glm::max(extent.y, extent.z));
  f32 scale = longest > 0.0001f ? BULLET_LENGTH / longest : 1.f;
  state->bullet_scale = glm::vec3(scale);

  state->bullet_colliders = NewArray<frag::AABB>(state->perm_arena, 1);
  frag::AABB col{
      .min = bullet_model->bounds.min * scale,
      .max = bullet_model->bounds.max * scale,
  };
  glm::vec3 pad(0.02f);
  col.min -= pad;
  col.max += pad;
  state->bullet_colliders.data[0] = col;
}

priv b32 LoadSound(Sound *sound, String8 relative) {
  auto scratch = Scratch();
  auto path = WithBasePath(scratch.arena, relative);
  if (!SDL_LoadWAV((char *)path.data, &sound->spec, &sound->buf, &sound->len)) {
    SDL_Log("Failed to load sound: %s", path.data);
    return false;
  }
  return true;
}

priv void FreeSound(Sound *sound) {
  if (sound->buf)
    SDL_free(sound->buf);
  *sound = {};
}

priv void InitAudio(State *state) {
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    SDL_Log("Failed to init audio: %s", SDL_GetError());
    return;
  }

  state->audio_device =
      SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
  if (!state->audio_device) {
    SDL_Log("Failed to open audio device: %s", SDL_GetError());
    return;
  }
  SDL_ResumeAudioDevice(state->audio_device);

  LoadSound(&state->shoot, Str8Lit("assets/sound/shoot.wav"));
  LoadSound(&state->jump, Str8Lit("assets/sound/jump.wav"));
}

void Init(State *state) {
  state->cam_pitch = 0.0f;
  state->cam_yaw = 3.14159265f;
  state->bullet_scale = glm::vec3(1.f);

  InitAudio(state);
  LoadSharedModels(state);

  auto scratch = Scratch();
  SetMap(state, WithBasePath(scratch.arena, Str8Lit("assets/maps/test_map.map")));

  auto &player = frag::Get(state->things, state->map_refs.player);
  state->cam_pos = player.pos + glm::vec3(0.f, VIEW_HEIGHT, 0.f);
  state->cam_yaw = player.rot.y;
}

void Shutdown(State *state) {
  for (s32 i = 0; i < MAX_SOUND_VOICES; i++) {
    if (state->voices[i]) {
      SDL_DestroyAudioStream(state->voices[i]);
      state->voices[i] = nullptr;
    }
  }
  FreeSound(&state->shoot);
  FreeSound(&state->jump);
  if (state->audio_device) {
    SDL_CloseAudioDevice(state->audio_device);
    state->audio_device = 0;
  }
}

void Update(State *state, f32 dt) {
  Movement(state, dt);
  PlayerShoot(state, dt);
  UpdateEnemies(state, dt);
  UpdateProjectiles(state, dt);
  SweepVoices(state);
}

void HandleEvent(State *state, SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    if (event->window.windowID == SDL_GetWindowID(state->renderer->window)) {
      u32 w = (u32)event->window.data1;
      u32 h = (u32)event->window.data2;
      if (frag::Resize(state->renderer, w, h)) {
        state->proj_mat = glm::perspectiveRH_ZO(glm::radians(FOV),
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
