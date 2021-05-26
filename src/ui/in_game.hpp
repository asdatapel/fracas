#pragma once

#include "../assets.hpp"
#include "../font.hpp"
#include "../net.hpp"
#include "../ui.hpp"

Rect sub_ui(RenderTarget target,
            Rect sub_target,
            float outer_border,
            String title,
            Color background_color,
            Font *font)
{
    Rect bordered_target = {sub_target.x + outer_border,
                            sub_target.y + outer_border,
                            sub_target.width - (outer_border * 2),
                            sub_target.height - (outer_border * 2)};

    if (title.len)
    {
        Rect title_rect = {bordered_target.x,
                           bordered_target.y,
                           bordered_target.width,
                           outer_border * 2};
        draw_rect(target, title_rect, background_color);
        draw_centered_text(*font, target, title, title_rect, .1f, 10, 1);

        bordered_target.y += title_rect.height + (outer_border / 2.f);
        bordered_target.height -= title_rect.height + (outer_border / 2.f);
    }
    draw_rect(target, bordered_target, background_color);

    float internal_border = outer_border / 2.f;
    Rect internal_target = {bordered_target.x + internal_border,
                            bordered_target.y + internal_border,
                            bordered_target.width - (internal_border * 2),
                            bordered_target.height - (internal_border * 2)};

    return internal_target;
}

void waiting_to_start_screen(RenderTarget target, InputState *input, Font *font, const float time_step)
{
    Rect sub_target = sub_ui(target,
                             {0.f, 0.f, (float)target.width, (float)target.height},
                             get_standard_border(target),
                             String::from(""),
                             {0, 0, 0, .4f},
                             font);
}

namespace IngameUi
{
    Font font;

    void init(Assets *assets)
    {
        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ROBOTOCONDENSED_REGULAR_TTF], 64);
    }

    bool update_and_draw(RenderTarget target, InputState *input, const float time_step)
    {
        waiting_to_start_screen(target, input, time_step);
    }
}
