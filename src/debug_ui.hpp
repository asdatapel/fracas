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
            RECT_PTR,
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
        struct RectPtrDrawItem
        {
            Rect *rect;
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
            RectPtrDrawItem rect_ptr_draw_item;
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
        void push_rect_ptr(Rect *rect, Color color)
        {
            DrawItem item;
            item.type = DrawItem::Type::RECT_PTR;
            item.rect_ptr_draw_item = {rect, color};
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
        AllocatedString<128> name;
        bool visible = true;
        Rect rect{};

        Rect content_rect = {};
        Vec2f next_elem_pos = {0, 0};
        float scroll = 0;
        float last_height = 0;

        int z;
        DrawList draw_list;

        Window() {}
        Window(String name, Rect rect)
        {
            this->name = string_to_allocated_string<128>(name);
            this->rect = rect;
        }
    };
    struct ImmStyle
    {
        float menubar_height = 30.f;

        float title_font_size = 24;
        float content_font_size = 16;
        Vec2f inner_padding = {5, 5};
        float window_control_border = 2;
        float element_gap = 1.5f;

        Color content_default_text_color = {0, 0, 0, 1};
        Color content_highlighted_text_color = {1, 1, 1, 1};
        Color content_background_color = {.1, .1, .1, 1};
    };
    struct UiState
    {
        Assets *assets;
        RenderTarget target;
        InputState *input;
        Camera *camera;

        ImmStyle style;

        bool menubar_visible = false;
        ImmId current_menubar_menu = 0;
        bool current_menubar_menu_selected = false;
        float next_menubar_x = 0;
        float next_menubar_menu_x = 0;
        float next_menubar_menu_y = 0;
        Rect menubar_menu_background_rect;

        Rect content_rect;

        ImmId current_window = 0;

        std::map<ImmId, Window> windows;
        ImmId top_window_at_current_mouse_pos = 0;

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

        Vec3f gizmo_last_contact_point;

        bool last_element_hot = false;
        bool last_element_active = false;
        bool last_element_dragging = false;
        bool last_element_selected = false;

        ImmId anchored_left = 0;
        int anchored_left_priority = 0;
        ImmId anchored_right = 0;
        int anchored_right_priority = 0;
        ImmId anchored_up = 0;
        int anchored_up_priority = 0;
        ImmId anchored_down = 0;
        int anchored_down_priority = 0;
        ImmId anchored_center = 0;

        AllocatedString<1024> in_progress_string;

        StackAllocator per_frame_alloc;

        DrawList global_draw_list;
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

    bool do_hoverable(ImmId id, bool is_hot)
    {
        if (is_hot)
        {
            state.hot = id;
        }
        else if (state.hot == id)
        {
            state.hot = 0;
        }
        state.last_element_hot = (state.hot == id);
        return state.last_element_hot;
    }
    bool do_hoverable(ImmId id, Rect rect, Rect mask = {})
    {
        bool interactible = (state.current_menubar_menu == 0) ||
                            (state.current_menubar_menu != 0 && state.current_window == 0);
        bool in_top_window = state.current_window == 0 ||
                             (state.top_window_at_current_mouse_pos == state.current_window);
        bool is_hot = interactible && in_top_window &&
                      in_rect(Vec2f{state.input->mouse_x, state.input->mouse_y}, rect, mask);
        return do_hoverable(id, is_hot);
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
        }

        if (state.input->mouse_down_event)
        {
            if (state.hot == id)
            {
                state.active = id;
                state.just_activated = id;
                state.active_position = {state.input->mouse_x, state.input->mouse_y};
            }
            else
            {
                if (state.active == id)
                {
                    state.active = 0;
                }
            }
        }
        state.last_element_active = (state.active == id);
        return state.last_element_active;
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
            state.last_element_dragging = (state.dragging == id);
            return (state.dragging == id);
        }
        else if (state.dragging == id)
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
            if (state.selected != 0)
            {
                state.just_unselected = state.selected;
            }
            state.selected = id;
            state.just_selected = id;
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

        state.last_element_selected = (state.selected == id);
        return state.last_element_selected;
    }

    void save_layout()
    {
        StackAllocator *a = &state.per_frame_alloc;

        YAML::Dict *out_layout = YAML::new_dict(a);

        YAML::List *out_windows = YAML::new_list(a);
        for (auto [id, in_window] : state.windows)
        {
            YAML::Dict *window = YAML::new_dict(a);
            window->push_back("id", YAML::new_literal(String::from(id, a), a), a);
            window->push_back("name", YAML::new_literal(in_window.name, a), a);
            window->push_back("visible", YAML::new_literal(in_window.visible ? (String) "true" : (String) "false", a), a);

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
        out_anchors->push_back("anchored_left", YAML::new_literal(String::from(state.anchored_left, a), a), a);
        out_anchors->push_back("anchored_left_priority", YAML::new_literal(String::from(state.anchored_left_priority, a), a), a);
        out_anchors->push_back("anchored_right", YAML::new_literal(String::from(state.anchored_right, a), a), a);
        out_anchors->push_back("anchored_right_priority", YAML::new_literal(String::from(state.anchored_right_priority, a), a), a);
        out_anchors->push_back("anchored_up", YAML::new_literal(String::from(state.anchored_up, a), a), a);
        out_anchors->push_back("anchored_up_priority", YAML::new_literal(String::from(state.anchored_up_priority, a), a), a);
        out_anchors->push_back("anchored_down", YAML::new_literal(String::from(state.anchored_down, a), a), a);
        out_anchors->push_back("anchored_down_priority", YAML::new_literal(String::from(state.anchored_down_priority, a), a), a);
        out_anchors->push_back("anchored_center", YAML::new_literal(String::from(state.anchored_center, a), a), a);
        out_layout->push_back("anchors", out_anchors, a);

        String out;
        out.data = a->next;
        YAML::serialize(out_layout, a, 0, false);
        out.len = a->next - out.data;
        printf("%.*s\n", out.len, out.data);

        write_file("resources/test/layout.yaml", out);
    }
    void load_layout()
    {
        StackAllocator *a = &state.per_frame_alloc;

        FileData file = read_entire_file("resources/test/layout.yaml", a);

        String in_str;
        in_str.data = file.data;
        in_str.len = file.length;
        YAML::Dict *root = YAML::deserialize(in_str, a)->as_dict();

        YAML::List *in_windows = root->get("windows")->as_list();
        for (int i = 0; i < in_windows->len; i++)
        {
            YAML::Dict *in_window = in_windows->get(i)->as_dict();
            ImmId id = in_window->get("id")->as_literal().to_uint64();
            
            Window &window = state.windows[id];
            window.name = string_to_allocated_string<128>(in_window->get("name")->as_literal());
            window.visible = in_window->get("visible") && strcmp(in_window->get("visible")->as_literal(), "true");

            YAML::Dict *rect = in_window->get("rect")->as_dict();
            window.rect.x = atof(rect->get("x")->as_literal().to_char_array(a));
            window.rect.y = atof(rect->get("y")->as_literal().to_char_array(a));
            window.rect.width = atof(rect->get("width")->as_literal().to_char_array(a));
            window.rect.height = atof(rect->get("height")->as_literal().to_char_array(a));
        }

        YAML::Dict *in_anchors = root->get("anchors")->as_dict();
        state.anchored_left = in_anchors->get("anchored_left")->as_literal().to_uint64();
        state.anchored_left_priority = in_anchors->get("anchored_left_priority")->as_literal().to_uint64();
        state.anchored_right = in_anchors->get("anchored_right")->as_literal().to_uint64();
        state.anchored_right_priority = in_anchors->get("anchored_right_priority")->as_literal().to_uint64();
        state.anchored_up = in_anchors->get("anchored_up")->as_literal().to_uint64();
        state.anchored_up_priority = in_anchors->get("anchored_up_priority")->as_literal().to_uint64();
        state.anchored_down = in_anchors->get("anchored_down")->as_literal().to_uint64();
        state.anchored_down_priority = in_anchors->get("anchored_down_priority")->as_literal().to_uint64();
        state.anchored_center = in_anchors->get("anchored_center")->as_literal().to_uint64();
    }
    void init()
    {
        state.per_frame_alloc.init(1034 * 1024 * 50); // 50mb

        load_layout();
    }

    void start_frame(RenderTarget target, InputState *input, Assets *assets, Camera *camera)
    {
        state.per_frame_alloc.reset();

        state.target = target;
        state.input = input;
        state.assets = assets;
        state.camera = camera;

        Rect new_content_rect = {0, 0, (float)target.width, (float)target.height};
        if (state.menubar_visible)
        {
            new_content_rect.y += state.style.menubar_height;
            new_content_rect.height -= state.style.menubar_height;
        }
        state.menubar_visible = false;

        // deal with rendertarget size changes
        if ((new_content_rect != state.content_rect) &&
            state.content_rect.width != 0 && state.content_rect.height != 0)
        {
            if (state.anchored_up != 0)
            {
                Window &window = state.windows[state.anchored_up];
                float height_pct = window.rect.height / state.content_rect.height;
                float new_height = height_pct * new_content_rect.height;
                window.rect.height = new_height;
            }
            if (state.anchored_down != 0)
            {
                Window &window = state.windows[state.anchored_down];
                float y_pct = window.rect.y / state.content_rect.height;
                float new_y = y_pct * new_content_rect.height;
                window.rect.y = new_y;
            }
            if (state.anchored_left != 0)
            {
                Window &window = state.windows[state.anchored_left];
                float width_pct = window.rect.width / state.content_rect.width;
                float new_width = width_pct * new_content_rect.width;
                window.rect.width = new_width;
            }
            if (state.anchored_right != 0)
            {
                Window &window = state.windows[state.anchored_right];
                float x_pct = window.rect.x / state.content_rect.width;
                float new_x = x_pct * new_content_rect.width;
                window.rect.x = new_x;
            }
        }
        state.content_rect = new_content_rect;

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

        state.current_window = 0;

        state.next_menubar_x = 0;

        state.just_activated = 0;
        state.just_deactivated = 0;
        state.just_stopped_dragging = 0;
        state.just_selected = 0;
        state.just_unselected = 0;

        state.last_element_hot = false;
        state.last_element_active = false;
        state.last_element_dragging = false;
        state.last_element_selected = false;
    }

    void start_menubar_menu(String name)
    {
        const float font_size = state.style.content_font_size;

        ImmId me = imm_hash(name);

        state.current_menubar_menu_selected = false;

        if (!state.menubar_visible)
        {
            state.global_draw_list.push_rect({0, 0, (float)state.target.width, state.style.menubar_height}, {1, .141, 0, 1});
            state.menubar_visible = true;
        }

        Rect interactive_rect = {state.next_menubar_x, 0, get_text_width(name, font_size) + (state.style.inner_padding.x * 2), state.style.menubar_height};
        state.next_menubar_x += interactive_rect.width;

        Vec2f text_pos = {interactive_rect.x + state.style.inner_padding.x,
                          (interactive_rect.height / 2) - (font_size / 2)};

        Color button_color = {0, 0, 1, 1};

        bool hot = do_hoverable(me, interactive_rect);
        bool active = do_active(me);
        bool selected = do_selectable(me);

        if (hot || selected)
        {
            button_color = lighten(button_color, 0.3f);
        }

        if (state.current_menubar_menu == me || state.just_selected == me)
        {
            state.top_window_at_current_mouse_pos = 0;
            state.current_menubar_menu = me;
            state.current_menubar_menu_selected = true;

            if (state.input->mouse_down_event && !in_rect({state.input->mouse_x, state.input->mouse_y}, state.menubar_menu_background_rect))
            {
                state.current_menubar_menu = 0;
            }

            state.next_menubar_menu_x = interactive_rect.x + 5;
            state.next_menubar_menu_y = interactive_rect.y + interactive_rect.height + 5;
            state.menubar_menu_background_rect = {
                interactive_rect.x,
                interactive_rect.y + interactive_rect.height,
                0,
                10,
            };
            state.global_draw_list.push_rect_ptr(&state.menubar_menu_background_rect, {1, 0, 0, 1});
        }

        state.global_draw_list.push_rect(interactive_rect, button_color);
        state.global_draw_list.push_text(name, text_pos, font_size, {1, 1, 1, 1});
    }

    void start_window(String title, Rect rect)
    {
        ImmId me = imm_hash(title);
        ImmId resize_handle_id = me + 1; // I don't know the hash function very well, hopefully this is ok
        ImmId scrollbar_id = me + 2;
        ImmId top_handle_id = me + 3;
        ImmId bottom_handle_id = me + 4;
        ImmId left_handle_id = me + 5;
        ImmId right_handle_id = me + 6;

        state.current_window = me;

        if (state.windows.count(me) == 0)
        {
            Window window(title, rect);
            window.z = state.windows.size();
            state.windows[me] = window;
        }
        Window &window = state.windows[me];

        // TODO this is not doing everything it needs to do to hide a window
        if (!window.visible)
        {
            return;
        }

        Rect titlebar_rect = {window.rect.x + state.style.window_control_border,
                              window.rect.y + state.style.window_control_border,
                              window.rect.width - (state.style.window_control_border * 2),
                              state.style.title_font_size + (state.style.inner_padding.y * 2) - state.style.window_control_border};
        Rect content_container_rect = {window.rect.x + state.style.window_control_border,
                                       window.rect.y + titlebar_rect.height,
                                       window.rect.width - (state.style.window_control_border * 2),
                                       window.rect.height - (titlebar_rect.height + state.style.window_control_border)};
        Color titlebar_color = {.9, .2, .3, 1};

        window.content_rect = {content_container_rect.x + state.style.inner_padding.x,
                               content_container_rect.y + state.style.inner_padding.y,
                               content_container_rect.width - (state.style.inner_padding.x * 2),
                               content_container_rect.height - (state.style.inner_padding.y * 2)};
        window.next_elem_pos = {window.content_rect.x, window.content_rect.y};

        // scrollbar
        bool scrollbar_visible = false;
        if (window.last_height > window.content_rect.height)
        {
            scrollbar_visible = true;

            float scrollbar_width = 10;
            float scrollbar_gap = 2;
            Rect scrollbar_area_rect = {window.content_rect.x + window.content_rect.width + scrollbar_gap - scrollbar_width,
                                        window.content_rect.y,
                                        scrollbar_width,
                                        window.content_rect.height};
            window.content_rect.width -= scrollbar_area_rect.width + scrollbar_gap;

            float scroll_vertical_range = window.last_height - window.content_rect.height;
            float scrollbar_height = window.content_rect.height / window.last_height * scrollbar_area_rect.height;
            float scrollbar_vertical_range = scrollbar_area_rect.height - scrollbar_height;
            float scrollbar_percent = -window.scroll / scroll_vertical_range;
            float scrollbar_ratio = scroll_vertical_range / scrollbar_vertical_range;

            Rect scrollbar_rect = {scrollbar_area_rect.x,
                                   scrollbar_area_rect.y + (scrollbar_vertical_range * scrollbar_percent),
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

            if (state.anchored_up == me)
            {
                state.anchored_up = 0;
                state.anchored_up_priority = 0;
            }
            if (state.anchored_down == me)
            {
                state.anchored_down = 0;
                state.anchored_down_priority = 0;
            }
            if (state.anchored_left == me)
            {
                state.anchored_left = 0;
                state.anchored_left_priority = 0;
            }
            if (state.anchored_right == me)
            {
                state.anchored_right = 0;
                state.anchored_right_priority = 0;
            }
            if (state.anchored_center == me)
            {
                state.anchored_center = 0;
            }
        }

        // snapping
        Rect top_handle = relative_to_absolute({0.45, 0, 0.1, 0.05});
        Rect bottom_handle = relative_to_absolute({0.45, .95, 0.1, 0.05});
        Rect left_handle = relative_to_absolute({0, 0.45, 0.025, 0.1});
        Rect right_handle = relative_to_absolute({.975, 0.45, 0.025, 0.1});
        Rect center_handle = relative_to_absolute({.475, 0.475, 0.05, 0.05});
        if (dragging)
        {
            window.draw_list.push_rect(top_handle, {1, 1, 1, 0.5});
            window.draw_list.push_rect(bottom_handle, {1, 1, 1, 0.5});
            window.draw_list.push_rect(left_handle, {1, 1, 1, 0.5});
            window.draw_list.push_rect(right_handle, {1, 1, 1, 0.5});
            window.draw_list.push_rect(center_handle, {1, 1, 1, 0.5});
        }
        if (state.just_stopped_dragging == me)
        {
            auto top_priority = [](int *new_top)
            {
                if (state.anchored_up_priority > *new_top)
                {
                    state.anchored_up_priority -= 1;
                }
                if (state.anchored_down_priority > *new_top)
                {
                    state.anchored_down_priority -= 1;
                }
                if (state.anchored_left_priority > *new_top)
                {
                    state.anchored_left_priority -= 1;
                }
                if (state.anchored_right_priority > *new_top)
                {
                    state.anchored_right_priority -= 1;
                }
                *new_top = 5;
            };
            if (in_rect({state.input->mouse_x, state.input->mouse_y}, top_handle))
            {
                state.anchored_up = me;
                top_priority(&state.anchored_up_priority);
            }
            if (in_rect({state.input->mouse_x, state.input->mouse_y}, bottom_handle))
            {
                state.anchored_down = me;
                top_priority(&state.anchored_down_priority);
            }
            if (in_rect({state.input->mouse_x, state.input->mouse_y}, left_handle))
            {
                state.anchored_left = me;
                top_priority(&state.anchored_left_priority);
            }
            if (in_rect({state.input->mouse_x, state.input->mouse_y}, right_handle))
            {
                state.anchored_right = me;
                top_priority(&state.anchored_right_priority);
            }
            if (in_rect({state.input->mouse_x, state.input->mouse_y}, center_handle))
            {
                state.anchored_center = me;
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
        Rect control_border_top = {window.rect.x, window.rect.y, window.rect.width, state.style.window_control_border};
        Color control_border_top_color = {0, 0, 0, 1};
        Rect resize_handle_top = {control_border_top.x, control_border_top.y, control_border_top.width, control_border_top.height + 5};
        Rect control_border_bottom = {window.rect.x, window.rect.y + window.rect.height - state.style.window_control_border, window.rect.width, state.style.window_control_border};
        Color control_border_bottom_color = {0, 0, 0, 1};
        Rect resize_handle_bottom = {control_border_bottom.x, control_border_bottom.y - 5, control_border_bottom.width, control_border_bottom.height + 5};
        Rect control_border_left = {window.rect.x, window.rect.y, state.style.window_control_border, window.rect.height};
        Color control_border_left_color = {0, 0, 0, 1};
        Rect resize_handle_left = {control_border_left.x, control_border_left.y, control_border_left.width + 5, control_border_left.height};
        Rect control_border_right = {window.rect.x + window.rect.width - state.style.window_control_border, window.rect.y, state.style.window_control_border, window.rect.height};
        Color control_border_right_color = {0, 0, 0, 1};
        Rect resize_handle_right = {control_border_right.x - 5, control_border_right.y, control_border_right.width + 5, control_border_right.height};
        bool top_handle_hot = do_hoverable(top_handle_id, resize_handle_top);
        bool top_handle_active = do_active(top_handle_id);
        bool top_handle_dragging = do_draggable(top_handle_id);
        bool bottom_handle_hot = do_hoverable(bottom_handle_id, resize_handle_bottom);
        bool bottom_handle_active = do_active(bottom_handle_id);
        bool bottom_handle_dragging = do_draggable(bottom_handle_id);
        bool left_handle_hot = do_hoverable(left_handle_id, resize_handle_left);
        bool left_handle_active = do_active(left_handle_id);
        bool left_handle_dragging = do_draggable(left_handle_id);
        bool right_handle_hot = do_hoverable(right_handle_id, resize_handle_right);
        bool right_handle_active = do_active(right_handle_id);
        bool right_handle_dragging = do_draggable(right_handle_id);
        if (top_handle_hot)
        {
            control_border_top_color = {0, 0, 1, 1};
        }
        else if (bottom_handle_hot)
        {
            control_border_bottom_color = {0, 0, 1, 1};
        }
        else if (left_handle_hot)
        {
            control_border_left_color = {0, 0, 1, 1};
        }
        else if (right_handle_hot)
        {
            control_border_right_color = {0, 0, 1, 1};
        }
        if (top_handle_dragging)
        {
            window.rect.y += state.input->mouse_y - state.input->prev_mouse_y;
            window.rect.height -= state.input->mouse_y - state.input->prev_mouse_y;
        }
        else if (bottom_handle_dragging)
        {
            window.rect.height += state.input->mouse_y - state.input->prev_mouse_y;
        }
        else if (left_handle_dragging)
        {
            window.rect.x += state.input->mouse_x - state.input->prev_mouse_x;
            window.rect.width -= state.input->mouse_x - state.input->prev_mouse_x;
        }
        else if (right_handle_dragging)
        {
            window.rect.width += state.input->mouse_x - state.input->prev_mouse_x;
        }

        if (state.anchored_up == me)
        {
            window.rect.y = 0;
            if (state.anchored_left_priority > state.anchored_up_priority)
                window.rect.x = state.windows[state.anchored_left].rect.x + state.windows[state.anchored_left].rect.width;
            else
                window.rect.x = 0;

            if (state.anchored_right_priority > state.anchored_up_priority)
                window.rect.set_right(state.windows[state.anchored_right].rect.x);
            else
                window.rect.set_right(state.content_rect.x + state.content_rect.width);
        }
        if (state.anchored_down == me)
        {
            window.rect.set_bottom(state.content_rect.y + state.content_rect.height);
            if (state.anchored_left_priority > state.anchored_down_priority)
                window.rect.x = state.windows[state.anchored_left].rect.x + state.windows[state.anchored_left].rect.width;
            else
                window.rect.x = 0;

            if (state.anchored_right_priority > state.anchored_down_priority)
                window.rect.set_right(state.windows[state.anchored_right].rect.x);
            else
                window.rect.set_right(state.content_rect.x + state.content_rect.width);
        }
        if (state.anchored_left == me)
        {
            window.rect.x = 0;
            if (state.anchored_up_priority > state.anchored_left_priority)
                window.rect.y = state.windows[state.anchored_up].rect.y + state.windows[state.anchored_up].rect.height;
            else
                window.rect.y = state.content_rect.y;

            if (state.anchored_down_priority > state.anchored_left_priority)
                window.rect.set_bottom(state.windows[state.anchored_down].rect.y);
            else
                window.rect.set_bottom(state.content_rect.y + state.content_rect.height);
        }
        if (state.anchored_right == me)
        {
            window.rect.set_right(state.content_rect.x + state.content_rect.width);
            if (state.anchored_up_priority > state.anchored_right_priority)
                window.rect.y = state.windows[state.anchored_up].rect.y + state.windows[state.anchored_up].rect.height;
            else
                window.rect.y = state.content_rect.y;

            if (state.anchored_down_priority > state.anchored_right_priority)
                window.rect.set_bottom(state.windows[state.anchored_down].rect.y);
            else
                window.rect.set_bottom(state.content_rect.y + state.content_rect.height);
        }
        if (state.anchored_center == me)
        {
            if (state.anchored_up)
                window.rect.y = state.windows[state.anchored_up].rect.y + state.windows[state.anchored_up].rect.height;
            else
                window.rect.y = state.content_rect.y;

            if (state.anchored_down)
                window.rect.set_bottom(state.windows[state.anchored_down].rect.y);
            else
                window.rect.set_bottom(state.content_rect.y + state.content_rect.height);

            if (state.anchored_left)
                window.rect.x = state.windows[state.anchored_left].rect.x + state.windows[state.anchored_left].rect.width;
            else
                window.rect.x = 0;

            if (state.anchored_right)
                window.rect.set_right(state.windows[state.anchored_right].rect.x);
            else
                window.rect.set_right(state.content_rect.x + state.content_rect.width);
        }

        // keep window in bounds
        if (window.rect.x > state.content_rect.width - 30)
            window.rect.x = state.content_rect.width - 30;
        if (window.rect.y > state.content_rect.height - 30)
            window.rect.y = state.content_rect.height - 30;
        if (window.rect.x + window.rect.width < 30)
            window.rect.x += 30 - (window.rect.x + window.rect.width);
        if (window.rect.y < state.content_rect.y)
            window.rect.y = state.content_rect.y;

        // clamp size
        window.rect.width = fmax(window.rect.width, 30);
        window.rect.height = fmax(window.rect.height, 50);

        // recalculate some rects
        titlebar_rect = {window.rect.x, window.rect.y, window.rect.width, state.style.title_font_size + state.style.inner_padding.y * 2};
        content_container_rect = {window.rect.x, window.rect.y + titlebar_rect.height, window.rect.width, window.rect.height - titlebar_rect.height};

        // draw
        window.draw_list.push_rect(titlebar_rect, titlebar_color);
        window.draw_list.push_text(title, {titlebar_rect.x + state.style.inner_padding.x, titlebar_rect.y + state.style.inner_padding.y}, state.style.title_font_size, {1, 1, 1, 1});
        window.draw_list.push_rect(content_container_rect, {1, 1, 1, .8});

        window.draw_list.push_rect(control_border_top, control_border_top_color);
        window.draw_list.push_rect(control_border_bottom, control_border_bottom_color);
        window.draw_list.push_rect(control_border_left, control_border_left_color);
        window.draw_list.push_rect(control_border_right, control_border_right_color);

        window.draw_list.push_rect(resize_handle_rect, {.5, .5, .5, 1});

        window.draw_list.push_clip(window.content_rect);
    }
    void end_window()
    {
        Window &window = state.windows[state.current_window];
        window.draw_list.push_clip({0, 0, 0, 0});
    }

    void end_frame(Assets *assets)
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

        // keep anchored windows at the bottom
        auto shuffle_down = [](Window &window)
        {
            for (auto &[id, other_window] : state.windows)
            {
                if (other_window.z > window.z)
                {
                    other_window.z -= 1;
                }
            }
            window.z = state.windows.size() - 1;
        };
        if (state.anchored_up)
        {
            shuffle_down(state.windows[state.anchored_up]);
        }
        if (state.anchored_down)
        {
            shuffle_down(state.windows[state.anchored_down]);
        }
        if (state.anchored_left)
        {
            shuffle_down(state.windows[state.anchored_left]);
        }
        if (state.anchored_right)
        {
            shuffle_down(state.windows[state.anchored_right]);
        }
        if (state.anchored_center)
        {
            shuffle_down(state.windows[state.anchored_center]);
        }

        state.target.bind();
        debug_begin_immediate();
        auto do_draw_list = [&](DrawList &draw_list)
        {
            for (int i = 0; i < draw_list.buf.size(); i++)
            {
                DrawItem item = draw_list.buf[i];
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
                    Font *font = assets->get_font(FONT_ROBOTO_CONDENSED_REGULAR, item.text_draw_item.size);
                    draw_text(*font, state.target, item.text_draw_item.text, item.text_draw_item.pos.x, item.text_draw_item.pos.y, item.text_draw_item.color);
                }
                break;
                case DrawItem::Type::RECT:
                {
                    draw_rect(state.target, item.rect_draw_item.rect, item.rect_draw_item.color);
                }
                break;
                case DrawItem::Type::RECT_PTR:
                {
                    draw_rect(state.target, *item.rect_ptr_draw_item.rect, item.rect_ptr_draw_item.color);
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
                    draw_textured_rect(state.target, item.tex_draw_item.rect, {1, 1, 1, 1}, item.tex_draw_item.texture);
                }
                break;
                }
            }
            draw_list.buf.clear();
            end_scissor();
        };
        for (int z = state.windows.size() - 1; z >= 0; z--)
        {
            for (auto &[id, window] : state.windows)
            {
                if (window.z == z)
                {
                    do_draw_list(window.draw_list);
                }
            }
        }
        do_draw_list(state.global_draw_list);
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

    bool button(ImmId me, String text)
    {
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
    bool button(String text)
    {
        ImmId me = imm_hash(text);
        return button(me, text);
    }

    bool list_item(ImmId me, String text, bool selected_flag = false, bool *as_checkbox = nullptr)
    {
        DrawList *draw_list;
        Rect rect;
        Rect container_rect;

        if (state.current_window != 0)
        {
            Window &window = state.windows[state.current_window];
            draw_list = &window.draw_list;

            rect = {window.next_elem_pos.x,
                    window.next_elem_pos.y,
                    window.content_rect.width - state.style.inner_padding.x * 2,
                    state.style.content_font_size + state.style.inner_padding.y * 2};
            container_rect = window.content_rect;

            window.next_elem_pos.y += rect.height + state.style.element_gap;
            window.last_height += rect.height + state.style.element_gap;
        }
        else if (state.current_menubar_menu != 0 && state.current_menubar_menu_selected)
        {
            draw_list = &state.global_draw_list;
            rect = {state.next_menubar_menu_x,
                    state.next_menubar_menu_y,
                    200,
                    state.style.content_font_size + state.style.inner_padding.y * 2};
            container_rect = {};

            state.next_menubar_menu_y += rect.height;
            state.menubar_menu_background_rect.width = fmax(rect.width + 10, state.menubar_menu_background_rect.width);
            state.menubar_menu_background_rect.height += rect.height;
        }
        else
        {
            return false;
        }

        bool hot = do_hoverable(me, rect, container_rect);
        bool active = do_active(me);
        bool dragging = do_draggable(me);
        bool selected = do_selectable(me, true) || selected_flag;

        if (as_checkbox)
        {
            if (state.just_selected == me)
            {
                *as_checkbox = !(*as_checkbox);
            }
            selected = *as_checkbox;
        }

        Color text_color = state.style.content_default_text_color;
        if (selected)
        {
            draw_list->push_rect(rect, state.style.content_background_color);
            text_color = state.style.content_highlighted_text_color;
        }

        draw_list->push_text(text,
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

    bool imm_translation_gizmo(Vec3f *p)
    {
        Window &window = state.windows[state.current_window];
        ImmId me = (ImmId)(uint64_t)p;

        auto screen_to_world = [](Rect target_rect, Camera *camera, Vec3f p, float z = 0.f)
        {
            glm::vec4 gl_screen = {(p.x - target_rect.x) / (target_rect.width / 2.f) - 1,
                                   -(p.y - target_rect.y) / (target_rect.height / 2.f) + 1,
                                   z, 1.f};
            glm::vec4 unprojects = glm::inverse(camera->perspective * camera->view) * gl_screen;
            unprojects /= unprojects.w;

            return Vec3f{unprojects.x, unprojects.y, unprojects.z};
        };

        float dist_from_camera_2 = powf(p->x - state.camera->pos_x, 2) + powf(p->y - state.camera->pos_y, 2) + powf(p->z - state.camera->pos_z, 2);
        float dist_from_camera = sqrtf(dist_from_camera_2);
        const float gizmoSize = 0.03f;
        float scale = gizmoSize * (dist_from_camera / tanf((3.1415 / 6) / 2.0f));

        Vec3f axes[3] = {{scale, 0, 0},
                         {0, scale, 0},
                         {0, 0, scale}};

        int selected_axis = -1;
        float min_distance = .1f;

        Vec3f closest_point_on_axis[3];

        Vec3f ray_origin = screen_to_world(window.content_rect, state.camera, {state.input->mouse_x, state.input->mouse_y});
        Vec3f ray_end = screen_to_world(window.content_rect, state.camera, {state.input->mouse_x, state.input->mouse_y}, .5);
        Vec3f ray_dir = {ray_end.x - ray_origin.x,
                         ray_end.y - ray_origin.y,
                         ray_end.z - ray_origin.z};

        for (int i = 0; i < 3; i++)
        {
            Vec3f axis_origin = *p;
            Vec3f axis_dir = axes[i];

            // stole this from somewhere
            float ray2 = ray_dir.x * ray_dir.x + ray_dir.y * ray_dir.y + ray_dir.z * ray_dir.z;
            float axis2 = axis_dir.x * axis_dir.x + axis_dir.y * axis_dir.y + axis_dir.z * axis_dir.z;
            float ray_axis = ray_dir.x * axis_dir.x + ray_dir.y * axis_dir.y + ray_dir.z * axis_dir.z;
            float det = -((ray2 * axis2) - (ray_axis * ray_axis));

            Vec3f d_origin = {ray_origin.x - axis_origin.x,
                              ray_origin.y - axis_origin.y,
                              ray_origin.z - axis_origin.z};
            float ray_dorigin = ray_dir.x * d_origin.x + ray_dir.y * d_origin.y + ray_dir.z * d_origin.z;
            float axis_dorigin = axis_dir.x * d_origin.x + axis_dir.y * d_origin.y + axis_dir.z * d_origin.z;

            if (fabs(det) > 0.0000001)
            {
                float ray_t = ((axis2 * ray_dorigin) - (axis_dorigin * ray_axis)) / det;
                float axis_t = (-(ray2 * axis_dorigin) + (ray_dorigin * ray_axis)) / det;

                closest_point_on_axis[i] = {
                    axis_origin.x + (axis_dir.x * axis_t),
                    axis_origin.y + (axis_dir.y * axis_t),
                    axis_origin.z + (axis_dir.z * axis_t),
                };

                if (axis_t >= 0 && axis_t <= 1)
                {
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

                    float dist2 = (p1.x - p0.x) * (p1.x - p0.x) +
                                  (p1.y - p0.y) * (p1.y - p0.y) +
                                  (p1.z - p0.z) * (p1.z - p0.z);

                    if (dist2 < min_distance)
                    {
                        min_distance = dist2;
                        selected_axis = i;
                    }
                }
            }
        }

        {
            auto ndc_to_screen = [](Rect draw_area, Vec3f p)
            {
                return Vec3f{(p.x + 1) * 0.5f * draw_area.width + draw_area.x, ((1 - p.y) * .5f * draw_area.height + draw_area.y), p.z};
            };

            glm::vec4 p_ndc = state.camera->perspective * state.camera->view * glm::vec4(p->x, p->y, p->z, 1.f);
            if (fabs(p_ndc.w) > 0)
            {
                p_ndc /= p_ndc.w;
                Vec3f p_screen = ndc_to_screen(window.content_rect, {p_ndc.x, p_ndc.y, p_ndc.z});

                Rect view_rect = {p_screen.x - 5, p_screen.y - 5, 10, 10};
                Rect interactive_rect = {p_screen.x - 15, p_screen.y - 15, 30, 30};

                Color color = {1, 0, 0, 1};
                window.draw_list.push_rect(view_rect, color);
            }
            Vec3f p_screen = ndc_to_screen(window.content_rect, {p_ndc.x, p_ndc.y, p_ndc.z});

            auto draw_axis = [&](int axis_i, Vec3f axis, Color color, ImmId id)
            {
                float handle_length = 100;
                glm::vec4 x_axis_offset_ndc = state.camera->perspective * state.camera->view * glm::vec4(p->x + axis.x, p->y + axis.y, p->z + axis.z, 1.f);
                float w = x_axis_offset_ndc.w;
                if (x_axis_offset_ndc.w > 0)
                {
                    x_axis_offset_ndc /= x_axis_offset_ndc.w;
                }
                Vec3f axis_offset_screen = ndc_to_screen(window.content_rect, {x_axis_offset_ndc.x, x_axis_offset_ndc.y, x_axis_offset_ndc.z});
                Vec2f axis_dir_screen = normalize(Vec2f{axis_offset_screen.x - p_screen.x, axis_offset_screen.y - p_screen.y});

                bool hot = do_hoverable(id, selected_axis == axis_i);
                bool active = do_active(id);
                bool dragging = do_draggable(id);

                if (state.just_activated == id)
                {
                    state.gizmo_last_contact_point = closest_point_on_axis[axis_i];
                }

                if (hot)
                {
                    color = lighten(color, 0.5f);
                }

                Vec2f line_normal = {axis_dir_screen.y, -axis_dir_screen.x};
                float line_thickness = 2;
                Vec2f line_p0 = {axis_offset_screen.x + line_normal.x * line_thickness, axis_offset_screen.y + line_normal.y * line_thickness};
                Vec2f line_p1 = {axis_offset_screen.x - line_normal.x * line_thickness, axis_offset_screen.y - line_normal.y * line_thickness};
                Vec2f line_p2 = {p_screen.x + line_normal.x * line_thickness, p_screen.y + line_normal.y * line_thickness};
                Vec2f line_p3 = {p_screen.x - line_normal.x * line_thickness, p_screen.y - line_normal.y * line_thickness};
                window.draw_list.push_quad(line_p0, line_p1, line_p3, line_p2, color);

                Vec2f arrow_p0 = {axis_offset_screen.x + line_normal.x * 5, axis_offset_screen.y + line_normal.y * 5};
                Vec2f arrow_p1 = {axis_offset_screen.x - line_normal.x * 5, axis_offset_screen.y - line_normal.y * 5};
                Vec2f arrow_p2 = {axis_offset_screen.x + axis_dir_screen.x * 10, axis_offset_screen.y + axis_dir_screen.y * 10};

                window.draw_list.push_quad(arrow_p0, arrow_p1, arrow_p2, arrow_p2, color);

                if (dragging)
                {
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

    void add_window_menubar_menu()
    {
        Imm::start_menubar_menu("Windows");
        for (auto &[id, window] : state.windows)
        {
            list_item((ImmId)&window.visible, window.name, window.visible, &window.visible);
        }
    }
}