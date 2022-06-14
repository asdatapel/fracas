#pragma once

#include "common.hpp"
#include "math.hpp"

#include "graphics/graphics.hpp"

namespace Dui
{
struct Vertex {
  Vec2f pos;
  Vec2f uv;
  Color color;
};
struct DrawCall {
  i32 vert_offset;
  i32 tri_count;

  // Texture texture;
};
struct DrawList {
  Vertex *verts        = nullptr;
  DrawCall *draw_calls = nullptr;
  i32 vert_count       = 0;
  i32 draw_call_count  = 0;

  VertexBuffer vb;
  Component vertex_components[3];
};

DrawList dl;

void init_draw_list()
{
  dl.verts      = (Vertex *)malloc(sizeof(Vertex) * 1024 * 1024);
  dl.draw_calls = (DrawCall *)malloc(sizeof(DrawCall) * 1024 * 1024);

  dl.vertex_components[0].offset = 0;
  dl.vertex_components[0].size = 2;
  dl.vertex_components[0].stride = 8;
  dl.vertex_components[1].offset = 2;
  dl.vertex_components[1].size = 2;
  dl.vertex_components[1].stride = 8;
  dl.vertex_components[2].offset = 4;
  dl.vertex_components[2].size = 4;
  dl.vertex_components[2].stride = 8;
  dl.vb = create_vertex_buffer();
  enable_vertex_componenets(dl.vb, dl.vertex_components, 3);

}

void clear_draw_list()
{
  dl.vert_count      = 0;
  dl.draw_call_count = 0;
}

void push_vert(Vec2f pos, Vec2f uv, Color color) { dl.verts[dl.vert_count++] = {pos, uv, color}; }

void push_draw_call(i32 tri_count)
{
  if (dl.draw_call_count == 0) {
    dl.draw_calls[0] = {0, tri_count};
    dl.draw_call_count++;
    return;
  }

  dl.draw_calls[0].tri_count += tri_count;
}

void push_rect(Rect rect, Color color)
{
  push_vert({rect.x, rect.y}, {}, color);
  push_vert({rect.x + rect.width, rect.y}, {}, color);
  push_vert({rect.x + rect.width, rect.y + rect.height}, {}, color);

  push_vert({rect.x, rect.y}, {}, color);
  push_vert({rect.x + rect.width, rect.y + rect.height}, {}, color);
  push_vert({rect.x, rect.y + rect.height}, {}, color);

  push_draw_call(2);
}

}  // namespace Dui