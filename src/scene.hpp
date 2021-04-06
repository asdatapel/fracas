#pragma once

#include "assets.hpp"
#include "camera.hpp"
#include "graphics.hpp"
#include "material.hpp"

#include "stb/stb_image.hpp"

const char *debug_hdr = "resources/hdri/Newport_Loft_Ref.hdr";

struct Scene
{
    Texture unfiltered_cubemap;
    StandardPbrEnvMaterial env_mat;

    RenderTarget answer_targets[8];
    RenderTarget score_targets[2];

    VertexBuffer x_verts;
    StandardPbrMaterial x_mat;
    VertexBuffer bar_verts;
    StandardPbrMaterial bar_mat;
    VertexBuffer test_verts;
    StandardPbrMaterial test_mat;

    Texture num_texs[8];

    Font font;

    Camera camera;
};

Texture load_hdri(const char *file)
{
    int width, height, components;
    stbi_set_flip_vertically_on_load(true);
    float *hdri = stbi_loadf(debug_hdr, &width, &height, &components, 0);
    Texture tex = to_texture(hdri, width, height);
    stbi_image_free(hdri);

    return tex;
}

Scene init_scene(Assets *assets)
{
    Scene scene;

    {
        Texture hdri_tex = load_hdri(debug_hdr);
        scene.unfiltered_cubemap = hdri_to_cubemap(hdri_tex, 1024);
        Texture irradiance_map = convolve_irradiance_map(scene.unfiltered_cubemap, 32);
        Texture env_map = filter_env_map(scene.unfiltered_cubemap, 512);
        Texture brdf_lut = generate_brdf_lut(512);
        scene.env_mat.texture_array[0] = irradiance_map;
        scene.env_mat.texture_array[1] = env_map;
        scene.env_mat.texture_array[2] = brdf_lut;
    }

    for (int i = 0; i < 8; i++)
    {
        scene.answer_targets[i] = new_render_target(2048, 2048, false);
    }
    for (int i = 0; i < 2; i++)
    {
        scene.score_targets[i] = new_render_target(1024, 1024, false);
    }

    scene.x_verts = assets->vertex_buffers[(int)MeshId::RESOURCES_MODELS_X2];
    scene.x_mat = assets->materials[(int)MaterialId::RESOURCES_MODELS_X2];
    scene.bar_verts = assets->vertex_buffers[(int)MeshId::RESOURCES_MODELS_BAR];
    scene.bar_mat = assets->materials[(int)MaterialId::RESOURCES_MODELS_BAR];
    scene.test_verts = assets->vertex_buffers[(int)MeshId::RESOURCES_MODELS_LONG_TABLE];
    scene.test_mat = assets->materials[(int)MaterialId::RESOURCES_MODELS_LONG_TABLE];

    scene.num_texs[0] = assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_1_BMP];
    scene.num_texs[1] = assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_2_BMP];
    scene.num_texs[2] = assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_3_BMP];
    scene.num_texs[3] = assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_4_BMP];
    scene.num_texs[4] = assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_5_BMP];
    scene.num_texs[5] = assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_6_BMP];
    scene.num_texs[6] = assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_7_BMP];
    scene.num_texs[7] = assets->textures[(int)TextureAssetId::RESOURCES_MODELS_BAR3_NUM_8_BMP];

    scene.font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ANTON_REGULAR_TTF], 128);

    return scene;
}

void draw_scene(Scene *scene, RenderTarget target, InputState *input)
{
    scene->camera.update(target, input);

    static float bar_ts[8] = {};
    static bool animating[8] = {};
    for (int i = 0; i < input->key_input.len; i++)
    {
        if (input->key_input[i] >= Keys::NUM_1 && input->key_input[i] <= Keys::NUM_8)
        {
            animating[(int)input->key_input[i] - (int)Keys::NUM_1] = !animating[(int)input->key_input[i] - (int)Keys::NUM_1];
        }
    }
    for (int i = 0; i < 8; i++)
    {
        float x = (i % 2) * 4.f;
        float y = (i / 2) * -0.8f;

        if (animating[i])
        {
            bar_ts[i] += 0.01f;
        }
        float bar_rot = (powf(bar_ts[i] * 4, 2) * 90.f) - 90.f;
        if (bar_rot < -90.f)
        {
            bar_rot = -90.f;
        }
        if (bar_rot > 90.f)
        {
            bar_rot = 90.f;
        }

        glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), {x, y, 0.f}), glm::radians(bar_rot), glm::vec3{1.f, 0.f, 0.f});

        bind_shader(bar_shader);
        bind_camera(bar_shader, scene->camera);
        bind_mat4(bar_shader, UniformId::MODEL, model);
        bind_material(bar_shader, scene->env_mat);
        bind_material(bar_shader, scene->bar_mat);
        bind_texture(bar_shader, UniformId::OVERLAY_TEXTURE, scene->answer_targets[i].color_tex);
        draw(target, bar_shader, scene->bar_verts);
    }

    float speed_denoms[3] = {2, 2.75, 2.15};
    for (int i = 0; i < 3; i++)
    {
        float t = 0.f;
        t += 0.01f;
        glm::vec3 initial_pos = {-1 + i, 0, 5};
        glm::quat initial_rot({(i * .5f), .1f + (i * .1f), -0.5f + (i * 1.4f)});
        glm::vec3 target_pos = {i - 1, 0.f, 0.f};
        glm::quat target_rot({glm::radians(90.f), 0.f, 0.f});
        float actual_t = 1.f - powf(glm::clamp(t / 2, 0.f, 1.f), speed_denoms[i]);
        glm::vec3 actual_pos = initial_pos + ((target_pos - initial_pos) * actual_t);
        glm::quat actual_rot = glm::normalize(initial_rot + ((target_rot - initial_rot) * actual_t));
        glm::mat4 model = glm::translate(glm::mat4(1.0f), actual_pos) * glm::toMat4(actual_rot);

        bind_shader(threed_shader);
        bind_camera(threed_shader, scene->camera);
        bind_mat4(threed_shader, UniformId::MODEL, model);
        bind_material(threed_shader, scene->x_mat);
        bind_material(threed_shader, scene->env_mat);
        draw(target, threed_shader, scene->x_verts);
    }

    static float t = 0.f;
    t += 0.01f;
    glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), {0.f, 2.f, 0.f}), glm::radians(t), glm::vec3{0.f, 1.f, 0.f});
    bind_shader(threed_shader);
    bind_camera(threed_shader, scene->camera);
    bind_mat4(threed_shader, UniformId::MODEL, model);
    bind_material(threed_shader, scene->env_mat);
    bind_material(threed_shader, scene->test_mat);
    draw(target, threed_shader, scene->test_verts);

    bind_shader(cubemap_shader);
    bind_mat4(cubemap_shader, UniformId::PROJECTION, scene->camera.perspective);
    bind_mat4(cubemap_shader, UniformId::VIEW, scene->camera.view);
    bind_texture(cubemap_shader, UniformId::ENV_MAP, scene->unfiltered_cubemap);
    draw_cubemap();
}

void draw_bar_overlay(RenderTarget previous_target, Scene *scene, int index, String answer, int score)
{
    RenderTarget target = scene->answer_targets[index];
    bind(target);
    clear_backbuffer();

    float aspect_ratio = 2.f;
    float text_scale = 4.f;
    {
        float target_border = 0.05f;
        Rect sub_target = {0, 0,
                           .8f * target.width,
                           .5f * target.height};
        draw_centered_text(scene->font, target, answer, sub_target, target_border, text_scale, aspect_ratio);
    }

    {
        assert(score > 0 && score < 100);
        char buf[3];
        _itoa_s(score, buf, 10);
        String text;
        text.data = buf;
        text.len = strlen(buf);

        float border = 0.025f;
        Rect sub_target = {(1 - .19f) * target.width,
                           0,
                           .19f * target.width,
                           .5f * target.height};
        draw_centered_text(scene->font, target, text, sub_target,  border, text_scale, aspect_ratio);
    }

    {
        Texture num_tex = scene->num_texs[index];
        float img_width = (float)num_tex.width / target.width;
        float img_height = (float)num_tex.height / target.height;

        float border = .04f;
        float scale = (.5f - (border * 2.f)) / (img_height * aspect_ratio);
        float height = img_height * scale;
        float width = img_width * scale;

        float top = .5f + border;
        float left = (1.f - width) / 2.f;

        Rect rect = {left * target.width, top * target.height, target.width * width, target.height * height * aspect_ratio};
        draw_textured_rect(target, rect, {}, num_tex);
    }

    gen_mips(target.color_tex);
    bind(previous_target);
}

void clear_bars(RenderTarget previous_target, Scene *scene)
{
    // TODO should be empty
    static String answers[8] = {
        String::from("RED ASJKDD ASKJHDQQW"),
        String::from("Blue"),
        String::from("ORANGE"),
        String::from("GREEN"),
        String::from("PURPLE"),
        String::from("VIOLET"),
        String::from("PINK"),
        String::from("CYAN"),
    };
    for (int i = 0; i < 8; i++)
    {
        draw_bar_overlay(previous_target, scene, i, answers[i], i + 8);
    }
}