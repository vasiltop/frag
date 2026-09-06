#include "test.h"
#include "base/mem.h"
#include "map.h"

TEST(map_loads_test_map) {
  Arena *arena = ArenaAlloc();
  defer { Release(arena); };

  frag::Map map{};
  String8 path = Str8C(FRAG_SOURCE_DIR "/assets/maps/test_map.map");
  EXPECT(frag::LoadMap(arena, path, &map));

  EXPECT(map.entities.size == 1);

  frag::Entity &entity = map.entities.data[0];
  EXPECT(Eq(entity.classname, Str8Lit("worldspawn")));
  EXPECT(entity.brushes.size == 2);

  for (s32 i = 0; i < entity.brushes.size; i++)
    EXPECT(entity.brushes.data[i].faces.size == 6);

  EXPECT(Eq(entity.brushes.data[0].faces.data[0].tex_name, Str8Lit("test/tex")));
  EXPECT(Eq(entity.brushes.data[1].faces.data[0].tex_name,
            Str8Lit("__TB_empty")));
}

TEST(map_load_missing_file_fails) {
  Arena *arena = ArenaAlloc();
  defer { Release(arena); };

  frag::Map map{};
  EXPECT(!frag::LoadMap(arena, Str8Lit("./no_such_map.map"), &map));
}

void run_map_tests() {
  printf("map tests\n");

  RUN_TEST(map_loads_test_map);
  RUN_TEST(map_load_missing_file_fails);
}
