#pragma once

#include "scene/scene.hpp"

struct ScriptEntityInput
{
    int entity_id;
};

struct BoardController
{
    ScriptEntityInput bar_1 = {71};
    ScriptEntityInput bar_2 = {73};
    ScriptEntityInput bar_3 = {75};
    ScriptEntityInput bar_4 = {77};
    ScriptEntityInput bar_5 = {72};
    ScriptEntityInput bar_6 = {74};
    ScriptEntityInput bar_7 = {76};
    ScriptEntityInput bar_8 = {78};

    int currently_flipping = -1;
    float flip_timer = 0;
    const float FLIP_DURATION = 2.f;

    void update(float timestep)
    {
        // if (currently_flipping > 0)
        // {
        //     float rotation = 180 * (flip_timer / FLIP_DURATION);
        //     bool complete = false;
        //     if (rotation >= 180.f)
        //     {
        //         rotation = 180.f;
        //         complete = true;
        //     }
        //     Entity *bar = scenes.main->get(index_to_id(currently_flipping));
        //     bar->transform.rotation.x = glm::radians(rotation);

        //     if (complete)
        //         currently_flipping = -1;
        // }
    }

    void flip(Scene *scene, Assets2 *assets, int index, String answer, int score)
    {
        // if (index < 1 || index > 8)
        //     return;

        // if (currently_flipping > 0)
        // {
        //     Entity *bar = scenes.main->get(index_to_id(currently_flipping));
        //     bar->transform.rotation.x = glm::radians(180.f);
        // }

        // currently_flipping = index;
        // flip_timer = 0;


        static float t = 0;
        t += 0.009f;
        if (t >= 1)
        {
            t = -1.f;
        }

        Entity *bar_1_entity = scene->get(bar_1.entity_id);
        if (bar_1_entity)
        {
            assets->materials.data[1].value.uniform_ids[0] = UniformId::ALBEDO_TEXTURE;
            assets->materials.data[1].value.uniform_ids[1] = UniformId::NORMAL_TEXTURE;
            assets->materials.data[1].value.uniform_ids[2] = UniformId::METAL_TEXTURE;
            assets->materials.data[1].value.uniform_ids[3] = UniformId::ROUGHNESS_TEXTURE;
            assets->materials.data[1].value.uniform_ids[4] = UniformId::EMIT_TEXTURE;
            assets->materials.data[1].value.uniform_ids[5] = UniformId::AO_TEXTURE;
            assets->materials.data[1].value.uniform_ids[6] = UniformId::NUM_TEX;
            bar_1_entity->material = &assets->materials.data[1].value;
            bar_1_entity->shader = &assets->shaders.data[1].value;
            assets->materials.data[1].value.parameters[0].uniform_id = UniformId::RIPPLE_T;
            assets->materials.data[1].value.parameters[0].value = t;
        }
        Entity *bar_2_entity = scene->get(bar_2.entity_id);
        if (bar_2_entity)
        {
            assets->materials.data[2].value.uniform_ids[0] = UniformId::ALBEDO_TEXTURE;
            assets->materials.data[2].value.uniform_ids[1] = UniformId::NORMAL_TEXTURE;
            assets->materials.data[2].value.uniform_ids[2] = UniformId::METAL_TEXTURE;
            assets->materials.data[2].value.uniform_ids[3] = UniformId::ROUGHNESS_TEXTURE;
            assets->materials.data[2].value.uniform_ids[4] = UniformId::EMIT_TEXTURE;
            assets->materials.data[2].value.uniform_ids[5] = UniformId::AO_TEXTURE;
            assets->materials.data[2].value.uniform_ids[6] = UniformId::NUM_TEX;
            bar_2_entity->material = &assets->materials.data[2].value;
            bar_2_entity->shader = &assets->shaders.data[1].value;
            assets->materials.data[2].value.parameters[0].uniform_id = UniformId::RIPPLE_T;
            assets->materials.data[2].value.parameters[0].value = t;
            // TODO stagger t so theres a delay between each 
        }


        bind_shader(assets->shaders.data[1].value);
        // bind_1f(assets->shaders.data[1].value, UniformId::RIPPLE_T, t);
        RenderTarget target = assets->render_targets.data[0].value;
        Texture num_tex = assets->textures.data[7].value;
        target.bind();
        target.clear();
        bind_texture(assets->shaders.data[0].value, UniformId::NUM_TEX, num_tex);
        bind_shader(assets->shaders.data[0].value);
        bind_1f(assets->shaders.data[0].value, UniformId::T, t);
        draw_rect();
        // draw_textured_rect(target, {0, 0, (float)target.width, (float)target.height}, {}, tex);
        target.color_tex.gen_mipmaps();
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