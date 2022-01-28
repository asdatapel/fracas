#include "scene.hpp"

#include <string>

#include <stb/stb_image.hpp>

#include "../assets.hpp"
#include "../camera.hpp"
#include "../graphics/bloomer.hpp"
#include "../graphics/framebuffer.hpp"
#include "../graphics/graphics.hpp"
#include "../material.hpp"
#include "../yaml.hpp"

const char *debug_hdr = "resources/hdri/Newport_Loft_Ref.hdr";
Texture2D load_hdri(const char *file) {
  int width, height, components;
  stbi_set_flip_vertically_on_load(true);
  float *hdri = stbi_loadf(file, &width, &height, &components, 0);

  Texture2D tex(width, height, TextureFormat::RGB16F, true);
  tex.upload(hdri, true);

  stbi_image_free(hdri);

  return tex;
}

StandardPbrEnvMaterial create_env_mat(RenderTarget temp_target, Texture unfiltered_cubemap) {
  Texture irradiance_map = convolve_irradiance_map(temp_target, unfiltered_cubemap, 32);
  Texture env_map        = filter_env_map(temp_target, unfiltered_cubemap, 512);

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

// void draw_bar_overlay(Scene *scene, RenderTarget previous_target, int index, String answer, int
// score)
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
//         draw_centered_text(scene->font, target, answer, sub_target, target_border, text_scale,
//         aspect_ratio);
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
//         draw_centered_text(scene->font, target, text, sub_target, border, text_scale,
//         aspect_ratio);
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

//         Rect rect = {left * target.width, top * target.height, target.width * width,
//         target.height * height * aspect_ratio}; draw_textured_rect(target, rect, {}, num_tex);
//     }

//     target.color_tex.gen_mipmaps();
//     previous_target.bind();
// }

Entity *Scene::get(int id) {
  return entities.data[id].assigned ? &entities.data[id].value : nullptr;
}

void Scene::init(Memory mem, TextureFormat texture_format) {
  entities.init(mem.allocator, 1024);
  target = RenderTarget(1920, 1080, texture_format, TextureFormat::DEPTH24);

  new (&saved_transforms) DynamicArray<EntityTransform, 128>(&scene_allocator);
}

void Scene::load(const char *filename, Assets *assets, Memory mem) {
  FileData file    = read_entire_file(filename, mem.temp);
  YAML::Dict *root = YAML::deserialize(String(file.data, file.length), mem.temp)->as_dict();

  YAML::List *in_entities = root->get("entities")->as_list();
  for (int i = 0; i < in_entities->len; i++) {
    YAML::Dict *in_e = in_entities->get(i)->as_dict();

    int id                    = atoi(in_e->get("id")->as_literal().to_char_array(mem.temp));
    Entity &entity            = entities.data[id].value;
    entities.data[i].assigned = true;
    if (entities.next == &entities.data[i]) {
      entities.next = entities.data[i].next;
    }

    entity.type           = entity_type_from_string(in_e->get("type")->as_literal());
    entity.debug_tag.name = string_to_allocated_string<32>(in_e->get("name")->as_literal());

    // transform
    YAML::Dict *in_transform    = in_e->get("transform")->as_dict();
    YAML::Dict *in_position     = in_transform->get("position")->as_dict();
    entity.transform.position.x = atof(in_position->get("x")->as_literal().to_char_array(mem.temp));
    entity.transform.position.y = atof(in_position->get("y")->as_literal().to_char_array(mem.temp));
    entity.transform.position.z = atof(in_position->get("z")->as_literal().to_char_array(mem.temp));
    YAML::Dict *in_rotation     = in_transform->get("rotation")->as_dict();
    entity.transform.rotation.x = atof(in_rotation->get("x")->as_literal().to_char_array(mem.temp));
    entity.transform.rotation.y = atof(in_rotation->get("y")->as_literal().to_char_array(mem.temp));
    entity.transform.rotation.z = atof(in_rotation->get("z")->as_literal().to_char_array(mem.temp));
    YAML::Dict *in_scale        = in_transform->get("scale")->as_dict();
    entity.transform.scale.x    = atof(in_scale->get("x")->as_literal().to_char_array(mem.temp));
    entity.transform.scale.y    = atof(in_scale->get("y")->as_literal().to_char_array(mem.temp));
    entity.transform.scale.z    = atof(in_scale->get("z")->as_literal().to_char_array(mem.temp));

    if (entity.type == EntityType::MESH) {
      YAML::Dict *in_mesh = in_e->get("mesh")->as_dict();
      int mesh_id         = atoi(in_mesh->get("mesh")->as_literal().to_char_array(mem.temp));
      int material_id     = atoi(in_mesh->get("material")->as_literal().to_char_array(mem.temp));

      entity.vert_buffer = assets->meshes.data[mesh_id].value;
      entity.material    = &assets->materials.data[material_id].value;

      if (auto shader_id_val = in_mesh->get("shader")) {
        int shader_id = atoi(in_mesh->get("shader")->as_literal().to_char_array(mem.temp));
        entity.shader = &assets->shaders.data[shader_id].value;
      } else {
        entity.shader = &assets->shaders.data[0].value;
      }
    } else if (entity.type == EntityType::LIGHT) {
      YAML::Dict *in_light      = in_e->get("spotlight")->as_dict();
      YAML::Dict *in_color      = in_light->get("color")->as_dict();
      entity.spot_light.color.x = atof(in_color->get("x")->as_literal().to_char_array(mem.temp));
      entity.spot_light.color.y = atof(in_color->get("y")->as_literal().to_char_array(mem.temp));
      entity.spot_light.color.z = atof(in_color->get("z")->as_literal().to_char_array(mem.temp));

      entity.spot_light.inner_angle =
          atof(in_light->get("inner_angle")->as_literal().to_char_array(mem.temp));
      entity.spot_light.outer_angle =
          atof(in_light->get("outer_angle")->as_literal().to_char_array(mem.temp));
    } else if (entity.type == EntityType::SPLINE) {
      YAML::List *points       = in_e->get("spline")->as_list();
      entity.spline.points.len = 0;
      for (int p = 0; p < points->len; p++) {
        Vec3f point;
        YAML::Dict *in_p = points->get(p)->as_dict();
        point.x          = atof(in_p->get("x")->as_literal().to_char_array(mem.temp));
        point.y          = atof(in_p->get("y")->as_literal().to_char_array(mem.temp));
        point.z          = atof(in_p->get("z")->as_literal().to_char_array(mem.temp));
        entity.spline.points.append(point);
      }
    }
  }

  String hdr_path = root->get("hdr")->as_literal();
  RenderTarget temp_target(0, 0, TextureFormat::NONE, TextureFormat::NONE);
  Texture hdri_tex   = load_hdri(hdr_path.to_char_array(mem.temp));
  unfiltered_cubemap = hdri_to_cubemap(temp_target, hdri_tex, 1024);
  env_mat            = create_env_mat(temp_target, unfiltered_cubemap);

  active_camera_id = atoi(root->get("active_camera_id")->as_literal().to_char_array(mem.temp));
  cubemap_visible =
      root->get("cubemap_visible") && strcmp(root->get("cubemap_visible")->as_literal(), "true");

  if (auto planar_reflector_val = root->get("planar_reflector")) {
    YAML::Dict *planar_reflector = planar_reflector_val->as_dict();
    int planar_reflector_entity_id =
        atoi(planar_reflector->get("entity_id")->as_literal().to_char_array(mem.temp));
    int render_target_id =
        atoi(planar_reflector->get("render_target")->as_literal().to_char_array(mem.temp));

    render_planar = true;
    planar_entity = get(planar_reflector_entity_id);
    planar_target = assets->render_targets.data[render_target_id].value;
  }
}

void Scene::serialize(const char *filename, Assets *assets, StackAllocator *alloc) {
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
  YAML::List entities_yaml;
  scene_yaml.push_back("entities", &entities_yaml, alloc);

  for (int i = 0; i < entities.size; i++) {
    if (entities.data[i].assigned) {
      Entity *e               = &entities.data[i].value;
      YAML::Dict *entity_yaml = new_dict();
      entity_yaml->push_back("id", new_literal(String::from(i, alloc)), alloc);
      entity_yaml->push_back("name", new_literal(e->debug_tag.name), alloc);
      entity_yaml->push_back("type", new_literal(to_string(e->type)), alloc);

      YAML::Dict *transform_yaml = new_dict();
      YAML::Dict *position_yaml  = new_dict();
      position_yaml->push_back("x", new_literal(String::from(e->transform.position.x, alloc)),
                               alloc);
      position_yaml->push_back("y", new_literal(String::from(e->transform.position.y, alloc)),
                               alloc);
      position_yaml->push_back("z", new_literal(String::from(e->transform.position.z, alloc)),
                               alloc);
      YAML::Dict *rotation_yaml = new_dict();
      rotation_yaml->push_back("x", new_literal(String::from(e->transform.rotation.x, alloc)),
                               alloc);
      rotation_yaml->push_back("y", new_literal(String::from(e->transform.rotation.y, alloc)),
                               alloc);
      rotation_yaml->push_back("z", new_literal(String::from(e->transform.rotation.z, alloc)),
                               alloc);
      YAML::Dict *scale_yaml = new_dict();
      scale_yaml->push_back("x", new_literal(String::from(e->transform.scale.x, alloc)), alloc);
      scale_yaml->push_back("y", new_literal(String::from(e->transform.scale.y, alloc)), alloc);
      scale_yaml->push_back("z", new_literal(String::from(e->transform.scale.z, alloc)), alloc);
      transform_yaml->push_back("position", position_yaml, alloc);
      transform_yaml->push_back("rotation", rotation_yaml, alloc);
      transform_yaml->push_back("scale", scale_yaml, alloc);
      entity_yaml->push_back("transform", transform_yaml, alloc);

      if (e->type == EntityType::MESH) {
        YAML::Dict *mesh_yaml = new_dict();
        mesh_yaml->push_back("mesh", new_literal(String::from(e->vert_buffer.asset_id, alloc)),
                             alloc);
        mesh_yaml->push_back("material", new_literal(String::from(e->material->asset_id, alloc)),
                             alloc);
        mesh_yaml->push_back("shader", new_literal(String::from(e->shader->asset_id, alloc)),
                             alloc);
        entity_yaml->push_back("mesh", mesh_yaml, alloc);
      } else if (e->type == EntityType::LIGHT) {
        YAML::Dict *light_yaml = new_dict();

        YAML::Dict *color_yaml = new_dict();
        color_yaml->push_back("x", new_literal(String::from(e->spot_light.color.x, alloc)), alloc);
        color_yaml->push_back("y", new_literal(String::from(e->spot_light.color.y, alloc)), alloc);
        color_yaml->push_back("z", new_literal(String::from(e->spot_light.color.z, alloc)), alloc);
        light_yaml->push_back("color", color_yaml, alloc);

        light_yaml->push_back("inner_angle",
                              new_literal(String::from(e->spot_light.inner_angle, alloc)), alloc);
        light_yaml->push_back("outer_angle",
                              new_literal(String::from(e->spot_light.outer_angle, alloc)), alloc);

        entity_yaml->push_back("spotlight", light_yaml, alloc);
      } else if (e->type == EntityType::SPLINE) {
        YAML::List *spline_yaml = new_list();
        for (int p = 0; p < e->spline.points.len; p++) {
          YAML::Dict *point_yaml = new_dict();
          point_yaml->push_back("x", new_literal(String::from(e->spline.points[p].x, alloc)),
                                alloc);
          point_yaml->push_back("y", new_literal(String::from(e->spline.points[p].y, alloc)),
                                alloc);
          point_yaml->push_back("z", new_literal(String::from(e->spline.points[p].z, alloc)),
                                alloc);
          spline_yaml->push_back(point_yaml, alloc);
        }
        entity_yaml->push_back("spline", spline_yaml, alloc);
      }
      entities_yaml.push_back(entity_yaml, alloc);
    }
  }

  scene_yaml.push_back("active_camera_id", new_literal(String::from(active_camera_id, alloc)),
                       alloc);
  scene_yaml.push_back("hdr", new_literal("resources/hdri/Newport_Loft_Ref.hdr"), alloc);
  scene_yaml.push_back("cubemap_visible",
                       new_literal(cubemap_visible ? (String) "true" : (String) "false"), alloc);

  YAML::Dict *planar_reflector_yaml = new_dict();
  planar_reflector_yaml->push_back(
      "entity_id", new_literal(String::from(entities.index_of(planar_entity), alloc)), alloc);
  planar_reflector_yaml->push_back("render_target",
                                   new_literal(String::from(planar_target.asset_id, alloc)), alloc);
  scene_yaml.push_back("planar_reflector", planar_reflector_yaml, alloc);

  String out;
  out.data = alloc->next;
  YAML::serialize(&scene_yaml, alloc, 0, false);
  out.len = alloc->next - out.data;

  write_file(filename, out);
}

void Scene::update(float timestep) {
  if (playing_sequence) {
    assert(current_sequence);
    apply_keyed_animation(current_sequence, sequence_t);
    sequence_t += timestep;
  }
}

void Scene::render(Camera *editor_camera, Vec3f editor_camera_pos) {
  // TODO handle target resize

  Camera *camera;
  Vec3f camera_pos;
  if (editor_camera) {
    camera     = editor_camera;
    camera_pos = editor_camera_pos;
  } else {
    if (active_camera_id < 0) {
      for (int i = 0; i < entities.size; i++) {
        if (entities.data[i].assigned) {
          Entity &e = entities.data[i].value;
          if (e.type == EntityType::CAMERA) {
            active_camera_id = i;
            break;
          }
        }
      }
    }
    if (active_camera_id < 0) {
      return;  // cant draw without camera
    }
    camera     = &entities.data[active_camera_id].value.camera;
    camera_pos = entities.data[active_camera_id].value.transform.position;
  }

  for (int i = 0; i < entities.size; i++) {
    if (entities.data[i].assigned) {
      Entity &e = entities.data[i].value;
      if (e.type == EntityType::CAMERA) {
        e.camera.update_from_transform(target, e.transform);
      }
    }
  }

  LightUniformBlock all_lights;
  all_lights.num_lights = 0;
  for (int i = 0; i < entities.size && all_lights.num_lights < MAX_LIGHTS; i++) {
    if (entities.data[i].assigned) {
      Entity &e = entities.data[i].value;
      if (e.type == EntityType::LIGHT) {
        SpotLight light;
        light.position = {e.transform.position.x, e.transform.position.y, e.transform.position.z};
        light.direction =
            glm::rotate(glm::quat(glm::vec3{e.transform.rotation.x, e.transform.rotation.y,
                                            e.transform.rotation.z}),
                        glm::vec3(0, -1, 0));
        light.color = glm::vec3{e.spot_light.color.x, e.spot_light.color.y, e.spot_light.color.z};
        light.outer_angle                             = e.spot_light.outer_angle;
        light.inner_angle                             = e.spot_light.inner_angle;
        all_lights.spot_lights[all_lights.num_lights] = light;
        all_lights.num_lights++;
      }
    }
  }
  upload_lights(all_lights);

  if (render_planar) {
    planar_target.bind();
    planar_target.clear();

    float planar_position_y = planar_entity->transform.position.y;

    // http://khayyam.kaplinski.com/2011/09/reflective-water-with-glsl-part-i.html
    glm::mat4 reflection_mat = {1, 0,  0, 0,                       //
                                0, -1, 0, -2 * planar_position_y,  //
                                0, 0,  1, 0,                       //
                                0, 0,  0, 1};

    Vec3f flipped_camera_pos = {camera_pos.x, (2 * planar_position_y) - camera_pos.y, camera_pos.z};
    Camera flipped_camera    = *camera;
    flipped_camera.view      = reflection_mat * camera->view * reflection_mat;

    render_entities(&flipped_camera, flipped_camera_pos);

    bind_shader(*planar_entity->shader);
    bind_mat4(*planar_entity->shader, UniformId::REFLECTED_PROJECTION,
              flipped_camera.perspective * flipped_camera.view);
  }

  target.bind();
  target.clear();
  render_entities(camera, camera_pos);

  if (cubemap_visible) {
    bind_shader(cubemap_shader);
    bind_mat4(cubemap_shader, UniformId::PROJECTION, camera->perspective);
    bind_mat4(cubemap_shader, UniformId::VIEW, camera->view);
    bind_texture(cubemap_shader, UniformId::ENV_MAP, unfiltered_cubemap);
    draw_cubemap();
  }
}

void Scene::set_planar_target(RenderTarget target) {
  render_planar = true;
  planar_target = target;
}

void Scene::render_entities(Camera *camera, Vec3f camera_postion) {
  for (int i = 0; i < entities.size; i++) {
    if (entities.data[i].assigned) {
      Entity &e = entities.data[i].value;

      if (e.type == EntityType::MESH) {
        glm::vec3 rot(e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z);
        glm::vec3 pos(e.transform.position.x, e.transform.position.y, e.transform.position.z);
        glm::vec3 scale(e.transform.scale.x, e.transform.scale.y, e.transform.scale.z);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.f), scale) *
                          glm::toMat4(glm::quat(rot));

        Shader shader = *e.shader;
        bind_shader(shader);
        bind_camera(shader, *camera, camera_postion);
        bind_mat4(shader, UniformId::MODEL, model);
        bind_material(shader, env_mat);
        bind_material(shader, *e.material, 3);

        if (e.animation) {
          for (int i = 0; i < e.animation->num_bones; i++) {
            // TODO shouldn't be querying location every time
            std::string uniform_name =
                std::string("bone_transforms[") + std::to_string(i) + std::string("]");
            int handle =
                glGetUniformLocation(threed_skinning_shader.shader_handle, uniform_name.c_str());
            glm::mat4 transform = e.animation->mats[i];
            glUniformMatrix4fv(handle, 1, false, &transform[0][0]);
          }
        }

        draw(target, shader, e.vert_buffer);
      }
    }
  }
}

void Scene::set_sequence(KeyedAnimation *seq) {
  if (current_sequence) {
    for (int i = 0; i < saved_transforms.count; i++) {
      EntityId id        = saved_transforms[i].id;
      get(id)->transform = saved_transforms[i].transform;
    }
    saved_transforms.count = 0;
  }

  if (seq) {
    for (int i = 0; i < seq->tracks.count; i++) {
      EntityId id = seq->tracks[i].entity_id;
      saved_transforms.push_back({id, get(id)->transform});
    }
  }

  current_sequence = seq;
}
void Scene::play_sequence() { playing_sequence = true; }
void Scene::stop_sequence() { playing_sequence = false; }
void Scene::set_t(f32 t) { sequence_t = t; }
void Scene::set_frame(u32 frame) {
  assert(current_sequence);
  sequence_t = (f32)frame / current_sequence->fps;
}
u32 Scene::get_frame() {
  assert(current_sequence);
  return sequence_t * current_sequence->fps;
}
void Scene::apply_keyed_animation(KeyedAnimation *keyed_anim, f32 t) {
  for (u32 i = 0; i < keyed_anim->tracks.count; i++) {
    KeyedAnimationTrack &track = keyed_anim->tracks[i];
    Entity *entity             = get(track.entity_id);

    entity->transform = track.eval(t, keyed_anim->fps);
  }
}
void Scene::apply_keyed_animation(KeyedAnimation *keyed_anim, i32 frame) {
  f32 t = (f32)frame / keyed_anim->fps;
  apply_keyed_animation(keyed_anim, t);
}