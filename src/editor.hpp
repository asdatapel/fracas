#pragma once

#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "sequence.hpp"
#include "spline.hpp"
#include "animation.hpp"
#include "yaml.hpp"

struct Editor
{
    SceneManager *editor_scenes;
    SceneManager play_scenes;

    bool playing = false;

    EditorCamera debug_camera;
    bool use_debug_camera = false;

    int selected_entity_i = -1;
    Entity *selected_entity = nullptr;
    int selected_spline_node = -1;

    void init(SceneManager *scenes, Assets *assets, Memory mem)
    {
        this->editor_scenes = scenes;

        scenes->main.load("resources/test/main_scene.yaml", assets, mem);
        scenes->xs.load("resources/test/eeegghhh_scene.yaml", assets, mem);
        play_scenes.init(mem);

        deserialize(&scenes->game, mem.temp);
    }

    void update_and_draw(Assets *assets, RpcClient *rpc_client, RenderTarget backbuffer, InputState *input, Memory mem)
    {
        if (playing)
        {
            play_scenes.update_scripts(1 / 60.f, assets, rpc_client, input);
            play_scenes.update_and_draw(nullptr, {});
            debug_ui(&play_scenes, backbuffer, input, assets, mem);
        }
        else
        {
            Transform camera_transform;
            Camera *camera = get_camera(&editor_scenes->main, &camera_transform);
            editor_scenes->update_and_draw(camera, camera_transform.position);
            debug_ui(editor_scenes, backbuffer, input, assets, mem);
        }

        // static float debug_t = 0.f;
        // static Animation anim = parse_animation(read_entire_file("resources/scenes/test/anim/anim.fanim"), mem);
        // debug_t += 0.001f;
        // anim.update(debug_t);

        // std::array<glm::vec3, 14> bone_positions;
        // bind_shader(assets->shaders.data[SHADER_THREED_SKINNING].value);
        // for (int i = 0; i < anim.num_bones; i++)
        // {
        //     glm::mat4 transform = anim.mats[i];
        //     std::string uniform_name = std::string("bone_transforms[") + std::to_string(i) + std::string("]");
        //     int handle = glGetUniformLocation(threed_skinning_shader.shader_handle, uniform_name.c_str());
        //     glUniformMatrix4fv(handle, 1, false, &transform[0][0]);
        // }
    }

    void debug_ui(SceneManager *scenes, RenderTarget target, InputState *input, Assets *assets, Memory mem)
    {
        // copy depth buffer to top level target so we can draw some more stuff
        glBindFramebuffer(GL_READ_FRAMEBUFFER, scenes->main.target.gl_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scenes->target.gl_fbo);
        glBlitFramebuffer(0, 0, scenes->target.width, scenes->target.height,
                          0, 0, scenes->target.width, scenes->target.height,
                          GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);

        static Array<float, 1024 * 1024> lines;
        lines.len = 0;

        for (int i = 0; i < input->key_input.len; i++)
        {
            // save scene
            if (input->key_input[i] == Keys::F1)
            {
                const char *script_file = "resources/test/scripts.yaml";
                String out = serialize(&editor_scenes->game, mem.temp);
                write_file(script_file, out);

                editor_scenes->main.serialize("resources/test/main_scene.yaml", assets, mem.temp);
            }
            // load scene
            if (input->key_input[i] == Keys::F2)
            {
                // deserialize(scene, game, mem.temp);
            }

            if (input->key_input[i] == Keys::F8)
            {
                do_breakpoint = !do_breakpoint;
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
                scenes->main.entities.push_back(new_spline);
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
                scenes->main.entities.push_back(new_e);
            }

            // play / pause
            if (input->key_input[i] == Keys::SPACE)
            {
                playing ? stop_play() : start_play();
            }

            if (input->key_input[i] == Keys::ESCAPE)
            {
                if (use_debug_camera)
                {
                    use_debug_camera = false;
                }
            }
        }

        Imm::start_frame(target, input, assets, &debug_camera);

        Imm::start_menubar_menu("Wind12312ows");
        static bool menu_1 = false;
        static bool menu_2 = false;
        static bool menu_3 = false;
        static bool menu_4 = false;
        static bool menu_5 = false;
        Imm::list_item((ImmId)&menu_1, "hello", menu_1, &menu_1);
        Imm::list_item((ImmId)&menu_2, "hello2", menu_2, &menu_2);
        Imm::list_item((ImmId)&menu_3, "hello3", menu_3, &menu_3);
        Imm::list_item((ImmId)&menu_4, "hello4", menu_4, &menu_4);
        Imm::list_item((ImmId)&menu_5, "hello5", menu_5, &menu_5);

        Imm::add_window_menubar_menu();

        Imm::start_window("Entities", {0, 0, 300, 600});
        for (int i = 0; i < scenes->main.entities.size; i++)
        {
            if (scenes->main.entities.data[i].assigned)
            {
                Entity &e = scenes->main.entities.data[i].value;
                if (Imm::list_item((ImmId)&e, e.debug_tag.name, selected_entity_i == i))
                {
                    selected_entity = &e;
                    selected_entity_i = i;
                }
            }
        }
        Imm::end_window();

        Imm::start_window("Deets", {target.width - 300.f, 0, 300, 400});
        if (selected_entity)
        {
            Imm::textbox(&selected_entity->debug_tag.name);

            Imm::label("Position");
            Imm::num_input(&selected_entity->transform.position.x);
            Imm::num_input(&selected_entity->transform.position.y);
            Imm::num_input(&selected_entity->transform.position.z);
            Imm::label("Rotation");
            Imm::num_input(&selected_entity->transform.rotation.x);
            Imm::num_input(&selected_entity->transform.rotation.y);
            Imm::num_input(&selected_entity->transform.rotation.z);

            if (selected_entity->type == EntityType::LIGHT)
            {
                Imm::label("Light");
                Imm::num_input(&selected_entity->spot_light.inner_angle);
                Imm::num_input(&selected_entity->spot_light.outer_angle);
            }
            else if (selected_entity->type == EntityType::SPLINE)
            {
                // draw_spline(selected_entity->spline, target, input, mem, get_camera(&scenes->main), true);

                Vec3f start = selected_entity->spline.points[1];
                for (int i = 0; i < 100; i++)
                {
                    Vec3f end = catmull_rom_const_distance((i + 1) / 100.f, selected_entity->spline);
                    lines.append(start.x);
                    lines.append(start.y);
                    lines.append(start.z);
                    lines.append(0);
                    lines.append(1);
                    lines.append(0);
                    lines.append(1);
                    lines.append(end.x);
                    lines.append(end.y);
                    lines.append(end.z);
                    lines.append(0);
                    lines.append(1);
                    lines.append(0);
                    lines.append(1);
                    start = end;
                }

                // for (int i = 0; i < selected_entity->spline.points.len; i++)
                // {
                //     if (imm_3d_point(&selected_entity->spline.points[i], selected_spline_node == i))
                //     {
                //         selected_spline_node = i;

                //         Vec3f temp = selected_entity->spline.points[i];
                //         Imm::label("Spline Node");
                //         Imm::num_input(&temp.x);
                //         Imm::num_input(&temp.y);
                //         Imm::num_input(&temp.z);
                //         selected_entity->spline.points[i] = temp;
                //     }
                // }

                if (Imm::button("Flip Spline"))
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
                Imm::label("Camera");
                if (Imm::button("View from camera"))
                {
                    if (!use_debug_camera && scenes->main.active_camera_id == selected_entity_i)
                    {
                        use_debug_camera = true;
                        scenes->main.active_camera_id = -1;
                    }
                    else
                    {
                        scenes->main.active_camera_id = selected_entity_i;
                        use_debug_camera = false;
                    }
                }
            }
        }
        Imm::end_window();

        std::vector<ScriptDefinition> scripts = scenes->game.get_script_defs();
        static int selected_script_i = -1;
        Imm::start_window("Scripts", {0, 0, 300, 600});
        for (int i = 0; i < scripts.size(); i++)
        {
            ScriptDefinition *script = &scripts[i];
            if (Imm::list_item((ImmId)script, script->name, selected_script_i == i))
            {
                selected_script_i = i;
            }
        }
        Imm::end_window();

        if (selected_script_i > -1)
        {
            ScriptDefinition *selected_script = &scripts[selected_script_i];
            Imm::start_window("Script Deets", {target.width - 300.f, target.height - 400.f, 300, 400});
            for (int i = 0; i < selected_script->inputs.size(); i++)
            {
                Imm::label(selected_script->inputs[i].name);

                Entity *input_entity = &scenes->main.entities.data[*selected_script->inputs[i].value].value;
                if (Imm::button((ImmId)selected_script->inputs[i].value, input_entity->debug_tag.name))
                {
                    if (selected_entity && selected_entity->type == selected_script->inputs[i].entity_type)
                    {
                        *selected_script->inputs[i].value = selected_entity_i;
                    }
                }
            }
            Imm::end_window();
        }

        Imm::start_window("Entitiesasd", {50, 50, 200, 500});
        Imm::label("asfasdf");
        if (Imm::button("do something"))
            printf("do something\n");
        Imm::label("ASad");
        Imm::label("Ahgh");
        if (Imm::button("another thing"))
            printf("another thing\n");
        Imm::label("ASadasds");
        if (Imm::button("BULLSHIT"))
            printf("BULLSHIT\n");
        if (Imm::button("Save layout"))
            Imm::save_layout();
        if (Imm::button("Load layout"))
            Imm::load_layout();
        for (int i = 0; i < 20; i++)
        {
            Imm::label("THIS then");
            Imm::label("OKAY");
        }

        String x, y, z, w;
        Imm::list_item((ImmId)&x, "One");
        Imm::list_item((ImmId)&y, "Two");
        Imm::list_item((ImmId)&z, "Three");
        static AllocatedString<64> aasdasd;
        Imm::textbox(&aasdasd);
        static float val = 1.432f;
        Imm::num_input(&val);
        Imm::list_item((ImmId)&w, "Four");
        Imm::end_window();

        static KeyedAnimation seq{30};
        static float play_t = 0;
        static bool playing_timeline = false;
        static int current_frame = 0;
        Imm::start_window("Camera Timeline", {50, 50, 200, 500});
        Imm::num_input(&current_frame);
        if (Imm::button("Add camera key"))
        {
            Transform t;
            Camera *camera = get_camera(&scenes->main, &t);
            seq.add_key(t, current_frame);
        }
        if (Imm::button("play"))
        {
            if (selected_entity && selected_entity->type == EntityType::CAMERA)
            {
                playing_timeline = !playing_timeline;
                play_t = 0;
            }
        }
        if (playing_timeline)
        {
            selected_entity->transform = seq.eval(play_t);
            play_t += 1 / 60.f;
        }
        for (int i = 0; i < seq.keys.len; i++)
        {
            Imm::label(String::from(i, mem.temp));
            Imm::num_input(&seq.keys[i].frame);
            Imm::num_input(&seq.keys[i].transform.position.x);
        }
        if (seq.keys.len)
        {
            Transform tr = seq.eval(0);
            for (float t = 0; t < 50.f; t += 0.01f)
            {
                Transform trn = seq.eval(t);
                lines.append(tr.position.x);
                lines.append(tr.position.y);
                lines.append(tr.position.z);
                lines.append(0);
                lines.append(1);
                lines.append(0);
                lines.append(1);
                lines.append(trn.position.x);
                lines.append(trn.position.y);
                lines.append(trn.position.z);
                lines.append(0);
                lines.append(1);
                lines.append(0);
                lines.append(1);
                tr = trn;
            }
        }
        Imm::end_window();

        bind_shader(lines_shader);
        glUniformMatrix4fv(lines_shader.uniform_handles[(int)UniformId::VIEW], 1, GL_FALSE, &debug_camera.view[0][0]);
        glUniformMatrix4fv(lines_shader.uniform_handles[(int)UniformId::PROJECTION], 1, GL_FALSE, &debug_camera.perspective[0][0]);
        debug_draw_lines(scenes->target, lines.arr, lines.len / (7 * 2));
        Imm::start_window("Scene", {700, 250, 100, 300});
        Imm::texture(&scenes->target.color_tex);
        if (Imm::state.last_element_selected || Imm::state.last_element_active)
        {
            if (!playing || use_debug_camera)
            {
                debug_camera.update(scenes->target, input);
            }
        }
        if (selected_entity)
        {
            Imm::imm_translation_gizmo(&selected_entity->transform.position);
            // Vec3f diff = Imm::imm_3d_point(&selected_entity->transform.position);
            // selected_entity->transform.position.x += diff.x;
            // selected_entity->transform.position.y += diff.y;
            // selected_entity->transform.position.z += diff.z;
        }
        Imm::end_window();

        Imm::end_frame(assets);
    }

    void start_play()
    {
        auto copy_scene = [](Scene *to, Scene *from)
        {
            for (int i = 0; i < from->entities.size; i++)
            {
                to->entities.data[i].assigned = from->entities.data[i].assigned;
                to->entities.data[i].value = from->entities.data[i].value;
            }
            to->unfiltered_cubemap = from->unfiltered_cubemap;
            to->env_mat = from->env_mat;

            to->render_planar = from->render_planar;
            to->planar_target = from->planar_target;
            to->planar_entity = from->planar_entity;

            to->active_camera_id = from->active_camera_id;
        };
        copy_scene(&play_scenes.main, &editor_scenes->main);
        copy_scene(&play_scenes.xs, &editor_scenes->xs);

        auto src_scripts = editor_scenes->game.get_script_defs();
        auto dst_scripts = play_scenes.game.get_script_defs();
        for (int i = 0; i < src_scripts.size(); i++)
        {
            ScriptDefinition src = src_scripts[i];
            ScriptDefinition dst = dst_scripts[i];
            for (int input_i = 0; input_i < src.inputs.size(); input_i++)
            {
                *dst.inputs[input_i].value = *src.inputs[input_i].value;
            }
        }
        play_scenes.game.init({&play_scenes.main, &play_scenes.xs});

        playing = true;
    }

    void stop_play()
    {
        playing = false;
    }

    String serialize(Game *game, StackAllocator *alloc)
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

    void deserialize(Game *game, StackAllocator *alloc)
    {
        Temp temp(alloc);

        const char *script_file = "resources/test/scripts.yaml";
        FileData in = read_entire_file(script_file, alloc);
        String in_str;
        in_str.data = in.data;
        in_str.len = in.length;
        YAML::Dict *root = YAML::deserialize(in_str, alloc)->as_dict();

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

    Camera *get_camera(Scene *scene, Transform *transform_out)
    {
        if (use_debug_camera || scene->active_camera_id < 0)
        {
            transform_out->position = {debug_camera.pos_x, debug_camera.pos_y, debug_camera.pos_z};
            transform_out->rotation = {glm::radians(debug_camera.y_rot), glm::radians(-(debug_camera.x_rot + 90)), 0};
            return &debug_camera;
        }
        if (scene->active_camera_id > -1)
        {
            *transform_out = scene->entities.data[scene->active_camera_id].value.transform;
            return &scene->entities.data[scene->active_camera_id].value.camera;
        }

        transform_out->position = {debug_camera.pos_x, debug_camera.pos_y, debug_camera.pos_z};
        transform_out->rotation = {glm::radians(debug_camera.y_rot), glm::radians(-(debug_camera.x_rot + 90)), 0};
        return &debug_camera;
    }
};