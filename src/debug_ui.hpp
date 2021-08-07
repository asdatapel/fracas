#pragma once

#include "graphics/graphics.hpp"

typedef unsigned long ImmId;

struct ImmWindow
{
    Rect content_rect = {};

    Vec2f next_elem_pos = {0, 0};

    ImmId selected;

    float scroll = 0;
    float last_height = 0;
};

struct UiState
{
    RenderTarget target;
    InputState *input;
    Font font;

    ImmId current_window;
    std::unordered_map<ImmId, ImmWindow> windows;

    ImmId hot = 0;
    ImmId selection_started = 0;
    Vec2f selection_started_location = {};
    ImmId selected = 0;

    AllocatedString<1024> in_progress_string;

    float button_y_offset = 0;
};
UiState state;

float border = 3.f;
float gap = 1.5f;

// http://www.cse.yorku.ca/~oz/hash.html
ImmId hash(String str)
{
    unsigned long hash = 5381;
    int c;

    for (int i = 0; i < str.len; i++)
    {
        int c = str.data[i];
        hash = ((hash << 5) + hash) ^ c;
    }

    return hash;
}

Rect align_right(Rect rect)
{
    rect.x = state.target.width - rect.width - 10;
    return rect;
}

bool imm_does_gui_have_focus()
{
    return state.selection_started || state.selected;
}

void imm_init(Assets *assets, Memory mem)
{
    state.font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_LIGHT_TTF], 24, mem.temp);
}

void imm_begin(RenderTarget target, InputState *input)
{
    state.target = target;
    state.input = input;

    state.button_y_offset = 10.f;
}

void imm_end()
{
}

bool imm_hot(ImmId id, Rect rect, Rect mask = {})
{
    ImmWindow &window = state.windows[state.current_window];
    bool hot = in_rect(Vec2f{state.input->mouse_x, state.input->mouse_y}, rect, mask);
    if (hot)
    {
        if (state.hot != id)
        {
            state.hot = id;
            return true;
        }
    }
    else if (state.hot == id)
    {
        state.hot = 0;
    }

    return false;
}

bool imm_start_selection(ImmId id, bool hot)
{
    if (state.input->mouse_up_event)
    {
        if (state.selection_started == id)
        {
            state.selection_started = 0;
        }
    }

    if (state.input->mouse_down_event && hot)
    {
        state.selection_started = id;
        state.selection_started_location = {state.input->mouse_x, state.input->mouse_y};
        return true;
    }

    return false;
}

bool imm_select(ImmId id, bool hot)
{
    bool just_selected = false;
    if (state.input->mouse_up_event)
    {
        if (hot && state.selection_started == id)
        {
            state.selected = id;
            just_selected = true;
        }
        else if (state.selected == id)
        {
            state.selected = 0;
        }
    }
    imm_start_selection(id, hot);
    return just_selected;
}

void imm_window(String title, Rect rect)
{
    ImmId me = hash(title);
    ImmWindow &window = state.windows[me];
    state.current_window = me;

    Rect title_rect = {
        rect.x,
        rect.y,
        rect.width,
        state.font.font_size_px + (border * 2),
    };
    Rect content_rect = {
        rect.x,
        rect.y + title_rect.height,
        rect.width,
        rect.height - title_rect.height,
    };

    Color window_color = {0, 0, 0, .8};
    Color scrollbar_color = {.2, .2, .2, 1};
    Rect scrollbar_rect = {0, 0, 0, 0};
    if (window.last_height > content_rect.height)
    {
        if (in_rect(Vec2f{state.input->mouse_x, state.input->mouse_y}, rect))
        {
            window.scroll += state.input->scrollwheel_count * 20;
        }

        float scrollbar_width = 10;
        Rect scrollbar_area_rect = {
            content_rect.x + content_rect.width - scrollbar_width - (border * 2),
            content_rect.y + border,
            scrollbar_width + (border * 2),
            content_rect.height - (border * 2)};

        float scroll_vertical_extent = window.last_height - content_rect.height;
        float scrollbar_height = content_rect.height / window.last_height * scrollbar_area_rect.height;
        float scrollbar_vertical_extent = scrollbar_area_rect.height - scrollbar_height;
        float scrollbar_pos_percent = -window.scroll / scroll_vertical_extent;

        imm_hot(me, scrollbar_area_rect);
        bool hot = (state.hot == me);
        if (hot)
        {
            scrollbar_color = lighten(scrollbar_color, 0.2);
        }
        imm_start_selection(me, hot);
        if (state.selection_started == me)
        {
            float new_scrollbar_position_y = state.input->mouse_y - (scrollbar_height / 2);
            scrollbar_pos_percent = (new_scrollbar_position_y - scrollbar_area_rect.y) / scrollbar_vertical_extent;
            window.scroll = -scrollbar_pos_percent * scroll_vertical_extent;
        }

        if (window.scroll > 0)
        {
            window.scroll = 0;
            scrollbar_pos_percent = 0;
        }
        if (-window.scroll > scroll_vertical_extent)
        {
            scrollbar_pos_percent = 1;
            window.scroll = -scroll_vertical_extent;
        }

        scrollbar_rect = {
            scrollbar_area_rect.x + border,
            scrollbar_area_rect.y + (scrollbar_pos_percent * scrollbar_vertical_extent),
            scrollbar_width,
            scrollbar_height};

        content_rect.width -= scrollbar_width + (border * 2);
    }

    debug_begin_immediate();

    draw_rect(state.target, rect, {0, 0, 0, 0.8});
    draw_rect(state.target, title_rect, {.7, .07, .13, 0.8});
    draw_text_cropped(state.font, state.target, title,
                      rect.x + border, rect.y + border, 1, 1);

    draw_rect(state.target, scrollbar_rect, scrollbar_color);

    debug_end_immediate();

    window.content_rect = content_rect;
    window.next_elem_pos = {content_rect.x + border, window.scroll + content_rect.y + border};
    window.last_height = 0;
}

void imm_label(String text)
{
    ImmWindow &window = state.windows[state.current_window];

    Rect rect = {window.next_elem_pos.x, window.next_elem_pos.y,
                 window.content_rect.width, 30};

    Color color = {0, 0, 0, 0};
    float text_scale = (rect.height - (2 * border)) / state.font.font_size_px;

    start_scissor(state.target, window.content_rect);
    debug_begin_immediate();

    draw_text_cropped(state.font, state.target, text,
                      rect.x + border, rect.y + border, text_scale, text_scale);

    debug_end_immediate();
    end_scissor();

    window.next_elem_pos.y += rect.height + gap;
    window.last_height += rect.height + gap;
}

bool imm_button(ImmId me, String text)
{
    bool triggered = false;

    Rect rect = {100, state.button_y_offset, 150, 40};
    float border = 7;
    float text_scale = (rect.height - (2 * border)) / state.font.font_size_px;
    rect.width = get_text_width(state.font, text, text_scale) + (2 * border);
    rect = align_right(rect);

    imm_hot(me, rect);
    bool hot = state.hot == me;
    imm_select(me, hot);

    Color color = {.9, .3, .2, 1};

    Color light = lighten(color, .1);
    Color lighter = lighten(color, .15);
    Color dark = darken(color, .1);
    Color darker = darken(color, .15);

    Color top = lighter;
    Color down = darker;
    Color left = light;
    Color right = dark;

    float text_y_offset = 0;
    if (state.selection_started == me)
    {
        color = darken(color, 0.05f);

        top = darker;
        down = lighter;
        left = dark;
        right = light;

        text_y_offset = 3.f;
    }
    else if (hot)
    {
        color = lighten(color, .05f);
    }

    float x1 = rect.x,
          x2 = rect.x + border,
          x3 = rect.x + rect.width - border,
          x4 = rect.x + rect.width;

    float y1 = rect.y,
          y2 = rect.y + border,
          y3 = rect.y + rect.height - border,
          y4 = rect.y + rect.height;

    debug_begin_immediate();

    // top
    debug_draw_immediate(state.target,
                         {x4, y1},
                         {x3, y2},
                         {x2, y2},
                         {x1, y1},
                         top);
    // down
    debug_draw_immediate(state.target,
                         {x4, y4},
                         {x1, y4},
                         {x2, y3},
                         {x3, y3},
                         down);
    // left
    debug_draw_immediate(state.target,
                         {x1, y4},
                         {x2, y3},
                         {x2, y2},
                         {x1, y1},
                         left);
    // right
    debug_draw_immediate(state.target,
                         {x4, y4},
                         {x3, y3},
                         {x3, y2},
                         {x4, y1},
                         right);

    // center
    debug_draw_immediate(state.target,
                         {x2, y2},
                         {x2, y3},
                         {x3, y3},
                         {x3, y2},
                         color);

    draw_text_cropped(state.font, state.target, text,
                      rect.x + border, rect.y + border + text_y_offset, text_scale, text_scale);

    debug_end_immediate();

    if (state.selected == me)
    {
        triggered = true;
        state.selected = 0;
    }

    state.button_y_offset += rect.height + 10;
    return triggered;
}

bool imm_button(String text)
{
    ImmId me = hash(text);
    return imm_button(me, text);
}

bool imm_list_item(ImmId me, String text)
{
    ImmWindow &window = state.windows[state.current_window];

    Rect rect = {window.next_elem_pos.x, window.next_elem_pos.y,
                 window.content_rect.width, 25};

    imm_hot(me, rect, window.content_rect);
    bool hot = state.hot == me;
    if (imm_start_selection(me, hot))
    {
        window.selected = me;
    }

    Color color = {0, 0, 0, 0};
    float text_scale = (rect.height - (2 * border)) / state.font.font_size_px;
    if (window.selected == me)
    {
        color = {.9, .3, .2, 1};
    }

    start_scissor(state.target, window.content_rect);
    debug_begin_immediate();

    draw_rect(state.target, rect, color);
    draw_text_cropped(state.font, state.target, text,
                      rect.x + border, rect.y + border, text_scale, text_scale);

    debug_end_immediate();
    end_scissor();

    window.next_elem_pos.y += rect.height + gap;
    window.last_height += rect.height + gap;

    return window.selected == me;
}

bool imm_list_item(String text)
{
    ImmId me = hash(text);
    return imm_list_item(me, text);
}

template <size_t N>
void imm_textbox(AllocatedString<N> *str)
{
    ImmId me = (ImmId)(uint64_t)str;
    ImmWindow &window = state.windows[state.current_window];

    Rect rect = {window.next_elem_pos.x, window.next_elem_pos.y,
                 window.content_rect.width, 25};

    imm_hot(me, rect, window.content_rect);
    bool hot = state.hot == me;
    imm_select(me, hot);

    if (state.selected == me)
    {
        for (int i = 0; i < state.input->text_input.len; i++)
        {
            str->append(state.input->text_input[i]);
        }

        for (int i = 0; i < state.input->key_input.len; i++)
        {
            if (state.input->key_input[i] == Keys::BACKSPACE && str->len > 0)
            {
                str->len--;
            }
        }
    }

    Color color = {.1, .1, .1, .8};
    float text_scale = (rect.height - (2 * border)) / state.font.font_size_px;
    if (state.selected == me)
    {
        color = {.8, .8, .8, .8};
    }

    start_scissor(state.target, window.content_rect);
    debug_begin_immediate();

    draw_rect(state.target, rect, color);
    draw_text_cropped(state.font, state.target, *str,
                      rect.x + border, rect.y + border, text_scale, text_scale);

    debug_end_immediate();
    end_scissor();

    window.next_elem_pos.y += rect.height + gap;
    window.last_height += rect.height + gap;
}

void imm_num_input(float *val)
{
    ImmId me = (ImmId)(uint64_t)val;
    ImmWindow &window = state.windows[state.current_window];

    Rect rect = {window.next_elem_pos.x, window.next_elem_pos.y,
                 window.content_rect.width, 25};

    AllocatedString<1024> str;
    str.len = snprintf(str.data, str.MAX_LEN, "%.9g", *val);
    String visible_string = str;

    imm_hot(me, rect, window.content_rect);
    bool hot = state.hot == me;

    if (imm_select(me, hot))
    {
        if (state.input->mouse_x != state.selection_started_location.x ||
            state.input->mouse_y != state.selection_started_location.y)
            state.selected = 0;
        state.in_progress_string = str;
    }

    if (state.selection_started == me && state.selected != me)
    {
        *val += (state.input->mouse_x - state.input->prev_mouse_x) / 50;
    }

    if (state.selected == me)
    {
        visible_string = state.in_progress_string;

        for (int i = 0; i < state.input->text_input.len; i++)
        {
            char c = state.input->text_input[i];
            if (std::isdigit(c) ||
                c == '.' ||
                (state.in_progress_string.len == 0 && c == '-'))
            {
                state.in_progress_string.append(c);
            }
        }

        for (int i = 0; i < state.input->key_input.len; i++)
        {
            if (state.input->key_input[i] == Keys::BACKSPACE && state.in_progress_string.len > 0)
            {
                state.in_progress_string.len--;
            }
        }

        static char buf[1025];
        memcpy(buf, state.in_progress_string.data, state.in_progress_string.len);
        buf[state.in_progress_string.len] = '\0';
        char *next;
        float converted_val = strtod(buf, &next);
        if (next == &buf[state.in_progress_string.len])
        {
            *val = converted_val;
        }
    }

    Color color = {.1, .1, .1, .8};
    float text_scale = (rect.height - (2 * border)) / state.font.font_size_px;
    if (state.selected == me)
    {
        color = {.8, .8, .8, .8};
    }

    start_scissor(state.target, window.content_rect);
    debug_begin_immediate();

    draw_rect(state.target, rect, color);
    draw_text(state.font, state.target, visible_string,
              rect.x + border, rect.y + border, text_scale, text_scale);

    debug_end_immediate();
    end_scissor();

    window.next_elem_pos.y += rect.height + gap;
    window.last_height += rect.height + gap;
}