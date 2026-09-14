#include "thing.h"
#include "map.h"
#include <SDL3/SDL.h>

namespace frag {

priv s32 Deref(Things *things, Ref ref) {
  if (ref.idx > 0 && ref.idx < MAX_THINGS && things->used[ref.idx] &&
      things->gen[ref.idx] == ref.gen) {
    return ref.idx;
  }

  return 0;
}

priv String8 EntityProperty(Entity *entity, String8 key) {
  for (s32 i = 0; i < entity->properties.size; i++) {
    if (Eq(entity->properties.data[i].key, key))
      return entity->properties.data[i].value;
  }

  return {};
}

void Init(Things *things) {
  things->first_free = 1;
  things->first_used = 0;

  for (s32 i = 1; i < MAX_THINGS - 1; ++i) {
    things->next_free[i] = i + 1;
    things->used[i] = false;
  }

  things->next_free[MAX_THINGS - 1] = 0;
}

Ref Add(Things *things) {
  s32 slot = things->first_free;

  if (slot) {
    things->slots[slot] = {};
    things->slots[slot].scale = glm::vec3(1.0f);
    things->used[slot] = true;
    things->gen[slot]++;
    things->first_free = things->next_free[slot];
    things->next_used[slot] = things->first_used;
    things->first_used = slot;
  }

  return Ref{.idx = slot, .gen = things->gen[slot]};
}

Thing &Get(Things *things, Ref ref) {
  return things->slots[Deref(things, ref)];
}

Ref MakeRef(Things *things, s32 idx) {
  return {.idx = idx, .gen = things->gen[idx]};
}

void Rem(Things *things, Ref ref) {
  if (s32 slot = Deref(things, ref)) {
    things->used[slot] = false;

    s32 *cur = &things->first_used;
    while (*cur) {
      if (*cur == slot) {
        *cur = things->next_used[slot];
        break;
      }
      cur = &things->next_used[*cur];
    }

    things->next_free[slot] = things->first_free;
    things->first_free = slot;
  }
}

MapRefs PopulateThingsFromMap(Arena *arena, SDL_GPUDevice *device,
                              Things *things, Map *map, Model *enemy_model) {
  MapRefs refs{};
  Array<AABB> enemy_colliders{};
  if (enemy_model) {
    enemy_colliders = NewArray<AABB>(arena, 1);
    enemy_colliders.data[0] = enemy_model->bounds;
  }

  for (auto &entity : map->entities) {
    auto ref = Add(things);
    auto &thing = Get(things, ref);

    thing.kind = ThingKind::Nil;
    if (Eq(entity.classname, Str8Lit("worldspawn")))
      thing.kind = ThingKind::Map;
    if (Eq(entity.classname, Str8Lit("info_player_start")))
      thing.kind = ThingKind::Player;
    if (Eq(entity.classname, Str8Lit("info_enemy_spawn")))
      thing.kind = ThingKind::Enemy;

    if (thing.kind == ThingKind::Map) {
      refs.map = ref;
      thing.model = Push<Model>(arena);
      thing.scale = glm::vec3(MAP_SCALE, MAP_SCALE, MAP_SCALE);
      BuildMapModel(arena, device, &entity, thing.model, &thing.colliders);
    } else if (thing.kind == ThingKind::Player) {
      refs.player = ref;

      thing.colliders = NewArray<AABB>(arena, 1);
      thing.colliders.data[0] = QuakeToEngine(
          AABB{.min = {-16.f, -16.f, -24.f}, .max = {16.f, 16.f, 32.f}},
          MAP_SCALE);
    } else if (thing.kind == ThingKind::Enemy) {
      if (enemy_model) {
        thing.model = enemy_model;
        thing.colliders = enemy_colliders;
      }
    }

    if (String8 origin = EntityProperty(&entity, Str8Lit("origin"));
        origin.size)
      thing.pos = QuakeToEngine(ParseMapVec3(origin), MAP_SCALE);
      
    if (String8 angles = EntityProperty(&entity, Str8Lit("angles"));
        angles.size)
      thing.rot = ParseMapAngles(angles);
    else if (String8 angle = EntityProperty(&entity, Str8Lit("angle"));
             angle.size)
      thing.rot.y = ParseMapAngles(angle).x;
  }

  return refs;
}

b32 Collision(Thing &a, Thing &b) {
  auto a_cols = a.colliders;
  auto b_cols = b.colliders;

  for (auto &a_col : a_cols) {
    AABB a_offset{
        .min = a_col.min + a.pos,
        .max = a_col.max + a.pos,
    };
    for (auto &b_col : b_cols) {
      AABB b_offset{
          .min = b_col.min + b.pos,
          .max = b_col.max + b.pos,
      };
      if (Collision(a_offset, b_offset))
        return true;
    }
  }

  return false;
}

} // namespace frag
