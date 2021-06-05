#pragma once

#include <vector>

#include "graphics.hpp"
#include "net/generated_rpc_client.hpp"

bool in_rect(Vec2f point, Rect rect, Rect mask = {})
{
    if (mask.width == 0 || mask.height == 0)
    {
        mask = rect;
    }

    return point.x > rect.x &&
           point.x < rect.x + rect.width &&
           point.y > rect.y &&
           point.y < rect.y + rect.height &&
           point.x > mask.x &&
           point.x < mask.x + mask.width &&
           point.y > mask.y &&
           point.y < mask.y + mask.height;
}

struct Selectable
{
    bool hot = false;
    bool focus_started = false;
    bool selected = false;
    bool just_selected = false;

    Selectable() {}

    static Selectable check(InputState *input, Rect rect, Selectable prev = {}, bool exclusive = true, bool on_up = true, Rect mask = {})
    {
        Selectable ret = prev;
        ret.just_selected = false;
        ret.hot = in_rect({(float)input->mouse_x, (float)input->mouse_y}, rect, mask);
        for (int i = 0; i < input->mouse_input.len; i++)
        {
            if (input->mouse_input[i].down)
            {
                if (ret.hot)
                {
                    ret.focus_started = true;
                    if (!on_up)
                    {
                        ret.selected = true;
                        ret.just_selected = true;
                    }
                }
                else if (exclusive)
                {
                    ret.selected = false;
                }
            }
            else
            {
                if (ret.hot)
                {
                    if (ret.focus_started)
                    {
                        ret.selected = true;
                        ret.just_selected = true;
                    }
                }

                ret.focus_started = false;
            }
        }

        return ret;
    }
};

Rect anchor_bottom_right(Vec2f dimensions, Vec2f distance)
{
    return {1920 - dimensions.x - distance.x,
            1080 - dimensions.y - distance.y,
            dimensions.x,
            dimensions.y};
}

Rect translate(Rect rect, Vec2f d)
{
    return {rect.x + d.x,
            rect.y + d.y,
            rect.width,
            rect.height};
}

Rect add_border(Rect rect, Vec2f border)
{
    return {rect.x + border.x,
            rect.y + border.y,
            rect.width - (border.x * 2.f),
            rect.height - (border.y * 2.f)};
}

Rect scale(Rect rect, Vec2f f)
{
    return {rect.x + ((1 - f.x) / 2) * rect.width,
            rect.y + ((1 - f.y) / 2) * rect.height,
            rect.width * f.x,
            rect.height * f.y};
}

struct Button2
{
    Rect rect;
    String text;

    Selectable selectable;

    float current_scale = 1.f;
    float target_scale = 1.f;

    const float NORMAL_TARGET_SCALE = 1.f;
    const float CLICKED_TARGET_SCALE = .95f;

    bool update_and_draw(RenderTarget target, InputState *input, Font *font)
    {
        if (selectable.focus_started)
            target_scale = CLICKED_TARGET_SCALE;
        else
            target_scale = NORMAL_TARGET_SCALE;
        current_scale = (1.25 * current_scale + .75 * target_scale) / 2.f;
        Rect actual_rect = scale(rect, {current_scale, current_scale});

        selectable = Selectable::check(input, actual_rect, selectable);

        Color color = {255 / 255.f, 125 / 255.f, 19 / 255.f, 1};
        if (selectable.hot || selectable.focus_started)
        {
            color = darken(color, 0.1f);
        }
        draw_rect(target, actual_rect, color);

        float text_border_x = actual_rect.width / 20;
        float text_border_y = actual_rect.height / 10;
        float text_height = actual_rect.height * .45f;
        float text_scale = text_height / -max_ascent(*font, text);
        draw_text_cropped(*font, target, text, actual_rect.x + text_border_x, (actual_rect.y + actual_rect.height) - text_border_y - text_height, text_scale, text_scale);

        return selectable.just_selected;
    }
};

template <size_t N>
struct TextBox2
{
    Rect rect;
    AllocatedString<N> text;

    Selectable selectable;

    void update_and_draw(RenderTarget target, InputState *input, Font *font)
    {
        Color color = {255 / 255.f, 125 / 255.f, 19 / 255.f, 1};

        selectable = Selectable::check(input, rect, selectable);
        if (selectable.selected)
        {
            color = darken(color, .1f);

            for (int i = 0; i < input->text_input.len; i++)
            {
                text.append(input->text_input[i]);
            }

            for (int i = 0; i < input->key_input.len; i++)
            {
                if (input->key_input[i] == Keys::BACKSPACE && text.len > 0)
                {
                    text.len--;
                }
            }
        }

        float text_border = rect.height / 10.f;
        float text_height = rect.height - (2 * text_border);
        float text_width = rect.width - (2 * text_border);
        float text_scale = text_height / font->font_size_px;
        float text_x = rect.x + text_border;
        float text_y = rect.y + text_border;
        draw_rect(target, rect, color);
        draw_rect(target, {text_x, text_y + (font->baseline * text_scale) + text_border / 2.f, text_width, text_border / 2.f}, darken(color, .1f));
        draw_text(*font, target, text, text_x, text_y, text_scale, text_scale);
    }
};

struct CheckBox
{
    Rect rect;
    bool checked = false;

    Selectable selectable;

    bool update_and_draw(RenderTarget target, InputState *input, Font *font)
    {
        Color base_color = {255 / 255.f, 125 / 255.f, 19 / 255.f, 1};
        Color selected_color = darken(base_color, .25f);

        selectable = Selectable::check(input, rect, selectable);
        if (selectable.just_selected)
            checked = !checked;
        if (checked)
            draw_rect(target, scale(rect, {.75f, .75f}), selected_color);

        draw_rect(target, rect, base_color);

        return selectable.just_selected;
    }
};

struct List
{
    struct Item
    {
        uint32_t id;
        String name;
    };

    std::vector<Item> items;
    Rect rect;
    int max_items = -1; // -1 for no limit

    Selectable scrollbar_selectable;
    float scrollbar_y_offset = 0.f;
    int selected_i = -1;
    uint32_t selected_item_id = 0;

    const float ITEM_HEIGHT = 60.f;
    const float ITEM_BORDER = 10.f;

    void update_and_draw(RenderTarget target, InputState *input, Font *font)
    {
        float item_width = rect.width * .925f;
        float vertical_border = rect.width * 0.0255f;
        float scrollbar_width = rect.width * 0.05f;

        float total_item_height = items.size() * (ITEM_HEIGHT + ITEM_BORDER) - ITEM_BORDER;

        if (max_items <= 0)
        {
            float total_item_height = items.size() * (ITEM_HEIGHT + ITEM_BORDER) - ITEM_BORDER;
            float visible_item_height = rect.height;
            float percent_visible = fminf(visible_item_height / total_item_height, 1.f);
            float scrollbar_height = percent_visible * rect.height;
            float scrollbar_max_y_offset = rect.height - scrollbar_height;

            Rect scrollbar_rect = {rect.x + item_width + vertical_border,
                                   rect.y + scrollbar_y_offset,
                                   scrollbar_width,
                                   scrollbar_height};
            scrollbar_selectable = Selectable::check(input, scrollbar_rect, scrollbar_selectable);
            if (scrollbar_selectable.focus_started)
                scrollbar_y_offset += input->mouse_y - input->prev_mouse_y;
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

            Color scrollbar_color = {255 / 255.f, 125 / 255.f, 19 / 255.f, 1};
            if (scrollbar_selectable.focus_started)
            {
                scrollbar_color = darken(scrollbar_color, 0.1f);
            }
            draw_rect(target, scrollbar_rect, scrollbar_color);
        }
        else
        {
            item_width = rect.width;
        }

        glEnable(GL_SCISSOR_TEST);
        glScissor(rect.x, target.height - (rect.y + rect.height), rect.width, rect.height);
        float scrollbar_percentage = scrollbar_y_offset / rect.height;
        Rect top_item_rect = {rect.x, rect.y - (total_item_height * scrollbar_percentage), item_width, ITEM_HEIGHT};
        for (int i = 0; i < items.size(); i++)
        {
            Rect item_rect = top_item_rect;
            item_rect.y += i * (ITEM_HEIGHT + ITEM_BORDER);

            Selectable item_selectable = Selectable::check(input, item_rect, {}, false, false, rect);
            if (item_selectable.focus_started)
            {
                selected_i = i;
                selected_item_id = items[i].id;
            }

            Color item_color = {255 / 255.f, 125 / 255.f, 19 / 255.f, 1};
            if (selected_i == i)
            {
                item_color = darken(item_color, 0.1f);
            }
            draw_rect(target, item_rect, item_color);

            float text_border = item_rect.height / 10.f;
            float text_height = item_rect.height - (2 * text_border);
            float text_width = item_rect.width - (2 * text_border);
            float text_scale = text_height / font->font_size_px;
            float text_x = item_rect.x + text_border;
            float text_y = item_rect.y + text_border;
            draw_text(*font, target, items[i].name, text_x, text_y, text_scale, text_scale);
        }
        glDisable(GL_SCISSOR_TEST);
    };

    void add_item(Item item)
    {
        if (items.size() == max_items)
        {
            return;
        }

        items.push_back(item);
    }

    uint32_t get_selected_id()
    {
        return selected_item_id;
    }

    void refresh(std::vector<Item> new_items)
    {
        items = new_items;

        for (int i = 0; i < items.size(); i++)
        {
            if (items[i].id == selected_item_id)
            {
                selected_i = i;
                return;
            }
        }
        selected_i = -1;
        selected_item_id = 0;
    }
};

struct MainMenu;

struct MenuPage
{
    virtual void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu) = 0;
};

struct MainMenu
{
    MenuPage *current;

    MenuPage *main;
    MenuPage *join;
    MenuPage *create;
    MenuPage *settings;
    MenuPage *lobby;
};

struct MainPage : MenuPage
{
    Button2 create_button;
    Button2 join_button;
    Button2 settings_button;
    Button2 exit_button;

    Font font;
    Font title_font;

    MainPage(Assets *assets)
    {
        title_font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ANTON_REGULAR_TTF], 256);
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 64);

        exit_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        exit_button.text = String::from("Exit");
        settings_button.rect = translate(exit_button.rect, {0, -(exit_button.rect.height + 25.f)});
        settings_button.text = String::from("Settings");
        join_button.rect = translate(settings_button.rect, {0, -(settings_button.rect.height + 25.f)});
        join_button.text = String::from("Join Game");
        create_button.rect = translate(join_button.rect, {0, -(join_button.rect.height + 25.f)});
        create_button.text = String::from("Create Game");
    }

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu) override
    {
        {
            String title_text_1 = String::from("FAMILY");
            String title_text_2 = String::from("FEUD");

            float want_width = 0.6f * target.width;
            float max_width = 0.6f * 1920;
            float width = fminf(max_width, want_width);
            float scale = width / get_text_width(title_font, title_text_1);
            float line1_height = get_single_line_height(title_font, title_text_1, scale);

            draw_text_cropped(title_font, target, title_text_1, 25.f, 25.f, scale, scale);
            draw_text_cropped(title_font, target, title_text_2, 25.f, 25.f + 15 + line1_height, scale, scale);
        }

        if (exit_button.update_and_draw(target, input, &font))
        {
            // TODO need to exit in a cleaner way
            exit(0);
        }
        if (settings_button.update_and_draw(target, input, &font))
        {
            menu->current = menu->settings;
        }
        if (create_button.update_and_draw(target, input, &font))
        {
            menu->current = menu->create;
        }
        if (join_button.update_and_draw(target, input, &font))
        {
            menu->current = menu->join;
        }
    }
};

struct LobbyPage : MenuPage
{
    Button2 back_button;
    Button2 start_game_button;

    Rect family_1_title_background;
    Rect family_1_panel;
    List family_1_list;

    Rect family_2_title_background;
    Rect family_2_panel;
    List family_2_list;

    Font font;
    RpcClient *rpc_client;
    uint32_t game_id;

    LobbyPage(Assets *assets, RpcClient *rpc_client)
    {
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 128);
        this->rpc_client = rpc_client;

        back_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        back_button.text = String::from("Back");
        start_game_button.rect = translate(back_button.rect, {0, -(back_button.rect.height + 25.f)});
        start_game_button.text = String::from("Create");

        Rect left_panels = add_border({0, 0, 1920 / 3.f, 1080}, {25.f, 25.f});
        family_1_title_background = {left_panels.x, left_panels.y, left_panels.width, 100.f};
        family_1_panel = {family_1_title_background.x,
                          family_1_title_background.y + family_1_title_background.height + 25.f,
                          family_1_title_background.width,
                          1080 - (family_1_title_background.y + family_1_title_background.height + 25.f) - 25.f};
        family_1_list.rect = add_border(family_1_panel, {25.f, 25.f});
        family_1_list.max_items = MAX_PLAYERS_PER_GAME * 2;

        Rect right_panels = add_border({1920 / 3.f, 0, 1920 / 3.f, 1080}, {25.f, 25.f});
        family_2_title_background = {right_panels.x, right_panels.y, right_panels.width, 100.f};
        family_2_panel = {family_2_title_background.x,
                          family_2_title_background.y + family_2_title_background.height + 25.f,
                          family_2_title_background.width,
                          1080 - (family_2_title_background.y + family_2_title_background.height + 25.f) - 25.f};
        family_2_list.rect = add_border(family_2_panel, {25.f, 25.f});
        family_2_list.max_items = MAX_PLAYERS_PER_GAME * 2;
    }

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu) override
    {
        if (back_button.update_and_draw(target, input, &font))
        {
            rpc_client->LeaveGame({});
            menu->current = menu->main;
        }

        draw_rect(target, family_1_title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, String::from("Family 1"), family_1_title_background, .1f, 10, 1);
        draw_rect(target, family_1_panel, {0.f, 1.f, 0.f, .4f});

        draw_rect(target, family_2_title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, String::from("Family 2"), family_2_title_background, .1f, 10, 1);
        draw_rect(target, family_2_panel, {0.f, 1.f, 0.f, .4f});

        auto resp = rpc_client->GetGame({game_id});
        std::vector<List::Item> family_1_list_items;
        std::vector<List::Item> family_2_list_items;
        for (auto p : resp.players)
        {
            if (!p.team)
                family_1_list_items.push_back({p.user_id, p.name.len > 0 ? p.name : String::from("Joining...")});
            else
                family_2_list_items.push_back({p.user_id, p.name.len > 0 ? p.name : String::from("Joining...")});
        }
        family_1_list.refresh(family_1_list_items);
        family_1_list.update_and_draw(target, input, &font);
        family_2_list.refresh(family_2_list_items);
        family_2_list.update_and_draw(target, input, &font);
    }

    void set_game_id(uint32_t game_id)
    {
        this->game_id = game_id;
    }
};

struct JoinGamePage : MenuPage
{
    Button2 back_button;
    Button2 join_button;

    Rect title_background;

    Rect game_list_panel;
    List game_list;

    Rect game_detail_panel;

    Font font;
    RpcClient *rpc_client;

    JoinGamePage(Assets *assets, RpcClient *rpc_client)
    {
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 128);
        this->rpc_client = rpc_client;

        back_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        back_button.text = String::from("Back");
        join_button.rect = back_button.rect;
        join_button.rect.y -= back_button.rect.height + 25.f;
        join_button.text = String::from("Join");

        Rect left_panels = add_border({0, 0, 1920 / 2.f, 1080 / 2.f}, {25.f, 25.f});
        title_background = {left_panels.x, left_panels.y, left_panels.width, 100.f};

        game_list_panel = {title_background.x,
                           title_background.y + title_background.height + 25.f,
                           title_background.width,
                           1080 - (title_background.y + title_background.height + 25.f) - 25.f};
        game_list.rect = add_border(game_list_panel, {25.f, 25.f});

        game_detail_panel = add_border({1920 / 2.f, 0.f, 1920 / 2.f, 1080 / 2.f}, {25.f, 25.f});
    }

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu) override
    {
        if (back_button.update_and_draw(target, input, &font))
        {
            menu->current = menu->main;
        }

        draw_rect(target, title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, String::from("Select Game"), title_background, .1f, 10, 1);

        draw_rect(target, game_list_panel, {0.f, 1.f, 0.f, .4f});
        {
            ListGamesRequest req;
            ListGamesResponse resp;
            rpc_client->ListGames(req, &resp);
            std::vector<List::Item> game_list_items;
            for (auto g : resp.games)
            {
                game_list_items.push_back({g.id, g.name});
            }
            game_list.refresh(game_list_items);
        }
        game_list.update_and_draw(target, input, &font);

        if (game_list.get_selected_id() > 0)
        {
            if (join_button.update_and_draw(target, input, &font))
            {
                rpc_client->JoinGame({game_list.get_selected_id()});
                ((LobbyPage *)menu->lobby)->set_game_id(game_list.get_selected_id());
                menu->current = menu->lobby;
            }
            draw_rect(target, game_detail_panel, {0.f, 1.f, 0.f, .4f});
        }
    }
};

struct CreateGamePage : MenuPage
{
    Button2 back_button;
    Button2 create_button;

    Rect title_background;

    Rect game_settings_panel;
    Rect game_name_label;
    TextBox2<64> game_name_text_box;

    Font font;
    RpcClient *rpc_client;

    CreateGamePage(Assets *assets, RpcClient *rpc_client)
    {
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 128);
        this->rpc_client = rpc_client;

        back_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        back_button.text = String::from("Back");
        create_button.rect = translate(back_button.rect, {0, -(back_button.rect.height + 25.f)});
        create_button.text = String::from("Create");

        Rect left_panels = add_border({0, 0, 1920 / 2.f, 1080 / 2.f}, {25.f, 25.f});
        title_background = {left_panels.x, left_panels.y, left_panels.width, 100.f};

        game_settings_panel = {title_background.x,
                               title_background.y + title_background.height + 25.f,
                               title_background.width,
                               1080 - (title_background.y + title_background.height + 25.f) - 25.f};

        Rect game_settings_content = add_border(game_settings_panel, {25.f, 25.f});
        game_name_text_box.rect = {game_settings_content.x,
                                   game_settings_content.y,
                                   game_settings_content.width,
                                   75.f}
    }

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu) override
    {
        if (back_button.update_and_draw(target, input, &font))
        {
            menu->current = menu->main;
        }
        if (create_button.update_and_draw(target, input, &font))
        {
            CreateGameResponse resp = rpc_client->CreateGame({game_name_text_box.text});
            ((LobbyPage *)menu->lobby)->set_game_id(resp.game_id);
            menu->current = menu->lobby;
        }

        draw_rect(target, title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, String::from("New Game"), title_background, .1f, 10, 1);

        draw_rect(target, game_settings_panel, {0.f, 1.f, 0.f, .4f});
        game_name_text_box.update_and_draw(target, input, &font);
    }
};

struct SettingsPage : MenuPage
{
    Button2 back_button;
    Button2 create_button;

    Rect title_background;

    Rect game_settings_panel;
    TextBox2<64> server_ip_address_text_box;
    Button2 connect_to_server_button;

    Font font;
    RpcClient *rpc_client;

    SettingsPage(Assets *assets, RpcClient *rpc_client)
    {
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 128);
        this->rpc_client = rpc_client;

        back_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        back_button.text = String::from("Back");
        create_button.rect = translate(back_button.rect, {0, -(back_button.rect.height + 25.f)});
        create_button.text = String::from("Create");

        Rect left_panels = add_border({0, 0, 1920 / 2.f, 1080 / 2.f}, {25.f, 25.f});
        title_background = {left_panels.x, left_panels.y, left_panels.width, 100.f};

        game_settings_panel = {title_background.x,
                               title_background.y + title_background.height + 25.f,
                               title_background.width,
                               1080 - (title_background.y + title_background.height + 25.f) - 25.f};

        Rect game_settings_content = add_border(game_settings_panel, {25.f, 25.f});
        server_ip_address_text_box.rect = {game_settings_content.x, game_settings_content.y, game_settings_content.width, 75.f};
        connect_to_server_button.rect = translate(server_ip_address_text_box.rect, {0, server_ip_address_text_box.rect.height + 5.f});
        connect_to_server_button.rect.width = connect_to_server_button.rect.height;
    }

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu) override
    {
        if (back_button.update_and_draw(target, input, &font))
        {
            menu->current = menu->main;
        }

        draw_rect(target, title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, String::from("New Game"), title_background, .1f, 10, 1);

        draw_rect(target, game_settings_panel, {0.f, 1.f, 0.f, .4f});
        server_ip_address_text_box.update_and_draw(target, input, &font);
        if (connect_to_server_button.update_and_draw(target, input, &font))
        {
            rpc_client->peer.close();
            char ip[65];
            memcpy(ip, server_ip_address_text_box.text.data, server_ip_address_text_box.text.len);
            ip[server_ip_address_text_box.text.len] = '\0';
            rpc_client->peer.open(ip, 6666, true);
        }
    }
};