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
const f32 WINDOW_CONTROL_WIDTH          = 20.f;
const f32 WINDOW_MARGIN_SIZE            = 4.f;
const f32 SCROLLBAR_WIDTH               = 8.f;
const f32 DOCK_CONTROLS_WIDTH           = 100.f;

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
Vec2f clamp_to_rect(Vec2f val, Rect rect)
{
  return max(min(val, rect.xy() + rect.span()), Vec2f{0, 0});
}

#define SUB_ID(name, id) DuiId name = extend_hash(id, #name)

#define INTERACTION_STATE(var)                                                  \
  DuiId var                      = -1;                                          \
  DuiId just_started_being_##var = -1;                                          \
  DuiId just_stopped_being_##var = -1;                                          \
  DuiId was_last_control_##var   = -1;                                          \
  Vec2f start_position_##var     = {};                                          \
                                                                                \
  b8 is_##var(DuiId id) { return var == id || just_stopped_being_##var == id; } \
                                                                                \
  void set_##var(DuiId id)                                                      \
  {                                                                             \
    if (var != id) {                                                            \
      just_started_being_##var = id;                                            \
      start_position_##var     = input->mouse_pos;                              \
    }                                                                           \
                                                                                \
    var = id;                                                                   \
  }                                                                             \
  void clear_##var(DuiId id)                                                    \
  {                                                                             \
    if (var == id) {                                                            \
      var                      = 0;                                             \
      just_stopped_being_##var = id;                                            \
    }                                                                           \
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

  // Either split into children groups...
  i32 split_axis = -1;
  StaticArray<Split, 32> splits;

  // or is a child node with windows
  StaticArray<DuiId, 32> windows;
  i32 active_window_idx = -1;

  b8 is_leaf() { return windows.count > 0; }

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

    // shrink left and right sides for window controls
    Rect tabs_rect;
    tabs_rect.x      = titlebar_content_rect.x + WINDOW_CONTROL_WIDTH;
    tabs_rect.y      = titlebar_content_rect.y;
    tabs_rect.width  = titlebar_content_rect.width - (WINDOW_CONTROL_WIDTH * 2);
    tabs_rect.height = titlebar_content_rect.height + WINDOW_MARGIN_SIZE;

    f32 UNSELECTED_TAB_WIDTH = 128.f;
    f32 tab_gap              = 2.f;

    Rect rect;
    rect.x      = tabs_rect.x + (UNSELECTED_TAB_WIDTH + tab_gap) * window_idx;
    rect.y      = tabs_rect.y;
    rect.width  = UNSELECTED_TAB_WIDTH;
    rect.height = tabs_rect.height;

    return rect;
  }
  i32 get_tab_at_pos(Vec2f pos)
  {
    for (i32 i = 0; i < windows.count; i++) {
      Rect tab_rect = get_tab_margin_rect(i);
      if (pos.x >= tab_rect.x && pos.x < tab_rect.x + tab_rect.width) return i;
    }
    return 0;
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
  StaticPoolAllocator<Group, 1024> groups;

  StaticArray<Group *, 1024> root_groups;
  Window *cw = 0;

  i64 frame = 0;

  InputState *input;
  Vec2f canvas_span;

  Group *fullscreen_group = nullptr;
  Group *empty_group      = nullptr;

  INTERACTION_STATE(hot);
  INTERACTION_STATE(active);
  INTERACTION_STATE(dragging);

  Vec2f dragging_total_delta;
  Vec2f dragging_frame_delta;
};

DuiState s;

// -1 if not root
i32 get_group_z(Group *g)
{
  for (i32 i = 0; i < s.root_groups.count; i++) {
    if (s.root_groups[i] == g) return i;
  }
  return -1;
}

b8 is_empty(Group *g) { return g == s.empty_group; }
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

    // prevent dragging off screen by clamping mouse_pos
    Vec2f clamped_mouse_pos    = clamp_to_rect(s.input->mouse_pos, {{0, 0}, s.canvas_span});
    Vec2f dragging_total_delta = clamped_mouse_pos - s.start_position_active;
    s.dragging_frame_delta     = dragging_total_delta - s.dragging_total_delta;
    s.dragging_total_delta     = dragging_total_delta;

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

void add_root_group(Group *g)
{
  // sanity check
  assert(get_group_z(g) == -1);

  g->parent = nullptr;
  s.root_groups.insert(0, g);
}

void remove_root_group(Group *g)
{
  i32 z = get_group_z(g);
  s.root_groups.shift_delete(z);
}

Group *create_group(Group *parent, Rect rect)
{
  Group *g = s.groups.push_back({});
  g->id    = rand();
  g->rect  = rect;

  g->parent = parent;
  if (!parent) {
    add_root_group(g);
  }

  return g;
}

void free_group(Group *g)
{
  if (!g->parent) {
    i32 z = get_group_z(g);
    s.root_groups.shift_delete(z);
  }

  i64 index = s.groups.index_of(g);
  s.groups.remove(index);
}

Window *get_window(DuiId id) { return &s.windows.wrapped_get(id); }

// walks through children of parent_group until a leaf is found at the given pos.
Group *get_leaf_group_at_pos(Vec2f pos, Group *parent_group)
{
  if (parent_group->is_leaf() || is_empty(parent_group)) {
    return in_rect(pos, parent_group->rect) ? parent_group : nullptr;
  }

  for (i32 i = 0; i < parent_group->splits.count; i++) {
    Group *g = parent_group->splits[i].child;

    Group *possible_result = get_leaf_group_at_pos(pos, g);
    if (possible_result) return possible_result;
  }
  return nullptr;
}
Group *get_top_leaf_group_at_pos(Vec2f pos, b8 ignore_top = false)
{
  for (i32 i = ignore_top ? 1 : 0; i < s.root_groups.count; i++) {
    if (in_rect(pos, s.root_groups[i]->rect)) {
      return get_leaf_group_at_pos(pos, s.root_groups[i]);
    }
  }
  return nullptr;
}

b8 contained_in(Group *g, Group *in)
{
  if (g == in) return true;
  if (g->parent == nullptr) return false;
  return contained_in(g->parent, in);
}

void propagate_groups(Group *g, Group *parent = nullptr)
{
  if (parent) g->parent = parent;

  if (!g->is_leaf()) {
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

      propagate_groups(child, g);
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

    if (old_g->windows.count == 0 && !is_empty(old_g)) {
      free_group(old_g);
    }
  }

  w->parent = g;

  g->windows.push_back(window_id);
  g->active_window_idx = g->windows.count - 1;
}

Group *unparent_window(DuiId window_id)
{
  Window *w    = get_window(window_id);
  Group *old_g = w->parent;

  if (!is_empty(old_g) && old_g->windows.count == 1) return old_g;

  Group *g = create_group(nullptr, old_g->rect);
  parent_window(g, window_id);

  return g;
}

Window *create_new_window(DuiId id, String name, Rect rect, WindowOptions options)
{
  Window *window = s.windows.emplace_wrapped(id, {});
  window->id     = id;
  window->title  = name;

  Group *g = create_group(nullptr, rect);
  parent_window(g, window->id);

  return window;
}

void merge_splits(Group *g)
{
  for (i32 i = 0; i < g->splits.count; i++) {
    Group *child = g->splits[i].child;
    if (!child->is_leaf() && !is_empty(child) && child->split_axis == g->split_axis) {
      Split split = g->splits[i];
      g->splits.shift_delete(i);

      for (i32 child_i = 0; child_i < child->splits.count; child_i++) {
        Split new_split = child->splits[child_i];
        new_split.div_pct *= split.div_pct;
        g->splits.insert(i + child_i, new_split);

        new_split.child->parent = g;
      }

      free_group(child);
    }
  }
}

// returns true if successful.
// if any new groups are created, they will be descendents of target (i.e. target will never move
// down the hierarchy)
b8 snap_group(Group *g, Group *target, i32 axis, b8 dir, Group *sibling = nullptr)
{
  // target must either be a leaf node or be split on the same axis as the sibling
  if (!target->is_leaf() && (sibling && target->split_axis != axis)) return false;

  // try adding it to the parent first
  if (target->parent && snap_group(g, target->parent, axis, dir, target)) return true;

  if (target->is_leaf()) {
    // create a new group to move the windows to
    Group *new_g             = create_group(target, target->rect);
    new_g->windows           = target->windows;
    new_g->active_window_idx = target->active_window_idx;
    if (s.empty_group == target) s.empty_group = new_g;

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
    Group *new_g      = create_group(target, target->rect);
    new_g->split_axis = target->split_axis;
    new_g->splits     = target->splits;
    for (i32 i = 0; i < new_g->splits.count; i++) {
      Group *target = new_g->splits[i].child;
    }
    if (s.empty_group == target) s.empty_group = new_g;

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
  } else if (sibling) {  // already split, same axis, in the middle
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
  } else {  // already split, same axis, at the ends
    f32 new_split_pct = 1.f / (target->splits.count + 1);

    for (i32 i = 0; i < target->splits.count; i++) {
      target->splits[i].div_pct *= 1 - new_split_pct;
    }

    Split new_split;
    new_split.child   = g;
    new_split.div_pct = new_split_pct;
    target->splits.insert(dir ? target->splits.count : 0, new_split);
  }

  if (!g->parent) {
    remove_root_group(g);
  }
  g->parent = target;

  merge_splits(target);

  return true;
}

Group *unsnap_group(Group *g)
{
  assert(g->is_leaf());

  if (is_empty(g)) {
    // move all windows to a new group so the empty stays where it is.
    Group *new_g             = create_group(nullptr, g->rect);
    new_g->windows           = g->windows;
    new_g->active_window_idx = g->active_window_idx;

    g->windows.clear();
    g->active_window_idx = -1;
    return new_g;
  }

  if (!g->parent) return g;
  Group *parent_g = g->parent;

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

  if (parent_g->splits.count == 1) {
    Group *child                = parent_g->splits[0].child;
    parent_g->split_axis        = child->split_axis;
    parent_g->splits            = child->splits;
    parent_g->windows           = child->windows;
    parent_g->active_window_idx = child->active_window_idx;
    if (is_empty(child)) s.empty_group = parent_g;

    free_group(child);

    if (parent_g->parent) {
      merge_splits(parent_g->parent);
    }
  }

  add_root_group(g);

  return g;
}

void combine_leaf_groups(Group *target, Group *src)
{
  assert(target->is_leaf() || is_empty(target));
  assert(src->is_leaf());

  while (src->windows.count > 0) {
    parent_window(target, src->windows[0]);
  }
}

// input group might get destroyed, so returns the new one
Group *handle_dragging_group(Group *g, DuiId id)
{
  auto center = [](f32 width1, f32 width2) { return (width1 * .5f) - (width2 * .5f); };

  if (s.fullscreen_group == g) return g;

  Group *target_group = get_top_leaf_group_at_pos(s.input->mouse_pos, true);

  if (target_group && !contained_in(target_group, g)) {
    Rect window_rect = target_group->get_window_rect();

    f32 dock_controls_width = DOCK_CONTROLS_WIDTH;
    dock_controls_width     = fminf(dock_controls_width, window_rect.width * .75f);
    dock_controls_width     = fminf(dock_controls_width, window_rect.height * .75f);

    f32 three_fifth_dock_controls_width = dock_controls_width / 2.f;
    f32 fifth_dock_controls_width       = dock_controls_width / 5.f;

    Rect dock_control_rect;
    dock_control_rect.x      = window_rect.x + center(window_rect.width, dock_controls_width);
    dock_control_rect.y      = window_rect.y + center(window_rect.height, dock_controls_width);
    dock_control_rect.width  = dock_controls_width;
    dock_control_rect.height = dock_controls_width;

    Rect left_dock_control_rect;
    left_dock_control_rect.x = dock_control_rect.x;
    left_dock_control_rect.y =
        dock_control_rect.y + center(dock_controls_width, three_fifth_dock_controls_width);
    left_dock_control_rect.width  = fifth_dock_controls_width;
    left_dock_control_rect.height = three_fifth_dock_controls_width;
    SUB_ID(left_dock_control_id, id);
    DuiId left_dock_control_hot = do_hot(left_dock_control_id, left_dock_control_rect);
    if (left_dock_control_hot)
      push_rect(&forground_dl, left_dock_control_rect, l_dark);
    else
      push_rect(&forground_dl, left_dock_control_rect, l);

    Rect right_dock_control_rect;
    right_dock_control_rect.x =
        dock_control_rect.x + dock_control_rect.width - fifth_dock_controls_width;
    right_dock_control_rect.y =
        dock_control_rect.y + center(dock_controls_width, three_fifth_dock_controls_width);
    right_dock_control_rect.width  = fifth_dock_controls_width;
    right_dock_control_rect.height = three_fifth_dock_controls_width;
    SUB_ID(right_dock_control_id, id);
    DuiId right_dock_control_hot = do_hot(right_dock_control_id, right_dock_control_rect);
    if (right_dock_control_hot)
      push_rect(&forground_dl, right_dock_control_rect, l_dark);
    else
      push_rect(&forground_dl, right_dock_control_rect, l);

    Rect top_dock_control_rect;
    top_dock_control_rect.x =
        dock_control_rect.x + center(dock_controls_width, three_fifth_dock_controls_width);
    top_dock_control_rect.y      = dock_control_rect.y;
    top_dock_control_rect.width  = three_fifth_dock_controls_width;
    top_dock_control_rect.height = fifth_dock_controls_width;
    SUB_ID(top_dock_control_id, id);
    DuiId top_dock_control_hot = do_hot(top_dock_control_id, top_dock_control_rect);
    if (top_dock_control_hot)
      push_rect(&forground_dl, top_dock_control_rect, l_dark);
    else
      push_rect(&forground_dl, top_dock_control_rect, l);

    Rect bottom_dock_control_rect;
    bottom_dock_control_rect.x =
        dock_control_rect.x + center(dock_controls_width, three_fifth_dock_controls_width);
    bottom_dock_control_rect.y =
        dock_control_rect.y + dock_control_rect.height - fifth_dock_controls_width;
    bottom_dock_control_rect.width  = three_fifth_dock_controls_width;
    bottom_dock_control_rect.height = fifth_dock_controls_width;
    SUB_ID(bottom_dock_control_id, id);
    DuiId bottom_dock_control_hot = do_hot(bottom_dock_control_id, bottom_dock_control_rect);
    if (bottom_dock_control_hot)
      push_rect(&forground_dl, bottom_dock_control_rect, l_dark);
    else
      push_rect(&forground_dl, bottom_dock_control_rect, l);

    if (left_dock_control_hot) {
      push_rect(&forground_dl, window_rect, {1, 1, 1, .5});  // preview
      if (s.just_stopped_being_dragging == id) {
        snap_group(g, target_group, 1, 0);
      }
    }
    if (right_dock_control_hot) {
      push_rect(&forground_dl, window_rect, {1, 1, 1, .5});  // preview
      if (s.just_stopped_being_dragging == id) {
        snap_group(g, target_group, 1, 1);
      }
    }
    if (top_dock_control_hot) {
      push_rect(&forground_dl, window_rect, {1, 1, 1, .5});  // preview
      if (s.just_stopped_being_dragging == id) {
        snap_group(g, target_group, 0, 0);
      }
    }
    if (bottom_dock_control_hot) {
      push_rect(&forground_dl, window_rect, {1, 1, 1, .5});  // preview
      if (s.just_stopped_being_dragging == id) {
        snap_group(g, target_group, 0, 1);
      }
    }

    if (g->is_leaf() && in_rect(s.input->mouse_pos, target_group->get_titlebar_full_rect())) {
      push_rect(&forground_dl, window_rect, {1, 1, 1, .5});  // preview
      if (s.just_stopped_being_dragging == id) {
        combine_leaf_groups(target_group, g);
        g = target_group;
      }
    }
  }

  g->rect.x += s.dragging_frame_delta.x;
  g->rect.y += s.dragging_frame_delta.y;

  return g;
}

}  // namespace Dui

namespace Dui
{

void draw_group_and_children(Group *g)
{
  if (!g->is_leaf()) {
    for (i32 i = 0; i < g->splits.count; i++) {
      draw_group_and_children(g->splits[i].child);
    }
    return;
  }

  Rect titlebar_rect               = g->get_titlebar_full_rect();
  Rect titlebar_bottom_border_rect = g->get_titlebar_bottom_border_rect();
  Rect group_border_rect           = g->get_border_rect();
  Rect window_rect                 = g->get_window_rect();

  push_rect(&main_dl, titlebar_rect, d);
  push_rect(&main_dl, titlebar_bottom_border_rect, d_light);
  push_rect(&main_dl, group_border_rect, d);
  push_rect(&main_dl, window_rect, d_dark);

  if (is_empty(g)) {
    push_rect(&main_dl, titlebar_rect, {1, 0, 0, 1});
  }

  Rect titlebar_content_rect = g->get_titlebar_content_rect();

  for (i32 w_i = 0; w_i < g->windows.count; w_i++) {
    DuiId window_id = g->windows[w_i];
    Window *w       = get_window(window_id);

    Rect tab_rect = g->get_tab_margin_rect(g->get_window_idx(window_id));

    Color tab_color = d_dark;
    if (w_i == g->active_window_idx) {
      tab_color = d_light;
    }
    push_rect(&main_dl, tab_rect, tab_color);
  }
}

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
    if (s.input->keys[(i32)Keys::LSHIFT]) {
      w->scroll_offset_target.x += s.input->scrollwheel_count * 100.f;
    }
  }
  if (w->last_frame_minimum_content_span.y > content_rect.height) {
    if (!s.input->keys[(i32)Keys::LSHIFT]) {
      w->scroll_offset_target.y += s.input->scrollwheel_count * 100.f;
    }
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

    i32 window_idx = g->get_window_idx(window_id);
    Rect tab_rect  = g->get_tab_margin_rect(window_idx);

    SUB_ID(tab_handle_id, window_id);
    DuiId tab_handle_hot      = do_hot(tab_handle_id, tab_rect);
    DuiId tab_handle_active   = do_active(tab_handle_id);
    DuiId tab_handle_dragging = do_dragging(tab_handle_id);
    if (tab_handle_active) {
      g->active_window_idx = w_i;
    }
    if (tab_handle_dragging) {
      if (g->windows.count == 1 || !in_rect(s.input->mouse_pos, g->get_titlebar_full_rect())) {
        g = unparent_window(window_id);
        g = unsnap_group(g);
        g = handle_dragging_group(g, tab_handle_id);
      } else {
        i32 target_tab_idx = g->get_tab_at_pos(s.input->mouse_pos);
        DuiId tmp          = g->windows[target_tab_idx];
        g->windows.shift_delete(window_idx);
        g->windows.insert(target_tab_idx, w->id);
        g->active_window_idx = target_tab_idx;
      }
    }
  }

  Rect titlebar_rect = g->get_titlebar_margin_rect();
  SUB_ID(root_handle_id, active_window_id);
  DuiId root_handle_hot      = do_hot(root_handle_id, titlebar_rect);
  DuiId root_handle_active   = do_active(root_handle_id);
  DuiId root_handle_dragging = do_dragging(root_handle_id);
  if (root_handle_dragging) {
    if (s.input->keys[(i32)Keys::LCTRL]) {
      g = unsnap_group(g);
      g = handle_dragging_group(g, root_handle_id);
    } else {
      Group *root_g = g;
      while (root_g->parent) {
        root_g = root_g->parent;
      }
      root_g = handle_dragging_group(root_g, root_handle_id);
    }
  }
}

void start_frame_for_group(Group *g)
{
  const f32 RESIZE_HANDLES_OVERSIZE = 2.f;

  if (!g->parent) {
    // only do movement/resizing logic if window is not fullscreen
    if (s.fullscreen_group != g) {
      // easy keyboard-based window manipulation
      if (s.input->keys[(i32)Keys::LALT]) {
        SUB_ID(root_controller_control_id, g->id);
        DuiId root_controller_control_hot      = do_hot(root_controller_control_id, g->rect);
        DuiId root_controller_control_active   = do_active(root_controller_control_id);
        DuiId root_controller_control_dragging = do_dragging(root_controller_control_id);
        if (root_controller_control_dragging) {
          g->rect.x += s.dragging_frame_delta.x;
          g->rect.y += s.dragging_frame_delta.y;
        }
      }

      // resize handles that extend out of group
      {
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
          push_rect(&forground_dl, left_handle_rect, {1, 1, 1, 1});
        }
        if (left_handle_dragging) {
          g->rect.x += s.dragging_frame_delta.x;
          g->rect.width -= s.dragging_frame_delta.x;
        }

        Rect right_handle_rect;
        right_handle_rect.x = g->rect.x + g->rect.width - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
        right_handle_rect.y = g->rect.y;
        right_handle_rect.width = RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        right_handle_rect.height = g->rect.height;
        SUB_ID(right_handle_id, g->id);
        DuiId right_handle_hot      = do_hot(right_handle_id, right_handle_rect);
        DuiId right_handle_active   = do_active(right_handle_id);
        DuiId right_handle_dragging = do_dragging(right_handle_id);
        if (right_handle_hot || right_handle_dragging) {
          push_rect(&forground_dl, right_handle_rect, {1, 1, 1, 1});
        }
        if (right_handle_dragging) {
          g->rect.width += s.dragging_frame_delta.x;
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
          push_rect(&forground_dl, top_handle_rect, {1, 1, 1, 1});
        }
        if (top_handle_dragging) {
          g->rect.y += s.dragging_frame_delta.y;
          g->rect.height -= s.dragging_frame_delta.y;
        }

        Rect bottom_handle_rect;
        bottom_handle_rect.x = g->rect.x;
        bottom_handle_rect.y = g->rect.y + g->rect.height - WINDOW_BORDER_SIZE - WINDOW_MARGIN_SIZE;
        bottom_handle_rect.width = g->rect.width;
        bottom_handle_rect.height =
            RESIZE_HANDLES_OVERSIZE + WINDOW_BORDER_SIZE + WINDOW_MARGIN_SIZE;
        SUB_ID(bottom_handle_id, g->id);
        DuiId bottom_handle_hot      = do_hot(bottom_handle_id, bottom_handle_rect);
        DuiId bottom_handle_active   = do_active(bottom_handle_id);
        DuiId bottom_handle_dragging = do_dragging(bottom_handle_id);
        if (bottom_handle_hot || bottom_handle_dragging) {
          push_rect(&forground_dl, bottom_handle_rect, {1, 1, 1, 1});
        }
        if (bottom_handle_dragging) {
          g->rect.height += s.dragging_frame_delta.y;
        }
      }

      // either the right or left window control should be fully in the canvas
      {
        Rect titlebar_content_rect = g->get_titlebar_content_rect();

        Rect left_window_control  = titlebar_content_rect;
        left_window_control.width = WINDOW_CONTROL_WIDTH;

        Rect right_window_control = titlebar_content_rect;
        right_window_control.x =
            titlebar_content_rect.x + titlebar_content_rect.width - WINDOW_CONTROL_WIDTH;
        right_window_control.width = WINDOW_CONTROL_WIDTH;

        if (left_window_control.x + left_window_control.width > s.canvas_span.x) {
          g->rect.x -= (left_window_control.x + left_window_control.width) - s.canvas_span.x;
        }
        if (left_window_control.y + left_window_control.height > s.canvas_span.y) {
          g->rect.y -= (left_window_control.y + left_window_control.height) - s.canvas_span.y;
        }
        if (right_window_control.x < 0) {
          g->rect.x -= right_window_control.x;
        }
        if (right_window_control.y < 0) {
          g->rect.y -= right_window_control.y;
        }
      }
    }
  }

  if (g->is_leaf()) {
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
      if (split_move_handle_dragging) {
        f32 move_pct = s.dragging_frame_delta.y / g->rect.height;
        g->splits[i - 1].div_pct += move_pct;
        g->splits[i].div_pct -= move_pct;
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
        push_rect(&forground_dl, {10, 10, 20, 20}, {1, 1, 1, 1});
      }
      if (split_move_handle_dragging) {
        f32 move_pct = s.dragging_frame_delta.x / g->rect.width;
        g->splits[i - 1].div_pct += move_pct;
        g->splits[i].div_pct -= move_pct;
      }
    }
  }

  for (i32 i = 0; i < g->splits.count; i++) {
    start_frame_for_group(g->splits[i].child);
  }

  // after handling children, honor any contraints here
  if (s.fullscreen_group == g) {
    g->rect.x      = 0;
    g->rect.y      = 0;
    g->rect.width  = s.canvas_span.x;
    g->rect.height = s.canvas_span.y;
  }
}

void start_frame(InputState *input, RenderTarget target)
{
  s.input       = input;
  s.canvas_span = {(f32)target.width, (f32)target.height};

  s.frame++;

  s.just_started_being_hot      = -1;
  s.just_stopped_being_hot      = -1;
  s.just_started_being_active   = -1;
  s.just_stopped_being_active   = -1;
  s.just_started_being_dragging = -1;
  s.just_stopped_being_dragging = -1;

  clear_draw_list(&forground_dl);
  clear_draw_list(&main_dl);

  // one pass for input handling
  for (i32 i = 0; i < s.root_groups.count; i++) {
    Group *g = s.root_groups[i];
    start_frame_for_group(g);
  }
  // for (i32 i = 0; i < s.groups.SIZE; i++) {
  //   if (!s.groups.exists(i)) continue;
  //   if (s.groups[i].parent) continue;

  //   Group *g = &s.groups[i];
  //   start_frame_for_group(g);
  // }

  // one pass to propagate changes
  for (i32 i = 0; i < s.root_groups.count; i++) {
    Group *g = s.root_groups[i];
    propagate_groups(g);
  }
  // for (i32 i = 0; i < s.groups.SIZE; i++) {
  //   if (!s.groups.exists(i)) continue;
  //   if (s.groups[i].parent) continue;

  //   Group *g = &s.groups[i];
  //   propagate_groups(g);
  // }

  // TODO: is this the best place to handle window focus
  if (s.input->mouse_button_down_events[(i32)MouseButton::LEFT]) {
    i32 top_window_at_mouse_pos_z = -1;
    for (i32 i = 0; i < s.root_groups.count; i++) {
      if (in_rect(s.input->mouse_pos, s.root_groups[i]->rect)) {
        top_window_at_mouse_pos_z = i;
        break;
      }
    }

    if (top_window_at_mouse_pos_z > -1) {
      Group *g = s.root_groups[top_window_at_mouse_pos_z];
      if (s.fullscreen_group != g) {
        s.root_groups.shift_delete(top_window_at_mouse_pos_z);
        s.root_groups.insert(0, g);
      }
    }
  }

  // one pass for drawing, in reverse z order
  for (i32 i = s.root_groups.count - 1; i >= 0; i--) {
    Group *g = s.root_groups[i];
    draw_group_and_children(g);
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
  // push_rect(&main_dl, content_rect, w->color);

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
  // push_rect(&main_dl, rect, color);

  w->cursor_height = fmaxf(w->cursor_height, size.y);
  w->cursor.x += size.x;

  w->current_frame_minimum_content_span.x =
      fmaxf(w->current_frame_minimum_content_span.x, w->cursor.x);
  w->current_frame_minimum_content_span.y =
      fmaxf(w->current_frame_minimum_content_span.y, w->cursor.y + w->cursor_height);
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
    draw_sub(target, debug_ui_shader, dl->vb, call.vert_offset, call.tri_count * 3);
  }
  glDisable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

void debug_ui_test(RenderTarget target, InputState *input, Memory memory)
{
  static b8 init = false;
  if (!init) {
    init = true;
    init_draw_list(&main_dl);
    init_draw_list(&forground_dl);

    s.empty_group      = create_group(nullptr, {});
    s.fullscreen_group = s.empty_group;
  }

  static b8 paused = false;
  if (input->key_down_events[(i32)Keys::Q]) {
    paused = !paused;
  }

  if (paused) {
    draw_draw_list(target, &main_dl);
    draw_draw_list(target, &forground_dl);

    return;
  }

  start_frame(input, target);

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
      snap_group(p(w2), p(w1), 1, true);
    }

    if (count == 1) {
      snap_group(p(w3), p(w1), 0, false);
    }

    if (count == 2) {
      root_2 = p(w2);
      snap_group(p(w4), p(w2), 0, false);
    }

    if (count == 3) {
      snap_group(p(w5), root, 0, true);
    }

    if (count == 4) {
      snap_group(p(w6), p(w4), 1, true);
    }

    if (count == 5) {
      parent_window(p(w1), w7);
      // snap_group(p(w7), root_2, 0, true);
    }

    count++;
  }

  draw_draw_list(target, &main_dl);
  draw_draw_list(target, &forground_dl);

  std::vector<Group *> groups;
  for (i32 i = 0; i < s.groups.SIZE; i++) {
    if (!s.groups.exists(i)) continue;

    groups.push_back(&s.groups[i]);
  }
  printf("groups count: %llu\n", groups.size());
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