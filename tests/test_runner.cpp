#include "test.h"

void run_arena_tests();
void run_thing_tests();
void run_map_tests();

int main() {
  run_arena_tests();
  run_thing_tests();
  run_map_tests();

  if (g_failed_tests > 0) {
    printf("%d/%d tests failed\n", g_failed_tests, g_test_count);
    return 1;
  }

  printf("%d tests passed\n", g_test_count);
  return 0;
}
