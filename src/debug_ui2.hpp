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

        for (int i = 0; i < str.len; i++)
        {
            int c = str.data[i];
            hash = ((hash << 5) + hash) + c;
        }

        return hash;
    }

    struct DrawItem
    {
        DrawItem() {}
        enum struct Type
        {
            CLIP,
            TEXT,
            RECT,
            QUAD,
            TEXTURE,
        };

        struct ClipDrawItem
        {
            Rect rect;
        };
        struct TextDrawItem
        {
            AllocatedString<128> text;
            Vec2f pos;
            float size;
            Color color;
        };
        struct RectDrawItem
        {
            Rect rect;
            Color color;
        };
        struct QuadDrawItem
        {
            Vec2f points[4];
            Color color;
        };
        struct TexDrawItem
        {
            Texture texture;
            Rect rect;
        };

        Type type;
        union
        {
            ClipDrawItem clip_draw_item;
            TextDrawItem text_draw_item;
            RectDrawItem rect_draw_item;
            QuadDrawItem quad_draw_item;
            TexDrawItem tex_draw_item;
        };
    };
    struct DrawList
    {
        std::vector<DrawItem> buf;

        void push_clip(Rect rect)
        {
            DrawItem item;
            item.type = DrawItem::Type::CLIP;
            item.clip_draw_item = {rect};
            buf.push_back(item);
        }
        void push_text(String text, Vec2f pos, float size, Color color)
        {
            DrawItem item;
            item.type = DrawItem::Type::TEXT;
            item.text_draw_item = {string_to_allocated_string<128>(text), pos, size, color};
            buf.push_back(item);
        }
        void push_rect(Rect rect, Color color)
        {
            DrawItem item;
            item.type = DrawItem::Type::RECT;
            item.rect_draw_item = {rect, color};
            buf.push_back(item);
        }
        void push_quad(Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3, Color color)
        {
            DrawItem item;
            item.type = DrawItem::Type::QUAD;
            item.quad_draw_item = {{p0, p1, p2, p3}, color};
            buf.push_back(item);
        }
        void push_tex(Texture texture, Rect rect)
        {
            DrawItem item;
            item.type = DrawItem::Type::TEXTURE;
            item.tex_draw_item = {texture, rect};
            buf.push_back(item);
        }
    };
    struct Window
    {
        Rect rect{};

        Rect content_rect = {};
        Vec2f next_elem_pos = {0, 0};
        float scroll = 0;
        float last_height = 0;

        int z;
        DrawList draw_list;

        Window() {}
        Window(Rect rect)
        {
            this->rect = rect;
        }
    };
    struct ImmStyle
    {
        float title_font_size = 24;
        float content_font_size = 24;
        Vec2f inner_padding = {5, 5};
        float element_gap = 1.5f;

        Color content_default_text_color = {.2, .2, .2, 1};
        Color content_highlighted_text_color = {.9, .9, .9, 1};
        Color content_background_color = {.1, .1, .1, 1};
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
        ImmId just_activated = 0;
        ImmId just_deactivated = 0;
        ImmId dragging = 0;
        Vec2f drag_distance = {};
        ImmId just_stopped_dragging = 0;
        ImmId selected = 0;
        ImmId just_selected = 0;
        ImmId just_unselected = 0;

        ImmId top_window_at_current_mouse_pos = 0;

        AllocatedString<1024> in_progress_string;

        StackAllocator per_frame_alloc;
    };

    UiState state;

    Rect relative_to_absolute(Rect relative)
    {
        return {
            relative.x * state.target.width,
            relative.y * state.target.height,
            relative.width * state.target.width,
            relative.height * state.target.height,
        };
    }

    float get_text_width(String text, float size)
    {
        Font *font = state.assets->get_font(FONT_ROBOTO_CONDENSED_LIGHT, size);
        return get_text_width(*font, text);
    }

    bool do_hoverable(ImmId id, Rect rect, Rect mask = {})
    {
        if (state.top_window_at_current_mouse_pos == state.current_window &&
            in_rect(Vec2f{state.input->mouse_x, state.input->mouse_y}, rect, mask))
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
            if (state.active == id)
            {
                state.active = 0;
                state.just_deactivated = id;
            }
            return false;
        }

        if (state.input->mouse_down_event)
        {
            if (state.hot == id)
            {
                state.active = id;
                state.just_activated = id;
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
            state.just_stopped_dragging = id;
        }
        return false;
    }
    bool do_selectable(ImmId id, bool on_down = false)
    {
        bool hot = state.hot == id;
        bool triggered = on_down ? state.just_activated == id : state.just_deactivated == id;
        bool just_stopped_dragging = state.just_stopped_dragging == id;
        if (hot && triggered && !just_stopped_dragging)
        {
            state.just_selected = id;
            if (state.selected != 0)
            {
                state.just_unselected = state.selected;
            }
            state.selected = id;
        }
        else
        {
            if (state.input->mouse_down_event)
            {
                if (state.selected == id)
                {
                    state.just_unselected = id;
                    state.selected = 0;
                }
            }
        }
        return (state.selected == id);
    }

    void init()
    {
        state.per_frame_alloc.init(1034 * 1024 * 50); // 50mb
    }
    void start_frame(RenderTarget target, InputState *input, Assets *assets)
    {
        state.assets = assets;
        state.input = input;
        state.target = target;

        state.per_frame_alloc.reset();

        // find and cache the top window at the current position
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

        state.just_activated = 0;
        state.just_deactivated = 0;
        state.just_stopped_dragging = 0;
        state.just_selected = 0;
        state.just_unselected = 0;
    }
    void start_window(String title, Rect rect)
    {
        ImmId me = imm_hash(title);
        ImmId resize_handle_id = me + 1; // I don't know the hash function very well, hopefully this is ok
        ImmId scrollbar_id = me + 2;

        state.current_window = me;

        if (state.windows.count(me) == 0)
        {
            Window window(rect);
            window.z = state.windows.size();
            state.windows[me] = window;
        }
        Window &window = state.windows[me];

        Rect titlebar_rect = {window.rect.x, window.rect.y, window.rect.width, state.style.title_font_size + state.style.inner_padding.y * 2};
        Rect content_container_rect = {window.rect.x, window.rect.y + titlebar_rect.height, window.rect.width, window.rect.height - titlebar_rect.height};
        Color titlebar_color = {.9, .2, .3, 1};

        window.content_rect = {content_container_rect.x + state.style.inner_padding.x,
                               content_container_rect.y + state.style.inner_padding.y,
                               content_container_rect.width - state.style.inner_padding.x * 2,
                               content_container_rect.height - state.style.inner_padding.y * 2};
        window.next_elem_pos = {window.content_rect.x, window.content_rect.y};

        // scrollbar
        bool scrollbar_visible = false;
        if (window.last_height > window.content_rect.height)
        {
            scrollbar_visible = true;

            float scrollbar_width = 10;
            Rect scrollbar_area_rect = {content_container_rect.x + content_container_rect.width - scrollbar_width,
                                        content_container_rect.y,
                                        scrollbar_width,
                                        content_container_rect.height};
            window.content_rect.width -= scrollbar_width;

            float scroll_vertical_range = window.last_height - window.content_rect.height;
            float scrollbar_height = window.content_rect.height / window.last_height * scrollbar_area_rect.height;
            float scrollbar_vertical_range = scrollbar_area_rect.height - scrollbar_height;
            float scrollbar_percent = -window.scroll / scroll_vertical_range;
            float scrollbar_ratio = scroll_vertical_range / scrollbar_vertical_range;

            Rect scrollbar_rect = {scrollbar_area_rect.x,
                                   content_container_rect.y + (scrollbar_vertical_range * scrollbar_percent),
                                   scrollbar_width,
                                   scrollbar_height};

            bool scrollbar_hot = do_hoverable(scrollbar_id, scrollbar_rect);
            bool scrollbar_active = do_active(scrollbar_id);
            bool scrollbar_dragging = do_draggable(scrollbar_id);

            Color scrollbar_color = {0, 1, 0, 1};
            if (scrollbar_hot)
            {
                scrollbar_color = lighten(scrollbar_color, 0.2f);
            }
            if (scrollbar_active)
            {
                scrollbar_color = lighten(scrollbar_color, 0.2f);
            }
            if (scrollbar_dragging)
            {
                window.scroll -= scrollbar_ratio * (state.input->mouse_y - state.input->prev_mouse_y);
            }
            if (in_rect(Vec2f{state.input->mouse_x, state.input->mouse_y}, content_container_rect))
            {
                window.scroll += state.input->scrollwheel_count * 20;
            }

            if (window.scroll > 0)
            {
                window.scroll = 0;
                scrollbar_percent = 0;
            }
            if (-window.scroll > scroll_vertical_range)
            {
                scrollbar_percent = 1;
                window.scroll = -scroll_vertical_range;
            }

            window.next_elem_pos.y += window.scroll;

            window.draw_list.push_rect(scrollbar_rect, scrollbar_color);
        }
        else
        {
            window.scroll = 0;
        }
        window.last_height = 0;

        // moving window
        bool hot = do_hoverable(me, titlebar_rect);
        bool active = do_active(me);
        bool dragging = do_draggable(me);
        if (dragging)
        {
            window.rect.x += state.input->mouse_x - state.input->prev_mouse_x;
            window.rect.y += state.input->mouse_y - state.input->prev_mouse_y;
            titlebar_color = darken(titlebar_color, 0.2f);
        }

        // keep window in bounds
        if (window.rect.x > state.target.width - 30)
            window.rect.x = state.target.width - 30;
        if (window.rect.y > state.target.height - 30)
            window.rect.y = state.target.height - 30;
        if (window.rect.x + window.rect.width < 30)
            window.rect.x += 30 - (window.rect.x + window.rect.width);
        if (window.rect.y < 0)
            window.rect.y = 0;

        // snapping
        Rect top_handle = relative_to_absolute({0.45, 0, 0.1, 0.05});
        if (dragging)
        {
            window.draw_list.push_rect(top_handle, {1, 1, 1, 0.5});
        }
        if (state.just_stopped_dragging == me)
        {
            if (in_rect({state.input->mouse_x, state.input->mouse_y}, top_handle))
            {
                window.rect.x = 0;
                window.rect.width = state.target.width;
                window.rect.y = 0;
                window.rect.height = state.target.height / 3;
            }
        }

        // resizing
        Rect resize_handle_rect = {
            content_container_rect.x + content_container_rect.width - state.style.inner_padding.x * 2,
            content_container_rect.y + content_container_rect.height - state.style.inner_padding.y * 2,
            state.style.inner_padding.x * 2,
            state.style.inner_padding.y * 2,
        };
        bool resize_handle_hot = do_hoverable(resize_handle_id, resize_handle_rect);
        bool resize_handle_active = do_active(resize_handle_id);
        bool resize_handle_dragging = do_draggable(resize_handle_id);
        if (resize_handle_dragging)
        {
            window.rect.width += state.input->mouse_x - state.input->prev_mouse_x;
            window.rect.height += state.input->mouse_y - state.input->prev_mouse_y;
            window.rect.width = fmax(window.rect.width, state.style.inner_padding.x * 2);
            window.rect.height = fmax(window.rect.height, titlebar_rect.height + state.style.inner_padding.y * 2);
        }

        // recalculate some rects
        titlebar_rect = {window.rect.x, window.rect.y, window.rect.width, state.style.title_font_size + state.style.inner_padding.y * 2};
        content_container_rect = {window.rect.x, window.rect.y + titlebar_rect.height, window.rect.width, window.rect.height - titlebar_rect.height};

        // draw
        window.draw_list.push_rect(titlebar_rect, titlebar_color);
        window.draw_list.push_text(title, {titlebar_rect.x + state.style.inner_padding.x, titlebar_rect.y + state.style.inner_padding.y}, state.style.title_font_size, {1, 1, 1, 1});
        window.draw_list.push_rect(content_container_rect, {1, 1, 1, .8});

        window.draw_list.push_rect(resize_handle_rect, {.5, .5, .5, 1});

        window.draw_list.push_clip(window.content_rect);
    }
    void end_window()
    {
        Window &window = state.windows[state.current_window];
        window.draw_list.push_clip({0, 0, 0, 0});
    }

    void end_frame(Camera *camera, Assets *assets)
    {
        // window ordering
        if (state.input->mouse_down_event && state.top_window_at_current_mouse_pos)
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
        for (int z = state.windows.size() - 1; z >= 0; z--)
        {
            for (auto &[id, window] : state.windows)
            {
                if (window.z == z)
                {
                    for (int i = 0; i < window.draw_list.buf.size(); i++)
                    {
                        DrawItem item = window.draw_list.buf[i];
                        switch (item.type)
                        {
                        case DrawItem::Type::CLIP:
                        {
                            if (item.clip_draw_item.rect.width == 0 && item.clip_draw_item.rect.height == 0)
                                end_scissor();
                            else
                                start_scissor(state.target, item.clip_draw_item.rect);
                        }
                        break;
                        case DrawItem::Type::TEXT:
                        {
                            Font *font = assets->get_font(FONT_ROBOTO_CONDENSED_LIGHT, item.text_draw_item.size);
                            draw_text(*font, state.target, item.text_draw_item.text, item.text_draw_item.pos.x, item.text_draw_item.pos.y, item.text_draw_item.color);
                        }
                        break;
                        case DrawItem::Type::RECT:
                        {
                            draw_rect(state.target, item.rect_draw_item.rect, item.rect_draw_item.color);
                        }
                        break;
                        case DrawItem::Type::QUAD:
                        {
                            debug_draw_immediate(state.target,
                                                 item.quad_draw_item.points[0],
                                                 item.quad_draw_item.points[1],
                                                 item.quad_draw_item.points[2],
                                                 item.quad_draw_item.points[3],
                                                 item.quad_draw_item.color);
                        }
                        break;
                        case DrawItem::Type::TEXTURE:
                        {
                            draw_textured_rect(state.target, item.tex_draw_item.rect, {}, item.tex_draw_item.texture);
                        }
                        break;
                        }
                    }
                    window.draw_list.buf.clear();
                    end_scissor();
                }
            }
        }
        debug_end_immediate();
    }

    void label(String text)
    {
        Window &window = state.windows[state.current_window];

        Rect rect = {window.next_elem_pos.x, window.next_elem_pos.y,
                     window.content_rect.width, state.style.content_font_size};
        window.next_elem_pos.y += rect.height + state.style.element_gap;
        window.last_height += rect.height + state.style.element_gap;

        window.draw_list.push_text(text,
                                   {rect.x, rect.y},
                                   state.style.content_font_size,
                                   state.style.content_default_text_color);
    }

    bool button(String text)
    {
        ImmId me = imm_hash(text);
        Window &window = state.windows[state.current_window];

        float border = 5;
        float text_width = get_text_width(text, state.style.content_font_size);
        Rect rect = {
            window.next_elem_pos.x,
            window.next_elem_pos.y,
            text_width + border * 2,
            state.style.content_font_size + border * 2,
        };

        window.next_elem_pos.y += rect.height + state.style.element_gap;
        window.last_height += rect.height + state.style.element_gap;

        bool hot = do_hoverable(me, rect, window.content_rect);
        bool active = do_active(me);
        bool triggered = (state.just_deactivated == me && hot);

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
        if (active)
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

        window.draw_list.push_quad({x4, y1},
                                   {x3, y2},
                                   {x2, y2},
                                   {x1, y1},
                                   top);
        window.draw_list.push_quad({x4, y4},
                                   {x1, y4},
                                   {x2, y3},
                                   {x3, y3},
                                   down);
        window.draw_list.push_quad({x1, y4},
                                   {x2, y3},
                                   {x2, y2},
                                   {x1, y1},
                                   left);
        window.draw_list.push_quad({x4, y4},
                                   {x3, y3},
                                   {x3, y2},
                                   {x4, y1},
                                   right);
        window.draw_list.push_quad({x2, y2}, // center
                                   {x2, y3},
                                   {x3, y3},
                                   {x3, y2},
                                   color);
        window.draw_list.push_text(text,
                                   {rect.x + border, rect.y + border + text_y_offset},
                                   state.style.content_font_size,
                                   state.style.content_highlighted_text_color);

        return triggered;
    }

    bool list_item(ImmId me, String text, bool selected_flag = false)
    {
        Window &window = state.windows[state.current_window];

        Rect rect = {window.next_elem_pos.x,
                     window.next_elem_pos.y,
                     window.content_rect.width - state.style.inner_padding.x * 2,
                     state.style.content_font_size + state.style.inner_padding.y * 2};
        window.next_elem_pos.y += rect.height + state.style.element_gap;
        window.last_height += rect.height + state.style.element_gap;

        bool hot = do_hoverable(me, rect, window.content_rect);
        bool active = do_active(me);
        bool dragging = do_draggable(me);
        bool selected = do_selectable(me, true) || selected_flag;

        Color text_color = state.style.content_default_text_color;
        if (selected)
        {
            window.draw_list.push_rect(rect, state.style.content_background_color);
            text_color = state.style.content_highlighted_text_color;
        }

        window.draw_list.push_text(text,
                                   {rect.x + state.style.inner_padding.x, rect.y + state.style.inner_padding.y},
                                   state.style.content_font_size,
                                   text_color);

        return selected;
    }

    // returns true on submit
    template <size_t N>
    bool textbox(ImmId me, AllocatedString<N> *str)
    {
        Window &window = state.windows[state.current_window];

        Rect rect = {window.next_elem_pos.x,
                     window.next_elem_pos.y,
                     window.content_rect.width - state.style.inner_padding.x * 2,
                     state.style.content_font_size + state.style.inner_padding.y * 2};
        window.next_elem_pos.y += rect.height + state.style.element_gap;
        window.last_height += rect.height + state.style.element_gap;

        bool hot = do_hoverable(me, rect, window.content_rect);
        bool active = do_active(me);
        bool dragging = do_draggable(me);
        bool selected = do_selectable(me);

        if (selected)
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

        Color color = state.style.content_background_color;
        if (selected)
        {
            color = lighten(state.style.content_background_color, .2);
        }

        window.draw_list.push_rect(rect, color);
        window.draw_list.push_text(*str,
                                   {rect.x + state.style.inner_padding.x, rect.y + state.style.inner_padding.y},
                                   state.style.content_font_size,
                                   state.style.content_highlighted_text_color);

        if (state.just_unselected == me ||
            (selected && state.input->key_down_events[(int)Keys::ENTER]))
            return true;
        return false;
    }
    template <size_t N>
    void textbox(AllocatedString<N> *str)
    {
        ImmId me = (ImmId)(uint64_t)str;
        textbox(me, str);
    }

    void num_input(float *val)
    {
        ImmId me = (ImmId)(uint64_t)val;
        Window &window = state.windows[state.current_window];

        Rect rect = {window.next_elem_pos.x,
                     window.next_elem_pos.y,
                     window.content_rect.width - state.style.inner_padding.x * 2,
                     state.style.content_font_size + state.style.inner_padding.y * 2};

        bool hot = do_hoverable(me, rect, window.content_rect);
        bool active = do_active(me);
        bool dragging = do_draggable(me);
        bool selected = do_selectable(me);

        if (state.just_selected == me)
        {
            state.in_progress_string = float_to_allocated_string<1024>(*val);
        }

        if (state.selected == me)
        {
            if (textbox(me, &state.in_progress_string))
            {
                *val = strtof(String(state.in_progress_string).to_char_array(&state.per_frame_alloc), nullptr);
            }
        }
        else
        {
            window.next_elem_pos.y += rect.height + state.style.element_gap;
            window.last_height += rect.height + state.style.element_gap;

            if (dragging)
            {
                *val += (state.input->mouse_x - state.input->prev_mouse_x) / 50;
            }

            AllocatedString<1024> val_str = float_to_allocated_string<1024>(*val);
            window.draw_list.push_rect(rect, state.style.content_background_color);
            window.draw_list.push_text(val_str,
                                       {rect.x + state.style.inner_padding.x, rect.y + state.style.inner_padding.y},
                                       state.style.content_font_size,
                                       state.style.content_highlighted_text_color);
        }
    }

    void texture(ImmId me, Texture val, Rect rect)
    {
        Window &window = state.windows[state.current_window];

        bool hot = do_hoverable(me, rect, window.content_rect);
        bool active = do_active(me);
        bool dragging = do_draggable(me);
        bool selected = do_selectable(me);

        window.draw_list.push_tex(val, rect);
    }
    void texture(Texture *val)
    {
        ImmId me = (ImmId)(uint64_t)val;
        Window &window = state.windows[state.current_window];
        texture(me, *val, {window.content_rect.x, window.content_rect.y, window.content_rect.width, window.content_rect.height});
    }
}