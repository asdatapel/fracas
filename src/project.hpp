#pragma once

#include "platform.hpp"
#include "scene/scene.hpp"
#include "renderer/view_layer.hpp"
#include "util.hpp"
#include "yaml.hpp"

struct Project {
  Array<String, 2> asset_files = {};
  Array<String, 1> scene_files = {};
  String renderer_file = {};
  String scripts_file          = {};
};


void load_assets_file(String filename, Assets *assets_o)
{
  Temp temp = Temp::start(&assets_temp_allocator);

  FileData file    = read_entire_file(filename, temp);
  YAML::Dict *root = YAML::deserialize(String(file.data, file.length), temp)->as_dict();

  if (auto in_meshes_val = root->get("meshes")) {
    YAML::List *in_meshes = in_meshes_val->as_list();
    for (int i = 0; i < in_meshes->len; i++) {
      YAML::Dict *in_mesh = in_meshes->get(i)->as_dict();
      int id              = atoi(in_mesh->get("id")->as_literal().to_char_array(temp));
      String path         = in_mesh->get("path")->as_literal();

      VertexBuffer mesh = load_and_upload_mesh(path, id, assets_memory);
      mesh.asset_id     = id;
      mesh.asset_name   = String::copy(path, &assets_allocator);
      assets_o->meshes.emplace(mesh, id);
    }
  }

  if (auto in_render_targets_val = root->get("render_targets")) {
    YAML::List *in_render_targets = in_render_targets_val->as_list();
    for (int i = 0; i < in_render_targets->len; i++) {
      YAML::Dict *in_render_target = in_render_targets->get(i)->as_dict();
      int id = atoi(in_render_target->get("id")->as_literal().to_char_array(temp));

      String color_format_string = in_render_target->get("color_format")->as_literal();
      String depth_format_string = in_render_target->get("depth_format")->as_literal();
      TextureFormat color_format = texture_format_from_string(color_format_string);
      TextureFormat depth_format = texture_format_from_string(depth_format_string);
      int width  = atoi(in_render_target->get("width")->as_literal().to_char_array(temp));
      int height = atoi(in_render_target->get("height")->as_literal().to_char_array(temp));

      RenderTarget target = RenderTarget(width, height, color_format, depth_format);
      target.asset_id     = id;
      target.asset_name =
          String::copy(in_render_target->get("name")->as_literal(), &assets_allocator);
      assets_o->render_targets.emplace(target, id);
    }
  }

  if (auto in_textures_val = root->get("textures")) {
    YAML::List *in_textures = in_textures_val->as_list();
    for (int i = 0; i < in_textures->len; i++) {
      YAML::Dict *in_texture = in_textures->get(i)->as_dict();
      int id                 = atoi(in_texture->get("id")->as_literal().to_char_array(temp));

      Texture texture;
      if (YAML::Value *path_val = in_texture->get("path")) {
        String path          = path_val->as_literal();
        String format_string = in_texture->get("format")->as_literal();
        TextureFormat format = texture_format_from_string(format_string);
        texture              = load_and_upload_texture(path, format, assets_memory);
      } else if (YAML::Value *render_target_val = in_texture->get("render_target")) {
        int render_target_id = atoi(render_target_val->as_literal().to_char_array(temp));
        texture              = assets_o->render_targets.data[render_target_id].value.color_tex;
      } else {
        assert(false);
      }

      assets_o->textures.emplace(texture, id);
    }
  }
  
  if (auto in_env_maps_val = root->get("env_maps")) {
    YAML::List *in_env_maps = in_env_maps_val->as_list();
    for (int i = 0; i < in_env_maps->len; i++) {
      YAML::Dict *in_env_map = in_env_maps->get(i)->as_dict();
      int id                 = atoi(in_env_map->get("id")->as_literal().to_char_array(temp));
      
      String path = in_env_map->get("path")->as_literal();
      RenderTarget temp_target(0, 0, TextureFormat::NONE, TextureFormat::NONE);
      Texture hdri_tex   = load_hdri(path, assets_memory);
      
      EnvMap env_map;
      env_map.unfiltered_cubemap = hdri_to_cubemap(temp_target, hdri_tex, 1024);
      env_map.env_mat            = create_env_mat(temp_target, env_map.unfiltered_cubemap);

      assets_o->env_maps.emplace(env_map, id);
    }
  }

  if (auto in_materials_val = root->get("materials")) {
    YAML::List *in_materials = in_materials_val->as_list();
    for (int i = 0; i < in_materials->len; i++) {
      YAML::Dict *in_material = in_materials->get(i)->as_dict();
      int id                  = atoi(in_material->get("id")->as_literal().to_char_array(temp));

      int num_parameters = 0;
      if (auto num_parameters_val = in_material->get("num_parameters")) {
        num_parameters = atoi(num_parameters_val->as_literal().to_char_array(temp));
      }

      YAML::List *texture_refs = in_material->get("textures")->as_list();
      Material material = Material::allocate(texture_refs->len, num_parameters, &assets_allocator);
      material.asset_id = id;
      for (int tex_i = 0; tex_i < texture_refs->len; tex_i++) {
        int texture_ref_id       = atoi(texture_refs->get(tex_i)->as_literal().to_char_array(temp));
        material.textures[tex_i] = assets_o->textures.data[texture_ref_id].value;
      }

      assets_o->materials.emplace(material, id);
    }
  }

  if (auto in_shaders_val = root->get("shaders")) {
    YAML::List *in_shaders = in_shaders_val->as_list();
    for (int i = 0; i < in_shaders->len; i++) {
      YAML::Dict *in_shader = in_shaders->get(i)->as_dict();
      int id                = atoi(in_shader->get("id")->as_literal().to_char_array(temp));
      String name           = in_shader->get("name")->as_literal();
      String vert_path      = in_shader->get("vert")->as_literal();
      String frag_path      = in_shader->get("frag")->as_literal();

      auto vert_src = read_entire_file(vert_path.to_char_array(temp), temp);
      auto frag_src = read_entire_file(frag_path.to_char_array(temp), temp);

      Shader shader =
          create_shader({vert_src.data, (uint16_t)vert_src.length},
                        {frag_src.data, (uint16_t)frag_src.length}, name.to_char_array(temp));
      shader.asset_id = id;
      assets_o->shaders.emplace(shader, id);
    }
  }

  if (auto in_fonts_val = root->get("fonts")) {
    YAML::List *in_fonts = in_fonts_val->as_list();
    for (int i = 0; i < in_fonts->len; i++) {
      YAML::Dict *in_font = in_fonts->get(i)->as_dict();
      int id              = atoi(in_font->get("id")->as_literal().to_char_array(temp));
      String path         = in_font->get("path")->as_literal();

      FileData file = read_entire_file(path.to_char_array(temp), &assets_allocator);
      assets_o->font_files.emplace(file, id);
    }
  }

  if (auto in_keyed_animations_val = root->get("keyed_animations")) {
    YAML::List *in_keyed_animations = in_keyed_animations_val->as_list();
    for (u32 i = 0; i < in_keyed_animations->len; i++) {
      YAML::Dict *in_keyed_animation = in_keyed_animations->get(i)->as_dict();

      KeyedAnimation ka(0);

      ka.asset_id = atoi(in_keyed_animation->get("asset_id")->as_literal().to_char_array(temp));
      ka.asset_name =
          String::copy(in_keyed_animation->get("asset_name")->as_literal(), &assets_allocator);
      ka.fps = atoi(in_keyed_animation->get("fps")->as_literal().to_char_array(temp));

      ka.start_frame =
          atoi(in_keyed_animation->get("start_frame")->as_literal().to_char_array(temp));
      ka.end_frame = atoi(in_keyed_animation->get("end_frame")->as_literal().to_char_array(temp));

      auto tracks_in = in_keyed_animation->get("tracks")->as_list();
      for (u32 track_i = 0; track_i < tracks_in->len; track_i++) {
        auto track_in = tracks_in->get(track_i)->as_dict();

        KeyedAnimationTrack track;

        track.entity_id = atoi(track_in->get("entity_id")->as_literal().to_char_array(temp));

        auto keys_in = track_in->get("keys")->as_list();
        for (u32 key_i = 0; key_i < keys_in->len; key_i++) {
          auto key_in = keys_in->get(key_i)->as_dict();

          KeyedAnimationTrack::Key key;

          YAML::Dict *in_transform = key_in->get("transform")->as_dict();
          YAML::Dict *in_position  = in_transform->get("position")->as_dict();
          key.transform.position.x = atof(in_position->get("x")->as_literal().to_char_array(temp));
          key.transform.position.y = atof(in_position->get("y")->as_literal().to_char_array(temp));
          key.transform.position.z = atof(in_position->get("z")->as_literal().to_char_array(temp));
          YAML::Dict *in_rotation  = in_transform->get("rotation")->as_dict();
          key.transform.rotation.x = atof(in_rotation->get("x")->as_literal().to_char_array(temp));
          key.transform.rotation.y = atof(in_rotation->get("y")->as_literal().to_char_array(temp));
          key.transform.rotation.z = atof(in_rotation->get("z")->as_literal().to_char_array(temp));
          YAML::Dict *in_scale     = in_transform->get("scale")->as_dict();
          key.transform.scale.x    = atof(in_scale->get("x")->as_literal().to_char_array(temp));
          key.transform.scale.y    = atof(in_scale->get("y")->as_literal().to_char_array(temp));
          key.transform.scale.z    = atof(in_scale->get("z")->as_literal().to_char_array(temp));

          key.interpolation_type = (KeyedAnimationTrack::Key::InterpolationType)atoi(
              key_in->get("interpolation_type")->as_literal().to_char_array(temp));
          key.frame = atoi(key_in->get("frame")->as_literal().to_char_array(temp));

          track.keys.push_back(key);
        }

        ka.tracks.push_back(track);
      }

      assets_o->keyed_animations.emplace(ka, ka.asset_id);
    }
  }
}

void load_assets(Project project, Assets *assets_o)
{
  for (i32 i = 0; i < project.asset_files.len; i++) {
    load_assets_file(project.asset_files[i], assets_o);
  }
}

void deserialize_scene_file(String filepath, Assets *assets, Memory mem, Scene *scene_o)
{
  Temp tmp = Temp::start(mem);

  FileData file    = read_entire_file(filepath, tmp);
  YAML::Dict *root = YAML::deserialize(String(file.data, file.length), tmp)->as_dict();

  YAML::List *in_entities = root->get("entities")->as_list();
  for (int i = 0; i < in_entities->len; i++) {
    YAML::Dict *in_e = in_entities->get(i)->as_dict();

    int id                             = atoi(in_e->get("id")->as_literal().to_char_array(tmp));
    Entity &entity                     = scene_o->entities.data[id].value;
    scene_o->entities.data[i].assigned = true;
    if (scene_o->entities.next == &scene_o->entities.data[i]) {
      scene_o->entities.next = scene_o->entities.data[i].next;
    }

    entity.type           = entity_type_from_string(in_e->get("type")->as_literal());
    entity.debug_tag.name = string_to_allocated_string<32>(in_e->get("name")->as_literal());
    entity.view_layer_mask =
        strtoul(in_e->get("view_layer")->as_literal().to_char_array(tmp), nullptr, 10);

    // transform
    YAML::Dict *in_transform    = in_e->get("transform")->as_dict();
    YAML::Dict *in_position     = in_transform->get("position")->as_dict();
    entity.transform.position.x = atof(in_position->get("x")->as_literal().to_char_array(tmp));
    entity.transform.position.y = atof(in_position->get("y")->as_literal().to_char_array(tmp));
    entity.transform.position.z = atof(in_position->get("z")->as_literal().to_char_array(tmp));
    YAML::Dict *in_rotation     = in_transform->get("rotation")->as_dict();
    entity.transform.rotation.x = atof(in_rotation->get("x")->as_literal().to_char_array(tmp));
    entity.transform.rotation.y = atof(in_rotation->get("y")->as_literal().to_char_array(tmp));
    entity.transform.rotation.z = atof(in_rotation->get("z")->as_literal().to_char_array(tmp));
    YAML::Dict *in_scale        = in_transform->get("scale")->as_dict();
    entity.transform.scale.x    = atof(in_scale->get("x")->as_literal().to_char_array(tmp));
    entity.transform.scale.y    = atof(in_scale->get("y")->as_literal().to_char_array(tmp));
    entity.transform.scale.z    = atof(in_scale->get("z")->as_literal().to_char_array(tmp));

    if (entity.type == EntityType::MESH) {
      YAML::Dict *in_mesh = in_e->get("mesh")->as_dict();
      int mesh_id         = atoi(in_mesh->get("mesh")->as_literal().to_char_array(tmp));
      int material_id     = atoi(in_mesh->get("material")->as_literal().to_char_array(tmp));

      entity.vert_buffer = assets->meshes.data[mesh_id].value;
      entity.material    = &assets->materials.data[material_id].value;

      if (auto shader_id_val = in_mesh->get("shader")) {
        int shader_id = atoi(in_mesh->get("shader")->as_literal().to_char_array(tmp));
        entity.shader = &assets->shaders.data[shader_id].value;
      } else {
        entity.shader = &assets->shaders.data[0].value;
      }
    } else if (entity.type == EntityType::LIGHT) {
      YAML::Dict *in_light      = in_e->get("spotlight")->as_dict();
      YAML::Dict *in_color      = in_light->get("color")->as_dict();
      entity.spot_light.color.x = atof(in_color->get("x")->as_literal().to_char_array(tmp));
      entity.spot_light.color.y = atof(in_color->get("y")->as_literal().to_char_array(tmp));
      entity.spot_light.color.z = atof(in_color->get("z")->as_literal().to_char_array(tmp));

      entity.spot_light.inner_angle =
          atof(in_light->get("inner_angle")->as_literal().to_char_array(tmp));
      entity.spot_light.outer_angle =
          atof(in_light->get("outer_angle")->as_literal().to_char_array(tmp));
    } else if (entity.type == EntityType::SPLINE) {
      YAML::List *points       = in_e->get("spline")->as_list();
      entity.spline.points.len = 0;
      for (int p = 0; p < points->len; p++) {
        Vec3f point;
        YAML::Dict *in_p = points->get(p)->as_dict();
        point.x          = atof(in_p->get("x")->as_literal().to_char_array(tmp));
        point.y          = atof(in_p->get("y")->as_literal().to_char_array(tmp));
        point.z          = atof(in_p->get("z")->as_literal().to_char_array(tmp));
        entity.spline.points.append(point);
      }
    }
  }
}

void deserialize_scene(Project project, Assets *assets, Memory mem, Scene *scene_o)
{
  for (i32 i = 0; i < project.scene_files.len; i++) {
    deserialize_scene_file(project.scene_files[i], assets, mem, scene_o);
  }
}

RefArray<ViewLayer> deserialize_renderer_config(String filepath, Assets *assets, Scene *scene, Memory mem) {
  Temp tmp = Temp::start(mem);

  FileData file    = read_entire_file(filepath, tmp);
  YAML::Dict *root = YAML::deserialize(String(file.data, file.length), tmp)->as_dict();
  YAML::List *in_layers = root->get("view_layers")->as_list();

  RefArray<ViewLayer> layers;
  layers.len = in_layers->len;
  layers.data = (ViewLayer*) mem.allocator->alloc(in_layers->len * sizeof(ViewLayer));

  for (i32 i = 0; i < layers.len; i++) {
    ViewLayer *view_layer = &layers[i];
    new (view_layer) ViewLayer();

    YAML::Dict *in_layer = in_layers->get(i)->as_dict();

    i32 env_map_id = atoi(in_layer->get("env_map")->as_literal().to_char_array(mem.temp));
    view_layer->env_map = &assets->env_maps.data[env_map_id].value;
    view_layer->visiblity_mask = 1 << atoi(in_layer->get("layer_index")->as_literal().to_char_array(mem.temp));
    view_layer->visible =  strcmp(in_layer->get("visible")->as_literal(), "true");

    view_layer->active_camera_id = atoi(in_layer->get("active_camera_id")->as_literal().to_char_array(mem.temp));
    view_layer->cubemap_visible =
        in_layer->get("cubemap_visible") && strcmp(in_layer->get("cubemap_visible")->as_literal(), "true");

    if (auto planar_reflector_val = in_layer->get("planar_reflector")) {
      YAML::Dict *planar_reflector = planar_reflector_val->as_dict();
      int planar_reflector_entity_id =
          atoi(planar_reflector->get("entity_id")->as_literal().to_char_array(mem.temp));
      int render_target_id =
          atoi(planar_reflector->get("render_target")->as_literal().to_char_array(mem.temp));

      view_layer->render_planar = true;
      view_layer->planar_entity = scene->get(planar_reflector_entity_id);
      view_layer->planar_target = assets->render_targets.data[render_target_id].value;
    }
  }

  return layers;
}

void deserialize_project(Project project, Memory mem, Assets *assets_o, Scene *scene_o, RefArray<ViewLayer> *renderer_o)
{
  load_assets(project, assets_o);
  deserialize_scene(project, assets_o, mem, scene_o);
  *renderer_o = deserialize_renderer_config(project.renderer_file, assets_o, scene_o, mem);
}
