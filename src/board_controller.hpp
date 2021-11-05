#pragma once

#include "resources.hpp"
#include "scene/scene.hpp"

struct ScriptEntityInput
{
    int entity_id;
};

struct BoardController
{
    ScriptEntityInput bar_1 = {71};
    ScriptEntityInput bar_2 = {72};
    ScriptEntityInput bar_3 = {73};
    ScriptEntityInput bar_4 = {74};
    ScriptEntityInput bar_5 = {75};
    ScriptEntityInput bar_6 = {76};
    ScriptEntityInput bar_7 = {77};
    ScriptEntityInput bar_8 = {78};

    int num_active = 0;
    float activation_t = 0.f;
    const float ACTIVATION_DURATION = 2.f;

    int currently_flipping = -1;
    float flip_timer = 0;
    const float FLIP_DURATION = .7f;

    void update(float timestep, Scene *scene, Assets *assets)
    {
        activation_t += timestep / ACTIVATION_DURATION;
        for (int i = 0; i < num_active; i++)
        {
            Entity *bar_entity = scene->get(index_to_id(i));
            bar_entity->material->parameters[0].uniform_id = UniformId::RIPPLE_T;
            bar_entity->material->parameters[0].value = fmin(activation_t - (i * 0.08), 1.f);
        }
        for (int i = num_active; i < 8; i++)
        {
            Entity *bar_entity = scene->get(index_to_id(i));
            bar_entity->material->parameters[0].uniform_id = UniformId::RIPPLE_T;
            bar_entity->material->parameters[0].value = -fmin(activation_t, 1.f);
        }

        if (currently_flipping >= 0)
        {
            flip_timer += timestep;
            float rotation = 180 * (flip_timer / FLIP_DURATION);
            bool complete = false;
            if (rotation >= 180.f)
            {
                rotation = 180.f;
                complete = true;
            }
            Entity *bar = scene->get(index_to_id(currently_flipping));
            bar->transform.rotation.x = glm::radians(rotation);

            if (complete)
                currently_flipping = -1;
        }
    }

    void activate(Scene *scene, int n)
    {
        reset(scene);
        num_active = n;
        activation_t = 0;
    }

    void flip(Scene *scene, Assets *assets, int index, String answer, int score)
    {
        if (index < 0 || index > 7)
            return;

        if (currently_flipping >= 0)
        {
            Entity *bar = scene->get(index_to_id(currently_flipping));
            bar->transform.rotation.x = glm::radians(180.f);
        }

        currently_flipping = index;
        flip_timer = 0;

        Font *font = assets->get_font(FONT_ANTON, 128);
        int render_target_id = 1 + index;
        RenderTarget render_target = assets->render_targets.data[render_target_id].value;
        render_target.bind();
        render_target.clear();
        const float text_scale = 2.f;
        {
            float target_border = 0.05f;
            Rect sub_target = {0, 0,
                               .8f * render_target.width,
                               (float)render_target.height};
            draw_centered_text(*font, render_target, answer, sub_target, target_border, text_scale, text_scale);
        }
        if (score > 0 && score < 100)
        {
            char buf[3];
            _itoa_s(score, buf, 10);

            String text;
            text.data = buf;
            text.len = strlen(buf);

            float border = 0.05f;
            Rect sub_target = {(1 - .18f) * render_target.width,
                               0,
                               .19f * render_target.width,
                               (float)render_target.height};
            draw_centered_text(*font, render_target, text, sub_target, border, text_scale, text_scale);
        }
        render_target.color_tex.gen_mipmaps();
    }

    void reset(Scene *scene)
    {
        for (int i = 0; i < 8; i++)
        {
            Entity *bar_entity = scene->get(index_to_id(i));
            bar_entity->transform.rotation.x = 0;
        }
    }

    int index_to_id(int index)
    {
        ScriptEntityInput ids[] =
            {
                bar_1,
                bar_2,
                bar_3,
                bar_4,
                bar_5,
                bar_6,
                bar_7,
                bar_8,
            };
        return ids[index].entity_id;
    }
};