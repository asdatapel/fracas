#pragma once

#include "scene/scene.hpp"
#include "spline.hpp"
#include "animation.hpp"

void animation(RenderTarget target, InputState *input)
{
    // static float debug_t = 0.f;
    // debug_t += 0.005f;

    // std::array<glm::vec3, 14> bone_positions;

    // for (int i = 0; i < bones.size(); i++)
    // {
    //     BoneState bs = interpolate_bone(bones[i], debug_t);
    //     // Vec3f final_pos = {bs.position.x - bone_rest_positions[i].x,
    //     //                    bs.position.y - bone_rest_positions[i].y,
    //     //                    bs.position.z - bone_rest_positions[i].z};
    //     // bone_positions[i] = {final_pos.x, final_pos.y, final_pos.z};

    //     imm_3d_point(&bs.position);

    //     // std::string uniform_name = std::string("bone_positions[") + std::to_string(i) + std::string("]");
    //     // int handle = glGetUniformLocation(threed_skinning_shader.shader_handle, uniform_name.c_str());
    //     // glUniform3f(handle, bone_positions[i].x, bone_positions[i].y, bone_positions[i].z);
    // }

    // glUniform3fv(threed_skinning_shader.uniform_handles[(int)UniformId::BONE_POSITIONS], bone_positions.size() * 3, (float *)bone_positions.data());
}

struct Editor
{
    EditorCamera debug_camera;
    Entity *selected_camera = nullptr;

    void update_and_draw(Scene *scene, RenderTarget backbuffer, InputState *input, Memory mem)
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
        }

        if (!imm_does_gui_have_focus())
        {
            if (!selected_camera)
                debug_camera.update(backbuffer, input);
        }
        scene->update_and_draw(backbuffer, input, selected_camera ? &selected_camera->camera : &debug_camera);
        debug_ui(scene, backbuffer, input, mem);
        animation(backbuffer, input);
    }

    void debug_ui(Scene *scene, RenderTarget target, InputState *input, Memory mem)
    {
        imm_begin(target, selected_camera ? selected_camera->camera : debug_camera, input);

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
            imm_label(String::from("Width"));
            imm_num_input(&selected->spot_light.inner_angle);
            imm_num_input(&selected->spot_light.outer_angle);

            if (selected->type == EntityType::SPLINE)
            {
                draw_spline(selected->spline, target, input, mem, selected_camera ? &selected_camera->camera : &debug_camera, true);
            }
        }

        if (imm_button(String::from("New Camera")))
        {
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
        }
        if (imm_button(String::from("Set Camera")))
        {
            if (selected)
                selected_camera = selected->type == EntityType::CAMERA ? selected : nullptr;
        }
        if (imm_button(String::from("Reset Camera")))
        {
            selected_camera = nullptr;
        }

        imm_end();
    }
};