#pragma once

#include <unordered_map>

#include "common.hpp"
#include "debug_ui/draw.hpp"
#include "math.hpp"
#include "util.hpp"
#include "util/containers/static_array.hpp"
#include "util/containers/static_linked_list.hpp"
#include "util/containers/static_pool_allocator.hpp"

const f32 TITLEBAR_HEIGHT               = 32;
const f32 TITLEBAR_BOTTOM_BORDER_HEIGHT = 3.f;
const f32 WINDOW_BORDER_SIZE            = 2.f;
const f32 WINDOW_MARGIN_SIZE            = 4.f;
const f32 SCROLLBAR_WIDTH               = 8.f;

Color d_dark  = {0, 0.13725, 0.27843, 1};
Color d       = {0, 0.2, 0.4, 1};
Color d_light = {0, 0.24706, 0.4902, 1};
Color l_light = {1, 0.55686, 0, 1};
Color l       = {0.99216, 0.46667, 0.00784, 1};
Color l_dark  = {1, 0.31373, 0.01176, 1};

Rect inset_rect(Rect rect, f32 inset)
{
  rect.x += inset;
  rect.y += inset;
  rect.width -= inset * 2;
  rect.height -= inset * 2;
  return rect;
}

#define SUB_ID(name, id) DuiId name = extend_hash(id, #name)

#define INTERACTION_STATE(var)                   \
  DuiId var                      = -1;           \
  DuiId just_started_being_##var = -1;           \
  DuiId just_stopped_being_##var = -1;           \
  DuiId was_last_control_##var   = -1;           \
  Vec2f start_position_##var     = {};           \
                                                 \
  b8 is_##var(DuiId id) { return var == id; }    \
  void set_##var(DuiId id)                       \
  {                                              \
    just_stopped_being_##var = var;              \
                                                 \
    var                      = id;               \
    just_started_being_##var = id;               \
    start_position_##var     = input->mouse_pos; \
  }                                              \
  void clear_##var(DuiId id)                     \
  {                                              \
    if (var == id) {                             \
      var                    = 0;                \
      just_stopped_being_hot = id;               \
    }                                            \
  }

typedef i64 DuiId;
namespace Dui
{
// http://www.cse.yorku.ca/~oz/hash.html
DuiId hash(String str)
{
  u64 hash = 5381;
  for (int i = 0; i < str.len; i++) {
    int c = str.data[i];
    hash  = ((hash << 5) + hash) + c;
  }

  return hash;
}
DuiId extend_hash(u64 hash, String str)
{
  for (int i = 0; i < str.len; i++) {
    int c = str.data[i];
    hash  = ((hash << 5) + hash) + c;
  }
  return hash;
}
}  // namespace Dui

namespace Dui
{

struct Group;
struct Split {
  Group *child;
  f32 div_pct = .5;

  f32 div_position;  // cache
};

struct Group {
  DuiId id      = -1;
  Group *parent = nullptr;
  Rect rect;

  i32 split_axis = -1;
  StaticArray<Split, 32> splits;

  // or

  StaticArray<DuiId, 32> windows;
  i32 active_window_idx = -1;

  Rect get_titlebar_full_rect()
  {
    Rect titlebar_rect;
    titlebar_rect.x      = rect.x;
    titlebar_rect.y      = rect.y;
    titlebar_rect.width  = rect.width;
    titlebar_rect.height = TITLEBAR_HEIGHT;

    return titlebar_rect;
  }
  Rect get_titlebar_margin_rect()
  {
    Rect titlebar_full_rect = get_titlebar_full_rect();
    titlebar_full_rect.height -= TITLEBAR_BOTTOM_BORDER_HEIGHT;

    return titlebar_full_rect;
  }
  Rect get_titlebar_bottom_border_rect()
  {
    Rect titlebar_full_rect = get_titlebar_full_rect();

    Rect rect;
    rect.x      = titlebar_full_rect.x;
    rect.y      = titlebar_full_rect.y + titlebar_full_rect.height - TITLEBAR_BOTTOM_BORDER_HEIGHT;
    rect.width  = titlebar_full_rect.width;
    rect.height = TITLEBAR_BOTTOM_BORDER_HEIGHT;
    return rect;
  }
  Rect get_titlebar_content_rect()
  {
    return inset_rect(get_titlebar_margin_rect(), WINDOW_MARGIN_SIZE);
  }
  Rect get_tab_margin_rect(i32 window_idx)
  {
    Rect titlebar_content_rect = get_titlebar_content_rect();

    f32 UNSELECTED_TAB_WIDTH  = 128.f;
    f32 tab_gap               = 2.f;
    f32 unselected_tab_height = titlebar_content_rect.height + WINDOW_MARGIN_SIZE;

    Rect rect;
    rect.x      = titlebar_content_rect.x + (UNSELECTED_TAB_WIDTH + tab_gap) * window_idx;
    rect.y      = titlebar_content_rect.y;
    rect.width  = UNSELECTED_TAB_WIDTH;
    rect.height = unselected_tab_height;

    return rect;
  }

  Rect get_border_rect()
  {
    Rect titlebar_rect = get_titlebar_full_rect();

    Rect window_border_rect;
    window_border_rect.x      = rect.x;
    window_border_rect.y      = rect.y + titlebar_rect.height;
    window_border_rect.width  = rect.width;
    window_border_rect.height = rect.height - TITLEBAR_HEIGHT;

    return window_border_rect;
  }
  Rect get_window_rect()
  {
    Rect window_border_rect = get_border_rect();
    return inset_rect(window_border_rect, WINDOW_BORDER_SIZE);
  }

  i32 get_window_idx(DuiId window_id)
  {
    for (i32 i = 0; i < windows.count; i++) {
      if (windows[i] == window_id) return i;
    }
    return -1;
  }
};

struct WindowOptions {
  b8 lock_position = false;
  b8 lock_size     = false;
  b8 show_title    = true;
  b8 can_minimize  = true;
  b8 can_close     = true;
};

struct Window {
  DuiId id;
  String title;

  Group *parent  = nullptr;
  i64 last_frame = -1;

  Vec2f scroll_offset_target = {};
  Vec2f scroll_offset        = {};

  Rect rect;

  // per frame data

  // total space requested by content last frame. this is whats used to determine free space and
  // stretch controls to bounds.
  Vec2f last_frame_minimum_content_span;

  Vec2f current_frame_minimum_content_span;

  Vec2f cursor;
  f32 cursor_height;

  Rect get_content_rect()
  {
    Rect content_rect = inset_rect(rect, WINDOW_MARGIN_SIZE);

    if (last_frame_minimum_content_span.x > content_rect.width) {
      content_rect.height -= 10;
    }
    if (last_frame_minimum_content_span.y > content_rect.height) {
      content_rect.width -= 10;
    }

    return content_rect;
  }

  // stretch + minimum size (causing scroll)
  // shrink + minimum size (causing scroll) + maximum size
  // forced size (causing scroll)

  // anything that expands scroll affects last_frame_minimum_content_span

  // actiual remaining space is calculated by max(window_rect, last_frame_minimum_content_span)

  f32 debug_last_frame_height = 0;
  f32 debug_last_frame_width  = 0;
  Color color;
};

}  // namespace Dui

namespace Dui
{
struct DuiState {
  StaticPoolAllocator<Window, 1024> windows;
  StaticPoolAllocator<Group, 1024> containers;

  i64 frame = 0;
  InputState *input;

  Window *cw = 0;

  INTERACTION_STATE(hot);
  INTERACTION_STATE(active);
  INTERACTION_STATE(dragging);

  Vec2f dragging_total_delta;
  Vec2f dragging_frame_delta;
};

DuiState s;
}  // namespace Dui

namespace Dui
{
b8 do_hot(DuiId id, Rect rect)
{
  b8 is_hot = in_rect(s.input->mouse_pos, rect);
  if (is_hot)
    s.set_hot(id);
  else
    s.clear_hot(id);

  return is_hot;
}

b8 do_active(DuiId id)
{
  // TODO handle left and right mouse buttons

  if (s.input->mouse_button_up_events[(int)MouseButton::LEFT]) {
    s.clear_active(id);
  }
  if (s.input->mouse_button_down_events[(int)MouseButton::LEFT]) {
    if (s.just_started_being_active == -1) {  // only one control should be activated in a frame
      if (s.is_hot(id)) {
        s.set_active(id);
      } else {
        s.clear_active(id);
      }
    }
  }

  s.was_last_control_active = s.is_active(id);
  return s.was_last_control_active;
}

b8 do_dragging(DuiId id)
{
  // TODO handle left and right mouse buttons

  const f32 drag_start_distance = 2.f;

  const b8 active = s.is_active(id);
  if (active) {
    if ((s.input->mouse_pos - s.start_position_active).len() > drag_start_distance) {
      s.set_dragging(id);
    }
    s.dragging_total_delta = s.input->mouse_pos - s.start_position_active;
    s.dragging_frame_delta = s.input->mouse_pos_delta;
  } else {
    s.clear_dragging(id);
  }

  s.was_last_control_dragging = s.is_dragging(id);
  return s.is_dragging(id);
}

b8 do_dragging_but_only_start_if(DuiId id, b8 condition)
{
  // TODO handle left and right mouse buttons

  const f32 drag_start_distance = 2.f;

  const b8 active = s.is_active(id);
  if (active) {
    if (condition && (s.input->mouse_pos - s.start_position_active).len() > drag_start_distance) {
      s.set_dragging(id);
    }
    s.dragging_total_delta = s.input->mouse_pos - s.start_position_active;
    s.dragging_frame_delta = s.input->mouse_pos_delta;
  } else {
    s.clear_dragging(id);
  }

  s.was_last_control_dragging = s.is_dragging(id);
  return s.is_dragging(id);
}

}  // namespace Dui

namespace Dui
{

Group *create_container(Group *parent, Rect rect)
{
  Group *g  = s.containers.push_back({});
  g->id     = rand();
  g->parent = parent;
  g->rect   = rect;

  return g;
}

void free_container(Group *g)
{
  assert(g->windows.count == 0 && g->splits.count == 0);
  i64 index = s.containers.index_of(g);
  s.containers.remove(index);
}

Window *get_window(DuiId id) { return &s.windows.wrapped_get(id); }

void propagate_containers(Group *g, Group *parent = nullptr)
{
  if (parent) g->parent = parent;

  if (g->windows.count == 0) {
    f32 x = g->rect.x;
    f32 y = g->rect.y;

    for (i32 i = 0; i < g->splits.count; i++) {
      Split &split = g->splits[i];
      Group *child = split.child;

      if (g->split_axis == 0) {
        split.div_position = y;

        f32 height  = g->rect.height * split.div_pct;
        child->rect = {x, y, g->rect.width, height};
        y += height;
      } else {
        split.div_position = x;

        f32 width   = g->rect.width * split.div_pct;
        child->rect = {x, y, width, g->rect.height};
        x += width;
      }

      propagate_containers(child, g);
    }
  } else {
    for (i32 i = 0; i < g->windows.count; i++) {
      Window *w = get_window(g->windows[i]);
      w->parent = g;
      w->rect   = g->get_window_rect();
    }
  }
}

// FIXME parent_window and unparent_window do too much. parent should only parent and unparent
// should only unparent.
void parent_window(Group *g, DuiId window_id)
{
  Window *w = &s.windows.wrapped_get(window_id);

  if (w->parent) {
    Group *old_g = w->parent;
    old_g->windows.shift_delete(old_g->get_window_idx(w->id));
    if (old_g->active_window_idx >= old_g->windows.count) {
      old_g->active_window_idx--;
    }

    if (old_g->windows.count == 0) {
      free_container(old_g);
    }
  }

  w->parent = g;

  g->windows.push_back(window_id);
  g->active_window_idx = g->windows.count - 1;

  propagate_containers(g);
}

Group *unparent_window(DuiId window_id)
{
  Window *w    = get_window(window_id);
  Group *old_g = w->parent;

  if (old_g->windows.count == 1) return old_g;

  Group *g = create_container(nullptr, old_g->rect);
  parent_window(g, window_id);
  return g;
}

Window *create_new_window(DuiId id, String name, Rect rect, WindowOptions options)
{
  Window *window = s.windows.emplace_wrapped(id, {});
  window->id     = id;
  window->title  = name;

  Group *g = create_container(nullptr, rect);
  parent_window(g, window->id);

  return window;
}

// returns true if successful.
b8 snap_container(Group *g, Group *target, i32 axis, b8 dir, Group *sibling = nullptr)
{
  // target must either be a leaf node or be split on the same axis as the sibling
  if (target->windows.count == 0 && sibling && target->split_axis != axis) return false;

  // try adding it to the parent first
  if (target->parent && snap_container(g, target->parent, axis, dir, target)) return true;

  if (target->windows.count > 0) {  // leaf
    // this is where the original leaf window will move
    Group *new_g             = create_container(target, target->rect);
    new_g->windows           = target->windows;
    new_g->active_window_idx = target->active_window_idx;

    target->windows.clear();
    target->split_axis = axis;

    Split orig_split;
    orig_split.child   = new_g;
    orig_split.div_pct = .5f;
    target->splits.push_back(orig_split);

    Split new_split;
    new_split.child   = g;
    new_split.div_pct = .5f;
    target->splits.insert(dir ? 1 : 0, new_split);

  } else if (target->split_axis != axis) {  // already split, but new axis
    assert(!sibling);

    // move original children to a new group
    Group *new_g      = create_container(target, target->rect);
    new_g->split_axis = target->split_axis;
    new_g->splits     = target->splits;
    for (i32 i = 0; i < new_g->splits.count; i++) {
      Group *target = new_g->splits[i].child;
    }

    target->splits.clear();

    Split orig_split;
    orig_split.child   = new_g;
    orig_split.div_pct = .5f;
    target->splits.push_back(orig_split);

    Split new_split;
    new_split.child   = g;
    new_split.div_pct = .5f;
    target->splits.insert(dir ? 1 : 0, new_split);

    target->split_axis = axis;
  } else if (sibling) {  // already split, same axis, at the ends
    i32 sibling_idx = -1;
    for (i32 i = 0; i < target->splits.count; i++) {
      if (target->splits[i].child == sibling) {
        sibling_idx = i;
        break;
      }
    }

    assert(sibling_idx > -1);

    f32 half_size                       = target->splits[sibling_idx].div_pct / 2;
    target->splits[sibling_idx].div_pct = half_size;

    Split new_split;
    new_split.child   = g;
    new_split.div_pct = half_size;
    target->splits.insert(dir ? sibling_idx + 1 : sibling_idx, new_split);
    g->parent = target;
  } else {  // already split, same axis, in the middle
    f32 new_split_pct = 1.f / (target->splits.count + 1);

    for (i32 i = 0; i < target->splits.count; i++) {
      target->splits[i].div_pct *= 1 - new_split_pct;
    }

    Split new_split;
    new_split.child   = g;
    new_split.div_pct = new_split_pct;
    target->splits.insert(dir ? target->splits.count : 0, new_split);
    g->parent = target;
  }

  propagate_containers(target);

  return true;
}

void unsnap_container(Group *g)
{
  if (!g->parent) return;

  Group *parent_g = g->parent;

  if (parent_g->windows.count == 1) return;  // only window in group

  DuiId idx;
  f32 free_space;
  for (i32 i = 0; i < parent_g->splits.count; i++) {
    Split split = parent_g->splits[i];
    if (split.child == g) {
      idx        = i;
      free_space = split.div_pct;
      break;
    }
  }
  parent_g->splits.shift_delete(idx);

  for (i32 i = 0; i < parent_g->splits.count; i++) {
    parent_g->splits[i].div_pct *= 1 / (1 - free_space);
  }

  if (parent_g->splits.count == 1 && parent_g->parent) {
    Group *other_child = parent_g->splits[0].child;

    parent_g->split_axis = other_child->split_axis;
    parent_g->splits     = other_child->splits;
    parent_g->windows    = other_child->windows;

    other_child->splits.clear();
    other_child->windows.clear();
    other_child->split_axis = -1;
    free_container(other_child);

    // transfer again if axes match up
    Group *double_parent = parent_g->parent;
    if (double_parent && double_parent->split_axis == parent_g->split_axis) {
      Split dp_split;
      i32 dp_split_i = -1;
      for (i32 i = 0; i < double_parent->splits.count; i++) {
        if (double_parent->splits[i].child == parent_g) {
          dp_split   = double_parent->splits[i];
          dp_split_i = i;
        }
      }

      assert(dp_split_i > -1);

      double_parent->splits.shift_delete(dp_split_i);
      for (i32 i = 0; i < parent_g->splits.count; i++) {
        Split new_split;
        new_split.child   = parent_g->splits[i].child;
        new_split.div_pct = parent_g->splits[i].div_pct * dp_split.div_pct;
        double_parent->splits.insert(dp_split_i + i, new_split);
      }

      parent_g->splits.clear();
      parent_g->windows.clear();
      parent_g->split_axis = -1;
      free_container(parent_g);

      parent_g = double_parent;
    }
  }

  propagate_containers(parent_g);

  g->parent = nullptr;
}

}  // namespace Dui

namespace Dui
{

void start_frame_for_window(Window *w)
{
  w->last_frame_minimum_content_span    = w->current_frame_minimum_content_span;
  w->current_frame_minimum_content_span = {0, 0};

  w->cursor        = {0, 0};
  w->cursor_height = 0;

  // FYI doing this here can cause the position of the scrollbar to lag behind a moving window. Its
  // ok though because it will be drawn in the correct position, and you shouldn't be interacting
  // with a scrollbar while moving a window anyway.
  Rect content_rect = w->get_content_rect();
  if (w->last_frame_minimum_content_span.x > content_rect.width) {
  }
  if (w->last_frame_minimum_content_span.y > content_rect.height) {
    w->scroll_offset_target.y += s.input->scrollwheel_count * 100.f;
  }

  w->scroll_offset_target =
      clamp(w->scroll_offset_target, -w->last_frame_minimum_content_span + content_rect.span(),
            {0.f, 0.f});

  w->scroll_offset = (w->scroll_offset + w->scroll_offset_target) / 2.f;
}

void start_frame_for_leaf(Group *g)
{
  assert(g->active_window_idx < g->windows.count);
  DuiId active_window_id = g->windows[g->active_window_idx];

  for (i32 w_i = 0; w_i < g->windows.count; w_i++) {
    DuiId window_id = g->windows[w_i];
    Window *w       = get_window(window_id);

    start_frame_for_window(w);

    Rect tab_rect = g->get_tab_margin_rect(g->get_window_idx(window_id));

    SUB_ID(tab_handle_id, window_id);
    DuiId tab_handle_hot      = do_hot(tab_handle_id, tab_rect);
    DuiId tab_handle_active   = do_active(tab_handle_id);
    DuiId tab_handle_dragging = do_dragging(tab_handle_id);
    if (tab_handle_active) {
      g->active_window_idx = w_i;
    }
    if (tab_handle_dragging) {
      g = unparent_window(window_id);
      unsnap_container(g);

      g->rect.x += s.dragging_frame_delta.x;
      g->rect.y += s.dragging_frame_delta.y;

      propagate_containers(g);
    }
  }

  Rect titlebar_rect = g->get_titlebar_margin_rect();
  SUB_ID(root_handle_id, active_window_id);
  DuiId root_handle_hot      = do_hot(root_handle_id, titlebar_rect);
  DuiId root_handle_active   = do_active(root_handle_id);
  DuiId root_handle_dragging = do_dragging(root_handle_id);
  if (root_handle_dragging) {
    Group *root_g = g;
    while (root_g->parent) {
      root_g = root_g->parent;
    }

    root_g->rect.x += s.dragging_frame_delta.x;
    root_g->rect.y += s.dragging_frame_delta.y;

    propagate_containers(root_g);
  }
}

void start_frame_for_container(Group *g)
{
  const f32 RESIZE_HANDLES_OVERSIZE = 2.f;

  if (!g->parent) {
    // easy keyboard-based window manipulation
    if (s.input->keys[(i32)Keys::LCTRL]) {
      SUB_ID(root_controller_control_id, g->id);
      DuiId root_controller_control_hot      = do_hot(root_controller_control_id, g->rect);
      DuiId root_controller_control_active   = do_active(root_controller_control_id);
      DuiId root_controller_control_dragging = do_dragging(root_controller_control_id);
      if (root_controller_control_dragging) {
        g->rect.x += s.dragging_frame_delta.x;
        g->rect.y += s.dragging_frame_delta.y;
        propagate_containers(g);
      }
    }

    // resize handles that extend out of group
    Rect left_handle_rect;
    left_handle_rect.x      = g->rect.x - RESIZE_HANDLES_OVERSIZE;
    left_handle_rect.y      = g->rect.y;
    left_handle_rect.width  = RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
    left_handle_rect.height = g->rect.height;
    SUB_ID(left_handle_id, g->id);
    DuiId left_handle_hot      = do_hot(left_handle_id, left_handle_rect);
    DuiId left_handle_active   = do_active(left_handle_id);
    DuiId left_handle_dragging = do_dragging(left_handle_id);
    if (left_handle_hot || left_handle_dragging) {
      push_rect(left_handle_rect, {1, 1, 1, 1});
    }
    if (left_handle_dragging) {
      g->rect.x += s.dragging_frame_delta.x;
      g->rect.width -= s.dragging_frame_delta.x;
      propagate_containers(g);
    }

    Rect right_handle_rect;
    right_handle_rect.x      = g->rect.x + g->rect.width - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
    right_handle_rect.y      = g->rect.y;
    right_handle_rect.width  = RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
    right_handle_rect.height = g->rect.height;
    SUB_ID(right_handle_id, g->id);
    DuiId right_handle_hot      = do_hot(right_handle_id, right_handle_rect);
    DuiId right_handle_active   = do_active(right_handle_id);
    DuiId right_handle_dragging = do_dragging(right_handle_id);
    if (right_handle_hot || right_handle_dragging) {
      push_rect(right_handle_rect, {1, 1, 1, 1});
    }
    if (right_handle_dragging) {
      g->rect.width += s.dragging_frame_delta.x;
      propagate_containers(g);
    }

    Rect top_handle_rect;
    top_handle_rect.x      = g->rect.x;
    top_handle_rect.y      = g->rect.y - RESIZE_HANDLES_OVERSIZE;
    top_handle_rect.width  = g->rect.width;
    top_handle_rect.height = RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
    SUB_ID(top_handle_id, g->id);
    DuiId top_handle_hot      = do_hot(top_handle_id, top_handle_rect);
    DuiId top_handle_active   = do_active(top_handle_id);
    DuiId top_handle_dragging = do_dragging(top_handle_id);
    if (top_handle_hot || top_handle_dragging) {
      push_rect(top_handle_rect, {1, 1, 1, 1});
    }
    if (top_handle_dragging) {
      g->rect.y += s.dragging_frame_delta.y;
      g->rect.height -= s.dragging_frame_delta.y;
      propagate_containers(g);
    }

    Rect bottom_handle_rect;
    bottom_handle_rect.x     = g->rect.x;
    bottom_handle_rect.y     = g->rect.y + g->rect.height - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
    bottom_handle_rect.width = g->rect.width;
    bottom_handle_rect.height = RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
    SUB_ID(bottom_handle_id, g->id);
    DuiId bottom_handle_hot      = do_hot(bottom_handle_id, bottom_handle_rect);
    DuiId bottom_handle_active   = do_active(bottom_handle_id);
    DuiId bottom_handle_dragging = do_dragging(bottom_handle_id);
    if (bottom_handle_hot || bottom_handle_dragging) {
      push_rect(bottom_handle_rect, {1, 1, 1, 1});
    }
    if (bottom_handle_dragging) {
      g->rect.height += s.dragging_frame_delta.y;
      propagate_containers(g);
    }
  }

  if (g->windows.count > 0) {
    start_frame_for_leaf(g);
    return;
  }

  for (i32 i = 1; i < g->splits.count; i++) {
    if (g->split_axis == 0) {
      Rect split_move_handle;
      split_move_handle.x     = g->rect.x;
      split_move_handle.y     = g->splits[i].div_position - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
      split_move_handle.width = g->rect.width;
      split_move_handle.height = 2 * (WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE);
      SUB_ID(split_move_handle_id, g->id + i);
      DuiId split_move_handle_hot      = do_hot(split_move_handle_id, split_move_handle);
      DuiId split_move_handle_active   = do_active(split_move_handle_id);
      DuiId split_move_handle_dragging = do_dragging(split_move_handle_id);
      if (split_move_handle_hot || split_move_handle_dragging) {
        push_rect({10, 10, 20, 20}, {1, 1, 1, 1});
      }
      if (split_move_handle_dragging) {
        f32 move_pct = s.dragging_frame_delta.y / g->rect.height;
        g->splits[i - 1].div_pct += move_pct;
        g->splits[i].div_pct -= move_pct;
        propagate_containers(g);
      }
    } else if (g->split_axis == 1) {
      Rect split_move_handle;
      split_move_handle.x     = g->splits[i].div_position - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
      split_move_handle.y     = g->rect.y;
      split_move_handle.width = 2 * (WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE);
      split_move_handle.height = g->rect.height;
      SUB_ID(split_move_handle_id, g->id + i);
      DuiId split_move_handle_hot      = do_hot(split_move_handle_id, split_move_handle);
      DuiId split_move_handle_active   = do_active(split_move_handle_id);
      DuiId split_move_handle_dragging = do_dragging(split_move_handle_id);
      if (split_move_handle_hot || split_move_handle_dragging) {
        push_rect({10, 10, 20, 20}, {1, 1, 1, 1});
      }
      if (split_move_handle_dragging) {
        f32 move_pct = s.dragging_frame_delta.x / g->rect.width;
        g->splits[i - 1].div_pct += move_pct;
        g->splits[i].div_pct -= move_pct;
        propagate_containers(g);
      }
    }
  }

  for (i32 i = 0; i < g->splits.count; i++) {
    start_frame_for_container(g->splits[i].child);
  }
}

void start_frame(InputState *input)
{
  s.input                       = input;
  s.just_started_being_hot      = -1;
  s.just_stopped_being_hot      = -1;
  s.just_started_being_active   = -1;
  s.just_stopped_being_active   = -1;
  s.just_started_being_dragging = -1;
  s.just_stopped_being_dragging = -1;

  s.frame++;

  clear_draw_list();

  for (i32 i = 0; i < s.containers.SIZE; i++) {
    if (!s.containers.exists(i)) continue;
    if (s.containers[i].parent) continue;

    Group *g = &s.containers[i];
    start_frame_for_container(g);
  }

  // draw

  for (i32 i = 0; i < s.containers.SIZE; i++) {
    if (!s.containers.exists(i)) continue;
    if (s.containers[i].windows.count == 0) continue;

    Group *g                         = &s.containers[i];
    Rect titlebar_rect               = g->get_titlebar_full_rect();
    Rect titlebar_bottom_border_rect = g->get_titlebar_bottom_border_rect();
    Rect container_border_rect       = g->get_border_rect();
    Rect window_rect                 = g->get_window_rect();

    push_rect(titlebar_rect, d);
    push_rect(titlebar_bottom_border_rect, d_light);
    push_rect(container_border_rect, d);
    push_rect(window_rect, d_dark);

    Rect titlebar_content_rect = g->get_titlebar_content_rect();

    for (i32 w_i = 0; w_i < g->windows.count; w_i++) {
      DuiId window_id = g->windows[w_i];
      Window *w       = get_window(window_id);

      Rect tab_rect = g->get_tab_margin_rect(g->get_window_idx(window_id));

      Color tab_color = d_dark;
      if (w_i == g->active_window_idx) {
        tab_color = d_light;
      }
      push_rect(tab_rect, tab_color);
    }
  }
}

DuiId start_window(String name, Rect initial_rect, WindowOptions options = {})
{
  DuiId id  = hash(name);
  Window *w = s.windows.wrapped_exists(id) ? &s.windows.wrapped_get(id)
                                           : create_new_window(id, name, initial_rect, options);
  Group *g  = w->parent;

  s.cw          = w;
  w->last_frame = s.frame;

  if (g->windows[g->active_window_idx] != id) return id;

  Rect content_rect = w->get_content_rect();
  push_rect(content_rect, w->color);

  // Rect titlebar_rect      = g->get_titlebar_margin_rect();
  // Rect window_border_rect = g->get_border_rect();
  // Rect window_margin_rect = g->get_margin_rect();

  // push_rect(window_border_rect, d);
  // push_rect(titlebar_rect, d_light);
  // push_rect(window_margin_rect, d_dark);

  // Rect titlebar_content_rect = g->get_titlebar_content_rect();
  // push_rect(titlebar_content_rect, {1, 0, 0, 1});

  // Rect tab_rect = g->get_tab_margin_rect(g->get_window_idx(id));
  // push_rect(tab_rect, w->color);

  return w->id;
}

void end_window() { s.cw = nullptr; }

}  // namespace Dui

namespace Dui
{

void set_window_color(Color color)
{
  if (s.cw) {
    s.cw->color = color;
  }
}

// TODO allow pushing a temporary drawcall then can be updated later.
// Useful for grouping controls with a background
DrawCall *push_temp();

void next_line()
{
  Window *w = s.cw;

  w->cursor.y += w->cursor_height;
  w->cursor.x      = 0;
  w->cursor_height = 0;
}

void basic_test_control(Vec2f size, Color color)
{
  Window *w = s.cw;

  Rect window_content_rect = w->get_content_rect();

  Vec2f scrolled_cursor = w->cursor + w->scroll_offset;

  Rect rect;
  rect.x      = window_content_rect.x + scrolled_cursor.x;
  rect.y      = window_content_rect.y + scrolled_cursor.y;
  rect.width  = size.x;
  rect.height = size.y;
  push_rect(rect, color);

  w->cursor_height = fmaxf(w->cursor_height, size.y);
  w->cursor.x += size.x;

  w->current_frame_minimum_content_span.x =
      fmaxf(w->current_frame_minimum_content_span.x, w->cursor.x);
  w->current_frame_minimum_content_span.y =
      fmaxf(w->current_frame_minimum_content_span.y, w->cursor.y + w->cursor_height);
}

void debug_ui_test(RenderTarget target, InputState *input, Memory memory)
{
  static b8 init = false;
  if (!init) {
    init = true;
    init_draw_list();
  }

  static b8 paused = false;
  if (input->key_down_events[(i32)Keys::Q]) {
    paused = !paused;
  }

  if (paused) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    reupload_vertex_buffer(dl.vb, (float *)dl.verts, dl.vert_count, dl.vert_count * sizeof(Vertex));
    bind_shader(debug_ui_shader);
    bind_2i(debug_ui_shader, UniformId::RESOLUTION, target.width, target.height);
    for (i32 i = 0; i < dl.draw_call_count; i++) {
      DrawCall call = dl.draw_calls[i];
      draw_sub(target, debug_ui_shader, dl.vb, call.vert_offset, call.tri_count * 3);
    }
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    return;
  }

  start_frame(input);

  DuiId w1 = start_window("first", {100, 200, 1500, 900});
  set_window_color({0, 0.13725, 0.27843, 1});
  end_window();

  DuiId w2 = start_window("second", {200, 300, 100, 300});
  set_window_color({0, 0.2, 0.4, .5});
  end_window();

  DuiId w3 = start_window("third", {300, 400, 200, 300});
  set_window_color({0, 0.24706, 0.4902, .5});
  end_window();

  DuiId w4 = start_window("fourth", {400, 500, 200, 300});
  set_window_color({1, 0.55686, 0, .5});
  end_window();

  DuiId w5 = start_window("fifth", {500, 600, 200, 300});
  set_window_color({0.99216, 0.46667, 0.00784, .5});
  basic_test_control({200, 300}, {1, 1, 1, 1});
  basic_test_control({150, 100}, {1, 0, 0, 1});
  basic_test_control({100, 400}, {1, 1, 0, 1});
  next_line();
  basic_test_control({200, 600}, {0, 1, 1, 1});
  next_line();
  basic_test_control({200, 700}, {1, 0, 1, 1});
  basic_test_control({200, 300}, {1, 1, 0, 1});
  next_line();
  basic_test_control({200, 100}, {0, 0, 1, 1});
  end_window();

  DuiId w6 = start_window("sizth", {600, 700, 200, 300});
  set_window_color({1, 0.31373, 0.01176, .5});
  end_window();

  DuiId w7 = start_window("seventh", {700, 800, 200, 300});
  set_window_color({.3, .6, .4, .5});
  end_window();

  auto p = [&](DuiId window_id) {
    Window *w = &s.windows.wrapped_get(window_id);
    return w->parent;
  };

  static Group *root   = p(w1);
  static Group *root_2 = p(w1);

  static i32 count = 0;
  if (input->key_down_events[(i32)Keys::SPACE]) {
    if (count == 0) {
      snap_container(p(w2), p(w1), 1, true);
    }

    if (count == 1) {
      snap_container(p(w3), p(w1), 0, false);
    }

    if (count == 2) {
      root_2 = p(w2);
      snap_container(p(w4), p(w2), 0, false);
    }

    if (count == 3) {
      snap_container(p(w5), root, 0, true);
    }

    if (count == 4) {
      snap_container(p(w6), p(w4), 1, true);
    }

    if (count == 5) {
      parent_window(p(w1), w7);
      // snap_container(p(w7), root_2, 0, true);
    }

    count++;
  }

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  reupload_vertex_buffer(dl.vb, (float *)dl.verts, dl.vert_count, dl.vert_count * sizeof(Vertex));
  bind_shader(debug_ui_shader);
  bind_2i(debug_ui_shader, UniformId::RESOLUTION, target.width, target.height);
  for (i32 i = 0; i < dl.draw_call_count; i++) {
    DrawCall call = dl.draw_calls[i];
    draw_sub(target, debug_ui_shader, dl.vb, call.vert_offset, call.tri_count * 3);
  }
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  std::vector<Group *> groups;
  for (i32 i = 0; i < s.containers.SIZE; i++) {
    if (s.containers.exists(i)) {
      groups.push_back(&s.containers[i]);
    }
  }
  printf("groups: %i\n", groups.size());

}
}  // namespace Dui

/*
TODOs:

!!! A 'Dockspace' is just a special group leaf (i.e no children but no window either)!
- allows us to have 'center' space
- doesn't show up as a tab when merged with another group, but still exists, so when the other
group is removed, blank space remains
- no titlebar

{
  b8 x = Dui::button(...);
  DuiId button_id = Dui::Id();
  if (x) {
    ... // can add more controls here
  }

  if (Dui::Clicked(button_id, RIGHT)) {
    ... // can add more controls here
  }

  if (Dui::StartedDrag(button_id, RIGHT)) {
    ... // can add more controls here
  }

  if (Dui::FinishedDrag(button_id, RIGHT))
    ... // can add more controls here
  }

}


*/