#include "thing.h"
namespace thing {

priv s32 Deref(Things *things, Ref ref) {
  if (ref.idx > 0 && ref.idx < max_things && things->used[ref.idx] &&
      things->gen[ref.idx] == ref.gen) {
    return ref.idx;
  }

  return 0;
}

void Init(Things *things) {
  things->first_free = 1;

  for (s32 i = 1; i < max_things - 1; ++i) {
    things->next_free[i] = i + 1;
    things->used[i] = false;
  }

  things->next_free[max_things - 1] = 0;
}

Ref Add(Things *things, Kind kind) {
  s32 slot = things->first_free;

  if (slot) {
    things->slots[slot] = {};
    things->slots[slot].kind = kind;
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
}; // namespace thing
