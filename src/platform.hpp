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
    Array<MouseEvent, 128> mouse_input;
    
    double mouse_x;
    double mouse_y;
    double prev_mouse_x;
    double prev_mouse_y;
};

struct FileData
{
    char *data;
    int length;
};
FileData read_entire_file(const char *);
void free_file(FileData);
uint64_t debug_get_cycle_count();

void set_fullscreen(bool enable);