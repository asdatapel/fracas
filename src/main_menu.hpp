#pragma once

#include <vector>

#include "graphics/graphics.hpp"
#include "net/generated_rpc_client.hpp"

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

        if (input->mouse_down_event)
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
        if (input->mouse_up_event)
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

Rect anchor_top_right(Vec2f dimensions, Vec2f distance)
{
    return {1920 - dimensions.x - distance.x,
            distance.y,
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
        float max_text_width = rect.width - (2 * text_border);
        float text_scale = text_height / font->font_size_px;
        float text_x = rect.x + text_border;
        float text_y = rect.y + text_border;
        draw_rect(target, rect, color);
        draw_rect(target, {text_x, text_y + (font->baseline * text_scale) + text_border / 2.f, max_text_width, text_border / 2.f}, darken(color, .1f));
        draw_text(*font, target, text, text_x, text_y, text_scale, text_scale);
    }
};

struct Label
{
    Rect rect;
    String text;
    Color background_color = {0, 0, 0, 0};

    void update_and_draw(RenderTarget target, InputState *input, Font *font)
    {
        float text_border = rect.height / 10.f;
        float text_height = rect.height - (2 * text_border);
        float text_width = rect.width - (2 * text_border);
        float text_scale = text_height / font->font_size_px;
        float text_x = rect.x + text_border;
        float text_y = rect.y + text_border;
        draw_rect(target, {rect.x, rect.y, text_width, rect.height}, background_color);
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

struct SelectionGroup
{
    void *selected = nullptr;
};

struct List : SelectionGroup
{
    struct Item
    {
        int32_t id;
        PlayerName name;
    };

    std::vector<Item> items;
    Rect rect;
    int max_items = -1; // -1 for no limit

    Selectable scrollbar_selectable;
    float scrollbar_y_offset = 0.f;
    SelectionGroup *selection_group = this;
    int selected_i = -1;
    int32_t selected_item_id = 0;

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
                selection_group->selected = this;
                selected_i = i;
                selected_item_id = items[i].id;
            }

            Color item_color = {255 / 255.f, 125 / 255.f, 19 / 255.f, 1};
            if (selection_group->selected == this && selected_i == i)
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

    int32_t get_selected_id()
    {
        if (selection_group->selected == this)
        {
            return selected_item_id;
        }
        return 0;
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
    virtual void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu, ClientGameData *game_data) = 0;
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

    MainPage(Assets *assets, Memory mem)
    {
        title_font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ANTON_REGULAR_TTF], 256, mem.temp);
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 64, mem.temp);

        exit_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        exit_button.text = "Exit";
        settings_button.rect = translate(exit_button.rect, {0, -(exit_button.rect.height + 25.f)});
        settings_button.text = "Settings";
        join_button.rect = translate(settings_button.rect, {0, -(settings_button.rect.height + 25.f)});
        join_button.text = "Join Game";
        create_button.rect = translate(join_button.rect, {0, -(join_button.rect.height + 25.f)});
        create_button.text = "Create Game";
    }

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu, ClientGameData *game_data) override
    {
        {
            String title_text_1 = "FAMILY";
            String title_text_2 = "FEUD";

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
            // exit(0);
            menu->current = nullptr;
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

    Button2 swap_button;

    SelectionGroup player_selection_group;

    Rect family_1_title_background;
    Rect family_1_panel;
    List family_1_list;

    Rect family_2_title_background;
    Rect family_2_panel;
    List family_2_list;

    Font font;
    RpcClient *rpc_client;
    GameId game_id = 0;
    ClientId game_owner_id = 0;

    LobbyPage(Assets *assets, RpcClient *rpc_client, Memory mem)
    {
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 128, mem.temp);
        this->rpc_client = rpc_client;

        back_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        back_button.text = "Back";
        start_game_button.rect = translate(back_button.rect, {0, -(back_button.rect.height + 25.f)});
        start_game_button.text = "Start";

        swap_button.rect = anchor_top_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        swap_button.text = "Swap Team";

        Rect left_panels = add_border({0, 0, 1920 / 3.f, 1080}, {25.f, 25.f});
        family_1_title_background = {left_panels.x, left_panels.y, left_panels.width, 100.f};
        family_1_panel = {family_1_title_background.x,
                          family_1_title_background.y + family_1_title_background.height + 25.f,
                          family_1_title_background.width,
                          1080 - (family_1_title_background.y + family_1_title_background.height + 25.f) - 25.f};
        family_1_list.selection_group = &player_selection_group;
        family_1_list.rect = add_border(family_1_panel, {25.f, 25.f});
        family_1_list.max_items = MAX_PLAYERS_PER_GAME * 2;

        Rect right_panels = add_border({1920 / 3.f, 0, 1920 / 3.f, 1080}, {25.f, 25.f});
        family_2_title_background = {right_panels.x, right_panels.y, right_panels.width, 100.f};
        family_2_panel = {family_2_title_background.x,
                          family_2_title_background.y + family_2_title_background.height + 25.f,
                          family_2_title_background.width,
                          1080 - (family_2_title_background.y + family_2_title_background.height + 25.f) - 25.f};
        family_2_list.selection_group = &player_selection_group;
        family_2_list.rect = add_border(family_2_panel, {25.f, 25.f});
        family_2_list.max_items = MAX_PLAYERS_PER_GAME * 2;
    }

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu, ClientGameData *game_data) override
    {
        if (back_button.update_and_draw(target, input, &font))
        {
            rpc_client->LeaveGame({});
            menu->current = menu->main;
        }

        draw_rect(target, family_1_title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, "Family 1", family_1_title_background, .1f, 10, 1);
        draw_rect(target, family_1_panel, {0.f, 1.f, 0.f, .4f});

        draw_rect(target, family_2_title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, "Family 2", family_2_title_background, .1f, 10, 1);
        draw_rect(target, family_2_panel, {0.f, 1.f, 0.f, .4f});

        rpc_client->GetGame({game_id});
        if (auto get_game_resp = rpc_client->get_GetGame_msg())
        {
            std::vector<List::Item> family_1_list_items;
            std::vector<List::Item> family_2_list_items;
            for (auto p : get_game_resp->players)
            {
                if (!p.team)
                    family_1_list_items.push_back({p.user_id, p.name});
                else
                    family_2_list_items.push_back({p.user_id, p.name});
            }
            family_1_list.refresh(family_1_list_items);
            family_2_list.refresh(family_2_list_items);
        }

        family_1_list.update_and_draw(target, input, &font);
        family_2_list.update_and_draw(target, input, &font);

        if (game_owner_id > 0)
        {
            if (player_selection_group.selected)
            {
                if (swap_button.update_and_draw(target, input, &font))
                {
                    rpc_client->SwapTeam({game_id, ((List *)player_selection_group.selected)->get_selected_id()});
                }
            }
            if (start_game_button.update_and_draw(target, input, &font))
            {
                rpc_client->StartGame({game_id});
            }
        }

        if (auto msg = rpc_client->get_GameStarted_msg())
        {
            game_data->my_id = msg->your_id;
            for (int i = 0; i < family_1_list.items.size(); i++)
            {
                game_data->game_state.players.append({family_1_list.items[i].id, family_1_list.items[i].name, 0});
            }
            for (int i = 0; i < family_2_list.items.size(); i++)
            {
                game_data->game_state.players.append({family_2_list.items[i].id, family_2_list.items[i].name, 1});
            }

            menu->current = nullptr;
        }
    }

    void set_game(uint32_t game_id, uint32_t game_owner_id)
    {
        this->game_id = game_id;
        this->game_owner_id = game_owner_id;
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

    Rect user_input_panel;
    Label username_label;
    TextBox2<PlayerName::MAX_LEN> username_textbox;

    Font font;
    RpcClient *rpc_client;

    JoinGamePage(Assets *assets, RpcClient *rpc_client, Memory mem)
    {
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 128, mem.temp);
        this->rpc_client = rpc_client;

        back_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        back_button.text = "Back";
        join_button.rect = back_button.rect;
        join_button.rect.y -= back_button.rect.height + 25.f;
        join_button.text = "Join";

        Rect left_panels = add_border({0, 0, 1920 / 2.f, 1080 / 2.f}, {25.f, 25.f});
        title_background = {left_panels.x, left_panels.y, left_panels.width, 100.f};

        game_list_panel = {title_background.x,
                           title_background.y + title_background.height + 25.f,
                           title_background.width,
                           1080 - (title_background.y + title_background.height + 25.f) - 25.f};
        game_list.rect = add_border(game_list_panel, {25.f, 25.f});

        game_detail_panel = add_border({1920 / 2.f, 0.f, 1920 / 2.f, 1080 / 2.f}, {25.f, 25.f});

        user_input_panel = translate(game_detail_panel, {0, game_detail_panel.height + 10.f});
        user_input_panel.height = 170.f;
        username_label.rect = add_border(user_input_panel, {25.f, 25.f});
        username_label.rect.height = 40.f;
        username_label.text = "Your Name:";
        username_textbox.rect = translate(username_label.rect, {0, username_label.rect.height + 10.f});
        username_textbox.rect.height = 70.f;
    }

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu, ClientGameData *game_data) override
    {
        if (back_button.update_and_draw(target, input, &font))
        {
            menu->current = menu->main;
        }

        draw_rect(target, title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, "Select Game", title_background, .1f, 10, 1);

        draw_rect(target, game_list_panel, {0.f, 1.f, 0.f, .4f});
        {
            ListGamesRequest req;
            rpc_client->ListGames(req);

            if (auto resp = rpc_client->get_ListGames_msg())
            {
                std::vector<List::Item> game_list_items;
                for (auto g : resp->games)
                {
                    game_list_items.push_back({g.id, g.name});
                }
                game_list.refresh(game_list_items);
            }
        }
        game_list.update_and_draw(target, input, &font);

        if (game_list.get_selected_id() > 0)
        {
            draw_rect(target, game_detail_panel, {0.f, 1.f, 0.f, .4f});
            draw_rect(target, user_input_panel, {0.f, 1.f, 0.f, .4f});
            username_label.update_and_draw(target, input, &font);
            username_textbox.update_and_draw(target, input, &font);

            if (join_button.update_and_draw(target, input, &font))
            {
                rpc_client->JoinGame({game_list.get_selected_id(), username_textbox.text});
                ((LobbyPage *)menu->lobby)->set_game(game_list.get_selected_id(), 0);
                menu->current = menu->lobby;
            }
        }
    }
};

struct CreateGamePage : MenuPage
{
    Button2 back_button;
    Button2 create_button;

    Rect title_background;

    Rect game_settings_panel;
    Label game_name_label;
    TextBox2<32> game_name_text_box;
    Label user_name_label;
    TextBox2<32> user_name_text_box;

    Font font;
    RpcClient *rpc_client;

    CreateGamePage(Assets *assets, RpcClient *rpc_client, Memory mem)
    {
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 128, mem.temp);
        this->rpc_client = rpc_client;

        back_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        back_button.text = "Back";
        create_button.rect = translate(back_button.rect, {0, -(back_button.rect.height + 25.f)});
        create_button.text = "Create";

        Rect left_panels = add_border({0, 0, 1920 / 2.f, 1080 / 2.f}, {25.f, 25.f});
        title_background = {left_panels.x, left_panels.y, left_panels.width, 100.f};

        game_settings_panel = {title_background.x,
                               title_background.y + title_background.height + 25.f,
                               title_background.width,
                               1080 - (title_background.y + title_background.height + 25.f) - 25.f};

        Rect game_settings_content = add_border(game_settings_panel, {25.f, 25.f});
        // auto place_next_option = [](Rect container, Rect *label, Rect *input)->float {};
        game_name_label = {{game_settings_content.x,
                            game_settings_content.y,
                            game_settings_content.width,
                            40.f},
                           "Game Name:"};
        game_name_text_box.rect = translate(game_name_label.rect, {0, game_name_label.rect.height + 10.f});
        game_name_text_box.rect.height = 70.f;

        user_name_label.rect = translate(game_name_text_box.rect, {0, game_name_text_box.rect.height + 10.f});
        user_name_label.rect.height = 40.f;
        user_name_label.text = "Your Name:";
        user_name_text_box.rect = translate(user_name_label.rect, {0, user_name_label.rect.height + 10.f});
        user_name_text_box.rect.height = 70.f;
    }

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu, ClientGameData *game_data) override
    {
        if (back_button.update_and_draw(target, input, &font))
        {
            menu->current = menu->main;
        }
        if (create_button.update_and_draw(target, input, &font))
        {
            rpc_client->CreateGame({game_name_text_box.text, user_name_text_box.text, true});
        }
        if (CreateGameResponse *resp = rpc_client->get_CreateGame_msg())
        {
            if (resp->game_id)
            {
                ((LobbyPage *)menu->lobby)->set_game(resp->game_id, resp->owner_id);
                menu->current = menu->lobby;
            }
        }

        draw_rect(target, title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, "New Game", title_background, .1f, 10, 1);

        draw_rect(target, game_settings_panel, {0.f, 1.f, 0.f, .4f});
        game_name_label.update_and_draw(target, input, &font);
        game_name_text_box.update_and_draw(target, input, &font);
        user_name_label.update_and_draw(target, input, &font);
        user_name_text_box.update_and_draw(target, input, &font);
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

    SettingsPage(Assets *assets, RpcClient *rpc_client, Memory mem)
    {
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 128, mem.temp);
        this->rpc_client = rpc_client;

        back_button.rect = anchor_bottom_right(
            {0.2f * 1920, 0.2f * 1920 * 9.f / 16.f / 2.f},
            {25.f, 25.f});
        back_button.text = "Back";
        create_button.rect = translate(back_button.rect, {0, -(back_button.rect.height + 25.f)});
        create_button.text = "Create";

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

    void update_and_draw(RenderTarget target, InputState *input, MainMenu *menu, ClientGameData *game_data) override
    {
        if (back_button.update_and_draw(target, input, &font))
        {
            menu->current = menu->main;
        }

        draw_rect(target, title_background, {1.f, 0.f, 1.f, .4f});
        draw_centered_text(font, target, "New Game", title_background, .1f, 10, 1);

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
