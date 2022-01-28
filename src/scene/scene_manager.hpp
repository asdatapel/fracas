#pragma once

#include "../game_scripts.hpp"

struct SceneManager
{
    Scene main;
    Scene xs;

    Game game;

    Bloomer bloomer;

    RenderTarget target;

    void init(Assets *assets, Memory mem)
    {
        target = RenderTarget(1920, 1080, TextureFormat::RGB8, TextureFormat::DEPTH24);

        main.init(mem);
        main.visible = true;
        xs.init(mem, TextureFormat::RGBA16F);

        game.init({&main, &xs, assets});

        bloomer = Bloomer(main.target.width, main.target.height);
    }

    void update_scripts(float timestep, Assets *assets, RpcClient *rpc_client, InputState *input)
    {
        game.update(timestep, {&main, &xs, assets}, rpc_client, input);
    }

    void update_and_draw(float timestep, Camera *debug_camera, Vec3f debug_camera_pos, float exposure = 1.f)
    {
        main.update(timestep);
        main.render(debug_camera, debug_camera_pos);

        if (xs.visible)
        {
            xs.render(debug_camera, debug_camera_pos);
            main.target.bind();
            glEnable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            draw_textured_rect(main.target, {0, 0, 1920, 1080}, {}, xs.target.color_tex);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        bloomer.do_bloom(main.target);

        bind_shader(tonemap_shader);
        bind_1f(tonemap_shader, UniformId::EXPOSURE, exposure);
        bind_texture(tonemap_shader, UniformId::BASE, main.target.color_tex);
        bind_texture(tonemap_shader, UniformId::BLOOM, bloomer.get_final().color_tex);

        target.bind();
        glDisable(GL_DEPTH_TEST);
        draw_rect();
        glEnable(GL_DEPTH_TEST);

        target.color_tex.gen_mipmaps();
    }
};