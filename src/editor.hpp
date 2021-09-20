#pragma once

#include "scene/scene.hpp"
#include "spline.hpp"
#include "animation.hpp"
#include "yaml.hpp"

struct Editor
{
    const char *temp_scene_file = "resources/test/test.yaml";

    EditorCamera debug_camera;
    Entity *selected_camera = nullptr;

    void update_and_draw(Scene *scene, Game *game, RenderTarget backbuffer, InputState *input, Memory mem)
    {
        static bool init = false;
        if (!init)
        {
            init = true;

            Entity new_e;
            new_e.type = EntityType::CAMERA;
            new_e.transform.position.x = (float)(rand() % 10000) / 10000 * 60 - 30;
            new_e.transform.position.z = (float)(rand() % 10000) / 10000 * 60 - 30;
            new_e.transform.position.y = (float)(rand() % 10000) / 10000 * 20;
            new_e.transform.rotation.x = 0;
            new_e.transform.rotation.z = 0;
            new_e.transform.rotation.y = 0;
            new_e.transform.scale.x = 1;
            new_e.transform.scale.z = 1;
            new_e.transform.scale.y = 1;
            scene->entities.push_back(new_e);

            Entity new_spline;
            new_spline.type = EntityType::SPLINE;
            new_spline.spline.points.append({-3, 1.5, 5});
            new_spline.spline.points.append({-3, 1.5, 3});
            new_spline.spline.points.append({-3, 1.5, 2});
            new_spline.spline.points.append({-3, 1.5, 0});
            scene->entities.push_back(new_spline);

            deserialize(scene, game, mem.temp);
        }

        if (!imm_does_gui_have_focus())
        {
            if (!selected_camera)
                debug_camera.update(backbuffer, input);
        }
        scene->update_and_draw(backbuffer, input, selected_camera ? &selected_camera->camera : &debug_camera);
        debug_ui(scene, game, backbuffer, input, mem);

        for (int i = 0; i < input->key_input.len; i++)
        {
            // save scene
            if (input->key_input[i] == Keys::F1)
            {
                String out = serialize(scene, game, mem.temp);
                write_file(temp_scene_file, out);
            }
            //load scene
            if (input->key_input[i] == Keys::F2)
            {
                deserialize(scene, game, mem.temp);
            }
        }
    }

    void debug_ui(Scene *scene, Game *game, RenderTarget target, InputState *input, Memory mem)
    {
        imm_begin(target, selected_camera ? selected_camera->camera : debug_camera, input);

        imm_window("Entities", {0, 0, 300, 600});
        int selected_entity_i = -1;
        Entity *selected_entity = nullptr;
        for (int i = 0; i < scene->entities.size; i++)
        {
            if (scene->entities.data[i].assigned)
            {
                Entity &e = scene->entities.data[i].value;
                if (imm_list_item((ImmId)i + 1, e.debug_tag.name))
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
            imm_label("Width");
            imm_num_input(&selected_entity->spot_light.inner_angle);
            imm_num_input(&selected_entity->spot_light.outer_angle);

            if (selected_entity->type == EntityType::SPLINE)
            {
                draw_spline(selected_entity->spline, target, input, mem, selected_camera ? &selected_camera->camera : &debug_camera, true);
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
                if (imm_button(input_entity->debug_tag.name))
                {
                    if (selected_entity)
                    {
                        *selected_script->inputs[i].value = selected_entity_i;
                    }
                }
            }
        }

        imm_end();
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
                case (EntityType::CAMERA):
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
        FileData in = read_entire_file(temp_scene_file);
        String in_str;
        in_str.data = in.data;
        in_str.len = in.length;
        YAML::Value *root = YAML::deserialize(in_str, alloc);

        std::vector<ScriptDefinition> script_defs = game->get_script_defs();
        YAML::List *in_scripts = root->as_dict()->get("scripts")->as_list();
        for (int i = 0; i < in_scripts->len; i++)
        {
            YAML::List *inputs = in_scripts->get(i)->as_dict()->get("inputs")->as_list();
            for (int input_i = 0; input_i < inputs->len; input_i++)
            {
                YAML::Dict *input = inputs->get(input_i)->as_dict();
                int id = atoi(input->get("id")->as_literal()->value.to_char_array(alloc));
                int value = atoi(input->get("value")->as_literal()->value.to_char_array(alloc));
                *script_defs[i].inputs[id].value = value;
            }
        }
    }
};