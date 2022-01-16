#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <stack>

#include "assets.hpp"
#include "common.hpp"
#include "graphics/graphics.hpp"
#include "math.hpp"
#include "resources.hpp"
#include "util.hpp"

typedef u64 ImmId;

namespace Imm {
// http://www.cse.yorku.ca/~oz/hash.html
ImmId hash(String str) {
  unsigned long hash = 5381;

  for (int i = 0; i < str.len; i++) {
    int c = str.data[i];
    hash  = ((hash << 5) + hash) + c;
  }

  return hash;
}

struct DrawItem {
  DrawItem() {}
  enum struct Type {
    CLIP,
    TEXT,
    RECT,
    RECT_PTR,
    QUAD,
    TEXTURE,
  };

  struct ClipDrawItem {
    Rect rect;
  };
  struct TextDrawItem {
    AllocatedString<128> text;
    Vec2f pos;
    float size;
    Color color;
  };
  struct RectDrawItem {
    Rect rect;
    Color color;
  };
  struct RectPtrDrawItem {
    Rect *rect;
    Color color;
  };
  struct QuadDrawItem {
    Vec2f points[4];
    Color color;
  };
  struct TexDrawItem {
    Texture texture;
    Rect rect;
  };

  Type type;
  union {
    ClipDrawItem clip_draw_item;
    TextDrawItem text_draw_item;
    RectDrawItem rect_draw_item;
    RectPtrDrawItem rect_ptr_draw_item;
    QuadDrawItem quad_draw_item;
    TexDrawItem tex_draw_item;
  };
};
struct DrawList {
  std::vector<DrawItem> buf;

  void push_clip(Rect rect) {
    DrawItem item;
    item.type           = DrawItem::Type::CLIP;
    item.clip_draw_item = {rect};
    buf.push_back(item);
  }
  void push_text(String text, Vec2f pos, float size, Color color) {
    DrawItem item;
    item.type           = DrawItem::Type::TEXT;
    item.text_draw_item = {string_to_allocated_string<128>(text), pos, size, color};
    buf.push_back(item);
  }
  void push_rect(Rect rect, Color color) {
    DrawItem item;
    item.type           = DrawItem::Type::RECT;
    item.rect_draw_item = {rect, color};
    buf.push_back(item);
  }
  void push_rect_ptr(Rect *rect, Color color) {
    DrawItem item;
    item.type               = DrawItem::Type::RECT_PTR;
    item.rect_ptr_draw_item = {rect, color};
    buf.push_back(item);
  }
  void push_quad(Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3, Color color) {
    DrawItem item;
    item.type           = DrawItem::Type::QUAD;
    item.quad_draw_item = {{p0, p1, p2, p3}, color};
    buf.push_back(item);
  }
  void push_tex(Texture texture, Rect rect) {
    DrawItem item;
    item.type          = DrawItem::Type::TEXTURE;
    item.tex_draw_item = {texture, rect};
    buf.push_back(item);
  }
};
struct Window {
  AllocatedString<128> name;
  bool visible = true;
  Rect rect{};

  Rect content_rect = {};
  float scroll      = 0;
  Vec2f last_size   = {0, 0};

  int z;
  DrawList draw_list;

  Window() {}
  Window(String name, Rect rect) {
    this->name = string_to_allocated_string<128>(name);
    this->rect = rect;
  }
};
struct ImmStyle {
  float menubar_height = 30.f;

  float title_font_size       = 20;
  float content_font_size     = 16;
  Vec2f inner_padding         = {4, 4};
  float window_control_border = 2;
  float element_gap           = 1.5f;

  Color content_default_text_color     = {0, 0, 0, 1};
  Color content_highlighted_text_color = {1, 1, 1, 1};
  Color content_background_color       = {.1, .1, .1, 1};
};
struct UiState {
  Assets *assets;
  RenderTarget target;
  InputState *input;
  EditorCamera *camera;

  ImmStyle style;

  bool menubar_visible       = false;
  ImmId current_menubar_menu = 0;
  float next_menubar_x       = 0;

  Rect content_rect;

  std::map<ImmId, Window> windows;

  ImmId current_window = 0;
  ImmId current_popup  = 0;

  ImmId top_window_at_current_mouse_pos = 0;
  ImmId popup_at_current_mouse_pos      = 0;

  ImmId hot                   = 0;
  ImmId active                = 0;
  Vec2f active_position       = {};
  ImmId just_activated        = 0;
  ImmId just_deactivated      = 0;
  ImmId dragging              = 0;
  Vec2f drag_distance         = {};
  ImmId just_stopped_dragging = 0;
  ImmId selected              = 0;
  ImmId just_selected         = 0;
  ImmId just_unselected       = 0;

  Vec3f gizmo_last_contact_point;

  bool last_element_hot      = false;
  bool last_element_active   = false;
  bool last_element_dragging = false;
  bool last_element_selected = false;

  ImmId anchored_left         = 0;
  int anchored_left_priority  = 0;
  ImmId anchored_right        = 0;
  int anchored_right_priority = 0;
  ImmId anchored_up           = 0;
  int anchored_up_priority    = 0;
  ImmId anchored_down         = 0;
  int anchored_down_priority  = 0;
  ImmId anchored_center       = 0;

  float num_input_starting_val;
  AllocatedString<1024> in_progress_string;

  StackAllocator per_frame_alloc;

  DrawList global_draw_list;
};

UiState state;

Window *get_current_window() {
  if (state.windows.count(state.current_window)) return &state.windows[state.current_window];
  return nullptr;
}
}  // namespace Imm

// containers
namespace Imm {

bool vertical_layout = true;

struct Container {
  Rect content_rect;
  DrawList *draw_list;
  bool visible = true;

  float next_line_y   = 0.f;
  Vec2f next_elem_pos = {0, 0};

  Vec4f actual_content_bounds = {0, 0, 0, 0};
  Vec2f *final_size_ptr       = nullptr;

  Container(Rect content_rect, DrawList *draw_list) {
    this->content_rect = content_rect;
    this->draw_list    = draw_list;

    next_elem_pos = {state.style.inner_padding.x, state.style.inner_padding.y};
    next_line_y   = next_elem_pos.y;
  }

  Rect place(Vec2f size, bool commit = true, bool stretch = false) {
    Rect rect;
    if (vertical_layout) {
      rect.x      = 0;
      rect.y      = next_line_y;
      rect.width  = size.x;
      rect.height = size.y;

      if (stretch && rect.x + rect.width < content_rect.width) {
        rect.width += content_rect.width - (rect.x + rect.width);
      }
    }

    if (commit) {
      next_elem_pos.x = rect.x + rect.width + state.style.element_gap;
      next_elem_pos.y = rect.y;
      next_line_y     = fmax(next_line_y, rect.y + rect.height + state.style.element_gap);

      actual_content_bounds.x = fmin(actual_content_bounds.x, rect.x);
      actual_content_bounds.y = fmin(actual_content_bounds.y, rect.y);
      actual_content_bounds.z = fmax(actual_content_bounds.z, rect.x + rect.width);
      actual_content_bounds.w = fmax(actual_content_bounds.w, rect.y + rect.height);

      if (final_size_ptr) {
        final_size_ptr->x = actual_content_bounds.z - actual_content_bounds.x;
        final_size_ptr->y = actual_content_bounds.w - actual_content_bounds.y;
      }
    }

    return {content_rect.x + rect.x, content_rect.y + rect.y, rect.width, rect.height};
  }
};

std::stack<Container> containers;

Container *push_container(Rect rect, DrawList *draw_list) {
  containers.push({rect, draw_list});
  return &containers.top();
}
Container *push_container(Container container) {
  containers.push(container);
  return &containers.top();
}

void push_subcontainer(Rect rect) {
  Container c = {rect, containers.top().draw_list};
  c.visible   = containers.top().visible;
  containers.push(c);
}

// TODO: pop_container is barely being called anywhere and when it is, theres no validation that
// they're popping the right container
Container pop_container() {
  auto top = containers.top();
  containers.pop();
  return top;
}

Container *get_current_container() {
  if (containers.size() == 0) return nullptr;
  return &containers.top();
}

}  // namespace Imm

// popups
namespace Imm {

struct Popup {
  Vec2f pos;
  Vec2f size;
};

// - there is only one layer of popups, they shouldn't overlap
// - popups are asssumed to always be on top
//
// - id need only be unique to popups
std::map<ImmId, Popup> popups;
std::deque<ImmId> open_popups;
std::stack<ImmId> popup_stack;

ImmId get_current_popup() {
  if (popup_stack.size()) {
    return popup_stack.top();
  }
  return 0;
}

bool is_popup_open(ImmId id) {
  for (int i = 0; i < open_popups.size(); i++) {
    if (open_popups[i] == id) return true;
  }
  return false;
}

void start_frame_popups() {
  // TODO: this doesn't need to be done for every popup, can be done once a frame
  if (state.input->mouse_button_down_events[(int)MouseButton::LEFT] &&
      state.popup_at_current_mouse_pos == 0) {
    open_popups.clear();
  }
}

void open_popup(ImmId id) {
  open_popups.clear();
  open_popups.push_back(id);
}
void open_popup(String name) {
  ImmId id = hash(name);
  open_popup(id);
}

void close_popup() {
  ImmId id = get_current_popup();
  while (open_popups.back() != id) {
    open_popups.pop_back();
  }
}

// TODO: might be able to merge popups and subpopups
void open_subpopup() {}

// totally fine if width / height = 0
Container *start_popup(ImmId id, Rect initial_rect) {
  if (popups.count(id) == 0) {
    popups[id] = {
        {initial_rect.x, initial_rect.y},
        {initial_rect.width, initial_rect.height},
    };
  }
  Popup *popup = &popups[id];

  Container *c      = push_container({initial_rect.x, initial_rect.y, popup->size.x, popup->size.y},
                                &state.global_draw_list);
  c->final_size_ptr = &popup->size;

  bool visible = is_popup_open(id);
  if (visible) {
    popup_stack.push(id);
    c->draw_list->push_rect(c->content_rect, {1, 0, 0, 1});
  }
  c->visible = visible;

  return c;
}

void end_popup() {
  if (pop_container().visible) popup_stack.pop();
}

ImmId get_popup_at_position(Vec2f pos) {
  for (auto &[id, popup] : popups) {
    Rect rect = {popup.pos.x, popup.pos.y, popup.size.x, popup.size.y};
    if (in_rect(pos, rect) && is_popup_open(id)) {
      return id;
    }
  }
  return 0;
}

}  // namespace Imm

namespace Imm {
Rect relative_to_absolute(Rect relative) {
  return {relative.x * state.target.width, relative.y * state.target.height,
          relative.width * state.target.width, relative.height * state.target.height};
}

float get_text_width(String text, float size) {
  Font *font = state.assets->get_font(FONT_ROBOTO_CONDENSED_LIGHT, size);
  return get_text_width(*font, text);
}

bool do_hoverable(ImmId id, bool is_hot) {
  if (is_hot) {
    state.hot = id;
  } else if (state.hot == id) {
    state.hot = 0;
  }
  state.last_element_hot = (state.hot == id);
  return state.last_element_hot;
}
bool do_hoverable(ImmId id, Rect rect, Rect mask = {}) {
  bool in_top_container = state.popup_at_current_mouse_pos
                              ? state.popup_at_current_mouse_pos == get_current_popup()
                              : state.top_window_at_current_mouse_pos == state.current_window;
  bool is_hot           = in_top_container && in_rect(state.input->mouse_pos, rect, mask);
  return do_hoverable(id, is_hot);
}

bool do_active(ImmId id) {
  if (state.input->mouse_button_up_events[(int)MouseButton::LEFT]) {
    if (state.active == id) {
      state.active           = 0;
      state.just_deactivated = id;
    }
  }

  if (state.input->mouse_button_down_events[(int)MouseButton::LEFT]) {
    if (state.hot == id) {
      state.active          = id;
      state.just_activated  = id;
      state.active_position = state.input->mouse_pos;
    } else {
      if (state.active == id) {
        state.active = 0;
      }
    }
  }
  state.last_element_active = (state.active == id);
  return state.last_element_active;
}
bool do_draggable(ImmId id) {
  bool active = state.active == id;
  if (active) {
    if ((state.input->mouse_pos - state.active_position).len() > 2) {
      state.dragging = id;
    }
    state.drag_distance         = state.input->mouse_pos - state.active_position;
    state.last_element_dragging = (state.dragging == id);
    return (state.dragging == id);
  } else if (state.dragging == id) {
    state.dragging              = 0;
    state.just_stopped_dragging = id;
  }
  return false;
}
bool do_selectable(ImmId id, bool on_down = false) {
  bool hot                   = state.hot == id;
  bool triggered             = on_down ? state.just_activated == id : state.just_deactivated == id;
  bool just_stopped_dragging = state.just_stopped_dragging == id;
  if (hot && triggered && !just_stopped_dragging) {
    if (state.selected != 0) {
      state.just_unselected = state.selected;
    }
    state.selected      = id;
    state.just_selected = id;
  } else {
    if (state.input->mouse_button_down_events[(int)MouseButton::LEFT]) {
      if (state.selected == id) {
        state.just_unselected = id;
        state.selected        = 0;
      }
    }
  }

  state.last_element_selected = (state.selected == id);
  return state.last_element_selected;
}

void start_frame(RenderTarget target, InputState *input, Assets *assets, EditorCamera *camera) {
  state.per_frame_alloc.reset();

  state.target = target;
  state.input  = input;
  state.assets = assets;
  state.camera = camera;

  Rect new_content_rect = {0, 0, (float)target.width, (float)target.height};
  if (state.menubar_visible) {
    new_content_rect.y += state.style.menubar_height;
    new_content_rect.height -= state.style.menubar_height;
  }
  state.menubar_visible = false;

  // deal with rendertarget size changes
  if ((new_content_rect != state.content_rect) && state.content_rect.width != 0 &&
      state.content_rect.height != 0) {
    if (state.anchored_up != 0) {
      Window &window     = state.windows[state.anchored_up];
      float height_pct   = window.rect.height / state.content_rect.height;
      float new_height   = height_pct * new_content_rect.height;
      window.rect.height = new_height;
    }
    if (state.anchored_down != 0) {
      Window &window = state.windows[state.anchored_down];
      float y_pct    = window.rect.y / state.content_rect.height;
      float new_y    = y_pct * new_content_rect.height;
      window.rect.y  = new_y;
    }
    if (state.anchored_left != 0) {
      Window &window    = state.windows[state.anchored_left];
      float width_pct   = window.rect.width / state.content_rect.width;
      float new_width   = width_pct * new_content_rect.width;
      window.rect.width = new_width;
    }
    if (state.anchored_right != 0) {
      Window &window = state.windows[state.anchored_right];
      float x_pct    = window.rect.x / state.content_rect.width;
      float new_x    = x_pct * new_content_rect.width;
      window.rect.x  = new_x;
    }
  }
  state.content_rect = new_content_rect;

  // find and cache the top window at the current position
  state.top_window_at_current_mouse_pos = 0;
  int highest_z                         = INT_MAX;
  for (const auto &[id, window] : state.windows) {
    if (window.visible && window.z < highest_z && in_rect(state.input->mouse_pos, window.rect)) {
      highest_z                             = window.z;
      state.top_window_at_current_mouse_pos = id;
    }
  }
  // find and cache the top popup at the current position
  state.popup_at_current_mouse_pos = get_popup_at_position(state.input->mouse_pos);

  state.current_window = 0;

  state.next_menubar_x = 0;

  state.just_activated        = 0;
  state.just_deactivated      = 0;
  state.just_stopped_dragging = 0;
  state.just_selected         = 0;
  state.just_unselected       = 0;

  state.last_element_hot      = false;
  state.last_element_active   = false;
  state.last_element_dragging = false;
  state.last_element_selected = false;

  start_frame_popups();
}

void start_window(String title, Rect rect) {
  ImmId me = hash(title);
  ImmId resize_handle_id =
      me + 1;  // I don't know the hash function very well, hopefully this is ok
  ImmId scrollbar_id     = me + 2;
  ImmId top_handle_id    = me + 3;
  ImmId bottom_handle_id = me + 4;
  ImmId left_handle_id   = me + 5;
  ImmId right_handle_id  = me + 6;

  state.current_window = me;

  if (state.windows.count(me) == 0) {
    Window window(title, rect);
    window.z          = state.windows.size();
    state.windows[me] = window;
  }
  Window &window = state.windows[me];

  Rect titlebar_rect   = {window.rect.x + state.style.window_control_border,
                        window.rect.y + state.style.window_control_border,
                        window.rect.width - (state.style.window_control_border * 2),
                        state.style.title_font_size + (state.style.inner_padding.y * 2) -
                            state.style.window_control_border};
  Color titlebar_color = {.9, .2, .3, 1};

  // moving window
  bool hot      = do_hoverable(me, titlebar_rect);
  bool active   = do_active(me);
  bool dragging = do_draggable(me);
  if (dragging) {
    window.rect.x += state.input->mouse_pos_delta.x;
    window.rect.y += state.input->mouse_pos_delta.y;
    titlebar_color = darken(titlebar_color, 0.2f);

    if (state.anchored_up == me) {
      state.anchored_up          = 0;
      state.anchored_up_priority = 0;
    }
    if (state.anchored_down == me) {
      state.anchored_down          = 0;
      state.anchored_down_priority = 0;
    }
    if (state.anchored_left == me) {
      state.anchored_left          = 0;
      state.anchored_left_priority = 0;
    }
    if (state.anchored_right == me) {
      state.anchored_right          = 0;
      state.anchored_right_priority = 0;
    }
    if (state.anchored_center == me) {
      state.anchored_center = 0;
    }
  }

  // snapping
  Rect top_handle    = relative_to_absolute({0.45, 0, 0.1, 0.05});
  Rect bottom_handle = relative_to_absolute({0.45, .95, 0.1, 0.05});
  Rect left_handle   = relative_to_absolute({0, 0.45, 0.025, 0.1});
  Rect right_handle  = relative_to_absolute({.975, 0.45, 0.025, 0.1});
  Rect center_handle = relative_to_absolute({.475, 0.475, 0.05, 0.05});
  if (dragging) {
    state.global_draw_list.push_rect(top_handle, {1, 1, 1, 0.5});
    state.global_draw_list.push_rect(bottom_handle, {1, 1, 1, 0.5});
    state.global_draw_list.push_rect(left_handle, {1, 1, 1, 0.5});
    state.global_draw_list.push_rect(right_handle, {1, 1, 1, 0.5});
    state.global_draw_list.push_rect(center_handle, {1, 1, 1, 0.5});
  }
  if (state.just_stopped_dragging == me) {
    auto top_priority = [](int *new_top) {
      if (state.anchored_up_priority > *new_top) {
        state.anchored_up_priority -= 1;
      }
      if (state.anchored_down_priority > *new_top) {
        state.anchored_down_priority -= 1;
      }
      if (state.anchored_left_priority > *new_top) {
        state.anchored_left_priority -= 1;
      }
      if (state.anchored_right_priority > *new_top) {
        state.anchored_right_priority -= 1;
      }
      *new_top = 5;
    };
    if (in_rect(state.input->mouse_pos, top_handle)) {
      state.anchored_up = me;
      top_priority(&state.anchored_up_priority);
    }
    if (in_rect(state.input->mouse_pos, bottom_handle)) {
      state.anchored_down = me;
      top_priority(&state.anchored_down_priority);
    }
    if (in_rect(state.input->mouse_pos, left_handle)) {
      state.anchored_left = me;
      top_priority(&state.anchored_left_priority);
    }
    if (in_rect(state.input->mouse_pos, right_handle)) {
      state.anchored_right = me;
      top_priority(&state.anchored_right_priority);
    }
    if (in_rect(state.input->mouse_pos, center_handle)) {
      state.anchored_center = me;
    }
  }

  // resizing
  Rect resize_handle_rect = {
      window.rect.x + window.rect.width - state.style.inner_padding.x * 2,
      window.rect.y + window.rect.height - state.style.inner_padding.y * 2,
      state.style.inner_padding.x * 2,
      state.style.inner_padding.y * 2,
  };
  bool resize_handle_hot      = do_hoverable(resize_handle_id, resize_handle_rect);
  bool resize_handle_active   = do_active(resize_handle_id);
  bool resize_handle_dragging = do_draggable(resize_handle_id);
  if (resize_handle_dragging) {
    window.rect.width += state.input->mouse_pos_delta.x;
    window.rect.height += state.input->mouse_pos_delta.y;
    window.rect.width = fmax(window.rect.width, state.style.inner_padding.x * 2);
    window.rect.height =
        fmax(window.rect.height, titlebar_rect.height + state.style.inner_padding.y * 2);
  }
  Rect control_border_top        = {window.rect.x, window.rect.y, window.rect.width,
                             state.style.window_control_border};
  Color control_border_top_color = {0, 0, 0, 1};
  Rect resize_handle_top = {control_border_top.x, control_border_top.y, control_border_top.width,
                            control_border_top.height + 5};
  Rect control_border_bottom = {
      window.rect.x, window.rect.y + window.rect.height - state.style.window_control_border,
      window.rect.width, state.style.window_control_border};
  Color control_border_bottom_color = {0, 0, 0, 1};
  Rect resize_handle_bottom         = {control_border_bottom.x, control_border_bottom.y - 5,
                               control_border_bottom.width, control_border_bottom.height + 5};
  Rect control_border_left = {window.rect.x, window.rect.y, state.style.window_control_border,
                              window.rect.height};
  Color control_border_left_color = {0, 0, 0, 1};
  Rect resize_handle_left         = {control_border_left.x, control_border_left.y,
                             control_border_left.width + 5, control_border_left.height};
  Rect control_border_right       = {
      window.rect.x + window.rect.width - state.style.window_control_border, window.rect.y,
      state.style.window_control_border, window.rect.height};
  Color control_border_right_color = {0, 0, 0, 1};
  Rect resize_handle_right         = {control_border_right.x - 5, control_border_right.y,
                              control_border_right.width + 5, control_border_right.height};
  bool top_handle_hot              = do_hoverable(top_handle_id, resize_handle_top);
  bool top_handle_active           = do_active(top_handle_id);
  bool top_handle_dragging         = do_draggable(top_handle_id);
  bool bottom_handle_hot           = do_hoverable(bottom_handle_id, resize_handle_bottom);
  bool bottom_handle_active        = do_active(bottom_handle_id);
  bool bottom_handle_dragging      = do_draggable(bottom_handle_id);
  bool left_handle_hot             = do_hoverable(left_handle_id, resize_handle_left);
  bool left_handle_active          = do_active(left_handle_id);
  bool left_handle_dragging        = do_draggable(left_handle_id);
  bool right_handle_hot            = do_hoverable(right_handle_id, resize_handle_right);
  bool right_handle_active         = do_active(right_handle_id);
  bool right_handle_dragging       = do_draggable(right_handle_id);
  if (top_handle_hot) {
    control_border_top_color = {0, 0, 1, 1};
  } else if (bottom_handle_hot) {
    control_border_bottom_color = {0, 0, 1, 1};
  } else if (left_handle_hot) {
    control_border_left_color = {0, 0, 1, 1};
  } else if (right_handle_hot) {
    control_border_right_color = {0, 0, 1, 1};
  }
  if (top_handle_dragging) {
    window.rect.y += state.input->mouse_pos_delta.y;
    window.rect.height -= state.input->mouse_pos_delta.y;
  } else if (bottom_handle_dragging) {
    window.rect.height += state.input->mouse_pos_delta.y;
  } else if (left_handle_dragging) {
    window.rect.x += state.input->mouse_pos_delta.x;
    window.rect.width -= state.input->mouse_pos_delta.x;
  } else if (right_handle_dragging) {
    window.rect.width += state.input->mouse_pos_delta.x;
  }

  auto constrained = [](i32 my_priority, ImmId other_window, i32 other_priority) {
    return other_window && state.windows[other_window].visible && other_priority > my_priority;
  };
  if (state.anchored_up == me) {
    window.rect.y = 0;
    if (constrained(state.anchored_up_priority, state.anchored_left, state.anchored_left_priority))
      window.rect.x =
          state.windows[state.anchored_left].rect.x + state.windows[state.anchored_left].rect.width;
    else
      window.rect.x = 0;

    if (constrained(state.anchored_up_priority, state.anchored_right,
                    state.anchored_right_priority))
      window.rect.set_right(state.windows[state.anchored_right].rect.x);
    else
      window.rect.set_right(state.content_rect.x + state.content_rect.width);
  }
  if (state.anchored_down == me) {
    window.rect.set_bottom(state.content_rect.y + state.content_rect.height);
    if (constrained(state.anchored_down_priority, state.anchored_left,
                    state.anchored_left_priority))
      window.rect.x =
          state.windows[state.anchored_left].rect.x + state.windows[state.anchored_left].rect.width;
    else
      window.rect.x = 0;

    if (constrained(state.anchored_down_priority, state.anchored_right,
                    state.anchored_right_priority))
      window.rect.set_right(state.windows[state.anchored_right].rect.x);
    else
      window.rect.set_right(state.content_rect.x + state.content_rect.width);
  }
  if (state.anchored_left == me) {
    window.rect.x = 0;
    if (constrained(state.anchored_left_priority, state.anchored_up, state.anchored_up_priority))
      window.rect.y =
          state.windows[state.anchored_up].rect.y + state.windows[state.anchored_up].rect.height;
    else
      window.rect.y = state.content_rect.y;

    if (constrained(state.anchored_left_priority, state.anchored_down,
                    state.anchored_down_priority))
      window.rect.set_bottom(state.windows[state.anchored_down].rect.y);
    else
      window.rect.set_bottom(state.content_rect.y + state.content_rect.height);
  }
  if (state.anchored_right == me) {
    window.rect.set_right(state.content_rect.x + state.content_rect.width);
    if (constrained(state.anchored_right_priority, state.anchored_up, state.anchored_up_priority))
      window.rect.y =
          state.windows[state.anchored_up].rect.y + state.windows[state.anchored_up].rect.height;
    else
      window.rect.y = state.content_rect.y;

    if (constrained(state.anchored_right_priority, state.anchored_down,
                    state.anchored_down_priority))
      window.rect.set_bottom(state.windows[state.anchored_down].rect.y);
    else
      window.rect.set_bottom(state.content_rect.y + state.content_rect.height);
  }
  if (state.anchored_center == me) {
    if (constrained(0, state.anchored_up, state.anchored_up_priority))
      window.rect.y =
          state.windows[state.anchored_up].rect.y + state.windows[state.anchored_up].rect.height;
    else
      window.rect.y = state.content_rect.y;

    if (constrained(0, state.anchored_down, state.anchored_down_priority))
      window.rect.set_bottom(state.windows[state.anchored_down].rect.y);
    else
      window.rect.set_bottom(state.content_rect.y + state.content_rect.height);

    if (constrained(0, state.anchored_left, state.anchored_left_priority))
      window.rect.x =
          state.windows[state.anchored_left].rect.x + state.windows[state.anchored_left].rect.width;
    else
      window.rect.x = 0;

    if (constrained(0, state.anchored_right, state.anchored_right_priority))
      window.rect.set_right(state.windows[state.anchored_right].rect.x);
    else
      window.rect.set_right(state.content_rect.x + state.content_rect.width);
  }

  // keep window in bounds
  if (window.rect.x > state.content_rect.width - 30) window.rect.x = state.content_rect.width - 30;
  if (window.rect.y > state.content_rect.height - 30)
    window.rect.y = state.content_rect.height - 30;
  if (window.rect.x + window.rect.width < 30)
    window.rect.x += 30 - (window.rect.x + window.rect.width);
  if (window.rect.y < state.content_rect.y) window.rect.y = state.content_rect.y;

  // clamp size
  window.rect.width  = fmax(window.rect.width, 30);
  window.rect.height = fmax(window.rect.height, 50);

  Rect content_container_rect = {
      window.rect.x + state.style.window_control_border, window.rect.y + titlebar_rect.height,
      window.rect.width - (state.style.window_control_border * 2),
      window.rect.height - (titlebar_rect.height + state.style.window_control_border)};
  window.content_rect = {content_container_rect.x, content_container_rect.y,
                         content_container_rect.width, content_container_rect.height};

  // scrollbar
  Rect scrollbar_rect;
  bool scrollbar_visible = false;
  Color scrollbar_color  = {0, 1, 0, 1};
  if (window.last_size.y > window.content_rect.height) {
    scrollbar_visible = true;

    float scrollbar_width    = 10;
    float scrollbar_gap      = 2;
    Rect scrollbar_area_rect = {
        window.content_rect.x + window.content_rect.width + scrollbar_gap - scrollbar_width,
        window.content_rect.y, scrollbar_width, window.content_rect.height};
    window.content_rect.width -= scrollbar_area_rect.width + scrollbar_gap;

    float scroll_vertical_range = window.last_size.y - window.content_rect.height;
    float scrollbar_height =
        window.content_rect.height / window.last_size.y * scrollbar_area_rect.height;
    float scrollbar_vertical_range = scrollbar_area_rect.height - scrollbar_height;
    float scrollbar_percent        = -window.scroll / scroll_vertical_range;
    float scrollbar_ratio          = scroll_vertical_range / scrollbar_vertical_range;

    scrollbar_rect = {scrollbar_area_rect.x,
                      scrollbar_area_rect.y + (scrollbar_vertical_range * scrollbar_percent),
                      scrollbar_width, scrollbar_height};

    bool scrollbar_hot      = do_hoverable(scrollbar_id, scrollbar_rect);
    bool scrollbar_active   = do_active(scrollbar_id);
    bool scrollbar_dragging = do_draggable(scrollbar_id);

    if (scrollbar_hot) {
      scrollbar_color = lighten(scrollbar_color, 0.05f);
    }
    if (scrollbar_active) {
      scrollbar_color = lighten(scrollbar_color, 0.05f);
    }
    if (scrollbar_dragging) {
      window.scroll -= scrollbar_ratio * state.input->mouse_pos_delta.y;
    }
    if (in_rect(state.input->mouse_pos, content_container_rect)) {
      window.scroll += state.input->scrollwheel_count * 20;
    }

    if (window.scroll > 0) {
      window.scroll     = 0;
      scrollbar_percent = 0;
    }
    if (-window.scroll > scroll_vertical_range) {
      scrollbar_percent = 1;
      window.scroll     = -scroll_vertical_range;
    }
  } else {
    window.scroll = 0;
  }
  window.last_size = {0, 0};

  // recalculate some rects
  titlebar_rect = {window.rect.x, window.rect.y, window.rect.width,
                   state.style.title_font_size + state.style.inner_padding.y * 2};

  // draw
  if (window.visible) {
    window.draw_list.push_rect(titlebar_rect, titlebar_color);
    window.draw_list.push_text(title,
                               {titlebar_rect.x + state.style.inner_padding.x,
                                titlebar_rect.y + state.style.inner_padding.y},
                               state.style.title_font_size, {1, 1, 1, 1});
    window.draw_list.push_rect(content_container_rect, {1, 1, 1, .95});

    if (scrollbar_visible) window.draw_list.push_rect(scrollbar_rect, scrollbar_color);

    window.draw_list.push_rect(control_border_top, control_border_top_color);
    window.draw_list.push_rect(control_border_bottom, control_border_bottom_color);
    window.draw_list.push_rect(control_border_left, control_border_left_color);
    window.draw_list.push_rect(control_border_right, control_border_right_color);

    window.draw_list.push_rect(resize_handle_rect, {.5, .5, .5, 1});

    window.draw_list.push_clip(window.content_rect);
  }

  auto container            = push_container(window.content_rect, &window.draw_list);
  container->visible        = window.visible;
  container->final_size_ptr = &window.last_size;
  container->next_line_y    = window.scroll;
}
void end_window() {
  Window *window = get_current_window();
  assert(window);

  window->draw_list.push_clip({0, 0, 0, 0});
  pop_container();
}

void end_frame(Assets *assets) {
  // window ordering
  if (state.input->mouse_button_down_events[(int)MouseButton::LEFT] &&
      state.top_window_at_current_mouse_pos) {
    int focused_window_z = state.windows[state.top_window_at_current_mouse_pos].z;
    for (auto &[id, window] : state.windows) {
      if (window.z <= focused_window_z) {
        window.z += 1;
      }
    }
    state.windows[state.top_window_at_current_mouse_pos].z = 0;
  }

  // keep anchored windows at the bottom
  auto shuffle_down = [](Window &window) {
    for (auto &[id, other_window] : state.windows) {
      if (other_window.z > window.z) {
        other_window.z -= 1;
      }
    }
    window.z = state.windows.size() - 1;
  };
  if (state.anchored_up) {
    shuffle_down(state.windows[state.anchored_up]);
  }
  if (state.anchored_down) {
    shuffle_down(state.windows[state.anchored_down]);
  }
  if (state.anchored_left) {
    shuffle_down(state.windows[state.anchored_left]);
  }
  if (state.anchored_right) {
    shuffle_down(state.windows[state.anchored_right]);
  }
  if (state.anchored_center) {
    shuffle_down(state.windows[state.anchored_center]);
  }

  state.target.bind();
  debug_begin_immediate();
  auto do_draw_list = [&](DrawList &draw_list) {
    for (int i = 0; i < draw_list.buf.size(); i++) {
      DrawItem item = draw_list.buf[i];
      switch (item.type) {
        case DrawItem::Type::CLIP: {
          if (item.clip_draw_item.rect.width == 0 && item.clip_draw_item.rect.height == 0)
            end_scissor();
          else
            start_scissor(state.target, item.clip_draw_item.rect);
        } break;
        case DrawItem::Type::TEXT: {
          Font *font = assets->get_font(FONT_ROBOTO_CONDENSED_REGULAR, item.text_draw_item.size);
          draw_text(*font, state.target, item.text_draw_item.text, item.text_draw_item.pos.x,
                    item.text_draw_item.pos.y, item.text_draw_item.color);
        } break;
        case DrawItem::Type::RECT: {
          draw_rect(state.target, item.rect_draw_item.rect, item.rect_draw_item.color);
        } break;
        case DrawItem::Type::RECT_PTR: {
          draw_rect(state.target, *item.rect_ptr_draw_item.rect, item.rect_ptr_draw_item.color);
        } break;
        case DrawItem::Type::QUAD: {
          debug_draw_immediate(state.target, item.quad_draw_item.points[0],
                               item.quad_draw_item.points[1], item.quad_draw_item.points[2],
                               item.quad_draw_item.points[3], item.quad_draw_item.color);
        } break;
        case DrawItem::Type::TEXTURE: {
          draw_textured_rect(state.target, item.tex_draw_item.rect, {1, 1, 1, 1},
                             item.tex_draw_item.texture);
        } break;
      }
    }
    draw_list.buf.clear();
    end_scissor();
  };
  for (int z = state.windows.size() - 1; z >= 0; z--) {
    for (auto &[id, window] : state.windows) {
      if (window.z == z) {
        do_draw_list(window.draw_list);
      }
    }
  }
  do_draw_list(state.global_draw_list);
  debug_end_immediate();
}

void label(String text) {
  auto c = get_current_container();
  if (!c || !c->visible) return;

  Rect rect = c->place({c->content_rect.width, state.style.content_font_size});

  c->draw_list->push_text(text, {rect.x, rect.y}, state.style.content_font_size,
                          state.style.content_default_text_color);
}

bool button(ImmId me, String text) {
  auto c = get_current_container();
  if (!c || !c->visible) return false;

  float border     = 5;
  float text_width = get_text_width(text, state.style.content_font_size);
  Rect rect = c->place({text_width + border * 2, state.style.content_font_size + border * 2});

  bool hot       = do_hoverable(me, rect, c->content_rect);
  bool active    = do_active(me);
  bool triggered = (state.just_deactivated == me && hot);

  Color color   = {.9, .3, .2, 1};
  Color light   = lighten(color, .1);
  Color lighter = lighten(color, .15);
  Color dark    = darken(color, .1);
  Color darker  = darken(color, .15);

  Color top   = lighter;
  Color down  = darker;
  Color left  = light;
  Color right = dark;

  float text_y_offset = 0;
  if (active) {
    color = darken(color, 0.05f);

    top   = darker;
    down  = lighter;
    left  = dark;
    right = light;

    text_y_offset = 3.f;
  } else if (hot) {
    color = lighten(color, .05f);
  }

  float x1 = rect.x, x2 = rect.x + border, x3 = rect.x + rect.width - border,
        x4 = rect.x + rect.width;

  float y1 = rect.y, y2 = rect.y + border, y3 = rect.y + rect.height - border,
        y4 = rect.y + rect.height;

  c->draw_list->push_quad({x4, y1}, {x3, y2}, {x2, y2}, {x1, y1}, top);
  c->draw_list->push_quad({x4, y4}, {x1, y4}, {x2, y3}, {x3, y3}, down);
  c->draw_list->push_quad({x1, y4}, {x2, y3}, {x2, y2}, {x1, y1}, left);
  c->draw_list->push_quad({x4, y4}, {x3, y3}, {x3, y2}, {x4, y1}, right);
  c->draw_list->push_quad({x2, y2},  // center
                          {x2, y3}, {x3, y3}, {x3, y2}, color);
  c->draw_list->push_text(text, {rect.x + border, rect.y + border + text_y_offset},
                          state.style.content_font_size,
                          state.style.content_highlighted_text_color);

  return triggered;
}
bool button(String text) {
  ImmId me = hash(text);
  return button(me, text);
}

bool list_item(ImmId me, String text, bool selected_flag = false, bool *as_checkbox = nullptr) {
  auto c = get_current_container();
  if (!c || !c->visible) return false;

  float want_width =
      get_text_width(text, state.style.content_font_size) + state.style.inner_padding.x * 2;
  Rect rect = c->place(
      {want_width, state.style.content_font_size + state.style.inner_padding.y * 2}, true, true);

  bool hot      = do_hoverable(me, rect, c->content_rect);
  bool active   = do_active(me);
  bool dragging = do_draggable(me);
  bool selected = do_selectable(me, true) || selected_flag;

  if (as_checkbox) {
    if (state.just_selected == me) {
      *as_checkbox = !(*as_checkbox);
    }
    selected = *as_checkbox;
  }

  Color text_color = state.style.content_default_text_color;
  if (selected) {
    c->draw_list->push_rect(rect, state.style.content_background_color);
    text_color = state.style.content_highlighted_text_color;
  }

  c->draw_list->push_text(
      text, {rect.x + state.style.inner_padding.x, rect.y + state.style.inner_padding.y},
      state.style.content_font_size, text_color);

  return selected;
}

// returns true on submit
template <size_t N>
bool textbox(ImmId me, AllocatedString<N> *str) {
  auto c = get_current_container();
  if (!c || !c->visible) return false;

  Rect rect = c->place({c->content_rect.width - state.style.inner_padding.x * 2,
                        state.style.content_font_size + state.style.inner_padding.y * 2});

  bool hot      = do_hoverable(me, rect, c->content_rect);
  bool active   = do_active(me);
  bool dragging = do_draggable(me);
  bool selected = do_selectable(me);

  if (selected) {
    for (int i = 0; i < state.input->text_input.len; i++) {
      str->append(state.input->text_input[i]);
    }

    for (int i = 0; i < state.input->key_input.len; i++) {
      if (state.input->key_input[i] == Keys::BACKSPACE && str->len > 0) {
        str->len--;
      }
    }
  }

  Color color = state.style.content_background_color;
  if (selected) {
    color = lighten(state.style.content_background_color, .2);
  }

  c->draw_list->push_rect(rect, color);
  c->draw_list->push_text(
      *str, {rect.x + state.style.inner_padding.x, rect.y + state.style.inner_padding.y},
      state.style.content_font_size, state.style.content_highlighted_text_color);

  if (state.just_unselected == me || (selected && state.input->key_down_events[(int)Keys::ENTER]))
    return true;
  return false;
}
template <size_t N>
void textbox(AllocatedString<N> *str) {
  ImmId me = (ImmId)(uint64_t)str;
  textbox(me, str);
}

template <typename T>
void num_input(T *val) {
  auto c = get_current_container();
  if (!c || !c->visible) return;

  ImmId me = (ImmId)(uint64_t)val;

  Rect rect = c->place({c->content_rect.width - state.style.inner_padding.x * 2,
                        state.style.content_font_size + state.style.inner_padding.y * 2},
                       false);

  bool hot      = do_hoverable(me, rect, c->content_rect);
  bool active   = do_active(me);
  bool dragging = do_draggable(me);
  bool selected = do_selectable(me);

  if (state.just_selected == me) {
    state.in_progress_string = float_to_allocated_string<1024>(*val);
  }

  if (state.selected == me) {
    if (textbox(me, &state.in_progress_string)) {
      *val =
          strtof(String(state.in_progress_string).to_char_array(&state.per_frame_alloc), nullptr);
    }
  } else {
    if (state.just_activated == me) {
      state.num_input_starting_val = *val;
    }
    if (dragging) {
      *val = state.num_input_starting_val + (state.drag_distance.x / 50);
    }

    c->place({rect.width, rect.height});

    AllocatedString<1024> val_str = float_to_allocated_string<1024>(*val);
    c->draw_list->push_rect(rect, state.style.content_background_color);
    c->draw_list->push_text(
        val_str, {rect.x + state.style.inner_padding.x, rect.y + state.style.inner_padding.y},
        state.style.content_font_size, state.style.content_highlighted_text_color);
  }
}

void texture(ImmId me, Texture val, Rect rect) {
  auto c = get_current_container();
  if (!c || !c->visible) return;

  bool hot      = do_hoverable(me, rect, c->content_rect);
  bool active   = do_active(me);
  bool dragging = do_draggable(me);
  bool selected = do_selectable(me);

  c->draw_list->push_tex(val, rect);
}
void texture(Texture *val) {
  ImmId me  = (ImmId)(uint64_t)val;
  Rect rect = get_current_container()->content_rect;
  texture(me, *val, rect);
}

bool translation_gizmo(Vec3f *p) {
  Window &window = state.windows[state.current_window];
  ImmId me       = (ImmId)(uint64_t)p;

  float dist_from_camera_2 = powf(p->x - state.camera->pos_x, 2) +
                             powf(p->y - state.camera->pos_y, 2) +
                             powf(p->z - state.camera->pos_z, 2);
  float dist_from_camera = sqrtf(dist_from_camera_2);
  const float gizmoSize  = 0.03f;
  float scale            = gizmoSize * (dist_from_camera / tanf((3.1415 / 6) / 2.0f));

  Vec3f axes[3] = {{scale, 0, 0}, {0, scale, 0}, {0, 0, scale}};

  int selected_axis  = -1;
  float min_distance = .1f;

  Vec3f closest_point_on_axis[3];

  Vec3f ray_origin = screen_to_world(window.content_rect, state.camera, state.input->mouse_pos);
  Vec3f ray_end    = screen_to_world(window.content_rect, state.camera, state.input->mouse_pos, .5);
  Vec3f ray_dir    = {ray_end.x - ray_origin.x, ray_end.y - ray_origin.y, ray_end.z - ray_origin.z};

  for (int i = 0; i < 3; i++) {
    Vec3f axis_origin = *p;
    Vec3f axis_dir    = axes[i];

    // stole this from somewhere
    float ray2     = ray_dir.x * ray_dir.x + ray_dir.y * ray_dir.y + ray_dir.z * ray_dir.z;
    float axis2    = axis_dir.x * axis_dir.x + axis_dir.y * axis_dir.y + axis_dir.z * axis_dir.z;
    float ray_axis = ray_dir.x * axis_dir.x + ray_dir.y * axis_dir.y + ray_dir.z * axis_dir.z;
    float det      = -((ray2 * axis2) - (ray_axis * ray_axis));

    Vec3f d_origin    = {ray_origin.x - axis_origin.x, ray_origin.y - axis_origin.y,
                      ray_origin.z - axis_origin.z};
    float ray_dorigin = ray_dir.x * d_origin.x + ray_dir.y * d_origin.y + ray_dir.z * d_origin.z;
    float axis_dorigin =
        axis_dir.x * d_origin.x + axis_dir.y * d_origin.y + axis_dir.z * d_origin.z;

    if (fabs(det) > 0.0000001) {
      float ray_t  = ((axis2 * ray_dorigin) - (axis_dorigin * ray_axis)) / det;
      float axis_t = (-(ray2 * axis_dorigin) + (ray_dorigin * ray_axis)) / det;

      closest_point_on_axis[i] = {
          axis_origin.x + (axis_dir.x * axis_t),
          axis_origin.y + (axis_dir.y * axis_t),
          axis_origin.z + (axis_dir.z * axis_t),
      };

      if (axis_t >= 0 && axis_t <= 1 && ray_t > 0) {
        Vec3f p0 = {
            ray_origin.x + (ray_dir.x * ray_t),
            ray_origin.y + (ray_dir.y * ray_t),
            ray_origin.z + (ray_dir.z * ray_t),
        };
        Vec3f p1 = {
            axis_origin.x + (axis_dir.x * axis_t),
            axis_origin.y + (axis_dir.y * axis_t),
            axis_origin.z + (axis_dir.z * axis_t),
        };

        float dist2 = (p1.x - p0.x) * (p1.x - p0.x) + (p1.y - p0.y) * (p1.y - p0.y) +
                      (p1.z - p0.z) * (p1.z - p0.z);

        if (dist2 < min_distance) {
          min_distance  = dist2;
          selected_axis = i;
        }
      }
    }
  }

  {
    auto ndc_to_screen = [](Rect draw_area, Vec3f p) {
      return Vec3f{(p.x + 1) * 0.5f * draw_area.width + draw_area.x,
                   ((1 - p.y) * .5f * draw_area.height + draw_area.y), p.z};
    };

    glm::vec4 p_ndc =
        state.camera->perspective * state.camera->view * glm::vec4(p->x, p->y, p->z, 1.f);
    if (p_ndc.w > 0) {
      p_ndc /= p_ndc.w;
      Vec3f p_screen = ndc_to_screen(window.content_rect, {p_ndc.x, p_ndc.y, p_ndc.z});

      Rect view_rect        = {p_screen.x - 5, p_screen.y - 5, 10, 10};
      Rect interactive_rect = {p_screen.x - 15, p_screen.y - 15, 30, 30};

      Color color = {1, 1, 0, 1};
      window.draw_list.push_rect(view_rect, color);
    }
    Vec3f p_screen = ndc_to_screen(window.content_rect, {p_ndc.x, p_ndc.y, p_ndc.z});

    auto draw_axis = [&](int axis_i, Vec3f axis, Color color, ImmId id) {
      bool hot      = do_hoverable(id, selected_axis == axis_i);
      bool active   = do_active(id);
      bool dragging = do_draggable(id);

      glm::vec4 x_axis_offset_ndc = state.camera->perspective * state.camera->view *
                                    glm::vec4(p->x + axis.x, p->y + axis.y, p->z + axis.z, 1.f);
      float w = x_axis_offset_ndc.w;
      if (x_axis_offset_ndc.w > 0) {
        x_axis_offset_ndc /= x_axis_offset_ndc.w;
        Vec3f axis_offset_screen = ndc_to_screen(
            window.content_rect, {x_axis_offset_ndc.x, x_axis_offset_ndc.y, x_axis_offset_ndc.z});
        Vec2f axis_dir_screen =
            normalize(Vec2f{axis_offset_screen.x - p_screen.x, axis_offset_screen.y - p_screen.y});

        if (state.just_activated == id) {
          state.gizmo_last_contact_point = closest_point_on_axis[axis_i];
        }

        if (hot) {
          color = lighten(color, 0.5f);
        }

        Vec2f line_normal    = {axis_dir_screen.y, -axis_dir_screen.x};
        float line_thickness = 2;
        Vec2f line_p0        = {axis_offset_screen.x + line_normal.x * line_thickness,
                         axis_offset_screen.y + line_normal.y * line_thickness};
        Vec2f line_p1        = {axis_offset_screen.x - line_normal.x * line_thickness,
                         axis_offset_screen.y - line_normal.y * line_thickness};
        Vec2f line_p2        = {p_screen.x + line_normal.x * line_thickness,
                         p_screen.y + line_normal.y * line_thickness};
        Vec2f line_p3        = {p_screen.x - line_normal.x * line_thickness,
                         p_screen.y - line_normal.y * line_thickness};
        window.draw_list.push_quad(line_p0, line_p1, line_p3, line_p2, color);

        Vec2f arrow_p0 = {axis_offset_screen.x + line_normal.x * 5,
                          axis_offset_screen.y + line_normal.y * 5};
        Vec2f arrow_p1 = {axis_offset_screen.x - line_normal.x * 5,
                          axis_offset_screen.y - line_normal.y * 5};
        Vec2f arrow_p2 = {axis_offset_screen.x + axis_dir_screen.x * 10,
                          axis_offset_screen.y + axis_dir_screen.y * 10};

        window.draw_list.push_quad(arrow_p0, arrow_p1, arrow_p2, arrow_p2, color);
      }

      if (dragging) {
        Vec3f diff = {closest_point_on_axis[axis_i].x - state.gizmo_last_contact_point.x,
                      closest_point_on_axis[axis_i].y - state.gizmo_last_contact_point.y,
                      closest_point_on_axis[axis_i].z - state.gizmo_last_contact_point.z};
        state.gizmo_last_contact_point = closest_point_on_axis[axis_i];
        return diff;
      }
      return Vec3f{0, 0, 0};
    };

    ImmId x_axis = me + 1;
    ImmId y_axis = me + 2;
    ImmId z_axis = me + 3;

    Vec3f diff;
    diff.x = draw_axis(0, axes[0], {.9, .2, .3, 1}, x_axis).x;
    diff.y = draw_axis(1, axes[1], {.2, .9, .3, 1}, y_axis).y;
    diff.z = draw_axis(2, axes[2], {.2, .3, .9, 1}, z_axis).z;

    p->x += diff.x;
    p->y += diff.y;
    p->z += diff.z;
  }

  return false;
}

void rotation_gizmo(Vec3f *rotation, Vec3f p) {
  Window &window = state.windows[state.current_window];
  ImmId me       = hash("rotation gizmo");

  Array<float, 5000> lines;
  auto add_line = [&](Vec3f s, Vec3f e, Color c) {
    lines.append(s.x);
    lines.append(s.y);
    lines.append(s.z);
    lines.append(c.r);
    lines.append(c.g);
    lines.append(c.b);
    lines.append(c.a);
    lines.append(e.x);
    lines.append(e.y);
    lines.append(e.z);
    lines.append(c.r);
    lines.append(c.g);
    lines.append(c.b);
    lines.append(c.a);
  };

  float dist_from_camera_2 = powf(p.x - state.camera->pos_x, 2) +
                             powf(p.y - state.camera->pos_y, 2) +
                             powf(p.z - state.camera->pos_z, 2);
  float dist_from_camera = sqrtf(dist_from_camera_2);
  const float gizmoSize  = 0.03f;
  float scale            = gizmoSize * (dist_from_camera / tanf((3.1415 / 6) / 2.0f));

  Vec3f ray_origin = screen_to_world(window.content_rect, state.camera, state.input->mouse_pos);
  Vec3f ray_end    = screen_to_world(window.content_rect, state.camera, state.input->mouse_pos, .5);
  Vec3f ray_dir    = ray_end - ray_origin;

  auto rotate_axis = [&](Vec3f axis) {
    glm::vec3 rotated_axis = glm::quat(glm::vec3(rotation->x, rotation->y, rotation->z)) *
                             glm::vec3(axis.x, axis.y, axis.z);
    return normalize(Vec3f{rotated_axis.x, rotated_axis.y, rotated_axis.z});
  };
  auto inverse_rotation = [&](Vec3f p) {
    glm::quat quat(glm::vec3(rotation->x, rotation->y, rotation->z));
    glm::vec3 r_p = glm::vec3(p.x, p.y, p.z) * -quat;
    return r_p;
  };

  Vec3f rotated_axes[4] = {
      rotate_axis({1, 0, 0}),
      rotate_axis({0, 1, 0}),
      rotate_axis({0, 0, 1}),
      normalize(p - Vec3f{state.camera->pos_x, state.camera->pos_y, state.camera->pos_z}),
  };
  glm::vec3 unrotated_axes[4] = {
      {1, 0, 0},
      {0, 1, 0},
      {0, 0, 1},
      inverse_rotation(rotated_axes[3]),
  };
  Color colors[4] = {
      {.9, .3, .2, 1},
      {.3, .9, .2, 1},
      {.3, .2, .9, 1},
      {.9, .9, .9, 1},
  };
  float radiuses[4] = {
      1.f,
      1.f,
      1.f,
      1.3f,
  };

  for (int i = 0; i < 4; i++) {
    glm::vec3 axis     = unrotated_axes[i];
    Vec3f rotated_axis = rotated_axes[i];
    Color color        = colors[i];
    float radius       = radiuses[i];

    Vec3f plane_p = p;
    Vec3f plane_n = {rotated_axis.x, rotated_axis.y, rotated_axis.z};
    float numom   = dot(plane_p - ray_origin, plane_n);
    float denom   = dot(ray_dir, plane_n);

    if (numom == 0 && denom == 0) {
      // TODO line is in plane, maybe do a sphere test
    } else if (denom != 0) {
      float d            = numom / denom;
      Vec3f intersection = ray_origin + (ray_dir * d);

      float distance_from_circle = fabs((intersection - p).len() - (scale * radius));
      Vec3f dir                  = normalize(intersection - p);
      Vec3f point_on_circle      = p + dir * scale;

      ImmId axis_id = me + i;
      bool hot      = do_hoverable(axis_id, distance_from_circle < scale / 10.f);
      bool active   = do_active(axis_id);
      bool dragging = do_draggable(axis_id);

      if ((state.dragging == 0 && hot) || dragging) {
        color = lighten(color, 0.2f);
      }

      if (state.just_activated == axis_id) {
        state.gizmo_last_contact_point = dir;
      }

      if (dragging) {
        Vec3f old_dir_projected =
            normalize(cross(plane_n, cross(state.gizmo_last_contact_point, plane_n)));
        Vec3f new_dir_projected        = normalize(cross(plane_n, cross(dir, plane_n)));
        state.gizmo_last_contact_point = dir;

        float angle = acos(clamp(dot(old_dir_projected, new_dir_projected), -1, 1));
        float sign  = dot(cross(old_dir_projected, new_dir_projected), plane_n);
        if (angle > 0 && sign != 0) {
          sign           = sign / fabs(sign);
          glm::quat quat = glm::rotate(glm::quat(glm::vec3(rotation->x, rotation->y, rotation->z)),
                                       sign * angle, axis);
          glm::vec3 new_rot_euler = glm::eulerAngles(quat);
          *rotation               = {new_rot_euler.x, new_rot_euler.y, new_rot_euler.z};
        }
      }
    }

    for (int i = 0; i < 50; i++) {
      auto circle = [&](float a) {
        Vec3f p;
        p.x = (1 - rotated_axis.x) * sinf(a * 2 * 3.1415);
        p.y = (1 - rotated_axis.y) *
              (rotated_axis.x == 1 ? sinf(a * 2 * 3.1415) : cosf(a * 2 * 3.1415));
        p.z = (1 - rotated_axis.z) * cosf(a * 2 * 3.1415);

        Vec3f other_vector         = {plane_n.y, plane_n.z, plane_n.x};
        Vec3f perpendicular_vector = normalize(cross(plane_n, other_vector));
        float angle                = a * 2 * 3.1415;
        glm::mat4 rot_mat =
            glm::rotate(glm::mat4(1.0), angle, glm::vec3(plane_n.x, plane_n.y, plane_n.z));
        glm::vec4 rotated_point =
            glm::vec4(perpendicular_vector.x, perpendicular_vector.y, perpendicular_vector.z, 1.f) *
            rot_mat;
        return Vec3f{rotated_point.x, rotated_point.y, rotated_point.z};
      };
      Vec3f s = p + scale * radius * circle(i / 20.f);
      Vec3f e = p + scale * radius * circle((i + 1.f) / 20);
      add_line(s, e, color);
    }
  }

  glDisable(GL_DEPTH_TEST);
  bind_shader(lines_shader);
  glUniformMatrix4fv(lines_shader.uniform_handles[(int)UniformId::VIEW], 1, GL_FALSE,
                     &state.camera->view[0][0]);
  glUniformMatrix4fv(lines_shader.uniform_handles[(int)UniformId::PROJECTION], 1, GL_FALSE,
                     &state.camera->perspective[0][0]);
  debug_draw_lines(state.target, lines.arr, lines.len / (7 * 2));
}
}  // namespace Imm

// timelines
namespace Imm {

struct TimelineParameters {
  int duration;
  String units;

  bool horizontal;

  bool clamp_start;
  bool clamp_end;
  float clamp_start_value;
  float clamp_end_value;
};
TimelineParameters timeline_params;

ImmId timeline_id;
float *tstart;
float *twidth;
int *tcurrent_frame;
bool nothing_else_touched;

void start_timeline(String name, int *current_frame, float *start, float *width) {
  ImmId me    = hash(name);
  timeline_id = me;

  tstart               = start;
  twidth               = width;
  tcurrent_frame       = current_frame;
  nothing_else_touched = true;

  auto c = get_current_container();
  if (!c || !c->visible) return;

  *start         = fmax(*start, 0.f);
  int left_bound = ((int)(*start / 5)) * 5;

  for (int i = 0; i < *width / 5; i++) {
    int frame = left_bound + i * 5;
    float pos = (frame - *start) / (*width) * c->content_rect.width;

    float mark_height = 10;
    if (frame % 10 == 0) {
      mark_height = 20;
    }

    float bar_width = 5;
    c->draw_list->push_rect(
        {
            c->content_rect.x + pos - (bar_width / 2.f),
            c->content_rect.y + 5,
            5,
            mark_height,
        },
        {.3, .3, .3, 1});
    c->draw_list->push_text(String::from(frame, &state.per_frame_alloc),
                            {c->content_rect.x + pos, c->content_rect.y + 5 + mark_height + 5}, 15,
                            {0, 0, 0, 1});
  }

  float pointer_pos = (*tcurrent_frame - *start) / (*width) * c->content_rect.width;
  c->draw_list->push_quad(
      {c->content_rect.x + pointer_pos - (20 / 2.f), c->content_rect.y},
      {c->content_rect.x + pointer_pos + (20 / 2.f), c->content_rect.y},
      {c->content_rect.x + pointer_pos + (7 / 2.f), c->content_rect.y + (15 / 2.f)},
      {c->content_rect.x + pointer_pos - (7 / 2.f), c->content_rect.y + (15 / 2.f)},
      {.2, .3, .9, 1});
  c->draw_list->push_rect(
      {
          c->content_rect.x + pointer_pos - (7 / 2.f),
          c->content_rect.y + (15 / 2.f),
          7,
          c->content_rect.height - (15 / 2.f),
      },
      {.2, .3, .9, 1});
}

void keyframe(ImmId id, int *frame, int track_i = 0) {
  auto c = get_current_container();

  float pos             = (*frame - *tstart) / (*twidth) * c->content_rect.width;
  float vertical_offset = track_i * 15;

  Rect hot_rect = {c->content_rect.x + pos - (10 / 2.f),
                   c->content_rect.y + 50 - (10 / 2.f) + vertical_offset, 10, 10};
  bool hot      = do_hoverable(id, hot_rect, c->content_rect);
  bool active   = do_active(id);
  bool dragging = do_draggable(id);

  Color color = {.2, .9, .3, 1};
  if (hot) {
    nothing_else_touched = false;
    color                = lighten(color, .2);
  }

  if (dragging) {
    float pos     = state.input->mouse_pos.x - c->content_rect.x;
    int new_frame = (pos / c->content_rect.width) * (*twidth) + (*tstart);
    *frame        = new_frame;
  }

  c->draw_list->push_quad(
      {c->content_rect.x + pos - (10 / 2.f), 50 + c->content_rect.y + vertical_offset},
      {c->content_rect.x + pos, 50 + c->content_rect.y - (10 / 2.f) + vertical_offset},
      {c->content_rect.x + pos + (10 / 2.f), 50 + c->content_rect.y + vertical_offset},
      {c->content_rect.x + pos, 50 + c->content_rect.y + (10 / 2.f) + vertical_offset}, color);
}

void end_timeline() {
  ImmId me = timeline_id;

  auto c = get_current_container();
  if (!c || !c->visible) return;

  bool hot =
      do_hoverable(me, nothing_else_touched && in_rect(state.input->mouse_pos, c->content_rect) &&
                           state.top_window_at_current_mouse_pos == state.current_window);
  bool active   = do_active(me);
  bool dragging = do_draggable(me);

  if ((dragging && hot) ||
      (state.just_deactivated == me && hot && !dragging && !state.just_stopped_dragging)) {
    // *tstart -= (state.input->mouse_x - state.input->prev_mouse_x) / 20.f;
    float pos       = state.input->mouse_pos.x - c->content_rect.x;
    int frame       = (pos / c->content_rect.width) * (*twidth) + (*tstart);
    *tcurrent_frame = frame;
  }

  if (hot) {
    if (state.input->keys[(int)Keys::LCTRL]) {
      *twidth *= 1 + (state.input->scrollwheel_count / 50);
    } else {
      *tstart += (state.input->scrollwheel_count * 3.f);
    }
  }
}

}  // namespace Imm

namespace Imm {
void first_column(float *width) {
  Rect new_rect  = get_current_container()->content_rect;
  new_rect.width = *width;
  new_rect.width = width ? *width
                         : (get_current_container()->content_rect.x +
                            get_current_container()->content_rect.width) -
                               new_rect.x;
  push_subcontainer(new_rect);
}
void next_column(float *width) {
  Rect new_rect = pop_container().content_rect;
  new_rect.x += new_rect.width;
  new_rect.width = width ? *width
                         : (get_current_container()->content_rect.x +
                            get_current_container()->content_rect.width) -
                               new_rect.x;
  push_subcontainer(new_rect);
}
void end_columns() { pop_container(); }

}  // namespace Imm

// menubar
namespace Imm {

ImmId current_menubar_menu = 0;

bool start_menubar_menu(String name) {
  const float font_size = state.style.content_font_size;

  ImmId me = hash(name);

  if (!state.menubar_visible) {
    state.global_draw_list.push_rect({0, 0, (float)state.target.width, state.style.menubar_height},
                                     {1, .141, 0, 1});
    state.menubar_visible = true;
  }

  Rect interactive_rect = {state.next_menubar_x, 0,
                           get_text_width(name, font_size) + (state.style.inner_padding.x * 2),
                           state.style.menubar_height};
  state.next_menubar_x += interactive_rect.width;
  current_menubar_menu = me;

  bool hot      = do_hoverable(me, interactive_rect);
  bool active   = do_active(me);
  bool selected = do_selectable(me, true);

  auto c = start_popup(
      me, {interactive_rect.x + 5, interactive_rect.y + interactive_rect.height + 5, 200, 0});

  if (c->visible) {
    if (state.just_selected == me) {
      state.current_menubar_menu = 0;
      close_popup();
    }
  } else {
    if (state.current_menubar_menu == me) {
      state.current_menubar_menu = 0;
    } else if (state.just_selected == me) {
      state.current_menubar_menu = me;
      open_popup(me);
    } else if (hot && state.current_menubar_menu != 0) {
      state.current_menubar_menu = me;
      open_popup(me);
    }
  }

  Vec2f text_pos     = {interactive_rect.x + state.style.inner_padding.x,
                    (interactive_rect.height / 2) - (font_size / 2)};
  Color button_color = {0, 0, 1, 1};
  if (hot || state.current_menubar_menu == me) {
    button_color = lighten(button_color, 0.3f);
  }
  state.global_draw_list.push_rect(interactive_rect, button_color);
  state.global_draw_list.push_text(name, text_pos, font_size, {1, 1, 1, 1});

  return state.current_menubar_menu == me;
}

void end_menubar_menu() { end_popup(); }

}  // namespace Imm

// save/load
namespace Imm {

void save_layout() {
  StackAllocator *a = &state.per_frame_alloc;

  YAML::Dict *out_layout = YAML::new_dict(a);

  YAML::List *out_windows = YAML::new_list(a);
  for (auto &[id, in_window] : state.windows) {
    YAML::Dict *window = YAML::new_dict(a);
    window->push_back("id", YAML::new_literal(String::from(id, a), a), a);
    window->push_back("name", YAML::new_literal(in_window.name, a), a);
    window->push_back(
        "visible", YAML::new_literal(in_window.visible ? (String) "true" : (String) "false", a), a);
    window->push_back("z", YAML::new_literal(String::from(in_window.z, a), a), a);

    YAML::Dict *rect = YAML::new_dict(a);
    rect->push_back("x", YAML::new_literal(String::from(in_window.rect.x, a), a), a);
    rect->push_back("y", YAML::new_literal(String::from(in_window.rect.y, a), a), a);
    rect->push_back("width", YAML::new_literal(String::from(in_window.rect.width, a), a), a);
    rect->push_back("height", YAML::new_literal(String::from(in_window.rect.height, a), a), a);
    window->push_back("rect", rect, a);

    out_windows->push_back(window, a);
  }
  out_layout->push_back("windows", out_windows, a);

  YAML::Dict *out_anchors = YAML::new_dict(a);
  out_anchors->push_back("anchored_left",
                         YAML::new_literal(String::from(state.anchored_left, a), a), a);
  out_anchors->push_back("anchored_left_priority",
                         YAML::new_literal(String::from(state.anchored_left_priority, a), a), a);
  out_anchors->push_back("anchored_right",
                         YAML::new_literal(String::from(state.anchored_right, a), a), a);
  out_anchors->push_back("anchored_right_priority",
                         YAML::new_literal(String::from(state.anchored_right_priority, a), a), a);
  out_anchors->push_back("anchored_up", YAML::new_literal(String::from(state.anchored_up, a), a),
                         a);
  out_anchors->push_back("anchored_up_priority",
                         YAML::new_literal(String::from(state.anchored_up_priority, a), a), a);
  out_anchors->push_back("anchored_down",
                         YAML::new_literal(String::from(state.anchored_down, a), a), a);
  out_anchors->push_back("anchored_down_priority",
                         YAML::new_literal(String::from(state.anchored_down_priority, a), a), a);
  out_anchors->push_back("anchored_center",
                         YAML::new_literal(String::from(state.anchored_center, a), a), a);
  out_layout->push_back("anchors", out_anchors, a);

  String out;
  out.data = a->next;
  YAML::serialize(out_layout, a, 0, false);
  out.len = a->next - out.data;
  printf("%.*s\n", out.len, out.data);

  write_file("resources/test/layout.yaml", out);
}
void load_layout() {
  StackAllocator *a = &state.per_frame_alloc;

  FileData file = read_entire_file("resources/test/layout.yaml", a);

  String in_str;
  in_str.data      = file.data;
  in_str.len       = file.length;
  YAML::Dict *root = YAML::deserialize(in_str, a)->as_dict();

  YAML::List *in_windows = root->get("windows")->as_list();
  for (int i = 0; i < in_windows->len; i++) {
    YAML::Dict *in_window = in_windows->get(i)->as_dict();
    ImmId id              = in_window->get("id")->as_literal().to_uint64();

    Window &window = state.windows[id];
    window.name    = string_to_allocated_string<128>(in_window->get("name")->as_literal());
    window.visible =
        in_window->get("visible") && strcmp(in_window->get("visible")->as_literal(), "true");
    window.z = in_window->get("z") && atoi(in_window->get("z")->as_literal().to_char_array(a));

    YAML::Dict *rect   = in_window->get("rect")->as_dict();
    window.rect.x      = atof(rect->get("x")->as_literal().to_char_array(a));
    window.rect.y      = atof(rect->get("y")->as_literal().to_char_array(a));
    window.rect.width  = atof(rect->get("width")->as_literal().to_char_array(a));
    window.rect.height = atof(rect->get("height")->as_literal().to_char_array(a));
  }

  YAML::Dict *in_anchors = root->get("anchors")->as_dict();
  state.anchored_left    = in_anchors->get("anchored_left")->as_literal().to_uint64();
  state.anchored_left_priority =
      in_anchors->get("anchored_left_priority")->as_literal().to_uint64();
  state.anchored_right = in_anchors->get("anchored_right")->as_literal().to_uint64();
  state.anchored_right_priority =
      in_anchors->get("anchored_right_priority")->as_literal().to_uint64();
  state.anchored_up          = in_anchors->get("anchored_up")->as_literal().to_uint64();
  state.anchored_up_priority = in_anchors->get("anchored_up_priority")->as_literal().to_uint64();
  state.anchored_down        = in_anchors->get("anchored_down")->as_literal().to_uint64();
  state.anchored_down_priority =
      in_anchors->get("anchored_down_priority")->as_literal().to_uint64();
  state.anchored_center = in_anchors->get("anchored_center")->as_literal().to_uint64();
}

void init() {
  state.per_frame_alloc.init(1034 * 1024 * 50);  // 50mb

  load_layout();
}

}  // namespace Imm

// utility
namespace Imm {

// for now this is also displaying some debug stuff
void add_window_menubar_menu() {
  if (Imm::start_menubar_menu("Windows")) {
    for (auto &[id, window] : state.windows) {
      list_item((ImmId)&window.visible, window.name, window.visible, &window.visible);
    }
    label(String::from(popup_stack.size(), &state.per_frame_alloc));
    if (button("printf")) {
      printf("%llu\n", open_popups.size());
    }
  }
  Imm::end_menubar_menu();
}

}  // namespace Imm
