#pragma once

#include "font.hpp"
#include "graphics.hpp"
#include "platform.hpp"
#include "util.hpp"

typedef void *Uiid;

struct UiContext
{
    Uiid hot;           // moused over
    Uiid focus_started; // moused down
    Uiid active;        // moused down and then moused up
};

bool reset_if_me(Uiid me, Uiid &val)
{
    if (val == me)
    {
        val = nullptr;
        return true;
    }

    return false;
}

template <size_t N>
void do_text_box(Uiid me, UiContext *context, RenderTarget target, InputState *input, Font *font, AllocatedString<N> *str, Rect rect, float border, Color color)
{
    if (input->mouse_x > rect.x && input->mouse_x < rect.x + rect.width && input->mouse_y > rect.y && input->mouse_y < rect.y + rect.height)
    {
        context->hot = me;
    }
    else
    {
        reset_if_me(me, context->hot);
    }

    for (int i = 0; i < input->mouse_input.len; i++)
    {
        if (input->mouse_input[i].down)
        {
            if (context->hot == me)
            {
                context->focus_started = me;
            }
        }
        else
        {
            if (context->hot == me)
            {
                if (context->focus_started == me)
                {
                    context->active = me;
                }
            }
            else
            {
                reset_if_me(me, context->active);
            }

            reset_if_me(me, context->focus_started);
        }
    }

    if (context->active == me)
    {
        color = lighten(color, 0.2f);

        for (int i = 0; i < input->text_input.len; i++)
        {
            str->append(input->text_input[i]);
        }

        for (int i = 0; i < input->key_input.len; i++)
        {
            if (input->key_input[i] == Keys::BACKSPACE && str->len > 0)
            {
                str->len--;
            }
        }
    }

    Color light = lighten(color, .3f);
    Color dark = darken(color, .3f);

    // draw_rect(target, {rect.x - border, rect.y - border, rect.width + (border * 2), rect.height + (border * 2)}, dark);
    // draw_rect(target, {rect.x - border, rect.y - border, rect.width + border, rect.height + border}, light);
    // draw_rect(target, rect, color);

    draw_rect(target, {rect.x, rect.y, rect.width, border}, dark);
    draw_rect(target, {rect.x, rect.y, border, rect.height}, dark);
    draw_rect(target, {rect.x, rect.y + rect.height - border, rect.width, border}, light);
    draw_rect(target, {rect.x + rect.width - border, rect.y, border, rect.height}, light);
    draw_rect(target, rect, color);

    draw_string(target, font, rect.x + border, rect.y, 1.f, *str);
}

bool do_button(Uiid me, UiContext *context, RenderTarget target, InputState *input, Font *font, String str, Rect rect, float border, Color color)
{
    if (rect.height == 0)
    {
        rect.height = font->size + (2 * border);
    }
    float text_scale = (rect.height - (2 * border)) / font->size;
    if (rect.width == 0)
    {
        rect.width = string_width(font, text_scale, str) + (2 * border);
    }

    if (input->mouse_x > rect.x && input->mouse_x < rect.x + rect.width && input->mouse_y > rect.y && input->mouse_y < rect.y + rect.height)
    {
        context->hot = me;
    }
    else
    {
        reset_if_me(me, context->hot);
    }

    for (int i = 0; i < input->mouse_input.len; i++)
    {
        if (input->mouse_input[i].down)
        {
            if (me == context->hot)
            {
                context->focus_started = me;
            }
        }
        else
        {
            if (me == context->hot)
            {
                if (context->focus_started == me)
                {
                    context->active = me;
                }
            }
            reset_if_me(me, context->focus_started);
        }
    }

    Color light = lighten(color, .3f);
    Color dark = darken(color, .3f);

    if (context->hot == me)
    {
        color = lighten(color, 0.16f);
    }
    if (context->focus_started == me)
    {
        Color tmp = dark;
        dark = light;
        light = tmp;
    }

    draw_rect(target, {rect.x, rect.y, rect.width, border}, light);
    draw_rect(target, {rect.x, rect.y, border, rect.height}, light);
    draw_rect(target, {rect.x, rect.y + rect.height - border, rect.width, border}, dark);
    draw_rect(target, {rect.x + rect.width - border, rect.y, border, rect.height}, dark);
    draw_rect(target, rect, color);

    draw_string(target, font, rect.x + border, rect.y, text_scale, str);

    return reset_if_me(me, context->active);
}

void do_label(Uiid me, UiContext *context, RenderTarget target, InputState *input, Font *font, String str, Rect rect, Color color)
{
    if (rect.height == 0)
    {
        rect.height = font->size;
    }
    float text_scale = rect.height / font->size;
    if (rect.width == 0)
    {
        rect.width = string_width(font, text_scale, str);
    }

    draw_rect(target, rect, color);
    draw_string(target, font, rect.x, rect.y, text_scale, str);
}




