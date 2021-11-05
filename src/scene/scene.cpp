#include "scene.hpp"

#include <string>

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
    float *hdri = stbi_loadf(file, &width, &height, &components, 0);

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

// void draw_bar_overlay(Scene *scene, RenderTarget previous_target, int index, String answer, int score)
// {
//     RenderTarget target = scene->bars[index].target;
//     target.bind();
//     target.clear();

//     float aspect_ratio = 2.f;
//     float text_scale = 4.f;
//     {
//         float target_border = 0.05f;
//         Rect sub_target = {0, 0,
//                            .8f * target.width,
//                            .5f * target.height};
//         draw_centered_text(scene->font, target, answer, sub_target, target_border, text_scale, aspect_ratio);
//     }

//     {
//         assert(score > 0 && score < 100);
//         char buf[3];
//         _itoa_s(score, buf, 10);
//         String text;
//         text.data = buf;
//         text.len = strlen(buf);

//         float border = 0.025f;
//         Rect sub_target = {(1 - .19f) * target.width,
//                            0,
//                            .19f * target.width,
//                            .5f * target.height};
//         draw_centered_text(scene->font, target, text, sub_target, border, text_scale, aspect_ratio);
//     }

//     {
//         Texture num_tex = scene->bars[index].num_tex;
//         float img_width = (float)num_tex.width / target.width;
//         float img_height = (float)num_tex.height / target.height;

//         float border = .04f;
//         float scale = (.5f - (border * 2.f)) / (img_height * aspect_ratio);
//         float height = img_height * scale;
//         float width = img_width * scale;

//         float top = .5f + border;
//         float left = (1.f - width) / 2.f;

//         Rect rect = {left * target.width, top * target.height, target.width * width, target.height * height * aspect_ratio};
//         draw_textured_rect(target, rect, {}, num_tex);
//     }

//     target.color_tex.gen_mipmaps();
//     previous_target.bind();
// }

Entity *Scene::get(int id)
{
    return entities.data[id].assigned ? &entities.data[id].value : nullptr;
}

void Scene::init(Memory mem, TextureFormat texture_format)
{
    entities.init(mem.allocator, 1024);
    target = RenderTarget(1920, 1080, texture_format, TextureFormat::DEPTH24);
}

void Scene::load(const char *filename, Assets *assets, Memory mem)
{
    FileData file = read_entire_file(filename, mem.temp);
    YAML::Dict *root = YAML::deserialize(String(file.data, file.length), mem.temp)->as_dict();

    YAML::List *in_entities = root->get("entities")->as_list();
    for (int i = 0; i < in_entities->len; i++)
    {
        YAML::Dict *in_e = in_entities->get(i)->as_dict();

        int id = atoi(in_e->get("id")->as_literal().to_char_array(mem.temp));
        Entity &entity = entities.data[id].value;
        entities.data[i].assigned = true;
        if (entities.next == &entities.data[i])
        {
            entities.next = entities.data[i].next;
        }

        entity.type = entity_type_from_string(in_e->get("type")->as_literal());
        entity.debug_tag.name = string_to_allocated_string<32>(in_e->get("name")->as_literal());

        // transform
        YAML::Dict *in_transform = in_e->get("transform")->as_dict();
        YAML::Dict *in_position = in_transform->get("position")->as_dict();
        entity.transform.position.x = atof(in_position->get("x")->as_literal().to_char_array(mem.temp));
        entity.transform.position.y = atof(in_position->get("y")->as_literal().to_char_array(mem.temp));
        entity.transform.position.z = atof(in_position->get("z")->as_literal().to_char_array(mem.temp));
        YAML::Dict *in_rotation = in_transform->get("rotation")->as_dict();
        entity.transform.rotation.x = atof(in_rotation->get("x")->as_literal().to_char_array(mem.temp));
        entity.transform.rotation.y = atof(in_rotation->get("y")->as_literal().to_char_array(mem.temp));
        entity.transform.rotation.z = atof(in_rotation->get("z")->as_literal().to_char_array(mem.temp));
        YAML::Dict *in_scale = in_transform->get("scale")->as_dict();
        entity.transform.scale.x = atof(in_scale->get("x")->as_literal().to_char_array(mem.temp));
        entity.transform.scale.y = atof(in_scale->get("y")->as_literal().to_char_array(mem.temp));
        entity.transform.scale.z = atof(in_scale->get("z")->as_literal().to_char_array(mem.temp));

        if (entity.type == EntityType::MESH)
        {
            YAML::Dict *in_mesh = in_e->get("mesh")->as_dict();
            int mesh_id = atoi(in_mesh->get("mesh")->as_literal().to_char_array(mem.temp));
            int material_id = atoi(in_mesh->get("material")->as_literal().to_char_array(mem.temp));

            entity.vert_buffer = assets->meshes.data[mesh_id].value;
            entity.material = &assets->materials.data[material_id].value;

            if (auto shader_id_val = in_mesh->get("shader"))
            {
                int shader_id = atoi(in_mesh->get("shader")->as_literal().to_char_array(mem.temp));
                entity.shader = &assets->shaders.data[shader_id].value;
            }
            else
            {
                entity.shader = &assets->shaders.data[0].value;
            }
        }
        else if (entity.type == EntityType::LIGHT)
        {
            YAML::Dict *in_light = in_e->get("spotlight")->as_dict();
            YAML::Dict *in_color = in_light->get("color")->as_dict();
            entity.spot_light.color.x = atof(in_color->get("x")->as_literal().to_char_array(mem.temp));
            entity.spot_light.color.y = atof(in_color->get("y")->as_literal().to_char_array(mem.temp));
            entity.spot_light.color.z = atof(in_color->get("z")->as_literal().to_char_array(mem.temp));

            entity.spot_light.inner_angle = atof(in_light->get("inner_angle")->as_literal().to_char_array(mem.temp));
            entity.spot_light.outer_angle = atof(in_light->get("outer_angle")->as_literal().to_char_array(mem.temp));
        }
        else if (entity.type == EntityType::SPLINE)
        {
            YAML::List *points = in_e->get("spline")->as_list();
            entity.spline.points.len = 0;
            for (int p = 0; p < points->len; p++)
            {
                Vec3f point;
                YAML::Dict *in_p = points->get(p)->as_dict();
                point.x = atof(in_p->get("x")->as_literal().to_char_array(mem.temp));
                point.y = atof(in_p->get("y")->as_literal().to_char_array(mem.temp));
                point.z = atof(in_p->get("z")->as_literal().to_char_array(mem.temp));
                entity.spline.points.append(point);
            }
        }
    }

    String hdr_path = root->get("hdr")->as_literal();
    RenderTarget temp_target(0, 0, TextureFormat::NONE, TextureFormat::NONE);
    Texture hdri_tex = load_hdri(hdr_path.to_char_array(mem.temp));
    unfiltered_cubemap = hdri_to_cubemap(temp_target, hdri_tex, 1024);
    env_mat = create_env_mat(temp_target, unfiltered_cubemap);

    active_camera_id = atoi(root->get("active_camera_id")->as_literal().to_char_array(mem.temp));
    cubemap_visible = root->get("cubemap_visible") && strcmp(root->get("cubemap_visible")->as_literal(), "true");

    if (auto planar_reflector_val = root->get("planar_reflector"))
    {
        YAML::Dict *planar_reflector = planar_reflector_val->as_dict();
        int planar_reflector_entity_id =  atoi(planar_reflector->get("entity_id")->as_literal().to_char_array(mem.temp));
        int render_target_id =  atoi(planar_reflector->get("render_target")->as_literal().to_char_array(mem.temp));

        render_planar = true;
        planar_entity = get(planar_reflector_entity_id);
        planar_target = assets->render_targets.data[render_target_id].value;
    }
}

void Scene::update_and_draw(Camera *editor_camera)
{
    // TODO handle target resize

    Camera *camera;
    if (editor_camera)
    {
        camera = editor_camera;
    }
    else
    {
        if (active_camera_id < 0)
        {
            for (int i = 0; i < entities.size; i++)
            {
                if (entities.data[i].assigned)
                {
                    Entity &e = entities.data[i].value;
                    if (e.type == EntityType::CAMERA)
                    {
                        active_camera_id = i;
                        break;
                    }
                }
            }
        }
        if (active_camera_id < 0)
        {
            return; // cant draw without camera
        }
        camera = &entities.data[active_camera_id].value.camera;
    }

    for (int i = 0; i < entities.size; i++)
    {
        if (entities.data[i].assigned)
        {
            Entity &e = entities.data[i].value;
            if (e.type == EntityType::CAMERA)
            {
                e.camera.update_from_transform(target, e.transform);
            }
        }
    }

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
    upload_lights(all_lights);

    if (render_planar)
    {
        planar_target.bind();
        planar_target.clear();

        float planar_position_y = planar_entity->transform.position.y;

        // http://khayyam.kaplinski.com/2011/09/reflective-water-with-glsl-part-i.html
        glm::mat4 reflection_mat = {1, 0, 0, 0,                       //
                                    0, -1, 0, -2 * planar_position_y, //
                                    0, 0, 1, 0,                       //
                                    0, 0, 0, 1};

        Camera flipped_camera = *camera;
        flipped_camera.view = reflection_mat * camera->view * reflection_mat;
        flipped_camera.pos_y = (2 * planar_position_y) - camera->pos_y;
        
        render_entities(&flipped_camera);

        bind_shader(*planar_entity->shader);
        bind_mat4(*planar_entity->shader, UniformId::REFLECTED_PROJECTION, flipped_camera.perspective * flipped_camera.view);
    }

    target.bind();
    target.clear();
    render_entities(camera);

    if (cubemap_visible)
    {
        bind_shader(cubemap_shader);
        bind_mat4(cubemap_shader, UniformId::PROJECTION, camera->perspective);
        bind_mat4(cubemap_shader, UniformId::VIEW, camera->view);
        bind_texture(cubemap_shader, UniformId::ENV_MAP, unfiltered_cubemap);
        draw_cubemap();
    }
}

void Scene::set_planar_target(RenderTarget target)
{
    render_planar = true;
    planar_target = target;
}

void Scene::render_entities(Camera *camera)
{
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
                bind_material(shader, *e.material, 3);

                draw(target, shader, e.vert_buffer);
            }
        }
    }
}
