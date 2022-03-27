#pragma once

#include "util.hpp"

enum struct Keys {
  NUM_0,
  NUM_1,
  NUM_2,
  NUM_3,
  NUM_4,
  NUM_5,
  NUM_6,
  NUM_7,
  NUM_8,
  NUM_9,
  SPACE,
  BACKSPACE,
  ENTER,
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  UP,
  DOWN,
  LEFT,
  RIGHT,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  LCTRL,
  RCTRL,
  LALT,
  RALT,
  ESCAPE,
  INS,
  DEL,
  HOME,
  END,
  PAGEUP,
  PAGEDOWN,
  INVALID
};

enum struct MouseButton { LEFT, RIGHT, COUNT };

struct InputState {
  bool keys[(int)Keys::INVALID]            = {};
  bool key_up_events[(int)Keys::INVALID]   = {};
  bool key_down_events[(int)Keys::INVALID] = {};

  bool mouse_buttons[(int)MouseButton::COUNT]            = {};
  bool mouse_button_down_events[(int)MouseButton::COUNT] = {};
  bool mouse_button_up_events[(int)MouseButton::COUNT]   = {};

  Array<char, 128> text_input = {};
  Array<Keys, 128> key_input  = {};

  double scrollwheel_count = 0;

  Vec2f mouse_pos       = {};
  Vec2f mouse_pos_prev  = {};
  Vec2f mouse_pos_delta = {};
};

struct FileData {
  char *data;
  int length;
};
FileData read_entire_file(const char *);
FileData read_entire_file(const char *, StackAllocator *);
FileData read_entire_file(String, StackAllocator *);
void write_file(const char *, String);
void free_file(FileData);
uint64_t debug_get_cycle_count();

void set_fullscreen(bool enable);