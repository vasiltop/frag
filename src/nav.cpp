#include "nav.h"
#include "base/mem.h"
#include <cmath>
#include <cstdlib>

namespace frag {

priv b32 InBounds(NavGrid *g, s32 x, s32 z) {
  return x >= 0 && z >= 0 && x < g->w && z < g->h;
}

priv b32 Walkable(NavGrid *g, s32 x, s32 z) {
  return InBounds(g, x, z) && !g->blocked[z * g->w + x];
}

priv s32 CellIndex(NavGrid *g, s32 x, s32 z) { return z * g->w + x; }

priv glm::ivec2 WorldToCell(NavGrid *g, glm::vec3 p) {
  s32 x = (s32)floorf((p.x - g->origin.x) / g->cell);
  s32 z = (s32)floorf((p.z - g->origin.z) / g->cell);
  return {Clamp(0, x, g->w - 1), Clamp(0, z, g->h - 1)};
}

priv glm::vec3 CellCenter(NavGrid *g, s32 x, s32 z) {
  return {g->origin.x + (x + 0.5f) * g->cell, 0.f,
          g->origin.z + (z + 0.5f) * g->cell};
}

priv glm::ivec2 SnapWalkable(NavGrid *g, s32 x, s32 z) {
  if (Walkable(g, x, z))
    return {x, z};

  s32 max_r = Max(g->w, g->h);
  for (s32 r = 1; r < max_r; r++) {
    glm::ivec2 best{x, z};
    s32 best_d = 0x7fffffff;
    b32 found = false;
    for (s32 dz = -r; dz <= r; dz++) {
      for (s32 dx = -r; dx <= r; dx++) {
        if (Max(abs(dx), abs(dz)) != r)
          continue;
        s32 nx = x + dx;
        s32 nz = z + dz;
        if (!Walkable(g, nx, nz))
          continue;
        s32 d = dx * dx + dz * dz;
        if (d < best_d) {
          best_d = d;
          best = {nx, nz};
          found = true;
        }
      }
    }
    if (found)
      return best;
  }
  return {x, z};
}

NavGrid BuildNavGrid(Arena *arena, Array<AABB> colliders, glm::vec3 offset) {
  NavGrid grid{};
  if (!colliders.size)
    return grid;

  AABB bounds{};
  for (s32 i = 0; i < colliders.size; i++) {
    AABB box{
        .min = colliders.data[i].min + offset,
        .max = colliders.data[i].max + offset,
    };
    bounds.min = glm::min(bounds.min, box.min);
    bounds.max = glm::max(bounds.max, box.max);
  }

  f32 pad = 0.4f;
  bounds.min.x -= pad;
  bounds.max.x += pad;
  bounds.min.z -= pad;
  bounds.max.z += pad;

  f32 cell = NAV_CELL;
  f32 extent_x = bounds.max.x - bounds.min.x;
  f32 extent_z = bounds.max.z - bounds.min.z;
  s32 w = Max(1, (s32)ceilf(extent_x / cell));
  s32 h = Max(1, (s32)ceilf(extent_z / cell));
  if (w > NAV_MAX_DIM || h > NAV_MAX_DIM) {
    cell = Max(extent_x / (f32)NAV_MAX_DIM, extent_z / (f32)NAV_MAX_DIM);
    w = Max(1, (s32)ceilf(extent_x / cell));
    h = Max(1, (s32)ceilf(extent_z / cell));
  }

  grid.origin = {bounds.min.x, 0.f, bounds.min.z};
  grid.cell = cell;
  grid.w = w;
  grid.h = h;
  grid.blocked = PushCount<u8>(arena, (u64)w * (u64)h);

  for (s32 z = 0; z < h; z++) {
    for (s32 x = 0; x < w; x++) {
      glm::vec3 c = CellCenter(&grid, x, z);
      AABB probe{
          .min = {c.x - NAV_PROBE_RADIUS, NAV_PROBE_MIN_Y,
                  c.z - NAV_PROBE_RADIUS},
          .max = {c.x + NAV_PROBE_RADIUS, NAV_PROBE_MAX_Y,
                  c.z + NAV_PROBE_RADIUS},
      };
      u8 blocked = 0;
      for (s32 i = 0; i < colliders.size; i++) {
        AABB box{
            .min = colliders.data[i].min + offset,
            .max = colliders.data[i].max + offset,
        };
        if (Collision(probe, box)) {
          blocked = 1;
          break;
        }
      }
      grid.blocked[z * w + x] = blocked;
    }
  }

  return grid;
}

struct OpenItem {
  s32 idx;
  f32 g;
  f32 f;
};

priv void HeapPush(OpenItem *heap, s32 *n, s32 cap, OpenItem item) {
  if (*n >= cap)
    return;
  s32 i = (*n)++;
  heap[i] = item;
  while (i > 0) {
    s32 p = (i - 1) / 2;
    if (heap[p].f <= heap[i].f)
      break;
    OpenItem tmp = heap[p];
    heap[p] = heap[i];
    heap[i] = tmp;
    i = p;
  }
}

priv OpenItem HeapPop(OpenItem *heap, s32 *n) {
  OpenItem top = heap[0];
  heap[0] = heap[--(*n)];
  s32 i = 0;
  while (true) {
    s32 l = i * 2 + 1;
    s32 r = l + 1;
    s32 smallest = i;
    if (l < *n && heap[l].f < heap[smallest].f)
      smallest = l;
    if (r < *n && heap[r].f < heap[smallest].f)
      smallest = r;
    if (smallest == i)
      break;
    OpenItem tmp = heap[i];
    heap[i] = heap[smallest];
    heap[smallest] = tmp;
    i = smallest;
  }
  return top;
}

s32 FindPath(NavGrid *grid, glm::vec3 from, glm::vec3 to, glm::vec3 *out,
             s32 out_cap) {
  if (!grid || !grid->blocked || grid->w <= 0 || grid->h <= 0 || !out ||
      out_cap <= 0)
    return 0;

  glm::ivec2 start_raw = WorldToCell(grid, from);
  glm::ivec2 goal_raw = WorldToCell(grid, to);
  glm::ivec2 start = SnapWalkable(grid, start_raw.x, start_raw.y);
  glm::ivec2 goal = SnapWalkable(grid, goal_raw.x, goal_raw.y);
  if (!Walkable(grid, start.x, start.y) || !Walkable(grid, goal.x, goal.y))
    return 0;

  if (start.x == goal.x && start.y == goal.y) {
    out[0] = CellCenter(grid, goal.x, goal.y);
    return 1;
  }

  s32 ncells = grid->w * grid->h;
  auto scratch = Scratch();
  auto *g_score = PushCount<f32>(scratch.arena, ncells);
  auto *came = PushCount<s32>(scratch.arena, ncells);
  s32 heap_cap = ncells * 8;
  auto *heap = PushCount<OpenItem>(scratch.arena, heap_cap);
  for (s32 i = 0; i < ncells; i++) {
    g_score[i] = FLT_MAX;
    came[i] = -1;
  }

  s32 start_i = CellIndex(grid, start.x, start.y);
  s32 goal_i = CellIndex(grid, goal.x, goal.y);
  g_score[start_i] = 0.f;
  auto Heur = [&](s32 x, s32 z) {
    f32 dx = (f32)(x - goal.x);
    f32 dz = (f32)(z - goal.y);
    return sqrtf(dx * dx + dz * dz);
  };

  s32 heap_n = 0;
  HeapPush(heap, &heap_n, heap_cap,
           {.idx = start_i, .g = 0.f, .f = Heur(start.x, start.y)});

  const s32 dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  const s32 dz[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

  b32 found = false;
  while (heap_n) {
    OpenItem cur = HeapPop(heap, &heap_n);
    if (cur.g > g_score[cur.idx])
      continue;
    if (cur.idx == goal_i) {
      found = true;
      break;
    }

    s32 cx = cur.idx % grid->w;
    s32 cz = cur.idx / grid->w;
    for (s32 k = 0; k < 8; k++) {
      s32 nx = cx + dx[k];
      s32 nz = cz + dz[k];
      if (!Walkable(grid, nx, nz))
        continue;
      if (dx[k] && dz[k] &&
          (!Walkable(grid, cx + dx[k], cz) || !Walkable(grid, cx, cz + dz[k])))
        continue;

      f32 step = (dx[k] && dz[k]) ? 1.41421356f : 1.f;
      f32 ng = cur.g + step;
      s32 ni = CellIndex(grid, nx, nz);
      if (ng >= g_score[ni])
        continue;
      g_score[ni] = ng;
      came[ni] = cur.idx;
      HeapPush(heap, &heap_n, heap_cap,
               {.idx = ni, .g = ng, .f = ng + Heur(nx, nz)});
    }
  }

  if (!found)
    return 0;

  s32 path_len = 0;
  for (s32 i = goal_i; i >= 0 && path_len < ncells; i = came[i])
    path_len++;

  auto *stack = PushCount<s32>(scratch.arena, path_len);
  s32 s = 0;
  for (s32 i = goal_i; i >= 0 && s < path_len; i = came[i])
    stack[s++] = i;

  s32 n = Min(path_len, out_cap);
  for (s32 i = 0; i < n; i++) {
    s32 cell = stack[path_len - 1 - i];
    s32 x = cell % grid->w;
    s32 z = cell / grid->w;
    out[i] = CellCenter(grid, x, z);
  }
  return n;
}

} // namespace frag
