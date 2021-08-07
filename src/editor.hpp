#pragma once

#include "scene/scene.hpp"

struct Editor
{
    EditorCamera debug_camera;
    Entity *selected_camera = nullptr;

    void update_and_draw(Scene *scene, RenderTarget backbuffer, InputState *input, Memory mem)
    {
        if (!imm_does_gui_have_focus())
        {
            if (!selected_camera)
                debug_camera.update(backbuffer, input);
        }
        scene->update_and_draw(backbuffer, input, selected_camera ? &selected_camera->camera : &debug_camera);
        debug_ui(scene, backbuffer, input, mem);
    }

    void debug_ui(Scene *scene, RenderTarget target, InputState *input, Memory mem)
    {
        imm_begin(target, input);

        imm_window(String::from("Entities"), {0, 0, 300, 750});
        Entity *selected = nullptr;
        for (int i = 0; i < scene->entities.size; i++)
        {
            if (scene->entities.data[i].assigned)
            {
                Entity &e = scene->entities.data[i].value;
                if (imm_list_item((ImmId)i + 1, e.debug_tag.name))
                {
                    selected = &e;
                }
            }
        }

        if (selected)
        {
            imm_window(String::from("Deets"), {target.width - 300.f, target.height - 400.f, 300, 400});
            imm_textbox(&selected->debug_tag.name);

            imm_label(String::from("Position"));
            imm_num_input(&selected->transform.position.x);
            imm_num_input(&selected->transform.position.y);
            imm_num_input(&selected->transform.position.z);
            imm_label(String::from("Rotation"));
            imm_num_input(&selected->transform.rotation.x);
            imm_num_input(&selected->transform.rotation.y);
            imm_num_input(&selected->transform.rotation.z);

        }

        if (imm_button(String::from("New Camera")))
        {
            Entity new_e;
            new_e.type = EntityType::CAMERA;
            new_e.transform.position.x = (float)(rand() % 10000) / 10000 * 60 - 30;
            new_e.transform.position.z = (float)(rand() % 10000) / 10000 * 60 - 30;
            new_e.transform.position.y = (float)(rand() % 10000) / 10000 * 20;
            new_e.transform.scale.x = 1;
            new_e.transform.scale.z = 1;
            new_e.transform.scale.y = 1;
            scene->entities.push_back(new_e);
        }
        if (imm_button(String::from("Set Camera")))
        {
            selected_camera = selected->type == EntityType::CAMERA ? selected : nullptr;
        }
        if (imm_button(String::from("Reset Camera")))
        {
            selected_camera = nullptr;
        }

        imm_end();
    }
};