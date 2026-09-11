#include "test.h"
#include "base/mem.h"
#include "map.h"
#include <cfloat>
#include <cmath>

priv b32 Near(f32 a, f32 b, f32 eps = 0.001f) {
  return std::abs(a - b) < eps;
}

priv void ExpectAABB(frag::AABB box, f32 min_x, f32 min_y, f32 min_z, f32 max_x,
                     f32 max_y, f32 max_z) {
  EXPECT(Near(box.min.x, min_x));
  EXPECT(Near(box.min.y, min_y));
  EXPECT(Near(box.min.z, min_z));
  EXPECT(Near(box.max.x, max_x));
  EXPECT(Near(box.max.y, max_y));
  EXPECT(Near(box.max.z, max_z));
}

priv frag::Brush MakeBoxBrush(Arena *arena, glm::vec3 mn, glm::vec3 mx) {
  // Inward-facing normals: +X min-x, -X max-x, +Y min-y, -Y max-y, +Z min-z, -Z max-z
  frag::Face faces[6] = {
      {{mn.x, mn.y, mn.z}, {mn.x, mx.y, mn.z}, {mn.x, mn.y, mx.z}, {}, 0, 0, 0,
       1, 1},
      {{mx.x, mn.y, mn.z}, {mx.x, mn.y, mx.z}, {mx.x, mx.y, mn.z}, {}, 0, 0, 0,
       1, 1},
      {{mn.x, mn.y, mn.z}, {mn.x, mn.y, mx.z}, {mx.x, mn.y, mn.z}, {}, 0, 0, 0,
       1, 1},
      {{mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z}, {mn.x, mx.y, mx.z}, {}, 0, 0, 0,
       1, 1},
      {{mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mn.x, mx.y, mn.z}, {}, 0, 0, 0,
       1, 1},
      {{mx.x, mn.y, mx.z}, {mn.x, mn.y, mx.z}, {mx.x, mx.y, mx.z}, {}, 0, 0, 0,
       1, 1},
  };

  frag::Brush brush{};
  brush.faces = NewArray<frag::Face>(arena, 6);
  for (s32 i = 0; i < 6; i++)
    brush.faces.data[i] = faces[i];
  return brush;
}

TEST(get_brush_aabb_empty_brush) {
  frag::Brush brush{};
  brush.faces = {nullptr, 0};

  frag::AABB box = frag::GetBrushAABB(brush);

  EXPECT(Near(box.min.x, FLT_MAX));
  EXPECT(Near(box.min.y, FLT_MAX));
  EXPECT(Near(box.min.z, FLT_MAX));
  EXPECT(Near(box.max.x, -FLT_MAX));
  EXPECT(Near(box.max.y, -FLT_MAX));
  EXPECT(Near(box.max.z, -FLT_MAX));
}

TEST(get_brush_aabb_unit_cube) {
  auto scratch = Scratch();

  frag::Brush brush = MakeBoxBrush(scratch.arena, {0.0f, 0.0f, 0.0f},
                                   {1.0f, 1.0f, 1.0f});
  frag::AABB box = frag::GetBrushAABB(brush);

  ExpectAABB(box, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

TEST(get_brush_aabb_offset_box) {
  auto scratch = Scratch();

  frag::Brush brush = MakeBoxBrush(scratch.arena, {-64.0f, -64.0f, -16.0f},
                                   {64.0f, 64.0f, 16.0f});
  frag::AABB box = frag::GetBrushAABB(brush);

  ExpectAABB(box, -64.0f, -64.0f, -16.0f, 64.0f, 64.0f, 16.0f);
}

TEST(get_brush_aabb_thin_box) {
  auto scratch = Scratch();

  frag::Brush brush = MakeBoxBrush(scratch.arena, {0.0f, 0.0f, 0.0f},
                                   {10.0f, 2.0f, 5.0f});
  frag::AABB box = frag::GetBrushAABB(brush);

  ExpectAABB(box, 0.0f, 0.0f, 0.0f, 10.0f, 2.0f, 5.0f);
}

TEST(map_loads_test_map) {
  auto scratch = Scratch();

  frag::Map map{};
  String8 path = Str8C(FRAG_SOURCE_DIR "/assets/maps/test_map.map");
  EXPECT(frag::LoadMap(scratch.arena, path, &map));

  EXPECT(map.entities.size == 2);

  frag::Entity &worldspawn = map.entities.data[0];
  EXPECT(Eq(worldspawn.classname, Str8Lit("worldspawn")));
  EXPECT(worldspawn.brushes.size == 6);

  for (s32 i = 0; i < worldspawn.brushes.size; i++)
    EXPECT(worldspawn.brushes.data[i].faces.size == 6);

  EXPECT(Eq(worldspawn.brushes.data[0].faces.data[0].tex_name,
            Str8Lit("test/stone")));

  frag::Entity &player_start = map.entities.data[1];
  EXPECT(Eq(player_start.classname, Str8Lit("info_player_start")));
  EXPECT(player_start.brushes.size == 0);
  EXPECT(player_start.properties.size == 2);
  EXPECT(Eq(player_start.properties.data[0].key, Str8Lit("classname")));
  EXPECT(Eq(player_start.properties.data[0].value,
            Str8Lit("info_player_start")));
  EXPECT(Eq(player_start.properties.data[1].key, Str8Lit("origin")));
  EXPECT(Eq(player_start.properties.data[1].value, Str8Lit("-336 -64 40")));
}

TEST(get_brush_aabb_test_map_brush0) {
  auto scratch = Scratch();

  frag::Map map{};
  String8 path = Str8C(FRAG_SOURCE_DIR "/assets/maps/test_map.map");
  EXPECT(frag::LoadMap(scratch.arena, path, &map));

  frag::AABB box = frag::GetBrushAABB(map.entities.data[0].brushes.data[0]);
  EXPECT(Near(box.min.x, -432.0f));
  EXPECT(Near(box.max.x, 160.0f));
  EXPECT(Near(box.min.y, -224.0f));
  EXPECT(Near(box.max.y, 80.0f));
  EXPECT(Near(box.min.z, -16.0f));
  EXPECT(Near(box.max.z, 16.0f));
}

TEST(map_parses_origin_vec3) {
  auto scratch = Scratch();

  frag::Map map{};
  String8 path = Str8C(FRAG_SOURCE_DIR "/assets/maps/test_map.map");
  EXPECT(frag::LoadMap(scratch.arena, path, &map));

  frag::Entity &player_start = map.entities.data[1];
  String8 origin = player_start.properties.data[1].value;
  glm::vec3 pos = frag::ParseMapVec3(origin);

  EXPECT(Near(pos.x, -336.0f));
  EXPECT(Near(pos.y, -64.0f));
  EXPECT(Near(pos.z, 40.0f));
}

TEST(quake_to_engine_swizzles_yzx) {
  frag::AABB quake_box{
      .min = {-432.0f, -224.0f, -16.0f},
      .max = {160.0f, 80.0f, 16.0f},
  };
  frag::AABB box = frag::QuakeToEngine(quake_box, 0.01f);

  EXPECT(Near(box.min.x, -2.24f));
  EXPECT(Near(box.max.x, 0.80f));
  EXPECT(Near(box.min.y, -0.16f));
  EXPECT(Near(box.max.y, 0.16f));
  EXPECT(Near(box.min.z, -4.32f));
  EXPECT(Near(box.max.z, 1.60f));
}

TEST(map_load_missing_file_fails) {
  auto scratch = Scratch();

  frag::Map map{};
  EXPECT(!frag::LoadMap(scratch.arena, Str8Lit("./no_such_map.map"), &map));
}

void run_map_tests() {
  printf("map tests\n");

  RUN_TEST(get_brush_aabb_empty_brush);
  RUN_TEST(get_brush_aabb_unit_cube);
  RUN_TEST(get_brush_aabb_offset_box);
  RUN_TEST(get_brush_aabb_thin_box);
  RUN_TEST(map_loads_test_map);
  RUN_TEST(get_brush_aabb_test_map_brush0);
  RUN_TEST(map_parses_origin_vec3);
  RUN_TEST(quake_to_engine_swizzles_yzx);
  RUN_TEST(map_load_missing_file_fails);
}
