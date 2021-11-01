#pragma once

#include "scene/scene.hpp"
#include "spline.hpp"
#include "animation.hpp"
#include "yaml.hpp"

struct Editor
{
    const char *temp_scene_file = "resources/test/test.yaml";

    EditorCamera debug_camera;
    bool use_debug_camera = false;

    bool playing = false;
    Scene play_scene;

    int selected_entity_i = -1;
    Entity *selected_entity = nullptr;
    int selected_spline_node = -1;

    void update_and_draw(Scene *scene, Scene *x_scene, Game *game, RpcClient *rpc_client, RenderTarget backbuffer, InputState *input, Memory mem)
    {
        static bool init = false;
        if (!init)
        {
            init = true;

            play_scene.init(mem);
        }

        static Bloomer bloomer(scene->target.width, scene->target.height);

        if (playing)
        {
            if (!imm_does_gui_have_focus())
            {
                if (use_debug_camera)
                    debug_camera.update(backbuffer, input);
            }

            play_scene.update_and_draw(nullptr);
            game->update(1 / 60.f, {&play_scene, x_scene}, rpc_client, input);

            bloomer.do_bloom(scene->target);
            bind_shader(tonemap_shader);
            bind_1f(tonemap_shader, UniformId::EXPOSURE, 1);
            bind_texture(tonemap_shader, UniformId::BASE, play_scene.target.color_tex);
            bind_texture(tonemap_shader, UniformId::BLOOM, bloomer.get_final().color_tex);
            backbuffer.bind();
            draw_rect();

            debug_ui(&play_scene, x_scene, game, backbuffer, input, mem);
        }
        else
        {
            if (!imm_does_gui_have_focus())
            {
                debug_camera.update(backbuffer, input);
            }

            scene->update_and_draw(get_camera(scene));

            bloomer.do_bloom(scene->target);
            bind_shader(tonemap_shader);
            bind_1f(tonemap_shader, UniformId::EXPOSURE, 1);
            bind_texture(tonemap_shader, UniformId::BASE, scene->target.color_tex);
            bind_texture(tonemap_shader, UniformId::BLOOM, bloomer.get_final().color_tex);
            backbuffer.bind();
            draw_rect();

            debug_ui(scene, x_scene, game, backbuffer, input, mem);
        }
    }

    void debug_ui(Scene *scene, Scene *x_scene, Game *game, RenderTarget target, InputState *input, Memory mem)
    {
        imm_begin(target, *get_camera(scene), input);

        for (int i = 0; i < input->key_input.len; i++)
        {
            // save scene
            if (input->key_input[i] == Keys::F1)
            {
                String out = serialize(scene, game, mem.temp);
                write_file(temp_scene_file, out);
            }
            // load scene
            if (input->key_input[i] == Keys::F2)
            {
                deserialize(scene, game, mem.temp);
            }

            // add spline
            if (input->keys[(int)Keys::LALT] && input->key_input[i] == Keys::S)
            {
                Entity new_spline;
                new_spline.type = EntityType::SPLINE;
                new_spline.spline.points.append({-3, 1.5, 5});
                new_spline.spline.points.append({-3, 1.5, 3});
                new_spline.spline.points.append({-3, 1.5, 2});
                new_spline.spline.points.append({-3, 1.5, 0});
                scene->entities.push_back(new_spline);
            }
            // add spline
            if (input->keys[(int)Keys::LALT] && input->key_input[i] == Keys::W)
            {
                Entity new_e;
                new_e.type = EntityType::CAMERA;
                new_e.transform.position.x = 0;
                new_e.transform.position.z = 0;
                new_e.transform.position.y = 10;
                new_e.transform.rotation.x = 0;
                new_e.transform.rotation.z = 0;
                new_e.transform.rotation.y = 0;
                new_e.transform.scale.x = 1;
                new_e.transform.scale.z = 1;
                new_e.transform.scale.y = 1;
                scene->entities.push_back(new_e);
            }

            // play / pause
            if (input->key_input[i] == Keys::SPACE)
            {
                playing ? stop_play() : start_play(scene, x_scene, game);
            }

            if (input->key_input[i] == Keys::ESCAPE)
            {
                if (use_debug_camera)
                {
                    use_debug_camera = false;
                }
            }
        }

        imm_window("Entities", {0, 0, 300, 600});
        for (int i = 0; i < scene->entities.size; i++)
        {
            if (scene->entities.data[i].assigned)
            {
                Entity &e = scene->entities.data[i].value;
                if (imm_list_item((ImmId)i + 1, e.debug_tag.name, selected_entity_i == i))
                {
                    selected_entity = &e;
                    selected_entity_i = i;
                }
            }
        }

        if (selected_entity)
        {
            imm_window("Deets", {target.width - 300.f, 0, 300, 400});
            imm_textbox(&selected_entity->debug_tag.name);

            imm_label("Position");
            imm_num_input(&selected_entity->transform.position.x);
            imm_num_input(&selected_entity->transform.position.y);
            imm_num_input(&selected_entity->transform.position.z);
            imm_label("Rotation");
            imm_num_input(&selected_entity->transform.rotation.x);
            imm_num_input(&selected_entity->transform.rotation.y);
            imm_num_input(&selected_entity->transform.rotation.z);

            if (selected_entity->type == EntityType::LIGHT)
            {
                imm_label("Light");
                imm_num_input(&selected_entity->spot_light.inner_angle);
                imm_num_input(&selected_entity->spot_light.outer_angle);
            }
            else if (selected_entity->type == EntityType::SPLINE)
            {
                draw_spline(selected_entity->spline, target, input, mem, get_camera(scene), true);

                for (int i = 0; i < selected_entity->spline.points.len; i++)
                {
                    if (imm_3d_point(&selected_entity->spline.points[i], selected_spline_node == i))
                    {
                        selected_spline_node = i;

                        Vec3f temp = selected_entity->spline.points[i];
                        imm_label("Spline Node");
                        imm_num_input(&temp.x);
                        imm_num_input(&temp.y);
                        imm_num_input(&temp.z);
                        selected_entity->spline.points[i] = temp;
                    }
                }

                if (imm_button("Flip Spline"))
                {
                    Vec3f p0 = selected_entity->spline.points[0];
                    Vec3f p1 = selected_entity->spline.points[1];
                    Vec3f p2 = selected_entity->spline.points[2];
                    Vec3f p3 = selected_entity->spline.points[3];

                    selected_entity->spline.points[0] = p3;
                    selected_entity->spline.points[1] = p2;
                    selected_entity->spline.points[2] = p1;
                    selected_entity->spline.points[3] = p0;
                }
            }
            else if (selected_entity->type == EntityType::CAMERA)
            {
                imm_label("Camera");
                if (imm_button("View from camera"))
                {
                    if (!use_debug_camera && scene->active_camera_id == selected_entity_i)
                    {
                        use_debug_camera = true;
                        scene->active_camera_id = -1;
                    }
                    else
                    {
                        scene->active_camera_id = selected_entity_i;
                        use_debug_camera = false;
                    }
                }
            }
        }

        // if (imm_button("New Camera"))
        // {
        //     Entity new_e;
        //     new_e.type = EntityType::CAMERA;
        //     new_e.transform.position.x = (float)(rand() % 10000) / 10000 * 60 - 30;
        //     new_e.transform.position.z = (float)(rand() % 10000) / 10000 * 60 - 30;
        //     new_e.transform.position.y = (float)(rand() % 10000) / 10000 * 20;
        //     new_e.transform.rotation.x = 0;
        //     new_e.transform.rotation.z = 0;
        //     new_e.transform.rotation.y = 0;
        //     new_e.transform.scale.x = 1;
        //     new_e.transform.scale.z = 1;
        //     new_e.transform.scale.y = 1;
        //     scene->entities.push_back(new_e);
        // }
        // if (imm_button("Set Camera"))
        // {
        //     if (selected_entity)
        //         selected_camera = selected_entity->type == EntityType::CAMERA ? selected_entity : nullptr;
        // }
        // if (imm_button("Reset Camera"))
        // {
        //     selected_camera = nullptr;
        // }

        std::vector<ScriptDefinition> scripts = game->get_script_defs();
        imm_window("Scripts", {0, target.height - 400.f, 300, 400});
        ScriptDefinition *selected_script = nullptr;
        for (int i = 0; i < scripts.size(); i++)
        {
            ScriptDefinition *script = &scripts[i];
            if (imm_list_item((ImmId)i + 1, script->name))
            {
                selected_script = script;
            }
        }
        if (selected_script)
        {
            imm_window("Script Deets", {target.width - 300.f, target.height - 400.f, 300, 400});
            for (int i = 0; i < selected_script->inputs.size(); i++)
            {
                imm_label(selected_script->inputs[i].name);

                Entity *input_entity = &scene->entities.data[*selected_script->inputs[i].value].value;
                if (imm_button((ImmId)selected_script->inputs[i].value, input_entity->debug_tag.name))
                {
                    if (selected_entity && selected_entity->type == selected_script->inputs[i].entity_type)
                    {
                        *selected_script->inputs[i].value = selected_entity_i;
                    }
                }
            }
        }

        imm_end();
    }

    void start_play(Scene *scene, Scene *x_scene, Game *game)
    {
        playing = true;

        // copy scene
        for (int i = 0; i < scene->entities.size; i++)
        {
            play_scene.entities.data[i].assigned = scene->entities.data[i].assigned;
            play_scene.entities.data[i].value = scene->entities.data[i].value;
        }
        play_scene.unfiltered_cubemap = scene->unfiltered_cubemap;
        play_scene.env_mat = scene->env_mat;
        // play_scene.entity_names = scene->entity_names;
        // play_scene.bars = scene->bars;

        // for (int i = 0; i < 3; i++)
        // {
        //     play_scene.score_targets[i] = scene->score_targets[i];
        //     play_scene.score_materials[i] = scene->score_materials[i];
        // }

        // play_scene.floor_id = scene->floor_id;
        // play_scene.flipped_camera = scene->flipped_camera;
        // play_scene.floor_target = scene->floor_target;
        // play_scene.floor_material = scene->floor_material;

        // play_scene.uv_sphere_id = scene->uv_sphere_id;
        // play_scene.icosahedron_id = scene->icosahedron_id;
        // play_scene.brick_id = scene->brick_id;

        // play_scene.font = scene->font;
        // play_scene.anim = scene->anim;

        game->init({&play_scene, x_scene});
    }

    void stop_play()
    {
        playing = false;
    }

    String serialize(Scene *scene, Game *game, StackAllocator *alloc)
    {
        auto new_dict = [&]()
        {
            YAML::Dict *dict = (YAML::Dict *)alloc->alloc(sizeof(YAML::Dict));
            new (dict) YAML::Dict;
            return dict;
        };
        auto new_list = [&]()
        {
            YAML::List *list = (YAML::List *)alloc->alloc(sizeof(YAML::List));
            new (list) YAML::List;
            return list;
        };
        auto new_literal = [&](String val)
        {
            YAML::Literal *lit = (YAML::Literal *)alloc->alloc(sizeof(YAML::Literal));
            new (lit) YAML::Literal(val);
            return lit;
        };

        YAML::Dict scene_yaml;

        YAML::List entities;
        scene_yaml.push_back("entities", &entities, alloc);
        for (int i = 0; i < scene->entities.size; i++)
        {
            if (scene->entities.data[i].assigned)
            {
                Entity &entity = scene->entities.data[i].value;

                YAML::Dict *e = new_dict();
                e->push_back("id", new_literal(String::from(i, alloc)), alloc);
                e->push_back("name", new_literal(entity.debug_tag.name), alloc);
                e->push_back("type", new_literal(to_string(entity.type)), alloc);

                YAML::Dict *transform = new_dict();
                YAML::Dict *position = new_dict();
                position->push_back("x", new_literal(String::from(entity.transform.position.x, alloc)), alloc);
                position->push_back("y", new_literal(String::from(entity.transform.position.y, alloc)), alloc);
                position->push_back("z", new_literal(String::from(entity.transform.position.z, alloc)), alloc);
                transform->push_back("position", position, alloc);
                YAML::Dict *rotation = new_dict();
                rotation->push_back("x", new_literal(String::from(entity.transform.rotation.x, alloc)), alloc);
                rotation->push_back("y", new_literal(String::from(entity.transform.rotation.y, alloc)), alloc);
                rotation->push_back("z", new_literal(String::from(entity.transform.rotation.z, alloc)), alloc);
                transform->push_back("rotation", rotation, alloc);
                YAML::Dict *scale = new_dict();
                scale->push_back("x", new_literal(String::from(entity.transform.scale.x, alloc)), alloc);
                scale->push_back("y", new_literal(String::from(entity.transform.scale.y, alloc)), alloc);
                scale->push_back("z", new_literal(String::from(entity.transform.scale.z, alloc)), alloc);
                transform->push_back("scale", scale, alloc);
                e->push_back("transform", transform, alloc);

                switch (entity.type)
                {
                case (EntityType::LIGHT):
                {
                    YAML::Dict *light = new_dict();
                    YAML::Dict *color = new_dict();
                    color->push_back("x", new_literal(String::from(entity.spot_light.color.x, alloc)), alloc);
                    color->push_back("y", new_literal(String::from(entity.spot_light.color.y, alloc)), alloc);
                    color->push_back("z", new_literal(String::from(entity.spot_light.color.z, alloc)), alloc);
                    light->push_back("color", color, alloc);

                    light->push_back("outer_angle", new_literal(String::from(entity.spot_light.outer_angle, alloc)), alloc);
                    light->push_back("inner_angle", new_literal(String::from(entity.spot_light.inner_angle, alloc)), alloc);

                    e->push_back("light", light, alloc);
                }
                break;
                case (EntityType::SPLINE):
                {
                    YAML::List *spline = new_list();
                    for (int p = 0; p < entity.spline.points.len; p++)
                    {
                        YAML::Dict *point = new_dict();
                        point->push_back("x", new_literal(String::from(entity.spline.points[p].x, alloc)), alloc);
                        point->push_back("y", new_literal(String::from(entity.spline.points[p].y, alloc)), alloc);
                        point->push_back("z", new_literal(String::from(entity.spline.points[p].z, alloc)), alloc);
                        spline->push_back(point, alloc);
                    }
                    e->push_back("spline", spline, alloc);
                }
                break;
                default:
                    break;
                }

                entities.push_back(e, alloc);
            }
        }

        YAML::List scripts;
        scene_yaml.push_back("scripts", &scripts, alloc);
        std::vector<ScriptDefinition> script_defs = game->get_script_defs();
        for (int i = 0; i < script_defs.size(); i++)
        {
            ScriptDefinition def = script_defs[i];

            YAML::Dict *script = new_dict();
            script->push_back("name", new_literal(def.name), alloc);

            YAML::List *inputs = new_list();
            script->push_back("inputs", inputs, alloc);
            for (int input_i = 0; input_i < def.inputs.size(); input_i++)
            {
                YAML::Dict *input = new_dict();
                input->push_back("name", new_literal(def.inputs[input_i].name), alloc);
                input->push_back("value", new_literal(String::from(*def.inputs[input_i].value, alloc)), alloc);
                input->push_back("id", new_literal(String::from(input_i, alloc)), alloc);

                inputs->push_back(input, alloc);
            }

            scripts.push_back(script, alloc);
        }

        String out;
        out.data = alloc->next;
        YAML::serialize(&scene_yaml, alloc, 0, false);
        out.len = alloc->next - out.data;

        return out;
    }

    void deserialize(Scene *scene, Game *game, StackAllocator *alloc)
    {
        Temp temp(alloc);

        FileData in = read_entire_file(temp_scene_file, alloc);
        String in_str;
        in_str.data = in.data;
        in_str.len = in.length;
        YAML::Dict *root = YAML::deserialize(in_str, alloc)->as_dict();

        YAML::List *in_entities = root->get("entities")->as_list();
        for (int i = 0; i < in_entities->len; i++)
        {
            YAML::Dict *in_e = in_entities->get(i)->as_dict();

            int id = atoi(in_e->get("id")->as_literal().to_char_array(alloc));
            Entity &entity = scene->entities.data[id].value;
            scene->entities.data[i].assigned = true;
            if (scene->entities.next == &scene->entities.data[i])
            {
                scene->entities.next = scene->entities.data[i].next;
            }

            entity.type = entity_type_from_string(in_e->get("type")->as_literal());
            entity.debug_tag.name = string_to_allocated_string<32>(in_e->get("name")->as_literal());

            // transform
            YAML::Dict *in_transform = in_e->get("transform")->as_dict();
            YAML::Dict *in_position = in_transform->get("position")->as_dict();
            entity.transform.position.x = atof(in_position->get("x")->as_literal().to_char_array(alloc));
            entity.transform.position.y = atof(in_position->get("y")->as_literal().to_char_array(alloc));
            entity.transform.position.z = atof(in_position->get("z")->as_literal().to_char_array(alloc));
            YAML::Dict *in_rotation = in_transform->get("rotation")->as_dict();
            entity.transform.rotation.x = atof(in_rotation->get("x")->as_literal().to_char_array(alloc));
            entity.transform.rotation.y = atof(in_rotation->get("y")->as_literal().to_char_array(alloc));
            entity.transform.rotation.z = atof(in_rotation->get("z")->as_literal().to_char_array(alloc));
            YAML::Dict *in_scale = in_transform->get("scale")->as_dict();
            entity.transform.scale.x = atof(in_scale->get("x")->as_literal().to_char_array(alloc));
            entity.transform.scale.y = atof(in_scale->get("y")->as_literal().to_char_array(alloc));
            entity.transform.scale.z = atof(in_scale->get("z")->as_literal().to_char_array(alloc));

            if (entity.type == EntityType::SPLINE)
            {
                YAML::List *points = in_e->get("spline")->as_list();
                entity.spline.points.len = 0;
                for (int p = 0; p < points->len; p++)
                {
                    Vec3f point;
                    YAML::Dict *in_p = points->get(p)->as_dict();
                    point.x = atof(in_p->get("x")->as_literal().to_char_array(alloc));
                    point.y = atof(in_p->get("y")->as_literal().to_char_array(alloc));
                    point.z = atof(in_p->get("z")->as_literal().to_char_array(alloc));
                    entity.spline.points.append(point);
                }
            }
        }

        std::vector<ScriptDefinition> script_defs = game->get_script_defs();
        YAML::List *in_scripts = root->get("scripts")->as_list();
        for (int i = 0; i < in_scripts->len; i++)
        {
            if (in_scripts->get(i)->as_dict()->get("inputs")->type == YAML::Value::Type::LIST)
            {
                YAML::List *inputs = in_scripts->get(i)->as_dict()->get("inputs")->as_list();
                for (int input_i = 0; input_i < inputs->len; input_i++)
                {
                    YAML::Dict *input = inputs->get(input_i)->as_dict();
                    int id = atoi(input->get("id")->as_literal().to_char_array(alloc));
                    int value = atoi(input->get("value")->as_literal().to_char_array(alloc));
                    *script_defs[i].inputs[id].value = value;
                }
            }
        }
    }

    Camera *get_camera(Scene *scene)
    {
        if (use_debug_camera || scene->active_camera_id < 0)
            return &debug_camera;
        if (scene->active_camera_id > -1)
            return &scene->entities.data[scene->active_camera_id].value.camera;
        return &debug_camera;
    }
};