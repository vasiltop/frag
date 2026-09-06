#include "map.h"
#include <cstdlib>

namespace frag {

priv void SkipUntil(MapParser *p, u8 c) {
  while (!Done(p) && Current(p) != c)
    p->pos++;

  if (!Done(p))
    p->pos++;
}

priv void SkipComment(MapParser *p) {
  SkipWhitespace(p);
  if (Current(p) != '/')
    return;

  SkipUntil(p, '\n');
}

priv f32 ParseFloat(MapParser *p) {
  SkipWhitespace(p);
  s32 start = p->pos;

  while (!Done(p) && !IsWhitespace(Current(p)) && Current(p) != ')' &&
         Current(p) != '(') {
    p->pos++;
  }

  char buf[64]{};
  s32 len = Min(p->pos - start, (s32)sizeof(buf) - 1);
  SDL_memcpy(buf, p->content.data + start, len);
  return strtof(buf, nullptr);
}

priv s32 ParseInt(MapParser *p) {
  SkipWhitespace(p);
  s32 start = p->pos;

  while (!Done(p) && !IsWhitespace(Current(p)) && Current(p) != ')' &&
         Current(p) != '(') {
    p->pos++;
  }

  char buf[64]{};
  s32 len = Min(p->pos - start, (s32)sizeof(buf) - 1);
  SDL_memcpy(buf, p->content.data + start, len);
  return (s32)strtol(buf, nullptr, 10);
}

priv glm::vec3 ParseVec3(MapParser *p) {
  return {ParseFloat(p), ParseFloat(p), ParseFloat(p)};
}

priv b32 ParseFace(Arena *arena, MapParser *p, Face *face) {
  if (!Expect(p, '('))
    return false;

  face->a = ParseVec3(p);
  if (!Expect(p, ')'))
    return false;

  if (!Expect(p, '('))
    return false;

  face->b = ParseVec3(p);
  if (!Expect(p, ')'))
    return false;

  if (!Expect(p, '('))
    return false;

  face->c = ParseVec3(p);
  if (!Expect(p, ')'))
    return false;

  face->tex_name = ParseToken(arena, p);
  face->u = ParseInt(p);
  face->v = ParseInt(p);
  face->tex_rot = ParseInt(p);
  face->u_scale = ParseInt(p);
  face->v_scale = ParseInt(p);

  return true;
}

priv b32 ParseBrush(Arena *arena, MapParser *p, Brush *brush) {
  if (!Expect(p, '{'))
    return false;

  brush->faces.data = PushCount<Face>(arena, MAX_FACES);
  brush->faces.size = 0;

  while (true) {
    SkipWhitespace(p);
    SkipComment(p);

    if (Done(p))
      return false;

    if (Current(p) == '}') {
      p->pos++;
      return true;
    }

    if (brush->faces.size >= MAX_FACES)
      return false;

    if (!ParseFace(arena, p, &brush->faces.data[brush->faces.size]))
      return false;

    brush->faces.size++;
  }
}

priv b32 ParseEntity(Arena *arena, MapParser *p, Entity *entity) {
  if (!Expect(p, '{'))
    return false;

  entity->brushes.data = PushCount<Brush>(arena, MAX_BRUSHES);
  entity->brushes.size = 0;

  while (true) {
    SkipWhitespace(p);
    SkipComment(p);

    if (Done(p))
      return false;

    u8 c = Current(p);
    if (c == '}') {
      p->pos++;
      return true;
    }

    if (c == '"') {
      String8 key = ParseQuoted(arena, p);
      String8 val = ParseQuoted(arena, p);
      if (Eq(key, Str8Lit("classname")))
        entity->classname = val;
      continue;
    }

    if (c == '{') {
      if (entity->brushes.size >= MAX_BRUSHES)
        return false;

      if (!ParseBrush(arena, p, &entity->brushes.data[entity->brushes.size]))
        return false;

      entity->brushes.size++;
      continue;
    }

    return false;
  }
}

priv b32 ParseMap(Arena *arena, MapParser *p, Map *result) {
  Entity *entities = PushCount<Entity>(arena, MAX_ENTITIES);
  s32 entity_count = 0;

  SkipComment(p);
  SkipComment(p);
  SkipComment(p);

  while (!Done(p)) {
    SkipWhitespace(p);
    SkipComment(p);

    if (Done(p))
      break;

    if (entity_count >= MAX_ENTITIES)
      return false;

    if (!ParseEntity(arena, p, &entities[entity_count]))
      return false;

    entity_count++;
  }

  result->entities = {entities, entity_count};
  return true;
}

b32 LoadMap(Arena *arena, String8 filename, Map *result) {
  size_t file_size{};
  void *data = SDL_LoadFile((char *)filename.data, &file_size);
  if (!data)
    return false;

  defer { SDL_free(data); };

  String8 content{.data = (u8 *)data, .size = (s32)file_size};

  MapParser parser{.content = content};
  return ParseMap(arena, &parser, result);
}

}; // namespace frag
