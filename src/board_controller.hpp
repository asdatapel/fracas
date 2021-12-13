#pragma once

#include "resources.hpp"
#include "scene/scene.hpp"

struct BoardController
{
    int bar_1 = 71;
    int bar_2 = 72;
    int bar_3 = 73;
    int bar_4 = 74;
    int bar_5 = 75;
    int bar_6 = 76;
    int bar_7 = 77;
    int bar_8 = 78;

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
        int ids[] =
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
        return ids[index];
    }
};

struct PlayerController
{
    int player_0_0 = 53;
    int player_0_1 = 54;
    int player_0_2 = 55;
    int player_0_3 = 56;
    int player_0_4 = 57;

    int player_1_0 = 59;
    int player_1_1 = 60;
    int player_1_2 = 61;
    int player_1_3 = 62;
    int player_1_4 = 63;

    int left_faceoffer = 91;
    int right_faceoffer = 50;

    float t = 0;

    Animation left_anim;
    Animation right_anim;
    float faceoff_timer = 0;
    const float FACEOFF_DURATION = 3.f;

    StackAllocator a;
    StackAllocator tmp;
    // TODO there should be no init function, the fanim file should be loaded in the assets file and referenced
    void init()
    {
        a.init(10000000);
        tmp.init(10000000);
        left_anim = parse_animation(read_entire_file("resources/scenes/test/anim/anim.fanim"), {&a, &tmp});
        right_anim = parse_animation(read_entire_file("resources/scenes/test/anim/anim.fanim"), {&a, &tmp});
    }

    void update(float timestep, Scene *scene, Assets *assets)
    {
        t += timestep;
        for (int i = 0; i < 5; i++)
        {
            int player_id = index_to_id(0, i);
            Entity *entity = scene->get(player_id);
            entity->transform.position.y = sinf(t * 1.f + (i * 0.5f));
        }
        for (int i = 0; i < 5; i++)
        {
            int player_id = index_to_id(1, i);
            Entity *entity = scene->get(player_id);
            entity->transform.position.y = sinf(t * 1.f + (i * 0.5f));
        }

        Entity *left = scene->get(left_faceoffer);
        left->animation = &left_anim;
        Entity *right = scene->get(right_faceoffer);
        right->animation = &right_anim;
        left_anim.update(clamp(t / FACEOFF_DURATION, 0, 0.99));
        right_anim.update(clamp(t / FACEOFF_DURATION - 0.1f, 0, 0.99));
    }

    void start_faceoff()
    {
        faceoff_timer = 0;
    }

    int index_to_id(int family, int index)
    {
        assert(family == 0 || family == 1);
        assert(index >= 0 && index <= 4);

        int family_0[] = {
            player_0_0,
            player_0_1,
            player_0_2,
            player_0_3,
            player_0_4,
        };
        int family_1[] = {
            player_1_0,
            player_1_1,
            player_1_2,
            player_1_3,
            player_1_4,
        };
        if (family == 0)
        {
            return family_0[index];
        }

        return family_1[index];
    }
};

struct UiController
{
    float banner_t;
    float banner_duration;
    AllocatedString<256> banner_text;

    void update(float timestep, Scene *scene, Assets *assets)
    {
        Font *font = assets->get_font(FONT_ROBOTO_CONDENSED_REGULAR, 64);

        if (banner_t < banner_duration)
        {
            float base_vertical_offset = 70;
            float in_transition_offset = base_vertical_offset - (fmax((.25f - banner_t) * 2, 0) * base_vertical_offset);
            float out_transition_offset = base_vertical_offset - (fmax((banner_t - (banner_duration - 0.25f)) * 4, 0) * base_vertical_offset);
            float transition_offset = fmin(in_transition_offset, out_transition_offset);

            glEnable(GL_BLEND);
            draw_text(*font, scene->target, banner_text, 10, scene->target.height - transition_offset, 1, 1);

            banner_t += timestep;
        }
    }

    void popup_banner(String text, float duration = 5.f)
    {
        banner_t = 0;
        banner_duration = duration;
        banner_text = string_to_allocated_string<256>(text);
    }
};