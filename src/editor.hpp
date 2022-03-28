#pragma once

#include "animation.hpp"
#include "debug_ui.hpp"
#include "game_state.hpp"
#include "keyed_animation.hpp"
#include "project.hpp"
#include "scene/scene.hpp"
#include "renderer/compositor.hpp"
#include "scripts.hpp"
#include "spline.hpp"
#include "yaml.hpp"

struct Editor {
  Assets assets;

  Project project;

  Scene editor_scene;
  Scene play_scene;

  Compositor compositor;

  bool playing = false;

  EditorCamera debug_camera;
  bool use_debug_camera = false;

  int selected_entity_i    = -1;
  Entity *selected_entity  = nullptr;
  int selected_spline_node = -1;

  void init(Memory mem)
  {
    project.asset_files = {
        "resources/fracas/main_assets.yaml",
        "resources/fracas/assets_out.yaml",
    };
    project.scene_files = {
        "resources/fracas/scene.yaml",
    };
    project.renderer_file = "resources/fracas/renderer.yaml";
    project.scripts_file = "resources/test/scripts.yaml";

    assets.init();
    editor_scene.init(mem);
    play_scene.init(mem);

    RefArray<ViewLayer> view_layers;
    deserialize_project(project, mem, &assets, &editor_scene, &view_layers);

    compositor.init(view_layers, &editor_scene, &assets, mem);
    
    InputState tmp;
    debug_camera.update(compositor.final_target, &tmp);
  }

  void update_and_draw(RenderTarget backbuffer, InputState *input, Memory mem)
  {
    if (playing) {
      // Vec2f backup_mouse_pos = input->mouse_pos;
      // if (Imm::state.windows.count(Imm::hash("Scene"))) {
      //   Imm::Window &window = Imm::state.windows[Imm::hash("Scene")];
      //   input->mouse_pos -= Vec2f{window.content_rect.x, window.content_rect.y};
      //   input->mouse_pos.x *= compositor.final_target.width / window.content_rect.width;
      //   input->mouse_pos.y *= compositor.final_target.height / window.content_rect.height;
      // }

      // // play_scenes.update_scripts(1 / 60.f, &assets, get_rpc_client(rpc_client), input);
      // play_scenes.update_and_draw(1 / 60.f, nullptr, {}, input, &assets, 1);

      // input->mouse_pos = backup_mouse_pos;

      // debug_ui(&play_scenes, backbuffer, input, mem);
    } else {
      static float exposure = 1.f;
      if (input->keys[(int)Keys::LEFT]) exposure -= 0.01f;
      if (input->keys[(int)Keys::RIGHT]) exposure += 0.01f;
      Transform camera_transform;
      Camera *camera = get_camera(&editor_scene, &camera_transform);
      compositor.render(camera, camera_transform.position, exposure);
      debug_ui(backbuffer, input, mem);
    }
  }

  void debug_ui(RenderTarget target, InputState *input, Memory mem)
  {
    // copy depth buffer to top level target so we can draw some more stuff
    glBindFramebuffer(GL_READ_FRAMEBUFFER, compositor.layer_targets[0].gl_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, compositor.final_target.gl_fbo);
    glBlitFramebuffer(0, 0, compositor.final_target.width, compositor.final_target.height, 0, 0, compositor.final_target.width,
                      compositor.final_target.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    static Array<float, 1024 * 1024> lines;
    lines.len = 0;

    for (int i = 0; i < input->key_input.len; i++) {
      // save scene
      if (input->key_input[i] == Keys::F1) {
        // const char *script_file = "resources/test/scripts.yaml";
        // String out              = serialize(&editor_scene.game, mem.temp);
        // write_file(script_file, out);

        // editor_scene.serialize("resources/test/main_scene.yaml", &assets, mem.temp);
      }
      // load scene
      if (input->key_input[i] == Keys::F2) {
        // deserialize(scene, game, mem.temp);
      }

      if (input->key_input[i] == Keys::F8) {
        do_breakpoint = !do_breakpoint;
      }

      // add spline
      if (input->keys[(int)Keys::LALT] && input->key_input[i] == Keys::S) {
        Entity new_spline;
        new_spline.type = EntityType::SPLINE;
        new_spline.spline.points.append({-3, 1.5, 5});
        new_spline.spline.points.append({-3, 1.5, 3});
        new_spline.spline.points.append({-3, 1.5, 2});
        new_spline.spline.points.append({-3, 1.5, 0});
        editor_scene.entities.push_back(new_spline);
      }
      // add spline
      if (input->keys[(int)Keys::LALT] && input->key_input[i] == Keys::W) {
        Entity new_e;
        new_e.type                 = EntityType::CAMERA;
        new_e.transform.position.x = 0;
        new_e.transform.position.z = 0;
        new_e.transform.position.y = 10;
        new_e.transform.rotation.x = 0;
        new_e.transform.rotation.z = 0;
        new_e.transform.rotation.y = 0;
        new_e.transform.scale.x    = 1;
        new_e.transform.scale.z    = 1;
        new_e.transform.scale.y    = 1;
        editor_scene.entities.push_back(new_e);
      }

      // play / pause
      if (input->key_input[i] == Keys::SPACE) {
        playing ? stop_play() : start_play();
      }

      if (input->key_input[i] == Keys::ESCAPE) {
        if (use_debug_camera) {
          use_debug_camera = false;
        }
      }
    }

    Imm::start_frame(target, input, &assets, &debug_camera);

    if (Imm::start_menubar_menu("File")) {
      if (Imm::button("Save")) {
        // const char *script_file = "resources/test/scripts.yaml";
        // String script_data      = serialize(&editor_scene.game, mem.temp);
        // write_file(script_file, script_data);

        // assets.save(filepath_concat(RESOURCE_PATH, "assets_out.yaml", &assets_temp_allocator));

        // editor_scene.serialize("resources/test/main_scene.yaml", &assets, mem.temp);

        // Imm::close_popup();
      }
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
    }
    Imm::end_menubar_menu();

    Imm::add_window_menubar_menu();

    Imm::start_window("Entities", {0, 0, 300, 600});
    for (int i = 0; i < editor_scene.entities.size; i++) {
      if (editor_scene.entities.data[i].assigned) {
        Entity &e = editor_scene.entities.data[i].value;
        if (Imm::list_item((ImmId)&e, e.debug_tag.name, selected_entity_i == i)) {
          selected_entity   = &e;
          selected_entity_i = i;
        }
      }
    }
    Imm::end_window();

    Imm::start_window("Deets", {target.width - 300.f, 0, 300, 400});

    // TODO this should be in another window, probably the main entities window
    if (Imm::button("Add Camera")) {
      Entity new_e;
      new_e.type                 = EntityType::CAMERA;
      new_e.transform.position.x = 0;
      new_e.transform.position.z = 0;
      new_e.transform.position.y = 10;
      new_e.transform.rotation.x = 0;
      new_e.transform.rotation.z = 0;
      new_e.transform.rotation.y = 0;
      new_e.transform.scale.x    = 1;
      new_e.transform.scale.z    = 1;
      new_e.transform.scale.y    = 1;
      editor_scene.entities.push_back(new_e);
    }

    if (selected_entity) {
      Imm::textbox(&selected_entity->debug_tag.name);

      Imm::label("Position");
      Imm::num_input(&selected_entity->transform.position.x);
      Imm::num_input(&selected_entity->transform.position.y);
      Imm::num_input(&selected_entity->transform.position.z);
      Imm::label("Rotation");
      Imm::num_input(&selected_entity->transform.rotation.x);
      Imm::num_input(&selected_entity->transform.rotation.y);
      Imm::num_input(&selected_entity->transform.rotation.z);

      if (selected_entity->type == EntityType::LIGHT) {
        Imm::label("Light");
        Imm::num_input(&selected_entity->spot_light.inner_angle);
        Imm::num_input(&selected_entity->spot_light.outer_angle);
      } else if (selected_entity->type == EntityType::SPLINE) {
        // draw_spline(selected_entity->spline, target, input, mem, get_camera(&scenes->main),
        // true);

        Vec3f start = selected_entity->spline.points[1];
        for (int i = 0; i < 100; i++) {
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

        if (Imm::button("Flip Spline")) {
          Vec3f p0 = selected_entity->spline.points[0];
          Vec3f p1 = selected_entity->spline.points[1];
          Vec3f p2 = selected_entity->spline.points[2];
          Vec3f p3 = selected_entity->spline.points[3];

          selected_entity->spline.points[0] = p3;
          selected_entity->spline.points[1] = p2;
          selected_entity->spline.points[2] = p1;
          selected_entity->spline.points[3] = p0;
        }
      } else if (selected_entity->type == EntityType::CAMERA) {
        Imm::label("Camera");
        if (Imm::button("View from camera")) {
          if (!use_debug_camera && compositor.view_layers[0].active_camera_id == selected_entity_i) {
            use_debug_camera              = true;
            compositor.view_layers[0].active_camera_id = -1;
          } else {
            compositor.view_layers[0].active_camera_id = selected_entity_i;
            use_debug_camera              = false;
          }
        }
      }
    }
    Imm::end_window();

    std::vector<ScriptDefinition> scripts = editor_scene.scripts.get_script_defs();
    static int selected_script_i          = -1;
    Imm::start_window("Scripts", {0, 0, 300, 600});
    for (int i = 0; i < scripts.size(); i++) {
      ScriptDefinition *script = &scripts[i];
      if (Imm::list_item((ImmId)script, script->name, selected_script_i == i)) {
        selected_script_i = i;
      }
    }
    Imm::end_window();

    if (selected_script_i > -1) {
      ScriptDefinition *selected_script = &scripts[selected_script_i];
      Imm::start_window("Script Deets", {target.width - 300.f, target.height - 400.f, 300, 400});
      for (int i = 0; i < selected_script->inputs.size(); i++) {
        Imm::label(selected_script->inputs[i].name);

        Entity *input_entity = &editor_scene.entities.data[*selected_script->inputs[i].value].value;
        if (Imm::button((ImmId)selected_script->inputs[i].value, input_entity->debug_tag.name)) {
          if (selected_entity && selected_entity->type == selected_script->inputs[i].entity_type) {
            *selected_script->inputs[i].value = selected_entity_i;
          }
        }
      }
      Imm::end_window();
    }

    Imm::start_window("Entitiesasd", {50, 50, 200, 500});
    Imm::label("asfasdf");
    if (Imm::button("do something")) printf("do something\n");
    Imm::label("ASad");
    Imm::label("Ahgh");
    if (Imm::button("another thing")) printf("another thing\n");
    Imm::label("ASadasds");
    if (Imm::button("BULLSHIT")) printf("BULLSHIT\n");
    if (Imm::button("Save layout")) Imm::save_layout();
    if (Imm::button("Load layout")) Imm::load_layout();
    for (int i = 0; i < 20; i++) {
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

    Imm::start_window("Assets", {1000, 500, 600, 400});
    if (Imm::button("Add KeyedAnimation")) {
      assets.create_keyed_animation(RESOURCE_PATH);
    }

    for (int i = 0; i < assets.keyed_animations.size; i++) {
      if (assets.keyed_animations.data[i].assigned) {
        KeyedAnimation *ka      = &assets.keyed_animations.data[i].value;
        AllocatedString<64> tmp = string_to_allocated_string<64>(ka->asset_name);
        Imm::listitem_with_editing((ImmId)ka, &tmp, ka == editor_scene.current_sequence);
        if (Imm::state.just_activated == (ImmId)ka) {
          editor_scene.stop_sequence();
          editor_scene.set_sequence(ka);
        }

        if (!strcmp(tmp, ka->asset_name)) {
          ka->asset_name = String::copy(tmp, &assets_allocator);
        }

        if (Imm::state.was_last_item_right_clicked) {
          Imm::open_popup((ImmId)ka, input->mouse_pos);
        }
        if (Imm::start_popup((ImmId)ka, {})->visible) {
          if (Imm::button("Delete")) {
            assets.keyed_animations.free(&assets.keyed_animations.data[i]);
            Imm::close_popup();
          }
        }
        Imm::end_popup();
      }
    }
    Imm::end_window();

    window_timeline(input);

    bind_shader(lines_shader);
    glUniformMatrix4fv(lines_shader.uniform_handles[(int)UniformId::VIEW], 1, GL_FALSE,
                       &debug_camera.view[0][0]);
    glUniformMatrix4fv(lines_shader.uniform_handles[(int)UniformId::PROJECTION], 1, GL_FALSE,
                       &debug_camera.perspective[0][0]);
    debug_draw_lines(compositor.final_target, lines.arr, lines.len / (7 * 2));

    Imm::start_window("Scene", {700, 250, 100, 300});
    Imm::texture(&compositor.final_target.color_tex);
    if (Imm::state.last_element_selected || Imm::state.last_element_active) {
      if (!playing || use_debug_camera) {
        debug_camera.update(compositor.final_target, input);
      }
    }
    if (selected_entity) {
      if (input->keys[(int)Keys::LCTRL]) {
        Imm::rotation_gizmo(&selected_entity->transform.rotation,
                            selected_entity->transform.position);
      } else {
        Imm::translation_gizmo(&selected_entity->transform.position);
      }
    }
    Imm::end_window();

    compositor.final_target.color_tex.gen_mipmaps();

    Imm::end_frame(&assets);
  }

  void start_play()
  {
    // auto copy_scene = [](Scene *to, Scene *from) {
    //   for (int i = 0; i < from->entities.size; i++) {
    //     to->entities.data[i].assigned = from->entities.data[i].assigned;
    //     to->entities.data[i].value    = from->entities.data[i].value;
    //   }

    //   to->active_camera_id = from->active_camera_id;

    //   to->unfiltered_cubemap = from->unfiltered_cubemap;
    //   to->env_mat            = from->env_mat;

    //   to->render_planar = from->render_planar;
    //   to->planar_target = from->planar_target;
    //   to->planar_entity = from->planar_entity;

    //   to->cubemap_visible = from->cubemap_visible;
    // };
    // copy_scene(&play_scenes.main, &editor_scene.);
    // copy_scene(&play_scenes.xs, &editor_scene.xs);

    // auto src_scripts = editor_scene.game.get_script_defs();
    // auto dst_scripts = play_scenes.game.get_script_defs();
    // for (int i = 0; i < src_scripts.size(); i++) {
    //   ScriptDefinition src = src_scripts[i];
    //   ScriptDefinition dst = dst_scripts[i];
    //   for (int input_i = 0; input_i < src.inputs.size(); input_i++) {
    //     *dst.inputs[input_i].value = *src.inputs[input_i].value;
    //   }
    // }

    // playing = true;
  }

  void stop_play() { playing = false; }

  String serialize(Scripts *game, StackAllocator *alloc)
  {
    auto new_dict = [&]() {
      YAML::Dict *dict = (YAML::Dict *)alloc->alloc(sizeof(YAML::Dict));
      new (dict) YAML::Dict;
      return dict;
    };
    auto new_list = [&]() {
      YAML::List *list = (YAML::List *)alloc->alloc(sizeof(YAML::List));
      new (list) YAML::List;
      return list;
    };
    auto new_literal = [&](String val) {
      YAML::Literal *lit = (YAML::Literal *)alloc->alloc(sizeof(YAML::Literal));
      new (lit) YAML::Literal(val);
      return lit;
    };

    YAML::Dict scene_yaml;

    YAML::List scripts;
    scene_yaml.push_back("scripts", &scripts, alloc);
    std::vector<ScriptDefinition> script_defs = game->get_script_defs();
    for (int i = 0; i < script_defs.size(); i++) {
      ScriptDefinition def = script_defs[i];

      YAML::Dict *script = new_dict();
      script->push_back("name", new_literal(def.name), alloc);

      YAML::List *inputs = new_list();
      script->push_back("inputs", inputs, alloc);
      for (int input_i = 0; input_i < def.inputs.size(); input_i++) {
        YAML::Dict *input = new_dict();
        input->push_back("name", new_literal(def.inputs[input_i].name), alloc);
        input->push_back("value", new_literal(String::from(*def.inputs[input_i].value, alloc)),
                         alloc);
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

  void deserialize(Scripts *game, StackAllocator *alloc)
  {
    Temp temp(alloc);

    const char *script_file = "resources/test/scripts.yaml";
    FileData in             = read_entire_file(script_file, alloc);
    String in_str;
    in_str.data      = in.data;
    in_str.len       = in.length;
    YAML::Dict *root = YAML::deserialize(in_str, alloc)->as_dict();

    std::vector<ScriptDefinition> script_defs = game->get_script_defs();
    YAML::List *in_scripts                    = root->get("scripts")->as_list();
    for (int i = 0; i < in_scripts->len; i++) {
      if (in_scripts->get(i)->as_dict()->get("inputs")->type == YAML::Value::Type::LIST) {
        YAML::List *inputs = in_scripts->get(i)->as_dict()->get("inputs")->as_list();
        for (int input_i = 0; input_i < inputs->len; input_i++) {
          YAML::Dict *input = inputs->get(input_i)->as_dict();
          int id            = atoi(input->get("id")->as_literal().to_char_array(alloc));
          int value         = atoi(input->get("value")->as_literal().to_char_array(alloc));
          *script_defs[i].inputs[id].value = value;
        }
      }
    }
  }

  Camera *get_camera(Scene *scene, Transform *transform_out)
  {
    if (use_debug_camera || compositor.view_layers[0].active_camera_id < 0) {
      transform_out->position = {debug_camera.pos_x, debug_camera.pos_y, debug_camera.pos_z};
      transform_out->rotation = {glm::radians(debug_camera.y_rot),
                                 glm::radians(-(debug_camera.x_rot + 90)), 0};
      transform_out->scale    = {1, 1, 1};
      return &debug_camera;
    }
    if (compositor.view_layers[0].active_camera_id > -1) {
      *transform_out = scene->entities.data[compositor.view_layers[0].active_camera_id].value.transform;
      return &scene->entities.data[compositor.view_layers[0].active_camera_id].value.camera;
    }

    transform_out->position = {debug_camera.pos_x, debug_camera.pos_y, debug_camera.pos_z};
    transform_out->rotation = {glm::radians(debug_camera.y_rot),
                               glm::radians(-(debug_camera.x_rot + 90)), 0};
    transform_out->scale    = {1, 1, 1};
    return &debug_camera;
  }

  void window_timeline(InputState *input)
  {
    auto type_to_shape = [](KeyedAnimationTrack::Key::InterpolationType type) -> Imm::Shape {
      switch (type) {
        case (KeyedAnimationTrack::Key::InterpolationType::CONSTANT):
          return Imm::Shape::BAR;
        case (KeyedAnimationTrack::Key::InterpolationType::LINEAR):
          return Imm::Shape::DIAMOND;
        case (KeyedAnimationTrack::Key::InterpolationType::SMOOTHSTEP):
          return Imm::Shape::TRIANGLE;
        case (KeyedAnimationTrack::Key::InterpolationType::SPLINE):
          return Imm::Shape::SQUARE;
      }
      return Imm::Shape::BAR;
    };

    auto keyframe_edit_popup =
        [](ImmId id, Optional<KeyedAnimationTrack::Key::InterpolationType> interp_type) {
          Optional<KeyedAnimationTrack::Key::InterpolationType> result = Optional<>::empty;

          if (Imm::start_popup(id, {})->visible) {
            auto add_list_item = [&](String name,
                                     KeyedAnimationTrack::Key::InterpolationType type) {
              Imm::list_item(name, interp_type.exists && type == interp_type.value);
              if (Imm::state.just_activated == Imm::hash(name)) {
                result = type;
                Imm::close_popup();
              }
            };

            add_list_item("Constant", KeyedAnimationTrack::Key::InterpolationType::CONSTANT);
            add_list_item("Linear", KeyedAnimationTrack::Key::InterpolationType::LINEAR);
            add_list_item("Smoothstep", KeyedAnimationTrack::Key::InterpolationType::SMOOTHSTEP);
            add_list_item("Spline", KeyedAnimationTrack::Key::InterpolationType::SPLINE);
          }
          Imm::end_popup();

          return result;
        };

    i32 selected_track_i                = -1;
    KeyedAnimationTrack *selected_track = nullptr;
    if (selected_entity_i > -1 && editor_scene.current_sequence) {
      for (u32 i = 0; i < editor_scene.current_sequence->tracks.count; i++) {
        if (editor_scene.current_sequence->tracks[i].entity_id == selected_entity_i) {
          selected_track_i = i;
          selected_track   = &editor_scene.current_sequence->tracks[i];
          break;
        }
      }
    }

    static DynamicArray<DynamicArray<i32>> selected_keys(&assets_temp_allocator);
    selected_keys.clear();
    if (editor_scene.current_sequence) {
      for (u32 i = 0; i < editor_scene.current_sequence->tracks.count; i++) {
        selected_keys.push_back({&assets_temp_allocator});
      }
    }

    b8 commit_move = false;
    i32 move       = 0;

    Imm::start_window("Timeline", {1000, 500, 600, 400});

    Imm::vertical_layout = false;
    if (editor_scene.current_sequence) {
      if (Imm::button("Play")) {
        if (!editor_scene.playing_sequence)
          editor_scene.play_sequence();
        else
          editor_scene.stop_sequence();
      }
      if (selected_track) {
        if (Imm::button(Imm::hash("Add Key From Camera Transform"),
                        "Add Key From Camera Transform")) {
          Transform t;
          Camera *camera = get_camera(&editor_scene, &t);
          selected_track->add_key(t, editor_scene.get_frame(),
                                  KeyedAnimationTrack::Key::InterpolationType::LINEAR);
        }
        if (Imm::button(Imm::hash("asdfcvbvbvx"), "Add Key From Entity Transform")) {
          Entity *entity = editor_scene.get(selected_track->entity_id);
          Transform t    = entity->transform;
          selected_track->add_key(t, editor_scene.get_frame(),
                                  KeyedAnimationTrack::Key::InterpolationType::LINEAR);
        }
      } else if (selected_entity) {
        if (Imm::button(Imm::hash("Add Track"), "Add Track")) {
          editor_scene.current_sequence->add_track(selected_entity_i);
        }
      }
    }
    Imm::vertical_layout = true;

    if (selected_track && selected_track->keys.count) {
      Transform tr = editor_scene.current_sequence->eval(selected_track_i, 0);
      for (f32 t = 0; t < 50.f; t += 0.01f) {
        Transform trn = editor_scene.current_sequence->eval(selected_track_i, t);
        // lines.append(tr.position.x);
        // lines.append(tr.position.y);
        // lines.append(tr.position.z);
        // lines.append(0);
        // lines.append(1);
        // lines.append(0);
        // lines.append(1);
        // lines.append(trn.position.x);
        // lines.append(trn.position.y);
        // lines.append(trn.position.z);
        // lines.append(0);
        // lines.append(1);
        // lines.append(0);
        // lines.append(1);
        tr = trn;
      }
    }

    static f32 column_width = 200.f;
    Imm::first_column(&column_width);

    if (editor_scene.current_sequence) {
      for (i32 track_i = 0; track_i < editor_scene.current_sequence->tracks.count;
           track_i++) {
        KeyedAnimationTrack *track = &editor_scene.current_sequence->tracks[track_i];
        Imm::list_item((ImmId)track, editor_scene.get(track->entity_id)->debug_tag.name,
                       track_i == selected_track_i);
        if (Imm::state.just_activated == (ImmId)track) {
          selected_entity_i = track->entity_id;
        }
      }
    }

    if (editor_scene.current_sequence)
      Imm::label(
          String::from(editor_scene.current_sequence->end_frame, &assets_temp_allocator));
    Imm::next_column(nullptr);

    static f32 timeline_start = 0;
    static f32 timeline_width = 60;
    i32 current_frame = editor_scene.current_sequence ? editor_scene.get_frame() : -1;
    Imm::start_timeline(
        "timeline", &current_frame, &timeline_start, &timeline_width,
        editor_scene.current_sequence ? &editor_scene.current_sequence->start_frame
                                             : nullptr,
        editor_scene.current_sequence ? &editor_scene.current_sequence->end_frame
                                             : nullptr);

    if (Imm::state.was_last_item_right_clicked) {
      Imm::open_popup("timeline_group_edit", input->mouse_pos);
    }

    if (editor_scene.current_sequence) {
      for (i32 track_i = 0; track_i < editor_scene.current_sequence->tracks.count;
           track_i++) {
        KeyedAnimationTrack &track = editor_scene.current_sequence->tracks[track_i];
        for (i32 i = 0; i < track.keys.count; i++) {
          i32 frame = track.keys[i].frame;
          if (Imm::keyframe((ImmId)&track.keys[i], &frame, track_i,
                            type_to_shape(track.keys[i].interpolation_type))) {
            selected_keys[track_i].push_back(i);

            if (frame != track.keys[i].frame) {
              move = frame - track.keys[i].frame;
            }

            if (Imm::state.just_stopped_dragging == (ImmId)&track.keys[i]) {
              commit_move = true;
            }

            if (Imm::state.was_last_item_right_clicked) {
              Imm::open_popup(Imm::hash("timeline_group_edit"), input->mouse_pos);
            }
          } else {
            if (Imm::state.was_last_item_right_clicked) {
              Imm::open_popup((ImmId)&track.keys[i], input->mouse_pos);
            }
            auto new_interp_type =
                keyframe_edit_popup((ImmId)&track.keys[i], track.keys[i].interpolation_type);
            if (new_interp_type.exists) {
              track.keys[i].interpolation_type = new_interp_type.value;
            }
          }
        }
      }
    }
    Imm::end_timeline();
    Imm::end_columns();
    Imm::end_window();

    // static f32 additive_t = 60.f;
    // static f32 blend      = 0.f;
    // additive_t += .25f;
    // if (additive_t > 90) {
    //   additive_t = 60 + (additive_t - 90);
    // }
    // Pose right_base = editor_scene.game.player_controller.right_anim.eval(additive_t);
    // Pose additive = editor_scene.game.player_controller.left_and_nod.eval_as_additive(additive_t);

    // editor_scene.game.player_controller.right = additive_blend(&right_base, &additive, blend);
    // editor_scene.game.player_controller.right.calculate_final_mats();

    // applying animation
    if (editor_scene.current_sequence && current_frame != editor_scene.get_frame()) {
      editor_scene.set_frame(current_frame);
      editor_scene.apply_keyed_animation(editor_scene.current_sequence,
                                                (i32)editor_scene.get_frame());

      // blend = editor_scene.get_frame() / 100.f - 1;
    }

    KeyedAnimation *ka = editor_scene.current_sequence;

    // right click popup
    Optional<KeyedAnimationTrack::Key::InterpolationType> interp_type_consensus = Optional<>::empty;
    for (i32 i = 0; i < selected_keys.count; i++) {
      for (i32 j = 0; j < selected_keys[i].count; j++) {
        KeyedAnimationTrack::Key *key = &ka->tracks[i].keys[selected_keys[i][j]];
        if (!interp_type_consensus.exists) {
          interp_type_consensus = interp_type_consensus.of(key->interpolation_type);
        } else if (key->interpolation_type != interp_type_consensus.value) {
          interp_type_consensus = Optional<>::empty;
          break;
        }
      }
    }
    auto new_interp_type =
        keyframe_edit_popup(Imm::hash("timeline_group_edit"), interp_type_consensus);
    if (new_interp_type.exists) {
      for (i32 i = 0; i < selected_keys.count; i++) {
        for (i32 j = 0; j < selected_keys[i].count; j++) {
          KeyedAnimationTrack::Key *key = &ka->tracks[i].keys[selected_keys[i][j]];
          key->interpolation_type       = new_interp_type.value;
        }
      }
    }

    // moving keys
    if (move) {
      for (i32 i = 0; i < selected_keys.count; i++) {
        for (i32 j = 0; j < selected_keys[i].count; j++) {
          KeyedAnimationTrack::Key *key = &ka->tracks[i].keys[selected_keys[i][j]];
          key->frame += move;
        }
      }
    }
    if (commit_move) {
      Imm::selected_keyframes.clear();

      for (i32 i = 0; i < selected_keys.count; i++) {
        for (i32 j = 0; j < selected_keys[i].count; j++) {
          KeyedAnimationTrack::Key tmp = ka->tracks[i].keys[selected_keys[i][j]];
          ka->tracks[i].keys.remove(selected_keys[i][j]);
          u32 new_i = ka->tracks[i].add_key(tmp);
          Imm::selected_keyframes.insert((ImmId)&ka->tracks[i].keys[new_i]);
        }
      }
    }

    // delete
    if (input->key_down_events[(i32)Keys::DEL] && selected_keys.count) {
      for (i32 i = 0; i < selected_keys.count; i++) {
        for (i32 j = selected_keys[i].count - 1; j >= 0; j--) {
          KeyedAnimationTrack::Key *key = &ka->tracks[i].keys[selected_keys[i][j]];
          ka->tracks[i].keys.remove(selected_keys[i][j]);
        }
      }

      Imm::selected_keyframes.clear();
    }

    // copy paste
    static DynamicArray<DynamicArray<i32>> key_clipboard(&assets_allocator);
    if (input->keys[(i32)Keys::LCTRL] && input->key_down_events[(i32)Keys::C] &&
        selected_keys.count) {
      // TODO: this is a memory leak
      key_clipboard.clear();
      for (i32 i = 0; i < selected_keys.count; i++) {
        key_clipboard.push_back({&assets_allocator});
        for (i32 j = 0; j < selected_keys[i].count; j++) {
          key_clipboard[i].push_back(selected_keys[i][j]);
        }
      }
    }

    if (input->keys[(i32)Keys::LCTRL] && input->key_down_events[(i32)Keys::V] &&
        key_clipboard.count) {
      Imm::selected_keyframes.clear();

      i32 base_frame = INT_MAX;
      for (i32 i = 0; i < key_clipboard.count; i++) {
        for (i32 j = 0; j < key_clipboard[i].count; j++) {
          i32 first_copied_frame_in_track = ka->tracks[i].keys[key_clipboard[i][0]].frame;
          if (first_copied_frame_in_track < base_frame) {
            base_frame = first_copied_frame_in_track;
          }
        }
      }

      for (i32 i = 0; i < key_clipboard.count; i++) {
        for (i32 j = 0; j < key_clipboard[i].count; j++) {
          KeyedAnimationTrack::Key tmp = ka->tracks[i].keys[key_clipboard[i][j]];
          tmp.frame                    = current_frame + (tmp.frame - base_frame);
          i32 new_i                    = ka->tracks[i].add_key(tmp);

          Imm::selected_keyframes.insert((ImmId)&ka->tracks[i].keys[new_i]);
        }
      }
    }
  }
};