// delete all this, just harcode values based on 1920/1080 and scale after that
#pragma once

#include <vector>

#include "../graphics.hpp"

enum struct Justification
{
    NORTH = 1,
    SOUTH = 2,
    EAST = 4,
    WEST = 8,
    NORTHEAST = 5,
    NORTHWEST = 9,
    SOUTHEAST = 6,
    SOUTHWEST = 10,
};
int operator&(Justification l, Justification r)
{
    return (int)l & (int)r;
}

struct UiElement
{
    float x_pt = 0, y_pt = 0;
    bool y_relative_to_x = false;
    float width_pt = 0, height_pt = 0;
    bool height_relative_to_width = false;
    float max_width = 0;
    float max_height = 0;
    float border_pt = 0;

    UiElement *parent = nullptr;
    bool outside = false;

    Justification justification;

    float last_width = 0, last_height = 0;
    Rect rect;

    void update(RenderTarget target)
    {
        if (target.width == last_width && target.height == last_height)
            return;
        last_width = target.width;
        last_height = target.height;

        Rect parent_rect = {0, 0, (float)target.width, (float)target.height};
        Rect container_rect = {0, 0, (float)target.width, (float)target.height};
        if (parent)
        {
            parent->update(target);
            parent_rect = parent->rect;
        }
        if (!outside)
        {
            container_rect = parent_rect;
        }

        rect.width = width_pt * container_rect.width;
        if (max_width > 0 && rect.width > max_width)
        {
            rect.width = max_width;
        }

        rect.height = height_pt * container_rect.height;
        if (height_relative_to_width)
        {
            rect.height = height_pt * rect.width;
        }
        if (max_height > 0 && rect.height > max_height)
        {
            rect.height = max_height;
        }

        int x_dir = 1, y_dir = 1;
        int x_body = 1, y_body = 1;
        float x_anchor = parent_rect.x, y_anchor = parent_rect.y;
        if (justification & Justification::WEST)
        {
            x_dir = 1;
            x_body = 0;
            x_anchor = parent_rect.x;
        }
        else if (justification & Justification::EAST)
        {
            x_dir = -1;
            x_body = 1;
            x_anchor = parent_rect.x + parent_rect.width;
        }
        if (justification & Justification::NORTH)
        {
            y_dir = 1;
            y_body = 0;
            y_anchor = parent_rect.y;
        }
        else if (justification & Justification::SOUTH)
        {
            y_dir = -1;
            y_body = 1;
            y_anchor = parent_rect.y + parent_rect.height;
        }
        if (outside)
        {
            x_dir = -x_dir;
            x_body = 1 - x_body;
            y_dir = -y_dir;
            y_body = 1 - y_body;
        }

        float x_dist = (x_body * rect.width) + (x_pt * container_rect.width);
        float y_dist = (y_body * rect.height) + (y_pt * container_rect.height);
        if (y_relative_to_x)
        {
            y_dist = (y_body * rect.height) + (y_pt * (x_pt * container_rect.width));
        }
        rect.x = x_anchor + (x_dir * x_dist);
        rect.y = y_anchor + (y_dir * y_dist);

        float border = border_pt * container_rect.width;
        rect.x += border;
        rect.y += border;
        rect.width -= border * 2;
        rect.height -= border * 2;
    };
};

struct Clickable : UiElement
{
    bool hot = false;

    bool focus_started = false;
    float focus_started_x, focus_started_y;

    bool update_and_draw(RenderTarget target, InputState *input)
    {
        update(target);

        bool clicked = false;

        hot = input->mouse_x > rect.x &&
              input->mouse_x < rect.x + rect.width &&
              input->mouse_y > rect.y &&
              input->mouse_y < rect.y + rect.height;
        for (int i = 0; i < input->mouse_input.len; i++)
        {
            if (input->mouse_input[i].down)
            {
                if (hot)
                {
                    focus_started = true;
                    focus_started_x = input->mouse_x - rect.x;
                    focus_started_y = input->mouse_y - rect.y;
                }
            }
            else
            {
                if (hot)
                {
                    if (focus_started)
                    {
                        clicked = true;
                    }
                }

                focus_started = false;
            }
        }

        Color color = {255 / 255.f, 125 / 255.f, 19 / 255.f, 1};
        if (focus_started)
        {
            color = darken(color, 0.1f);
        }
        draw_rect(target, rect, color);

        return clicked;
    }
};

struct GameList : UiElement
{
    std::vector<std::string> items;

    float scrollbar_position_pt = 0.f;

    Clickable scrollbar_beg;
    Clickable scrollbar_end;
    UiElement list_item_container;
    UiElement top_list_item;

    GameList()
    {
        items.push_back("hello");
        items.push_back("hellasas");
        items.push_back("qweqwe");
        items.push_back("mbokijfvchnbgf");

        scrollbar_beg.parent = this;
        scrollbar_beg.width_pt = .075f;
        scrollbar_beg.height_pt = .15f;
        scrollbar_beg.border_pt = .025f / 2;
        scrollbar_beg.justification = Justification::NORTHEAST;
        scrollbar_beg.x_pt = 0.f;
        scrollbar_beg.y_pt = 0.f;

        scrollbar_end.parent = this;
        scrollbar_end.width_pt = .075f;
        scrollbar_end.height_pt = .15f;
        scrollbar_end.border_pt = .025f / 2;
        scrollbar_end.justification = Justification::SOUTHEAST;
        scrollbar_end.x_pt = 0.f;
        scrollbar_end.y_pt = 0.f;

        list_item_container.parent = this;
        list_item_container.width_pt = 0.925f;
        list_item_container.height_pt = 1.f;
        list_item_container.border_pt = .025f / 2;
        list_item_container.justification = Justification::NORTHWEST;
        list_item_container.x_pt = 0.f;
        list_item_container.y_pt = 0.f;

        top_list_item.parent = &list_item_container;
        top_list_item.width_pt = 1.f;
        top_list_item.height_pt = .075f;
        top_list_item.justification = Justification::NORTHWEST;
        top_list_item.x_pt = 0.f;
        top_list_item.y_pt = 0.f;
    }

    void update_and_draw(RenderTarget target, InputState *input)
    {
        list_item_container.update(target);
        top_list_item.update(target);

        scrollbar_beg.update_and_draw(target, input);
        if (scrollbar_beg.focus_started)
        {
            scrollbar_beg.rect.y += input->mouse_y - input->prev_mouse_y;
        }
        scrollbar_end.update_and_draw(target, input);

        // draw_rect(target, list_item_container.to_rect(target), {1.f, 1.f, 1.f, .4f});
        Rect top_rect = top_list_item.rect;
        for (int i = 0; i < items.size(); i++)
        {
            Rect rect = top_rect;
            rect.y += i * (rect.height + (rect.height / 4.f));
            draw_rect(target, rect, {0.f, 0.f, 0.f, .4f});
        }
    }
};

struct JoinGamePage2
{
    // Peer server;

    // UiContainer list_container;
    // UiLabel list_title_label;
    // UiList game_list;

    // UiContainer list_container;
    // UiLabel selected_game_label;

    // UiButton join_button;
    // UiButton back_button;

    UiElement back_button;
    UiElement join_button;

    UiElement left_container;
    UiElement list_title_label;
    UiElement game_list_container;

    GameList list;

    JoinGamePage2()
    {
        back_button.width_pt = 0.2f;
        back_button.max_width = 0.2f * 1920;
        back_button.height_relative_to_width = true;
        back_button.height_pt = (9.f / 16.f) / 2.f;
        back_button.justification = Justification::SOUTHEAST;
        back_button.x_pt = 0.025f / 2.f;
        back_button.y_relative_to_x = true;
        back_button.y_pt = 1.f;

        join_button.parent = &back_button;
        join_button.outside = true;
        join_button.justification = Justification::NORTH;
        join_button.width_pt = 0.2f;
        join_button.max_width = 0.2f * 1920;
        join_button.height_relative_to_width = true;
        join_button.height_pt = (9.f / 16.f) / 2.f;
        join_button.x_pt = 0;
        join_button.y_pt = 0.025f / 2.f;

        left_container.width_pt = 0.5f;
        left_container.height_pt = 1.f;
        left_container.justification = Justification::NORTHWEST;
        left_container.x_pt = 0.f;
        left_container.y_pt = 0.f;

        list_title_label.parent = &left_container;
        list_title_label.width_pt = 1.f;
        list_title_label.height_pt = .15f;
        list_title_label.border_pt = .025f;
        list_title_label.justification = Justification::NORTHWEST;
        list_title_label.x_pt = 0.f;
        list_title_label.y_pt = 0.f;

        game_list_container.parent = &left_container;
        game_list_container.width_pt = 1.f;
        game_list_container.height_pt = .85f;
        game_list_container.border_pt = .025f;
        game_list_container.justification = Justification::SOUTHWEST;
        game_list_container.x_pt = 0.f;
        game_list_container.y_pt = 0.f;

        list.parent = &game_list_container;
        list.width_pt = 1.f;
        list.height_pt = 1.f;
        list.border_pt = .025f / 2;
        list.justification = Justification::NORTHWEST;
        list.x_pt = 0.f;
        list.y_pt = 0.f;
    }

    void update_and_draw(RenderTarget target, InputState *input)
    {
        back_button.update(target);
        join_button.update(target);
        left_container.update(target);
        list_title_label.update(target);
        game_list_container.update(target);

        glDisable(GL_DEPTH_TEST);
        draw_rect(target, back_button.rect, {1, 1, 0, 1});
        draw_rect(target, join_button.rect, {.5f, .5f, 0, 1});

        draw_rect(target, left_container.rect, {0.f, 0.f, 0.f, 0.4f});
        draw_rect(target, list_title_label.rect, {1.f, 0.f, 0.f, .4f});
        draw_rect(target, game_list_container.rect, {1.f, 0.f, 0.f, .4f});
        list.update_and_draw(target, input);

        glEnable(GL_DEPTH_TEST);
    }
};

void menu()
{
    Button exit_button;
}