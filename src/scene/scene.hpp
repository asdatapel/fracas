#pragma once

#include <array>

#include "../stb/stb_image.hpp"

#include "../assets.hpp"
#include "../camera.hpp"
#include "../graphics.hpp"
#include "../material.hpp"
#include <scene_def.hpp>

const char *debug_hdr = "resources/hdri/Newport_Loft_Ref.hdr";
Texture load_hdri(const char *file)
{
    int width, height, components;
    stbi_set_flip_vertically_on_load(true);
    float *hdri = stbi_loadf(debug_hdr, &width, &height, &components, 0);
    Texture tex = to_texture(hdri, width, height);
    stbi_image_free(hdri);

    return tex;
}

struct Bar
{
    Entity *entity;
    Texture num_tex;
    RenderTarget target;
    AllocatedMaterial<StandardPbrMaterial::N + 1> material;

    float animation_t = 0.f;
};

struct Scene
{
    Texture unfiltered_cubemap;
    StandardPbrEnvMaterial env_mat;

    Camera camera;
    std::array<Entity, ENTITY_DEF_COUNT> entities;

    Font font;

    RenderTarget score_targets[3];
    AllocatedMaterial<StandardPbrMaterial::N + 1> score_materials[3];

    std::array<Bar, 8> bars;

    // TODO remove
    VertexBuffer x_verts;
    StandardPbrMaterial x_mat;
    VertexBuffer bar_verts;
    StandardPbrMaterial bar_mat;
    VertexBuffer test_verts;
    StandardPbrMaterial test_mat;

    void init(Assets *assets)
    {
        // load entities
        for (int i = 0; i < ENTITY_DEF_COUNT; i++)
        {
            Entity &e = entities[i];
            e.vert_buffer = assets->vertex_buffers[(int)EntityDefs[i].mesh_id];
            e.material = &assets->materials[(int)EntityDefs[i].material_id];
            e.position = EntityDefs[i].position;
            e.rotation = EntityDefs[i].rotation;
            e.scale = EntityDefs[i].scale;
        }

        // setup env material
        {
            Texture hdri_tex = load_hdri(debug_hdr);
            unfiltered_cubemap = hdri_to_cubemap(hdri_tex, 1024);
            Texture irradiance_map = convolve_irradiance_map(unfiltered_cubemap, 32);
            Texture env_map = filter_env_map(unfiltered_cubemap, 512);
            Texture brdf_lut = generate_brdf_lut(512);
            env_mat.texture_array[0] = irradiance_map;
            env_mat.texture_array[1] = env_map;
            env_mat.texture_array[2] = brdf_lut;
        }

        // setup bespoke materials for answer bars and score board
        {
            bars[0].entity = &entities[(int)EntityDefId::BAR_1];
            bars[1].entity = &entities[(int)EntityDefId::BAR_2];
            bars[2].entity = &entities[(int)EntityDefId::BAR_3];
            bars[3].entity = &entities[(int)EntityDefId::BAR_4];
            bars[4].entity = &entities[(int)EntityDefId::BAR_5];
            bars[5].entity = &entities[(int)EntityDefId::BAR_6];
            bars[6].entity = &entities[(int)EntityDefId::BAR_7];
            bars[7].entity = &entities[(int)EntityDefId::BAR_8];
            for (int i = 0; i < 8; i++)
            {
                bars[i].target = new_render_target(1024, 1024, false);
                Material *base_material = bars[i].entity->material;
                bars[i].material = AllocatedMaterial<StandardPbrMaterial::N + 1>::from(base_material);
                bars[i].material.append(bars[i].target.color_tex, UniformId::OVERLAY_TEXTURE);
                bars[i].entity->material = &bars[i].material;
            }
            bars[0].num_tex = assets->textures[(int)TextureAssetId::BAR_NUM_1_BMP];
            bars[1].num_tex = assets->textures[(int)TextureAssetId::BAR_NUM_2_BMP];
            bars[2].num_tex = assets->textures[(int)TextureAssetId::BAR_NUM_3_BMP];
            bars[3].num_tex = assets->textures[(int)TextureAssetId::BAR_NUM_4_BMP];
            bars[4].num_tex = assets->textures[(int)TextureAssetId::BAR_NUM_5_BMP];
            bars[5].num_tex = assets->textures[(int)TextureAssetId::BAR_NUM_6_BMP];
            bars[6].num_tex = assets->textures[(int)TextureAssetId::BAR_NUM_7_BMP];
            bars[7].num_tex = assets->textures[(int)TextureAssetId::BAR_NUM_8_BMP];

            for (int i = 0; i < 3; i++)
            {
                score_targets[i] = new_render_target(1024, 1024, false);

                Material *base_material = entities[(int)EntityDefId::LEFT_SCORE_SCREEN].material;
                score_materials[i] = AllocatedMaterial<StandardPbrMaterial::N + 1>::from(base_material);
                score_materials[i].append(score_targets[i].color_tex, UniformId::OVERLAY_TEXTURE);
            }
            entities[(int)EntityDefId::LEFT_SCORE_SCREEN].material = &score_materials[0];
            entities[(int)EntityDefId::RIGHT_SCORE_SCREEN].material = &score_materials[1];
            entities[(int)EntityDefId::RIGHT_SCORE_SCREEN].material = &score_materials[2];
        }

        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ANTON_REGULAR_TTF], 128, &assets->temp_allocator);

        x_verts = assets->vertex_buffers[(int)MeshId::X2];
        x_mat = assets->materials[(int)MaterialId::X2];
    }
};

void draw_scene(Scene *scene, RenderTarget target, Assets *assets, InputState *input)
{
    scene->camera.update(target, input);

    for (int i = 0; i < input->key_input.len; i++)
    {
        if (input->key_input[i] >= Keys::NUM_1 && input->key_input[i] <= Keys::NUM_8)
        {
            Bar *targetted_bar = &scene->bars[(int)input->key_input[i] - (int)Keys::NUM_1];
            targetted_bar->animation_t += 0.001f;
        }
    }
    for (int i = 0; i < 8; i++)
    {
        Bar *bar = &scene->bars[i];
        if (bar->animation_t > 0.f)
        {
            bar->animation_t += 0.01f;
        }
        float rotation = (powf(bar->animation_t * 4, 2) * 90.f);
        if (rotation < 0.f)
        {
            rotation= 0.f;
        }
        if (rotation> 180.f)
        {
            rotation= 180.f;
        }

        bar->entity->rotation.x = glm::radians(rotation);
        // glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), {x, y, 0.f}), glm::radians(bar_rot), glm::vec3{1.f, 0.f, 0.f});

        
        
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

    // static float t = 0.f;
    // t += 0.01f;
    // glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), {0.f, 2.f, 0.f}), glm::radians(t), glm::vec3{0.f, 1.f, 0.f});
    // bind_shader(threed_shader);
    // bind_camera(threed_shader, scene->camera);
    // bind_mat4(threed_shader, UniformId::MODEL, model);
    // bind_material(threed_shader, scene->env_mat);
    // bind_material(threed_shader, scene->test_mat);
    // draw(target, threed_shader, scene->test_verts);

    {
        bind(scene->score_targets[0]);
        clear_backbuffer();

        String strs[5] = {
            String::from("hellp"),
            String::from("khgjkh"),
            String::from("yertyerty"),
            String::from("nvcbnp"),
            String::from("hesdfg"),
        };

        static int t = 0;
        t = (t + 1) % 5;

        float aspect_ratio = 2.f;
        float text_scale = 4.f;
        {
            float target_border = 0.05f;
            Rect sub_target = {0, 0,
                               .8f * target.width,
                               .5f * target.height};
            draw_centered_text(scene->font, target, strs[t], sub_target, target_border, text_scale, aspect_ratio);
        }

        gen_mips(scene->score_targets[0].color_tex);
        bind(target);
    }

    for (int i = 0; i < scene->entities.size(); i++)
    {
        Entity &e = scene->entities[i];

        glm::vec3 rot(e.rotation.x, e.rotation.y, e.rotation.z);
        glm::vec3 pos(e.position.x, e.position.y, e.position.z);
        glm::vec3 scale(e.scale.x, e.scale.y, e.scale.z);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                          glm::scale(glm::mat4(1.f), scale) *
                          glm::toMat4(glm::quat(rot));

        bind_shader(bar_shader);
        bind_camera(bar_shader, scene->camera);
        bind_mat4(bar_shader, UniformId::MODEL, model);
        bind_material(bar_shader, scene->env_mat);
        bind_material(bar_shader, *e.material);
        draw(target, bar_shader, e.vert_buffer);
    }

    bind_shader(cubemap_shader);
    bind_mat4(cubemap_shader, UniformId::PROJECTION, scene->camera.perspective);
    bind_mat4(cubemap_shader, UniformId::VIEW, scene->camera.view);
    bind_texture(cubemap_shader, UniformId::ENV_MAP, scene->unfiltered_cubemap);
    draw_cubemap();
}

void draw_bar_overlay(RenderTarget previous_target, Scene *scene, int index, String answer, int score)
{
    RenderTarget target = scene->bars[index].target;
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
        draw_centered_text(scene->font, target, text, sub_target, border, text_scale, aspect_ratio);
    }

    {
        Texture num_tex = scene->bars[index].num_tex;
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