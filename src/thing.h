#include "./base/base.h"
#include <glm/glm.hpp>

constexpr auto max_things = 1024;

namespace thing {

struct Ref {
  s32 idx;
  s32 gen;
};

enum class Kind { Nil, Player };

struct Thing {
  Kind kind;
  glm::vec3 pos;
};

struct Things {
  Thing slots[max_things];
  b32 used[max_things];
  s32 gen[max_things];

  s32 first_free;
  s32 next_free[max_things];
};

void Init(Things *things);
Ref Add(Things *things, Kind kind);
Thing &Get(Things *things, Ref ref);
void Rem(Things *things, Ref ref);

}; // namespace thing
