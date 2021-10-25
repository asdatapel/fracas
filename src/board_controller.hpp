#pragma once

struct ScriptEntityInput
{
    int entity_id;
};

struct BoardController
{
    ScriptEntityInput bar_1;
    ScriptEntityInput bar_2;
    ScriptEntityInput bar_3;
    ScriptEntityInput bar_4;
    ScriptEntityInput bar_5;
    ScriptEntityInput bar_6;
    ScriptEntityInput bar_7;
    ScriptEntityInput bar_8;

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

    void flip(Assets2 *assets, int index, String answer, int score)
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

        RenderTarget target = assets->render_targets.data[0].value;
        Texture tex = assets->textures.data[7].value;
        target.bind();
        target.clear();
        draw_textured_rect(target, {0, 0, (float)target.width, (float)target.height}, {}, tex);
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