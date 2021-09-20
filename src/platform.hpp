#pragma once

#include "util.hpp"

enum struct Keys
{
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
    BACKSPACE,
    W,
    A,
    S,
    D,
    Z,
    X,
    C,
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
    INVALID
};

struct InputState
{
    bool keys[(int)Keys::INVALID];
    bool w, a, s, d;
    bool escape;

    bool mouse_left;
    bool mouse_right;
    int mouse_click_x;
    int mouse_click_y;
    
    bool quit;

    // @TODO currently only used for debug actions
    bool up, down, left, right;
    bool zoom_in, zoom_out;

    struct MouseEvent
    {
        bool down; // otherwise up
    };
    
    Array<char, 128> text_input;
    Array<Keys, 128> key_input;
    // Array<MouseEvent, 128> mouse_input;
    
    bool mouse_down_event;
    bool mouse_up_event;
    float mouse_x;
    float mouse_y;
    float prev_mouse_x;
    float prev_mouse_y;

    double scrollwheel_count;
};

struct FileData
{
    char *data;
    int length;
};
FileData read_entire_file(const char *);
FileData read_entire_file(const char *, StackAllocator *);
void write_file(const char *, String);
void free_file(FileData);
uint64_t debug_get_cycle_count();

void set_fullscreen(bool enable);