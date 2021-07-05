#pragma once

#include <array>

#include "../stb/stb_image.hpp"

#include "../assets.hpp"
#include "../camera.hpp"
#include "../graphics.hpp"
#include "../graphics/framebuffer.hpp"
#include "../graphics/bloomer.hpp"
#include "../material.hpp"
#include <scene_def.hpp>

const char *debug_hdr = "resources/hdri/Newport_Loft_Ref.hdr";
Texture2D load_hdri(const char *file)
{
    int width, height, components;
    stbi_set_flip_vertically_on_load(true);
    float *hdri = stbi_loadf(debug_hdr, &width, &height, &components, 0);

    Texture2D tex(width, height, TextureFormat::RGB16F, true);
    tex.upload(hdri, true);

    stbi_image_free(hdri);

    return tex;
}

StandardPbrEnvMaterial create_env_mat(RenderTarget temp_target, Texture unfiltered_cubemap)
{
    Texture irradiance_map = convolve_irradiance_map(temp_target, unfiltered_cubemap, 32);
    Texture env_map = filter_env_map(temp_target, unfiltered_cubemap, 512);

    Texture2D brdf_lut = Texture2D(512, 512, TextureFormat::RGB16F, false);
    temp_target.change_color_target(brdf_lut);
    temp_target.clear();
    temp_target.bind();
    bind_shader(brdf_lut_shader);
    draw_rect();

    StandardPbrEnvMaterial env_mat;
    env_mat.texture_array[0] = irradiance_map;
    env_mat.texture_array[1] = env_map;
    env_mat.texture_array[2] = brdf_lut;

    return env_mat;
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
    RenderTarget floor_target;
    AllocatedMaterial<StandardPbrMaterial::N + 1> floor_material;
    Camera flipped_camera;


    std::array<Bar, 8> bars;

    // TODO remove
    VertexBuffer x_verts;
    StandardPbrMaterial x_mat;

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
            e.shader = &threed_shader;
        }

        // setup env material
        {
            RenderTarget temp_target(0, 0, TextureFormat::NONE, TextureFormat::NONE);
            Texture hdri_tex = load_hdri(debug_hdr);
            unfiltered_cubemap = hdri_to_cubemap(temp_target, hdri_tex, 1024);
            env_mat = create_env_mat(temp_target, unfiltered_cubemap);
        }

        // setup bespoke materials
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
                bars[i].target = RenderTarget(1024, 1024, TextureFormat::RGBA8, TextureFormat::NONE);
                Material *base_material = bars[i].entity->material;
                bars[i].material = AllocatedMaterial<StandardPbrMaterial::N + 1>::from(base_material);

                bars[i].material.append(bars[i].target.color_tex, UniformId::OVERLAY_TEXTURE);
                bars[i].entity->material = &bars[i].material;
                bars[i].entity->shader = &bar_shader;
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
                score_targets[i] = RenderTarget(1024, 1024, TextureFormat::RGBA8, TextureFormat::NONE);

                Material *base_material = entities[(int)EntityDefId::LEFT_SCORE_SCREEN].material;
                score_materials[i] = AllocatedMaterial<StandardPbrMaterial::N + 1>::from(base_material);
                score_materials[i].append(score_targets[i].color_tex, UniformId::OVERLAY_TEXTURE);
            }
            entities[(int)EntityDefId::LEFT_SCORE_SCREEN].material = &score_materials[0];
            entities[(int)EntityDefId::LEFT_SCORE_SCREEN].shader = &bar_shader;
            entities[(int)EntityDefId::RIGHT_SCORE_SCREEN].material = &score_materials[1];
            entities[(int)EntityDefId::RIGHT_SCORE_SCREEN].shader = &bar_shader;
            entities[(int)EntityDefId::BIG_SCREEN].material = &score_materials[2];
            entities[(int)EntityDefId::RIGHT_SCORE_SCREEN].shader = &bar_shader;

            Entity *floor = &entities[(int)EntityDefId::FLOOR];
            floor_target = RenderTarget(1280, 720, TextureFormat::RGB16F, TextureFormat::DEPTH24);
            Material *base_material = floor->material;
            floor_material = AllocatedMaterial<StandardPbrMaterial::N + 1>::from(base_material);
            floor_material.append(floor_target.color_tex, UniformId::REFLECTION_TEXTURE);
            floor->material = &floor_material;
            floor->shader = &threed_with_planar_shader;
        }

        font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ANTON_REGULAR_TTF], 128, &assets->temp_allocator);

        x_verts = assets->vertex_buffers[(int)MeshId::X2];
        x_mat = assets->materials[(int)MaterialId::X2];
    }

    RenderTarget do_floor()
    {
        Entity *floor = &entities[(int)EntityDefId::FLOOR];

        Vec3f plane_normal = {0, 1, 0};
        float plane_d = floor->position.y;

        floor_target.clear();
        floor_target.bind();

        flipped_camera = camera;
        flipped_camera.y_rot *= -1;
        flipped_camera.pos_y = (2 * plane_d) - flipped_camera.pos_y;
        flipped_camera.update_transforms(floor_target);

        static float t = 0.f;
        t += 0.01f;
        LightUniformBlock all_lights;
        all_lights.num_lights = 0;
        for (int i = 0; i < LIGHT_DEF_COUNT && i < 10; i++)
        {
            const LightDef *def = &LightDefs[i];
            Vec3f pos = entities[(int)def->parent].position;
            Vec3f rot = entities[(int)def->parent].rotation;
            SpotLight light;
            light.position = {pos.x, pos.y, pos.z};
            light.direction = glm::quat({rot.x, rot.y, rot.z}) * glm::vec3(0, -1, 0);
            light.color = glm::vec3{def->color.x, def->color.y, def->color.z} * (sinf(t) / 2 + 0.5f);
            light.outer_angle = def->outer_angle;
            light.inner_angle = def->inner_angle;

            all_lights.spot_lights[i] = light;
            all_lights.num_lights++;
        }
        update_lights(all_lights);

        for (int i = 0; i < entities.size(); i++)
        {
            Entity &e = entities[i];

            glm::vec3 rot(e.rotation.x, e.rotation.y, e.rotation.z);
            glm::vec3 pos(e.position.x, e.position.y, e.position.z);
            glm::vec3 scale(e.scale.x, e.scale.y, e.scale.z);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                              glm::scale(glm::mat4(1.f), scale) *
                              glm::toMat4(glm::quat(rot));

            bind_shader(*e.shader);

            // TODO HACK
            if (e.shader == &threed_with_planar_shader)
            {
                bind_mat4(*e.shader, UniformId::REFLECTED_PROJECTION, flipped_camera.perspective * flipped_camera.view * model);
            }

            bind_camera(*e.shader, flipped_camera);
            bind_mat4(*e.shader, UniformId::MODEL, model);
            bind_material(*e.shader, env_mat);
            bind_material(*e.shader, *e.material);
            draw(floor_target, *e.shader, e.vert_buffer);
        }

        bind_shader(cubemap_shader);
        bind_mat4(cubemap_shader, UniformId::PROJECTION, camera.perspective);
        bind_mat4(cubemap_shader, UniformId::VIEW, camera.view);
        bind_texture(cubemap_shader, UniformId::ENV_MAP, unfiltered_cubemap);
        draw_cubemap();

        return floor_target;
    }

    void update_and_draw(RenderTarget backbuffer, Assets *assets, InputState *input)
    {
        //TODO check backbuffer resize

        static RenderTarget hdr_target(1920, 1080, TextureFormat::RGB16F, TextureFormat::DEPTH24);
        camera.update(hdr_target, input);

        RenderTarget floor_reflection = do_floor();
        floor_reflection.color_tex.gen_mipmaps();
        
        hdr_target.bind();
        hdr_target.clear();

        static float t = 0.f;
        t += 0.01f;
        LightUniformBlock all_lights;
        all_lights.num_lights = 0;
        for (int i = 0; i < LIGHT_DEF_COUNT && i < 10; i++)
        {
            const LightDef *def = &LightDefs[i];
            Vec3f pos = entities[(int)def->parent].position;
            Vec3f rot = entities[(int)def->parent].rotation;
            SpotLight light;
            light.position = {pos.x, pos.y, pos.z};
            light.direction = glm::quat({rot.x, rot.y, rot.z}) * glm::vec3(0, -1, 0);
            light.color = glm::vec3{def->color.x, def->color.y, def->color.z} * (sinf(t) / 2 + 0.5f);
            light.outer_angle = def->outer_angle;
            light.inner_angle = def->inner_angle;

            all_lights.spot_lights[i] = light;
            all_lights.num_lights++;
        }
        update_lights(all_lights);

        for (int i = 0; i < input->key_input.len; i++)
        {
            if (input->key_input[i] >= Keys::NUM_1 && input->key_input[i] <= Keys::NUM_8)
            {
                Bar *targetted_bar = &bars[(int)input->key_input[i] - (int)Keys::NUM_1];
                targetted_bar->animation_t += 0.001f;
            }
        }
        for (int i = 0; i < 8; i++)
        {
            Bar *bar = &bars[i];
            if (bar->animation_t > 0.f)
            {
                bar->animation_t += 0.01f;
            }
            float rotation = (powf(bar->animation_t * 4, 2) * 90.f);
            if (rotation < 0.f)
            {
                rotation = 0.f;
            }
            if (rotation > 180.f)
            {
                rotation = 180.f;
            }

            bar->entity->rotation.x = glm::radians(rotation);
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
            bind_camera(threed_shader, camera);
            bind_mat4(threed_shader, UniformId::MODEL, model);
            bind_material(threed_shader, x_mat);
            bind_material(threed_shader, env_mat);
            draw(hdr_target, threed_shader, x_verts);
        }

        {
            score_targets[0].bind();
            score_targets[0].clear();

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
                                   .8f * score_targets[0].width,
                                   .5f * score_targets[0].height};
                draw_centered_text(font, score_targets[0], strs[t], sub_target, target_border, text_scale, aspect_ratio);
            }

            score_targets[0].color_tex.gen_mipmaps();
            hdr_target.bind();
        }

        for (int i = 0; i < entities.size(); i++)
        {
            Entity &e = entities[i];

            glm::vec3 rot(e.rotation.x, e.rotation.y, e.rotation.z);
            glm::vec3 pos(e.position.x, e.position.y, e.position.z);
            glm::vec3 scale(e.scale.x, e.scale.y, e.scale.z);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                              glm::scale(glm::mat4(1.f), scale) *
                              glm::toMat4(glm::quat(rot));

            bind_shader(*e.shader);
            bind_camera(*e.shader, camera);
            bind_mat4(*e.shader, UniformId::MODEL, model);
            bind_material(*e.shader, env_mat);
            bind_material(*e.shader, *e.material);
            draw(hdr_target, *e.shader, e.vert_buffer);
        }

        bind_shader(cubemap_shader);
        bind_mat4(cubemap_shader, UniformId::PROJECTION, camera.perspective);
        bind_mat4(cubemap_shader, UniformId::VIEW, camera.view);
        bind_texture(cubemap_shader, UniformId::ENV_MAP, unfiltered_cubemap);
        draw_cubemap();

        ////////////////////////
        // bloom and tonemapping
        ////////////////////////
        glDisable(GL_DEPTH_TEST);
        static float exposure = 1;
        if (input->keys[(int)Keys::UP])
            exposure += 0.01f;
        if (input->keys[(int)Keys::DOWN])
            exposure -= 0.01f;

        static Bloomer bloomer(hdr_target.width, hdr_target.height);
        bloomer.do_bloom(hdr_target);

        bind_shader(tonemap_shader);
        //printf("%f\n", t);
        bind_1f(tonemap_shader, UniformId::EXPOSURE, exposure);
        bind_texture(tonemap_shader, UniformId::BASE, hdr_target.color_tex);
        bind_texture(tonemap_shader, UniformId::BLOOM, bloomer.get_final().color_tex);
        backbuffer.bind();
        draw_rect();
        glEnable(GL_DEPTH_TEST);
    }
};

void draw_bar_overlay(RenderTarget previous_target, Scene *scene, int index, String answer, int score)
{
    RenderTarget target = scene->bars[index].target;
    target.bind();
    target.clear();

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

    target.color_tex.gen_mipmaps();
    previous_target.bind();
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