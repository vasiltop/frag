#include "thing.h"
#include "map.h"
#include <SDL3/SDL.h>

namespace frag {

priv s32 Deref(Things *things, Ref ref) {
  if (ref.idx > 0 && ref.idx < max_things && things->used[ref.idx] &&
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

  for (s32 i = 1; i < max_things - 1; ++i) {
    things->next_free[i] = i + 1;
    things->used[i] = false;
  }

  things->next_free[max_things - 1] = 0;
}

Ref Add(Things *things) {
  s32 slot = things->first_free;

  if (slot) {
    things->slots[slot] = {};
    things->slots[slot].scale = glm::vec3(1.0f);
    things->used[slot] = true;
    things->gen[slot]++;
    things->first_free = things->next_free[slot];
  }

  return Ref{.idx = slot, .gen = things->gen[slot]};
}

Thing &Get(Things *things, Ref ref) {
  return things->slots[Deref(things, ref)];
}

void Rem(Things *things, Ref ref) {
  if (s32 slot = Deref(things, ref)) {
    things->used[slot] = false;
    things->next_free[slot] = things->first_free;
    things->first_free = slot;
  }
}

MapRefs PopulateThingsFromMap(Arena *arena, SDL_GPUDevice *device, Things *things, Map *map) {
	MapRefs refs{};
  for (auto &entity : map->entities) {
    auto ref = Add(things);
    auto &thing = Get(things, ref);

    if (Eq(entity.classname, Str8Lit("worldspawn"))) {
			refs.map = ref;
      thing.kind = ThingKind::Map;
			thing.model = Push<Model>(arena);
			thing.scale = glm::vec3(MAP_SCALE, MAP_SCALE, MAP_SCALE);
			BuildMapModel(arena, device, &entity, thing.model);
    } else if (Eq(entity.classname, Str8Lit("info_player_start"))) {
      refs.player = ref;
      thing.kind = ThingKind::Player;

      if (String8 origin = EntityProperty(&entity, Str8Lit("origin")); origin.size)
        thing.pos = QuakeToEngine(ParseMapVec3(origin), MAP_SCALE);

      if (String8 angles = EntityProperty(&entity, Str8Lit("angles")); angles.size)
        thing.rot = ParseMapAngles(angles);
      else if (String8 angle = EntityProperty(&entity, Str8Lit("angle")); angle.size)
        thing.rot.y = ParseMapAngles(angle).x;
    }
  }

	return refs;
}

} // namespace frag
