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

  EXPECT(map.entities.size == 1);

  frag::Entity &entity = map.entities.data[0];
  EXPECT(Eq(entity.classname, Str8Lit("worldspawn")));
  EXPECT(entity.brushes.size == 2);

  for (s32 i = 0; i < entity.brushes.size; i++)
    EXPECT(entity.brushes.data[i].faces.size == 6);

  EXPECT(Eq(entity.brushes.data[0].faces.data[0].tex_name, Str8Lit("test/tex")));
  EXPECT(Eq(entity.brushes.data[1].faces.data[0].tex_name,
            Str8Lit("__TB_empty")));

  EXPECT(Near(entity.brushes.data[0].faces.data[0].u_scale, 0.5f));
  EXPECT(Near(entity.brushes.data[0].faces.data[0].v_scale, 1.0f));
  EXPECT(Near(entity.brushes.data[1].faces.data[0].u_scale, 1.0f));
  EXPECT(Near(entity.brushes.data[1].faces.data[0].v_scale, 1.0f));
}

TEST(get_brush_aabb_test_map_brush0) {
  auto scratch = Scratch();

  frag::Map map{};
  String8 path = Str8C(FRAG_SOURCE_DIR "/assets/maps/test_map.map");
  EXPECT(frag::LoadMap(scratch.arena, path, &map));

  frag::AABB box = frag::GetBrushAABB(map.entities.data[0].brushes.data[0]);
  EXPECT(Near(box.min.x, 32.0f));
  EXPECT(Near(box.max.x, 160.0f));
  EXPECT(Near(box.min.y, -112.0f));
  EXPECT(Near(box.max.y, 80.0f));
  EXPECT(Near(box.min.z, -16.0f));
  EXPECT(Near(box.max.z, 16.0f));
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
  RUN_TEST(map_load_missing_file_fails);
}
