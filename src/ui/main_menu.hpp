#pragma once

#include "../assets.hpp"
#include "../graphics.hpp"
#include "../ui.hpp"

// TODO very wrong place for this

enum struct PageId
{
    TITLE,
    CREATE_GAME,
    JOIN_GAME,
    SETTINGS,

    INGAME,
    EXIT,
};

struct Page
{
    UiContext2 ui;

    virtual void enter_page()
    {
        ui = {};
    }
    virtual PageId update_and_draw(RenderTarget target, InputState *input, Font *font, float time_step) {}
};

class TitlePage : public Page
{
    Button buttons[4];

    PageId update_and_draw(RenderTarget target, InputState *input, Font *font, float time_step) override
    {
        glDisable(GL_DEPTH_TEST);
        float border = target.width / 50.f;
        { // title text
            String text_1 = String::from("FAMILY");
            String text_2 = String::from("FEUD");

            float want_width = 0.6f * target.width;
            float max_width = 0.6f * 1920;
            float width = fminf(max_width, want_width);
            float scale = width / get_text_width(*font, text_1);

            float line1_height = get_single_line_height(*font, text_1, scale);

            draw_text_cropped(*font, target, text_1, border, border, scale, scale);
            draw_text_cropped(*font, target, text_2, border, border + 15 + line1_height, scale, scale);
        }
        { // buttons
            float max_width = 0.2f * 1920;
            float want_width = 0.2f * target.width;
            float width = fminf(max_width, want_width);
            float height = width * 9.f / 16.f / 2.f;

            String button_text[4] = {
                String::from("Exit"),
                String::from("Settings"),
                String::from("Join Game"),
                String::from("Create Game"),
            };
            for (int i = 0; i < 4; i++)
            {
                float center_x = target.width - border - (width / 2.f);
                float center_y = target.height - border - i * (15 + height) - (height / 2.f);
                float left = center_x - (width / 2.f);
                float top = center_y - (height / 2.f);

                buttons[i].color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
                buttons[i].rect = {left, top, width, height};
                buttons[i].str = button_text[i];
            }

            bool create_game = do_button2(&buttons[3], &ui, target, input, *font);
            if (create_game)
            {
                return PageId::CREATE_GAME;
            }

            bool join_game = do_button2(&buttons[2], &ui, target, input, *font);
            if (join_game)
            {
                return PageId::JOIN_GAME;
            }

            bool options = do_button2(&buttons[1], &ui, target, input, *font);
            if (options)
            {
                return PageId::SETTINGS;
            }

            bool exit = do_button2(&buttons[0], &ui, target, input, *font);
            if (exit)
            {
                return PageId::EXIT;
            }
        }

        glEnable(GL_DEPTH_TEST);

        return PageId::TITLE;
    }
};

struct CreateGamePage : public Page
{
    TextBox<64> game_name_text_box;

    void enter_page() override
    {
        game_name_text_box.str.len = 0;
    }

    PageId update_and_draw(RenderTarget target, InputState *input, Font *font, float time_step) override
    {
        glDisable(GL_DEPTH_TEST);
        // button
        float standard_border = get_standard_border(target);
        float max_width = 0.2f * 1920;
        float want_width = 0.2f * target.width;
        float button_width = fminf(max_width, want_width);
        float button_height = button_width * 9.f / 16.f / 2.f;

        float center_x = target.width - standard_border - (button_width / 2.f);
        float center_y = target.height - standard_border - (button_height / 2.f);
        float left = center_x - (button_width / 2.f);
        float top = center_y - (button_height / 2.f);

        static Button back_button;
        back_button.color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
        back_button.rect = {left, top, button_width, button_height};
        back_button.str = String::from("Back");
        bool back = do_button2(&back_button, &ui, target, input, *font);
        if (back)
        {
            return PageId::TITLE;
        }

        {
            float standard_border = get_standard_border(target);
            float title_width = (target.width / 2.f) - (standard_border * 2);
            float title_height = standard_border * 2;
            Rect title_rect = {standard_border, standard_border, title_width, title_height};
            draw_rect(target, title_rect, {0, 0, 0, 0.4});
            draw_centered_text(*font, target, String::from("New Game"), title_rect, .1f, 10, 1);

            float gap = (standard_border / 2.f);
            float internal_border = gap;
            float width = title_width;
            float height = target.height - (2.f * standard_border) - title_height - gap;

            Rect container_rect = {standard_border, title_rect.y + title_height + gap, width, height};
            draw_rect(target, container_rect, {0, 0, 0, 0.4});

            float x = container_rect.x + internal_border;
            float y = container_rect.y + internal_border;

            String text = String::from("Game Name:");
            float text_scale = standard_border / font->font_size_px;
            draw_text(*font, target, text, x, y, text_scale, text_scale);
            y += standard_border + gap;

            game_name_text_box.rect = {x, y, container_rect.width - (internal_border * 2), standard_border * 2};
            game_name_text_box.color = {31 / 255.f, 121 / 255.f, 197 / 255.f, 1};
            do_text_box(&game_name_text_box, &ui, target, input, *font);
            y += game_name_text_box.rect.height + gap;

            text = String::from("I want to host this:");
            text_scale = standard_border / font->font_size_px;
            draw_text(*font, target, text, x, y, text_scale, text_scale);
            y += standard_border + gap;

            static bool is_self_hosted = false;
            do_checkbox(&is_self_hosted, &ui, target, input, *font, {x, y, standard_border, standard_border}, {255 / 255.f, 125 / 255.f, 19 / 255.f, .7});

            float create_button_center_x = target.width - standard_border - (button_width / 2.f);
            float create_button_center_y = target.height - standard_border - (button_height / 2.f) - (button_height + gap);
            float create_button_left = create_button_center_x - (button_width / 2.f);
            float create_button_top = create_button_center_y - (button_height / 2.f);
            static Button create_button;
            create_button.color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
            create_button.rect = {create_button_left, create_button_top, button_width, button_height};
            create_button.str = String::from("Create");
            bool create = do_button2(&create_button, &ui, target, input, *font);
            if (create)
            {
                MessageBuilder msg((char)ClientMessageType::CREATE_GAME);
                msg.append(game_name_text_box.str);
                msg.append(is_self_hosted);
                msg.send(&server);

                return PageId::TITLE;
            }
        }
        glEnable(GL_DEPTH_TEST);

        return PageId::CREATE_GAME;
    }
};

struct ListElement {


};

struct JoinGamePage : public Page
{
    Array<GameProperties, MAX_GAMES> games;
    float last_refresh;

    Button back_button;

    void refresh_list_data()
    {
        MessageBuilder msg((char)ClientMessageType::LIST_GAMES);
        msg.send(&server);
        last_refresh = 0.f;
    }

    void draw_list(RenderTarget target, InputState *input, Font *font, Rect rect)
    {
        draw_rect(target, rect, {0, 0, 0, 0.4});

        float border = get_standard_border(target);
        float gap = border / 5.f;
        float item_width = rect.width - (border * 2);
        float item_height = 60; // not scaling for now

        float actual_height = rect.height - (border * 2);

        float total_item_height = games.len * (gap + item_height) - gap;
        float percent_visible = fminf(actual_height / total_item_height, 1.f);

        float scrollbar_width = border * .75f;
        float scrollbar_height = percent_visible * actual_height;
        float scrollbar_border = (border - scrollbar_width) / 2.f;
        float scrollbar_x = rect.x + rect.width - border + scrollbar_border;
        float scrollbar_max_y_offset = actual_height - scrollbar_height;

        static float scrollbar_y_offset = 0;
        for (int i = 0; i < input->key_input.len; i++)
        {
            if (input->key_input[i] == Keys::UP)
            {
                scrollbar_y_offset -= 5;
            }
            if (input->key_input[i] == Keys::DOWN)
            {
                scrollbar_y_offset += 5;
            }
        }
        if (scrollbar_y_offset < 0)
            scrollbar_y_offset = 0;
        if (scrollbar_y_offset > scrollbar_max_y_offset)
            scrollbar_y_offset = scrollbar_max_y_offset;

        float scrollbar_y = rect.y + border + scrollbar_y_offset;
        float scrollbar_percentage = scrollbar_y_offset / actual_height;
        static Rect scrollbar_rect;
        scrollbar_rect = {scrollbar_x, scrollbar_y, scrollbar_width, scrollbar_height};
        do_draggable(&ui, input, &scrollbar_rect, scrollbar_rect);
        Color scrollbar_color = {255 / 255.f, 125 / 255.f, 19 / 255.f, 1};
        if (ui.focus_started == &scrollbar_rect)
        {
            scrollbar_color = darken(scrollbar_color, 0.1f);
            scrollbar_y_offset += input->mouse_y - input->prev_mouse_y;
        }
        draw_rect(target, scrollbar_rect, scrollbar_color);

        Rect mask_rect = {rect.x + border, rect.y + border, rect.width - (border * 2), actual_height};
        glEnable(GL_SCISSOR_TEST);
        glScissor(mask_rect.x, target.height - (mask_rect.y + mask_rect.height), mask_rect.width, mask_rect.height);

        for (int i = 0; i < games.len; i++)
        {
            float y = -(total_item_height * scrollbar_percentage) + rect.y + border + i * (gap + item_height);
            Rect item_rect = {rect.x + border, y, item_width, item_height};
            Color color = {31 / 255.f, 121 / 255.f, 197 / 255.f, 1};
            do_selectable(&ui, input, (Uiid)(i + 1), item_rect, mask_rect);
            if (ui.active == (Uiid)(i + 1))
            {
                color = darken(color, .1f);
            }
            draw_rect(target, item_rect, color);
            draw_text(*font, target, games[i].name, item_rect.x + border, y, 1, 1);
        }
        glDisable(GL_SCISSOR_TEST);
    };

    void enter_page() override
    {
        refresh_list_data();
    }

    PageId update_and_draw(RenderTarget target, InputState *input, Font *font, float time_step) override
    {
        last_refresh += time_step;
        if (last_refresh > 2.f)
        {
            refresh_list_data();
        }

        glDisable(GL_DEPTH_TEST);
        { // button
            float standard_border = get_standard_border(target);
            float max_width = 0.2f * 1920;
            float want_width = 0.2f * target.width;
            float width = fminf(max_width, want_width);
            float height = width * 9.f / 16.f / 2.f;

            float center_x = target.width - standard_border - (width / 2.f);
            float center_y = target.height - standard_border - (height / 2.f);
            float left = center_x - (width / 2.f);
            float top = center_y - (height / 2.f);

            back_button.color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
            back_button.rect = {left, top, width, height};
            back_button.str = String::from("Back");
            bool back = do_button2(&back_button, &ui, target, input, *font);
            if (back)
            {
                return PageId::TITLE;
            }
        }

        {
            float standard_border = get_standard_border(target);
            float title_width = (target.width / 2.f) - (standard_border * 2);
            float title_height = standard_border * 2;
            Rect title_rect = {standard_border, standard_border, title_width, title_height};
            draw_rect(target, title_rect, {0, 0, 0, 0.4});
            draw_centered_text(*font, target, String::from("Select Game"), title_rect, .1f, 10, 1);

            float gap = (standard_border / 2.f);
            float internal_border = gap;
            float width = title_width;
            float height = target.height - (2.f * standard_border) - title_height - gap;

            Rect container_rect = {standard_border, title_rect.y + title_height + gap, width, height};
            draw_list(target, input, font, container_rect);
        }
        glEnable(GL_DEPTH_TEST);

        return PageId::JOIN_GAME;
    }
};

struct SettingsPage : public Page
{
    Button back_button;

    PageId update_and_draw(RenderTarget target, InputState *input, Font *font, float time_step) override
    {
        float border = target.width / 50.f;

        glDisable(GL_DEPTH_TEST);
        { // button
            float max_width = 0.2f * 1920;
            float want_width = 0.2f * target.width;
            float width = fminf(max_width, want_width);
            float height = width * 9.f / 16.f / 2.f;

            float center_x = target.width - border - (width / 2.f);
            float center_y = target.height - border - (height / 2.f);
            float left = center_x - (width / 2.f);
            float top = center_y - (height / 2.f);

            back_button.color = {255 / 255.f, 125 / 255.f, 19 / 255.f, .7};
            back_button.rect = {left, top, width, height};
            back_button.str = String::from("Back");
            bool back = do_button2(&back_button, &ui, target, input, *font);
            if (back)
            {
                return PageId::TITLE;
            }
        }

        {
            float title_width = (target.width / 2.f) - (border * 2);
            float title_height = border * 2;
            Rect title_rect = {border, border, title_width, title_height};
            draw_rect(target, title_rect, {0, 0, 0, 0.4});
            draw_centered_text(*font, target, String::from("Settings"), title_rect, .1f, 10, 1);

            float gap = (border / 2.f);
            float width = title_width;
            float height = target.height - (2.f * border) - title_height - gap;
            Rect container_rect = {border, title_rect.y + title_height + gap, width, height};
            draw_rect(target, container_rect, {0, 0, 0, 0.4});

            float internal_border = gap / 2.f;
            static TextBox<64> server_text_box;
            server_text_box.rect = {container_rect.x + internal_border, container_rect.y + internal_border, container_rect.width - (internal_border * 2), border * 2};
            server_text_box.color = {31 / 255.f, 121 / 255.f, 197 / 255.f, 1};
            do_text_box(&server_text_box, &ui, target, input, *font);
        }
        glEnable(GL_DEPTH_TEST);

        return PageId::SETTINGS;
    }
};

namespace MainMenu
{
    TitlePage title_page;
    CreateGamePage create_game_page;
    JoinGamePage join_game_page;
    SettingsPage settings_page;
    Page *pages[4];

    PageId current_page;

    Font font;

    void init(Assets *assets)
    {
        pages[0] = &title_page;
        pages[1] = &create_game_page;
        pages[2] = &join_game_page;
        pages[3] = &settings_page;

        current_page = PageId::TITLE;

        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 64);
    }

    // false if user quit
    bool update_and_draw(const float time_step, RenderTarget target, InputState *input)
    {
        { // background
            static float t = 0;
            t += time_step;
            bind_shader(blurred_colors_shader);
            bind_1f(blurred_colors_shader, UniformId::T, t);
            bind_2i(blurred_colors_shader, UniformId::RESOLUTION, target.width, target.height);
            bind_2f(blurred_colors_shader, UniformId::POS, 0, 0);
            bind_2f(blurred_colors_shader, UniformId::SCALE, target.width, target.height);
            draw_rect();
        }

        PageId next_page = pages[(int)current_page]->update_and_draw(target, input, &font, time_step);
        if (next_page != current_page)
        {
            current_page = next_page;
            pages[(int)current_page]->enter_page();
        }

        return current_page != PageId::EXIT;
    }
};
