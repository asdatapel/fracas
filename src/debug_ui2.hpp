#pragma once

#include <cstdint>
#include <map>

#include "assets.hpp"
#include "math.hpp"
#include "graphics/graphics.hpp"
#include "resources.hpp"
#include "util.hpp"

typedef uint64_t uint64;
typedef uint64 ImmId;

namespace Imm
{
    // http://www.cse.yorku.ca/~oz/hash.html
    ImmId imm_hash(String str)
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

    struct SelectionContext
    {
        ImmId selected;
    };
    struct PendingAction
    {
        // for example scroll on window
    };
    struct DrawItem
    {
        DrawItem() {}
        enum struct Type
        {
            RECT,
            TEXT,
        };

        struct RectDrawItem
        {
            Rect rect;
            Color color;
        };
        struct TextDrawItem
        {
            AllocatedString<128> text;
            Vec2f pos;
            float size;
            Color color;
        };

        Type type;
        union
        {
            RectDrawItem rect_draw_item;
            TextDrawItem text_draw_item;
        };
    };
    struct DrawList
    {
        std::vector<DrawItem> buf;

        void push_rect(Rect rect, Color color)
        {
            DrawItem item;
            item.type = DrawItem::Type::RECT;
            item.rect_draw_item = {rect, color};
            buf.push_back(item);
        }

        void push_text(String text, Vec2f pos, float size, Color color)
        {
            DrawItem item;
            item.type = DrawItem::Type::TEXT;
            item.text_draw_item = {string_to_allocated_string<128>(text), pos, size, color};
            buf.push_back(item);
        }
    };
    struct Window : SelectionContext
    {
        Rect rect{};

        Rect content_rect = {};
        Vec2f next_elem_pos = {0, 0};
        float scroll = 0;
        float last_height = 0;

        int z;

        Window() {}
        Window(Rect rect)
        {
            this->rect = rect;
        }
    };
    struct ImmStyle
    {
        float font_size = 24;
        Vec2f inner_padding = {1.5f, 3.f};
    };
    struct UiState
    {
        Assets *assets;
        RenderTarget target;
        InputState *input;

        std::map<ImmId, Window> windows;
        ImmStyle style;

        ImmId current_window;

        ImmId hot = 0;
        ImmId active = 0;
        Vec2f active_position = {};
        ImmId dragging = 0;
        Vec2f drag_distance = {};
        ImmId selected = 0;

        ImmId top_window_at_current_mouse_pos = 0;

        AllocatedString<1024> in_progress_string;
    };

    UiState state;
    DrawList draw_list;

    bool do_hoverable(ImmId id, ImmId window_id, Rect rect)
    {
        if (state.top_window_at_current_mouse_pos != window_id)
            return false;

        if (in_rect(Vec2f{state.input->mouse_x, state.input->mouse_y}, rect))
        {
            state.hot = id;
            return true;
        }
        if (state.hot == id)
        {
            state.hot = 0;
        }
        return false;
    }
    bool do_active(ImmId id)
    {
        if (state.input->mouse_up_event)
        {
            state.active = 0;
            return false;
        }

        if (state.input->mouse_down_event)
        {
            if (state.hot == id)
            {
                state.active = id;
                state.active_position = {state.input->mouse_x, state.input->mouse_y};
                return true;
            }
            else
            {
                if (state.active == id)
                {
                    state.active = 0;
                }
                return false;
            }
        }
        return (state.active == id);
    }
    bool do_draggable(ImmId id)
    {
        bool active = state.active == id;
        if (active)
        {
            if (abs(state.input->mouse_x - state.active_position.x) > 2 ||
                abs(state.input->mouse_y - state.active_position.y) > 2)
            {
                state.dragging = id;
            }
            state.drag_distance = {state.input->mouse_x - state.active_position.x,
                                   state.input->mouse_y - state.active_position.y};
            return (state.dragging == id);
        }
        if (state.dragging == id)
        {
            state.dragging = 0;
        }
        return false;
    }

    void start_frame(RenderTarget target, InputState *input, Assets *assets)
    {
        state.assets = assets;
        state.input = input;
        state.target = target;

        draw_list.buf.clear();

        state.top_window_at_current_mouse_pos = 0;
        int highest_z = INT_MAX;
        for (const auto &[id, window] : state.windows)
        {
            if (window.z < highest_z &&
                in_rect({state.input->mouse_x, state.input->mouse_y}, window.rect))
            {
                highest_z = window.z;
                state.top_window_at_current_mouse_pos = id;
            }
        }
    }
    void start_window(String title, Rect rect)
    {
        ImmId me = imm_hash(title);
        state.current_window = me;

        if (state.windows.count(me) == 0)
        {
            Window window(rect);
            window.z = state.windows.size();
            state.windows[me] = window;
        }
        Window &window = state.windows[me];

        Rect titlebar_rect = {window.rect.x, window.rect.y, window.rect.width, state.style.font_size + state.style.inner_padding.y * 2};
        Rect content_rect = {window.rect.x, window.rect.y + titlebar_rect.height, window.rect.width, window.rect.height - titlebar_rect.height};

        bool hot = do_hoverable(me, state.current_window, titlebar_rect);
        bool active = do_active(me);
        bool dragging = do_draggable(me);

        Color titlebar_color = {.8, .1, .2, 1};
        if (dragging)
        {
            window.rect.x += state.input->mouse_x - state.input->prev_mouse_x;
            window.rect.y += state.input->mouse_y - state.input->prev_mouse_y;
            titlebar_color = {.9, .2, .3, 1};
        }

        draw_list.push_rect(titlebar_rect, titlebar_color);
        draw_list.push_text(title, {titlebar_rect.x + state.style.inner_padding.x, titlebar_rect.y + state.style.inner_padding.y}, state.style.font_size, {.9, .2, .3, 1});
        draw_list.push_rect(content_rect, {1, 1, 1, .8});
    }
    void end_frame(Camera *camera, Assets *assets)
    {
        if (state.input->mouse_down_event && state.top_window_at_current_mouse_pos != -1)
        {
            int focused_window_z = state.windows[state.top_window_at_current_mouse_pos].z;
            for (auto &[id, window] : state.windows)
            {
                if (window.z < focused_window_z)
                {
                    window.z += 1;
                }
            }
            state.windows[state.top_window_at_current_mouse_pos].z = 0;
        }

        debug_begin_immediate();
        for (int i = 0; i < draw_list.buf.size(); i++)
        {
            DrawItem item = draw_list.buf[i];
            switch (item.type)
            {
            case DrawItem::Type::RECT:
            {
                draw_rect(state.target, item.rect_draw_item.rect, item.rect_draw_item.color);
            }
            break;
            case DrawItem::Type::TEXT:
            {
                Font *font = assets->get_font(FONT_ROBOTO_CONDENSED_LIGHT, item.text_draw_item.size);
                draw_text(*font, state.target, item.text_draw_item.text, item.text_draw_item.pos.x, item.text_draw_item.pos.y, 1, 1);
            }
            break;
            }
        }
        debug_end_immediate();
    }
}