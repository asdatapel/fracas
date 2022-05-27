#pragma once

#include <stdio.h>

#include "platform.hpp"
#include "util.hpp"

struct AABB {
  Vec3f min = {100000, 100000, 100000};
  Vec3f max = {-100000, -100000, -100000};
};
struct Component {
  int offset, size, stride;
};
struct Mesh : Asset {
  float *data;
  int verts;
  uint64_t buf_size;

  int components_count;
  Component *components;

  AABB bounding_box;
};

template <typename T>
struct Buffer {
  T *data;
  int length;

  static Buffer<T> from_char_array(char *data, int length)
  {
    return Buffer{(T *)data, length / (int)sizeof(T)};
  }
};

// v1: pos, uv, normal, one optional uv
Mesh load_fmesh_v1(Buffer<float> buffer, Memory mem)
{
  int data_length = (buffer.length - 1) * sizeof(float);
  float *f        = (float *)mem.allocator->alloc(data_length);
  memcpy(f, buffer.data + 1, data_length);

  float multiple_uvs = buffer.data[0];
  int stride         = multiple_uvs ? 10 : 8;
  int vert_count     = (buffer.length - 1) / stride;

  Mesh mesh;
  mesh.data = f;
  mesh.verts = vert_count;
  mesh.buf_size = (uint64_t)data_length;

  mesh.components_count = multiple_uvs ? 4 : 3;
  mesh.components    = (Component *)mem.allocator->alloc(mesh.components_count * sizeof(Component));
  mesh.components[0] = {0, 3, stride};
  mesh.components[1] = {3, 2, stride};
  mesh.components[2] = {5, 3, stride};
  if (multiple_uvs) mesh.components[3] = {8, 2, stride};

  return mesh;
}

// v2: pos, uv, normal, tangent, bitangent
// manually calculated bounding box
Mesh load_fmesh_v2(Buffer<float> buffer, Memory mem)
{
  float *data = buffer.data;

  Mesh mesh;
  mesh.components_count = *data++;
  mesh.components = (Component *)mem.allocator->alloc(mesh.components_count * sizeof(Component));
  int stride      = *data++;
  int offset      = 0;
  for (int i = 0; i < mesh.components_count; i++) {
    int length         = *data++;
    mesh.components[i] = {offset, length, stride};
    offset += length;
  }

  int vertex_data_length = buffer.length - (data - buffer.data);

  mesh.verts    = vertex_data_length / stride;
  mesh.buf_size = vertex_data_length * sizeof(float);
  mesh.data     = (float *)mem.allocator->alloc(mesh.buf_size);
  memcpy(mesh.data, data, mesh.buf_size);

  for (i32 vert_i = 0; vert_i < mesh.verts; vert_i++) {
    Vec3f position = *((Vec3f*)mesh.data + vert_i * stride);
    if (position.x < mesh.bounding_box.min.x) mesh.bounding_box.min.x = position.x;
    if (position.x > mesh.bounding_box.max.x) mesh.bounding_box.max.x = position.x;
    if (position.y < mesh.bounding_box.min.y) mesh.bounding_box.min.y = position.y;
    if (position.y > mesh.bounding_box.max.y) mesh.bounding_box.max.y = position.y;
    if (position.z < mesh.bounding_box.min.z) mesh.bounding_box.min.z = position.z;
    if (position.z > mesh.bounding_box.max.z) mesh.bounding_box.max.z = position.z;
  }

  return mesh;
}

Mesh load_fmesh(FileData file, Memory mem)
{
  Buffer<float> buffer = Buffer<float>::from_char_array(file.data, file.length);
  float file_version   = buffer.data[0];
  if (file_version == 2) {
    buffer.data++;
    buffer.length--;
    return load_fmesh_v2(buffer, mem);
  } else if (file_version == 3) {
    buffer.data++;
    buffer.length--;
    return load_fmesh_v2(buffer, mem);
  } else {
    return load_fmesh_v1(buffer, mem);
  }
}