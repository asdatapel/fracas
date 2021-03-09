#pragma once

#include "util.hpp"
#include "net.hpp"

enum struct Keys
{
    BACKSPACE,
    W,
    A,
    S,
    D,
    Z,
    X,
    C,
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
};

struct FileData
{
    char *data;
    int length;
};
FileData read_entire_file(const char *);
uint64_t debug_get_cycle_count();

struct Client
{
    Peer peer = {};

    bool ready = false;

    AllocatedString<32> username;
    void set_username(String str)
    {
        username.len = 0;
        for (int i = 0; i < str.len && i < username.MAX_LEN; i++)
        {
            if (str.data[i] > 32 && str.data[i] < 127)
            {
                username.data[username.len] = str.data[i];
                username.len++;
            }
        }
    }
};