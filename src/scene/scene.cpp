#include "scene.hpp"

#include <stb/stb_image.hpp>

#include "../assets.hpp"
#include "../camera.hpp"
#include "../debug_ui.hpp"
#include "../graphics/graphics.hpp"
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

void draw_bar_overlay(Scene *scene, RenderTarget previous_target, int index, String answer, int score)
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

void clear_bars(Scene *scene, RenderTarget previous_target)
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
        draw_bar_overlay(scene, previous_target, i, answers[i], i + 8);
    }
}

RenderTarget do_floor(Scene *scene, int camera_entity_id)
{
    Entity *floor = &scene->entities.data[scene->floor_id].value;

    Vec3f plane_normal = {0, 1, 0};
    float plane_d = floor->transform.position.y;

    scene->floor_target.clear();
    scene->floor_target.bind();

    Transform flipped_camera_transform = scene->entities.data[camera_entity_id].value.transform;
    flipped_camera_transform.rotation.y *= -1;
    flipped_camera_transform.position.y = (2 * plane_d) - flipped_camera_transform.position.y;
    scene->flipped_camera.update_from_transform(scene->floor_target, flipped_camera_transform);

    LightUniformBlock all_lights;
    all_lights.num_lights = 0;
    for (int i = 0; i < scene->entities.size && all_lights.num_lights < MAX_LIGHTS; i++)
    {
        if (scene->entities.data[i].assigned)
        {
            Entity &e = scene->entities.data[i].value;
            if (e.type == EntityType::LIGHT)
            {
                SpotLight light;
                light.position = {e.transform.position.x, e.transform.position.y, e.transform.position.z};
                light.direction = glm::quat({e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z}) * glm::vec3(0, -1, 0);
                light.color = glm::vec3{e.spot_light.color.x, e.spot_light.color.y, e.spot_light.color.z};
                light.outer_angle = e.spot_light.outer_angle;
                light.inner_angle = e.spot_light.inner_angle;

                all_lights.spot_lights[all_lights.num_lights] = light;
                all_lights.num_lights++;
            }
        }
    }
    update_lights(all_lights);

    for (int i = 0; i < scene->entities.size; i++)
    {
        if (scene->entities.data[i].assigned)
        {
            Entity &e = scene->entities.data[i].value;

            if (e.type == EntityType::MESH)
            {
                glm::vec3 rot(e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z);
                glm::vec3 pos(e.transform.position.x, e.transform.position.y, e.transform.position.z);
                glm::vec3 scale(e.transform.scale.x, e.transform.scale.y, e.transform.scale.z);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                                  glm::scale(glm::mat4(1.f), scale) *
                                  glm::toMat4(glm::quat(rot));

                bind_shader(*e.shader);
                bind_camera(*e.shader, scene->flipped_camera);
                bind_mat4(*e.shader, UniformId::MODEL, model);
                bind_material(*e.shader, scene->env_mat);
                bind_material(*e.shader, *e.material);
                draw(scene->floor_target, *e.shader, e.vert_buffer);
            }
        }
    }

    bind_shader(cubemap_shader);
    bind_mat4(cubemap_shader, UniformId::PROJECTION, scene->flipped_camera.perspective);
    bind_mat4(cubemap_shader, UniformId::VIEW, scene->flipped_camera.view);
    bind_texture(cubemap_shader, UniformId::ENV_MAP, scene->unfiltered_cubemap);
    draw_cubemap();

    return scene->floor_target;
}

void Scene::init(Assets *assets, Memory mem)
{
    entities.init(mem.allocator, 1024);
    for (int i = 0; i < assets->entities.size(); i++)
    {
        entities.push_back(assets->entities[i]);
    }

    // setup env material
    {
        RenderTarget temp_target(0, 0, TextureFormat::NONE, TextureFormat::NONE);
        Texture hdri_tex = load_hdri(debug_hdr);
        unfiltered_cubemap = hdri_to_cubemap(temp_target, hdri_tex, 1024);
        env_mat = create_env_mat(temp_target, unfiltered_cubemap);
    }

    // bespoke materials
    {
        bars[0].entity_id = assets->entity_names["bar_1"];
        bars[1].entity_id = assets->entity_names["bar_2"];
        bars[2].entity_id = assets->entity_names["bar_3"];
        bars[3].entity_id = assets->entity_names["bar_4"];
        bars[4].entity_id = assets->entity_names["bar_5"];
        bars[5].entity_id = assets->entity_names["bar_6"];
        bars[6].entity_id = assets->entity_names["bar_7"];
        bars[7].entity_id = assets->entity_names["bar_8"];
        for (int i = 0; i < bars.size(); i++)
        {
            Entity *bar = &entities.data[bars[i].entity_id].value;

            Material *base_material = bar->material;
            bars[i].target = RenderTarget(1024, 1024, TextureFormat::RGBA8, TextureFormat::NONE);
            bars[i].material = AllocatedMaterial<StandardPbrMaterial::N + 1>::from(base_material);
            bars[i].material.append(bars[i].target.color_tex, UniformId::OVERLAY_TEXTURE);
            bars[i].num_tex = assets->bar_num_textures[i];

            bar->material = &bars[i].material;
            bar->shader = &bar_shader;
        }

        int screens_entity_ids[] = {
            assets->entity_names["left_score_screen"],
            assets->entity_names["right_score_screen"],
            assets->entity_names["big_screen"],
        };
        for (int i = 0; i < 3; i++)
        {
            Entity *screen = &entities.data[screens_entity_ids[i]].value;

            Material *base_material = screen->material;
            score_targets[i] = RenderTarget(1024, 1024, TextureFormat::RGBA8, TextureFormat::NONE);
            score_materials[i] = AllocatedMaterial<StandardPbrMaterial::N + 1>::from(base_material);
            score_materials[i].append(score_targets[i].color_tex, UniformId::OVERLAY_TEXTURE);

            screen->material = &score_materials[i];
            screen->shader = &bar_shader;
        }
        {
            floor_id = assets->entity_names["floor"];
            Entity *floor = &entities.data[floor_id].value;

            Material *base_material = floor->material;
            floor_target = RenderTarget(1280, 720, TextureFormat::RGB16F, TextureFormat::DEPTH24);
            floor_material = AllocatedMaterial<StandardPbrMaterial::N + 1>::from(base_material);
            floor_material.append(floor_target.color_tex, UniformId::REFLECTION_TEXTURE);

            floor->material = &floor_material;
            floor->shader = &threed_with_planar_shader;
        }
        {
            brick_id = assets->entity_names["bricks"];
            Entity *bricks = &entities.data[brick_id].value;
            bricks->shader = &threed_with_normals_shader;

            bricks->transform.position = {0, 3.5, 0};
        }
        {
            uv_sphere_id = assets->entity_names["uv_sphere"];
        }
    }

    font = load_font(assets->font_files[(int)FontId::RESOURCES_FONTS_ANTON_REGULAR_TTF], 128, mem.temp);
}

void Scene::update_and_draw(RenderTarget backbuffer, InputState *input, Camera *camera)
{
    //TODO check backbuffer resize

    static RenderTarget hdr_target(1920, 1080, TextureFormat::RGB16F, TextureFormat::DEPTH24);

    clear_bars(this, hdr_target);

    RenderTarget floor_reflection;// = do_floor(this, camera);
    floor_reflection.color_tex.gen_mipmaps();

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
        t++;
        int idx = (t / 5) % 5;

        float aspect_ratio = 2.f;
        float text_scale = 4.f;
        {
            float target_border = 0.05f;
            Rect sub_target = {0, 0,
                               .8f * score_targets[0].width,
                               .5f * score_targets[0].height};
            draw_centered_text(font, score_targets[0], strs[idx], sub_target, target_border, text_scale, aspect_ratio);
        }

        score_targets[0].color_tex.gen_mipmaps();
    }
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

        entities.data[bar->entity_id].value.transform.rotation.x = glm::radians(rotation);
    }

    // float speed_denoms[3] = {2, 2.75, 2.15};
    // for (int i = 0; i < 3; i++)
    // {
    //     float t = 0.f;
    //     t += 0.01f;
    //     glm::vec3 initial_pos = {-1 + i, 0, 5};
    //     glm::quat initial_rot({(i * .5f), .1f + (i * .1f), -0.5f + (i * 1.4f)});
    //     glm::vec3 target_pos = {i - 1, 0.f, 0.f};
    //     glm::quat target_rot({glm::radians(90.f), 0.f, 0.f});
    //     float actual_t = 1.f - powf(glm::clamp(t / 2, 0.f, 1.f), speed_denoms[i]);
    //     glm::vec3 actual_pos = initial_pos + ((target_pos - initial_pos) * actual_t);
    //     glm::quat actual_rot = glm::normalize(initial_rot + ((target_rot - initial_rot) * actual_t));
    //     glm::mat4 model = glm::translate(glm::mat4(1.0f), actual_pos) * glm::toMat4(actual_rot);

    //     bind_shader(threed_shader);
    //     bind_camera(threed_shader, camera);
    //     bind_mat4(threed_shader, UniformId::MODEL, model);
    //     bind_material(threed_shader, x_mat);
    //     bind_material(threed_shader, env_mat);
    //     draw(hdr_target, threed_shader, x_verts);
    // }

    hdr_target.bind();
    hdr_target.clear();

    LightUniformBlock all_lights;
    all_lights.num_lights = 0;
    for (int i = 0; i < entities.size && all_lights.num_lights < MAX_LIGHTS; i++)
    {
        if (entities.data[i].assigned)
        {
            Entity &e = entities.data[i].value;
            if (e.type == EntityType::LIGHT)
            {
                SpotLight light;
                light.position = {e.transform.position.x, e.transform.position.y, e.transform.position.z};
                light.direction = glm::rotate(glm::quat(glm::vec3{e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z}), glm::vec3(0, -1, 0));
                light.color = glm::vec3{e.spot_light.color.x, e.spot_light.color.y, e.spot_light.color.z};
                light.outer_angle = e.spot_light.outer_angle;
                light.inner_angle = e.spot_light.inner_angle;
                all_lights.spot_lights[all_lights.num_lights] = light;
                all_lights.num_lights++;
            }
        }
    }
    update_lights(all_lights);

    for (int i = 0; i < entities.size; i++)
    {
        if (entities.data[i].assigned)
        {
            Entity &e = entities.data[i].value;
            if (e.type == EntityType::CAMERA)
            {
                e.camera.update_from_transform(hdr_target, e.transform);
            }
        }
    }

    for (int i = 0; i < entities.size; i++)
    {
        if (entities.data[i].assigned)
        {
            Entity &e = entities.data[i].value;

            if (e.type == EntityType::MESH)
            {
                glm::vec3 rot(e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z);
                glm::vec3 pos(e.transform.position.x, e.transform.position.y, e.transform.position.z);
                glm::vec3 scale(e.transform.scale.x, e.transform.scale.y, e.transform.scale.z);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                                  glm::scale(glm::mat4(1.f), scale) *
                                  glm::toMat4(glm::quat(rot));

                Shader shader = *e.shader;
                bind_shader(shader);
                bind_camera(shader, *camera);
                bind_mat4(shader, UniformId::MODEL, model);
                bind_material(shader, env_mat);
                bind_material(shader, *e.material);

                // TODO HACK
                if (i == floor_id)
                {
                    bind_mat4(shader, UniformId::REFLECTED_PROJECTION, flipped_camera.perspective * flipped_camera.view * model);
                }

                draw(hdr_target, shader, e.vert_buffer);
            }
            else if (e.type == EntityType::CAMERA)
            {
                glm::vec3 rot(e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z);
                glm::vec3 pos(e.transform.position.x, e.transform.position.y, e.transform.position.z);
                glm::vec3 scale(e.transform.scale.x / 3, e.transform.scale.y / 3, e.transform.scale.z / 3);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                                  glm::scale(glm::mat4(1.f), scale) *
                                  glm::toMat4(glm::quat(rot));

                Entity &uv_sphere = entities.data[uv_sphere_id].value;

                bind_shader(*uv_sphere.shader);
                bind_camera(*uv_sphere.shader, *camera);
                bind_mat4(*uv_sphere.shader, UniformId::MODEL, model);
                bind_material(*uv_sphere.shader, env_mat);
                bind_material(*uv_sphere.shader, *uv_sphere.material);
                draw(hdr_target, *uv_sphere.shader, uv_sphere.vert_buffer);
            }
        }
    }

    bind_shader(cubemap_shader);
    bind_mat4(cubemap_shader, UniformId::PROJECTION, camera->perspective);
    bind_mat4(cubemap_shader, UniformId::VIEW, camera->view);
    bind_texture(cubemap_shader, UniformId::ENV_MAP, unfiltered_cubemap);
    draw_cubemap();

    ////////////////////////
    // bloom and tonemapping
    ////////////////////////
    glDisable(GL_DEPTH_TEST);
    static float exposure = 1;
    // if (input->keys[(int)Keys::UP])
    //     exposure += 0.01f;
    // if (input->keys[(int)Keys::DOWN])
    //     exposure -= 0.01f;

    static Bloomer bloomer(hdr_target.width, hdr_target.height);
    bloomer.do_bloom(hdr_target);

    bind_shader(tonemap_shader);
    bind_1f(tonemap_shader, UniformId::EXPOSURE, exposure);
    bind_texture(tonemap_shader, UniformId::BASE, hdr_target.color_tex);
    bind_texture(tonemap_shader, UniformId::BLOOM, bloomer.get_final().color_tex);
    backbuffer.bind();
    draw_rect();
    glEnable(GL_DEPTH_TEST);
}