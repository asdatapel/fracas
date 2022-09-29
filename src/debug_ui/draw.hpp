#pragma once

#include "common.hpp"
#include "math.hpp"

#include "graphics/graphics.hpp"

namespace Dui
{

template <typename T, i64 SIZE>
struct StaticStack {
  T elements[SIZE];
  i64 count = 0;

  StaticStack() = default;

  T &top()
  {
    assert(count > 0);
    return elements[count - 1];
  };

  T pop()
  {
    assert(count > 0);
    count--;
    return elements[count];
  };

  T &operator[](i64 i)
  {
    assert(i < count);
    return elements[i];
  }

  int push_back(T val)
  {
    if (count >= SIZE) {
      DEBUG_PRINT("static stack overfull\n");
      return -1;
    }

    elements[count] = val;
    return count++;
  }

  void clear() { count = 0; }
};

struct Vertex {
  Vec2f pos;
  Vec2f uv;
  Color color;
};
struct DrawCall {
  i32 vert_offset;
  i32 tri_count;

  Rect scissor;
  // Texture texture;
};
struct DrawList {
  Vertex *verts        = nullptr;
  DrawCall *draw_calls = nullptr;
  i32 vert_count       = 0;
  i32 draw_call_count  = 0;

  VertexBuffer vb;
  Component vertex_components[3];

  StaticStack<Rect, 1024> scissors;
  void push_scissor(Rect rect) { scissors.push_back(rect); }
  void pop_scissor() { scissors.pop(); }
};

DrawList main_dl;
DrawList forground_dl;

void init_draw_list(DrawList *dl)
{
  dl->verts      = (Vertex *)malloc(sizeof(Vertex) * 1024 * 1024);
  dl->draw_calls = (DrawCall *)malloc(sizeof(DrawCall) * 1024 * 1024);

  dl->vertex_components[0].offset = 0;
  dl->vertex_components[0].size   = 2;
  dl->vertex_components[0].stride = 8;
  dl->vertex_components[1].offset = 2;
  dl->vertex_components[1].size   = 2;
  dl->vertex_components[1].stride = 8;
  dl->vertex_components[2].offset = 4;
  dl->vertex_components[2].size   = 4;
  dl->vertex_components[2].stride = 8;
  dl->vb                          = create_vertex_buffer();
  enable_vertex_componenets(dl->vb, dl->vertex_components, 3);
}

void clear_draw_list(DrawList *dl)
{
  dl->vert_count      = 0;
  dl->draw_call_count = 0;
}

void draw_draw_list(RenderTarget target, DrawList *dl)
{
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  reupload_vertex_buffer(dl->vb, (float *)dl->verts, dl->vert_count,
                         dl->vert_count * sizeof(Vertex));
  bind_shader(debug_ui_shader);
  bind_2i(debug_ui_shader, UniformId::RESOLUTION, target.width, target.height);
  for (i32 i = 0; i < dl->draw_call_count; i++) {
    DrawCall call = dl->draw_calls[i];
    
    start_scissor(target, call.scissor);
    draw_sub(target, debug_ui_shader, dl->vb, call.vert_offset, call.tri_count * 3);
    end_scissor();
  }
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

void push_vert(DrawList *dl, Vec2f pos, Vec2f uv, Color color)
{
  dl->verts[dl->vert_count++] = {pos, uv, color};
}

void push_draw_call(DrawList *dl, i32 tri_count)
{
  Rect current_scissor_rect =
      (dl->scissors.count == 0) ? Rect{0, 0, 10000000, 10000000} : dl->scissors.top();

  if (dl->draw_call_count > 0 &&
      dl->draw_calls[dl->draw_call_count - 1].scissor == current_scissor_rect) {
    dl->draw_calls[dl->draw_call_count - 1].tri_count += tri_count;
    return;
  }

  DrawCall dc = {dl->vert_count - (tri_count * 3), tri_count};
  dc.scissor  = current_scissor_rect;

  dl->draw_calls[dl->draw_call_count] = dc;
  dl->draw_call_count++;
}

void push_rect(DrawList *dl, Rect rect, Color color)
{
  push_vert(dl, {rect.x, rect.y}, {}, color);
  push_vert(dl, {rect.x + rect.width, rect.y}, {}, color);
  push_vert(dl, {rect.x + rect.width, rect.y + rect.height}, {}, color);

  push_vert(dl, {rect.x, rect.y}, {}, color);
  push_vert(dl, {rect.x + rect.width, rect.y + rect.height}, {}, color);
  push_vert(dl, {rect.x, rect.y + rect.height}, {}, color);

  push_draw_call(dl, 2);
}

// TODO allow pushing a temporary drawcall then can be updated later.
// Useful for grouping controls with a background
DrawCall *push_temp();

}  // namespace Dui