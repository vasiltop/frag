#include "asset.h"
#include "base/base.h"
#define STB_IMAGE_IMPLEMENTATION
#include "gpu.h"
#include "stb_image.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

namespace frag {

SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, void *data, s32 width,
                            s32 height) {
  SDL_GPUTextureCreateInfo texture_info{
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = (u32)width,
      .height = (u32)height,
      .layer_count_or_depth = 1,
      .num_levels = 1,
  };

  auto *texture = SDL_CreateGPUTexture(device, &texture_info);
  u32 size_bytes = width * height * 4;

  auto transfer = TransferData(device, data, size_bytes);

  if (!transfer)
    return nullptr;

  defer { SDL_ReleaseGPUTransferBuffer(device, transfer); };

  SDL_GPUTextureTransferInfo source_info = {
      .transfer_buffer = transfer,
      .offset = 0,
  };

  SDL_GPUTextureRegion destination_region = {
      .texture = texture,
      .w = (u32)width,
      .h = (u32)height,
      .d = 1,
  };

  auto upload = BeginUpload(device);
  if (!upload.pass)
    return nullptr;

  SDL_UploadToGPUTexture(upload.pass, &source_info, &destination_region, false);
  SDL_EndGPUCopyPass(upload.pass);

  if (!SDL_SubmitGPUCommandBuffer(upload.buf)) {
    SDL_Log("Could not submit GPU command buffer: %s", SDL_GetError());
    return nullptr;
  }

  return texture;
}

SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, String8 filename) {
  s32 tex_width;
  s32 tex_height;
  s32 channels;

  u8 *tex =
      stbi_load((char *)filename.data, &tex_width, &tex_height, &channels, 4);

  if (!tex) {
    SDL_Log("Failed to load texture: %s", stbi_failure_reason());
    return nullptr;
  }

  defer { stbi_image_free(tex); };

  return LoadTexture(device, tex, tex_width, tex_height);
}

b32 LoadGlb(Arena *arena, SDL_GPUDevice *device, String8 filename, Model *out) {
  cgltf_options options{};
  cgltf_data *data{};

  cgltf_result result =
      cgltf_parse_file(&options, (char *)filename.data, &data);
  if (result != cgltf_result_success)
    return false;

  defer { cgltf_free(data); };

  result = cgltf_load_buffers(&options, data, (char *)filename.data);
  if (result != cgltf_result_success)
    return false;

  SDL_GPUTexture *texture = nullptr;

  if (data->images_count > 0) {
    cgltf_image *cgltf_img = &data->images[0];
    u8 *img_data{};
    s32 img_size{};

    if (cgltf_img->buffer_view) {
      auto *view = cgltf_img->buffer_view;
      img_data = (u8 *)view->buffer->data + view->offset;
      img_size = view->size;
    }

    int width, height, channels;
    u8 *tex = stbi_load_from_memory((const stbi_uc *)img_data, (s32)img_size,
                                    &width, &height, &channels, 4);
    if (tex) {
      texture = LoadTexture(device, tex, width, height);
      stbi_image_free(tex);
    }
  }

  if (!texture) {
    texture = LoadTexture(device, Str8Lit("./assets/tex.png"));
  }

  if (data->meshes_count == 0)
    return false;

  cgltf_mesh *cg_mesh = &data->meshes[0];
  cgltf_primitive *prim = &cg_mesh->primitives[0];

  u64 vertex_count = 0;
  u64 index_count = 0;

  for (size_t i = 0; i < prim->attributes_count; ++i) {
    if (prim->attributes[i].type == cgltf_attribute_type_position) {
      vertex_count = prim->attributes[i].data->count;
      break;
    }
  }

  if (prim->indices) {
    index_count = prim->indices->count;
  }

  if (vertex_count == 0)
    return false;

  TempArena scratch = Scratch();
  Vertex *vertices = PushCount<Vertex>(scratch.arena, vertex_count);
  u32 *indices =
      index_count > 0 ? PushCount<u32>(scratch.arena, index_count) : nullptr;

  for (u64 j = 0; j < vertex_count; ++j) {
    vertices[j] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                   1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  }

  for (size_t i = 0; i < prim->attributes_count; ++i) {
    cgltf_attribute *attr = &prim->attributes[i];
    cgltf_accessor *acc = attr->data;

    for (size_t j = 0; j < acc->count; ++j) {
      if (attr->type == cgltf_attribute_type_position) {
        f32 val[3];
        cgltf_accessor_read_float(acc, j, val, 3);
        vertices[j].x = val[0];
        vertices[j].y = val[1];
        vertices[j].z = val[2];
      } else if (attr->type == cgltf_attribute_type_color) {
        f32 val[4];
        cgltf_accessor_read_float(acc, j, val, 4);
        vertices[j].r = val[0];
        vertices[j].g = val[1];
        vertices[j].b = val[2];
        vertices[j].a = val[3];
      } else if (attr->type == cgltf_attribute_type_texcoord) {
        f32 val[2];
        cgltf_accessor_read_float(acc, j, val, 2);
        vertices[j].u = val[0];
        vertices[j].v = val[1];
      } else if (attr->type == cgltf_attribute_type_normal) {
        f32 val[3];
        cgltf_accessor_read_float(acc, j, val, 3);
        vertices[j].nx = val[0];
        vertices[j].ny = val[1];
        vertices[j].nz = val[2];
      }
    }
  }

  if (prim->indices && indices) {
    cgltf_accessor *acc = prim->indices;
    for (size_t j = 0; j < acc->count; ++j) {
      cgltf_uint val;
      cgltf_accessor_read_uint(acc, j, &val, 1);
      indices[j] = static_cast<u32>(val);
    }
  }

  out->vertex_buffer = CreateVertexBuffer(device, {vertices, vertex_count});
  out->index_buffer = index_count > 0
                          ? CreateIndexBuffer(device, {indices, index_count})
                          : nullptr;
  out->sub_meshes = NewArray<SubMesh>(arena, 1);
  out->sub_meshes[0] = {texture, 0, (u32)index_count};

  return true;
}

priv String8 MapTexturePath(Arena *arena, String8 tex_name) {
  if (tex_name.size == 0 || Eq(tex_name, Str8Lit("__TB_empty")))
    return Str8Lit("./assets/textures/test/tex.png");
  return Cat(arena, Cat(arena, Str8Lit("./assets/textures/"), tex_name),
              Str8Lit(".png"));
}

priv void TextureDimensions(Arena *arena, String8 tex_name, s32 *width,
                            s32 *height) {
  *width = 128;
  *height = 128;
  auto path = MapTexturePath(arena, tex_name);
  stbi_info((char *)path.data, width, height, nullptr);
}

priv void QuakeTextureAxes(glm::vec3 normal, glm::vec3 *u_axis,
                           glm::vec3 *v_axis) {
  static const glm::vec3 baseaxis[] = {
      {0, 0, 1},  {1, 0, 0},  {0, -1, 0}, // floor
      {0, 0, -1}, {1, 0, 0},  {0, -1, 0}, // ceiling
      {1, 0, 0},  {0, 1, 0},  {0, 0, -1}, // west wall
      {-1, 0, 0}, {0, 1, 0},  {0, 0, -1}, // east wall
      {0, 1, 0},  {1, 0, 0},  {0, 0, -1}, // south wall
      {0, -1, 0}, {1, 0, 0},  {0, 0, -1}, // north wall
  };

  s32 bestaxis = 0;
  f32 best = 0.0f;
  for (s32 i = 0; i < 6; i++) {
    f32 d = glm::dot(normal, baseaxis[i * 3]);
    if (d > best) {
      best = d;
      bestaxis = i;
    }
  }

  *u_axis = baseaxis[bestaxis * 3 + 1];
  *v_axis = baseaxis[bestaxis * 3 + 2];
}

priv glm::vec2 MapTexCoord(glm::vec3 pos, Face face, s32 tex_width,
                            s32 tex_height) {
  glm::vec3 n = FaceNormal(face);
  glm::vec3 u_axis, v_axis;
  QuakeTextureAxes(n, &u_axis, &v_axis);

  if (face.tex_rot != 0) {
    f32 sinv, cosv;
    if (face.tex_rot == 90) {
      sinv = 1.0f;
      cosv = 0.0f;
    } else if (face.tex_rot == 180) {
      sinv = 0.0f;
      cosv = -1.0f;
    } else if (face.tex_rot == 270) {
      sinv = -1.0f;
      cosv = 0.0f;
    } else {
      f32 rad = glm::radians((f32)face.tex_rot);
      sinv = sinf(rad);
      cosv = cosf(rad);
    }

    glm::vec3 vecs[2] = {u_axis, v_axis};
    s32 sv = vecs[0].x ? 0 : vecs[0].y ? 1 : 2;
    s32 tv = vecs[1].x ? 0 : vecs[1].y ? 1 : 2;

    for (s32 i = 0; i < 2; i++) {
      f32 ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
      f32 nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
      vecs[i][sv] = ns;
      vecs[i][tv] = nt;
    }

    u_axis = vecs[0];
    v_axis = vecs[1];
  }

  f32 u_scale = face.u_scale != 0.0f ? face.u_scale : 1.0f;
  f32 v_scale = face.v_scale != 0.0f ? face.v_scale : 1.0f;
  if (u_scale < 0.0f) {
    u_axis = -u_axis;
    u_scale = -u_scale;
  }
  if (v_scale < 0.0f) {
    v_axis = -v_axis;
    v_scale = -v_scale;
  }

  f32 s = glm::dot(pos, u_axis) / u_scale + (f32)face.u;
  f32 t = glm::dot(pos, v_axis) / v_scale + (f32)face.v;

  return {s / (f32)tex_width, t / (f32)tex_height};
}

priv Face BrushFaceForNormal(Brush brush, glm::vec3 normal) {
  Face result = brush.faces.size > 0 ? brush.faces[0] : Face{};
  f32 best = -1.0f;
  // Map face normals point inward; box face normals point outward.
  glm::vec3 query = -normal;

  for (s32 i = 0; i < brush.faces.size; i++) {
    f32 d = glm::dot(FaceNormal(brush.faces[i]), query);
    if (d > best) {
      best = d;
      result = brush.faces[i];
    }
  }

  return result;
}

priv SDL_GPUTexture *LoadMapTexture(SDL_GPUDevice *device, Arena *arena,
                                    String8 tex_name) {
  return LoadTexture(device, MapTexturePath(arena, tex_name));
}

void BuildFaceGeometry(Array<Vertex> vertices, Array<u32> indices, glm::vec3 p0,
                       glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
                       glm::vec3 normal, Face face, s32 tex_width,
                       s32 tex_height, s32 face_idx) {

  s32 vertex_idx = face_idx * vertices_per_face;
  s32 index_idx = face_idx * indices_per_face;

  glm::vec3 quake_points[4] = {p0, p1, p2, p3};

  for (s32 i = 0; i < 4; i++) {
    glm::vec3 pos = {quake_points[i].y, quake_points[i].z, quake_points[i].x};
    glm::vec2 uv = MapTexCoord(quake_points[i], face, tex_width, tex_height);

    vertices[vertex_idx + i] = Vertex{
        pos.x, pos.y, pos.z, 1.0f, 1.0f, 1.0f, 1.0f,
        uv.x,  uv.y,  normal.x, normal.y, normal.z};
  }

  indices[index_idx + 0] = vertex_idx + 0;
  indices[index_idx + 1] = vertex_idx + 1;
  indices[index_idx + 2] = vertex_idx + 2;

  indices[index_idx + 3] = vertex_idx + 0;
  indices[index_idx + 4] = vertex_idx + 2;
  indices[index_idx + 5] = vertex_idx + 3;
}

priv void BuildBrushFace(Array<Vertex> vertices, Array<u32> indices, Brush brush,
                         SDL_GPUDevice *device, Arena *arena,
                         glm::vec3 p0, glm::vec3 p1, glm::vec3 p2,
                         glm::vec3 p3, glm::vec3 normal, SubMesh *sub_meshes,
                         s32 *sub_mesh_count, s32 *face_idx) {
  Face face = BrushFaceForNormal(brush, normal);
  s32 tex_width, tex_height;
  TextureDimensions(arena, face.tex_name, &tex_width, &tex_height);

  u32 index_offset = (u32)(*face_idx * indices_per_face);
  BuildFaceGeometry(vertices, indices, p0, p1, p2, p3, normal, face, tex_width,
                    tex_height, (*face_idx)++);

  sub_meshes[(*sub_mesh_count)++] = SubMesh{
      .texture = LoadMapTexture(device, arena, face.tex_name),
      .index_offset = index_offset,
      .index_count = indices_per_face,
  };
}

b32 BuildMapModel(Arena *arena, SDL_GPUDevice *device, Entity *entity,
                  Model *out) {
  s32 total_brushes = entity->brushes.size;

  if (total_brushes == 0)
    return false;

  s32 vertex_count{total_brushes * vertices_per_brush};
  s32 index_count{total_brushes * indices_per_brush};

  auto scratch = Scratch();
  auto vertices = NewArray<Vertex>(scratch.arena, vertex_count);
  auto indices = NewArray<u32>(scratch.arena, index_count);
  s32 total_faces = total_brushes * 6;
  SubMesh *sub_meshes = PushCount<SubMesh>(scratch.arena, total_faces);

	out->colliders = NewArray<AABB>(arena, total_brushes);

  s32 face_idx = 0;
  s32 sub_mesh_count = 0;

  for (s32 brush_idx{}; brush_idx < entity->brushes.size; brush_idx++) {
    auto brush = entity->brushes.data[brush_idx];
    auto map_aabb = GetBrushAABB(brush);
    out->colliders.data[brush_idx] = QuakeToEngine(map_aabb, MAP_SCALE);

    glm::vec3 p0 = {map_aabb.min.x, map_aabb.min.y, map_aabb.min.z};
    glm::vec3 p1 = {map_aabb.max.x, map_aabb.min.y, map_aabb.min.z};
    glm::vec3 p2 = {map_aabb.max.x, map_aabb.max.y, map_aabb.min.z};
    glm::vec3 p3 = {map_aabb.min.x, map_aabb.max.y, map_aabb.min.z};

    glm::vec3 p4 = {map_aabb.min.x, map_aabb.min.y, map_aabb.max.z};
    glm::vec3 p5 = {map_aabb.max.x, map_aabb.min.y, map_aabb.max.z};
    glm::vec3 p6 = {map_aabb.max.x, map_aabb.max.y, map_aabb.max.z};
    glm::vec3 p7 = {map_aabb.min.x, map_aabb.max.y, map_aabb.max.z};

    glm::vec3 n0 = {0.0f, 0.0f, -1.0f};
    glm::vec3 n1 = {0.0f, 0.0f, 1.0f};
    glm::vec3 n2 = {0.0f, -1.0f, 0.0f};
    glm::vec3 n3 = {0.0f, 1.0f, 0.0f};
    glm::vec3 n4 = {-1.0f, 0.0f, 0.0f};
    glm::vec3 n5 = {1.0f, 0.0f, 0.0f};

			BuildBrushFace(vertices, indices, brush, device, arena, p3, p2, p1, p0, n0,
										 sub_meshes, &sub_mesh_count, &face_idx);
			BuildBrushFace(vertices, indices, brush, device, arena, p4, p5, p6, p7, n1,
										 sub_meshes, &sub_mesh_count, &face_idx);
			BuildBrushFace(vertices, indices, brush, device, arena, p0, p1, p5, p4, n2,
										 sub_meshes, &sub_mesh_count, &face_idx);
			BuildBrushFace(vertices, indices, brush, device, arena, p2, p3, p7, p6, n3,
										 sub_meshes, &sub_mesh_count, &face_idx);
			BuildBrushFace(vertices, indices, brush, device, arena, p3, p0, p4, p7, n4,
										 sub_meshes, &sub_mesh_count, &face_idx);
    BuildBrushFace(vertices, indices, brush, device, arena, p1, p2, p6, p5, n5,
                   sub_meshes, &sub_mesh_count, &face_idx);
  }

  out->vertex_buffer =
      CreateVertexBuffer(device, {vertices.data, (u64)vertices.size});
  out->index_buffer =
      CreateIndexBuffer(device, {indices.data, (u64)indices.size});
  out->sub_meshes = NewArray<SubMesh>(arena, sub_mesh_count);
  for (s32 i = 0; i < sub_mesh_count; i++)
    out->sub_meshes[i] = sub_meshes[i];
  return true;
}

} // namespace frag
