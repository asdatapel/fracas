#pragma once

#include "font.hpp"
#include "graphics/graphics.hpp"
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
void do_text_box(Uiid me, UiContext *context, RenderTarget target, InputState *input, const Font &font, AllocatedString<N> *str, Rect rect, float border, Color color)
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

    draw_text_cropped(font, target, *str, rect.x + border, rect.y, 1.f, 1.f);
}

bool do_button(Uiid me, UiContext *context, RenderTarget target, InputState *input, const Font &font, String str, Rect rect, float border, Color color)
{
    if (rect.height == 0)
    {
        rect.height = font.font_size_px + (2 * border);
    }
    float text_scale = (rect.height - (2 * border)) / font.font_size_px;
    if (rect.width == 0)
    {
        rect.width = get_text_width(font, str, text_scale) + (2 * border);
    }

    if (input->mouse_x > rect.x && input->mouse_x < rect.x + rect.width && input->mouse_y > rect.y && input->mouse_y < rect.y + rect.height)
    {
        context->hot = me;
    }
    else
    {
        reset_if_me(me, context->hot);
    }

    if (input->mouse_down_event)
    {
        if (me == context->hot)
        {
            context->focus_started = me;
        }
    }
    if (input->mouse_up_event)
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

    draw_text_cropped(font, target, str, rect.x + border, rect.y + border, text_scale, text_scale);

    return reset_if_me(me, context->active);
}

void do_label(Uiid me, UiContext *context, RenderTarget target, InputState *input, const Font &font, String str, Rect rect, Color color)
{
    if (rect.height == 0)
    {
        rect.height = font.font_size_px;
    }
    float text_scale = rect.height / font.font_size_px;
    if (rect.width == 0)
    {
        rect.width = get_text_width(font, str, text_scale);
    }

    draw_rect(target, rect, color);
    draw_text_cropped(font, target, str, rect.x, rect.y, text_scale, text_scale);
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

float get_standard_border(RenderTarget target)
{
    return target.width / 50.f;
}

struct UiContext2
{
    Uiid hot;           // moused over
    Uiid focus_started; // moused down
    Uiid active;        // moused down and then moused up

    float focus_started_x, focus_started_y;
    float focus_last_x, focus_last_y;
};
bool reset_hot(Uiid me, UiContext2 *context)
{
    if (context->hot == me)
    {
        context->hot = nullptr;
        return true;
    }

    return false;
}
bool reset_focus(Uiid me, UiContext2 *context)
{
    if (context->focus_started == me)
    {
        context->focus_started = nullptr;
        return true;
    }

    return false;
}
bool reset_active(Uiid me, UiContext2 *context)
{
    if (context->active == me)
    {
        context->active = nullptr;
        return true;
    }

    return false;
}

struct Button
{
    String str;
    Rect rect;
    Color color;
    float text_scale;

    float scale = 1.f;
};

void start_focus(UiContext2 *context, InputState *input, Uiid me)
{
    context->focus_started = me;

    context->focus_started_x = input->mouse_x;
    context->focus_started_y = input->mouse_y;
}

void do_selectable(UiContext2 *context, InputState *input, Uiid me, Rect rect, Rect mask = {})
{
    if (mask.width == 0 || mask.height == 0)
    {
        mask = rect;
    }

    if (input->mouse_x > rect.x &&
        input->mouse_x < rect.x + rect.width &&
        input->mouse_y > rect.y &&
        input->mouse_y < rect.y + rect.height &&
        input->mouse_x > mask.x &&
        input->mouse_x < mask.x + mask.width &&
        input->mouse_y > mask.y &&
        input->mouse_y < mask.y + mask.height)
    {
        context->hot = me;
    }
    else
    {
        reset_if_me(me, context->hot);
    }

    if (input->mouse_down_event)
    {
        start_focus(context, input, me);
    }
    if (input->mouse_up_event)
    {
        if (context->hot != me)
        {
            reset_active(me, context);
        }
        else if (context->focus_started == me)
        {
            context->active = me;
        }

        reset_if_me(me, context->focus_started);
    }
}

void do_draggable(UiContext2 *context, InputState *input, Uiid me, Rect rect, Rect mask = {})
{

    if (mask.width == 0 || mask.height == 0)
    {
        mask = rect;
    }

    if (input->mouse_x > rect.x &&
        input->mouse_x < rect.x + rect.width &&
        input->mouse_y > rect.y &&
        input->mouse_y < rect.y + rect.height &&
        input->mouse_x > mask.x &&
        input->mouse_x < mask.x + mask.width &&
        input->mouse_y > mask.y &&
        input->mouse_y < mask.y + mask.height)
    {
        context->hot = me;
    }
    else
    {
        reset_if_me(me, context->hot);
    }

    if (input->mouse_down_event)
    {
        start_focus(context, input, me);
    }
    if (input->mouse_up_event)
    {
        reset_if_me(me, context->focus_started);
    }
}

bool do_button2(Button *button, UiContext2 *context, RenderTarget target, InputState *input, const Font &font)
{
    Rect actual_rect;
    actual_rect.x = button->rect.x + (button->rect.width - button->rect.width * button->scale) / 2.f;
    actual_rect.y = button->rect.y + (button->rect.height - button->rect.height * button->scale) / 2.f;
    actual_rect.width = button->rect.width * button->scale;
    actual_rect.height = button->rect.height * button->scale;

    do_selectable(context, input, button, button->rect);

    Color color = button->color;
    Color highlight = darken(color, .1f);
    highlight.a = 1.f;

    float button_scale_target = 1.f;
    if (context->hot == button || context->focus_started == button)
    {
        color = highlight;
    }
    if (context->focus_started == button)
    {
        button_scale_target = 0.95f;
    }
    button->scale = (1.25 * button->scale + .75 * button_scale_target) / 2.f;

    draw_rect(target, actual_rect, color);

    float text_border_x = actual_rect.width / 20;
    float text_border_y = actual_rect.height / 10;
    float text_height = actual_rect.height * .45f;
    float text_scale = text_height / -max_ascent(font, button->str);
    draw_text_cropped(font, target, button->str, actual_rect.x + text_border_x, (actual_rect.y + actual_rect.height) - text_border_y - text_height, text_scale, text_scale);

    return reset_active(button, context);
}

template <size_t N>
struct TextBox
{
    AllocatedString<N> str;
    Rect rect;
    Color color;
};
template <size_t N>
void do_text_box(TextBox<N> *me, UiContext2 *context, RenderTarget target, InputState *input, const Font &font)
{
    do_selectable(context, input, me, me->rect);

    Color color = me->color;
    Color highlight = darken(color, .1f);
    highlight.a = 1.f;

    if (context->active == me)
    {
        color = highlight;

        for (int i = 0; i < input->text_input.len; i++)
        {
            me->str.append(input->text_input[i]);
        }

        for (int i = 0; i < input->key_input.len; i++)
        {
            if (input->key_input[i] == Keys::BACKSPACE && me->str.len > 0)
            {
                me->str.len--;
            }
        }
    }

    draw_rect(target, me->rect, color);

    float text_border = me->rect.height / 10.f;
    float text_height = me->rect.height - (2 * text_border);
    float text_width = me->rect.width - (2 * text_border);
    float text_scale = text_height / font.font_size_px;
    float text_x = me->rect.x + text_border;
    float text_y = me->rect.y + text_border;
    draw_rect(target, {text_x, text_y + (font.baseline * text_scale) + text_border / 2.f, text_width, text_border / 2.f}, darken(highlight, .1f));
    draw_text(font, target, me->str, text_x, text_y, text_scale, text_scale);
}

void do_checkbox(bool *me, UiContext2 *context, RenderTarget target, InputState *input, const Font &font, Rect rect, Color color, Rect mask = {})
{
    do_selectable(context, input, me, rect, mask);
    if (reset_active(me, context))
    {
        *me = !*me;
    }

    Color highlight = darken(color, .1f);
    highlight.a = 1.f;
    Color even_highlightier = darken(highlight, .1f);

    if (context->hot == me || context->focus_started == me)
    {
        color = highlight;
    }
    if (*me)
    {
        color = even_highlightier;
    }

    draw_rect(target, rect, color);
}