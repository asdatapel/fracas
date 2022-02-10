#pragma once

#include <array>
#include <map>
#include <unordered_map>
#include <vector>

#include "asset.hpp"
#include "font.hpp"
#include "global_allocators.hpp"
#include "graphics/graphics.hpp"
#include "keyed_animation.hpp"
#include "material.hpp"
#include "scene/entity.hpp"
#include "yaml.hpp"

const String RESOURCE_PATH = "resources/test";

Texture load_and_upload_texture(String filepath, TextureFormat format, Memory mem) {
  auto t = Temp::start(mem);

  char *filepath_chars = filepath.to_char_array(&assets_temp_allocator);
  FileData file        = read_entire_file(filepath_chars, &assets_temp_allocator);
  if (!file.length) {
    // TODO return default checkerboard texture
    return Texture{};
  }

  Bitmap bmp = parse_bitmap(file, &assets_temp_allocator);
  Texture2D tex(bmp.width, bmp.height, format, true);
  tex.upload((uint8_t *)bmp.data, true);
  return tex;
}

VertexBuffer load_and_upload_mesh(String filepath, int asset_id, Memory mem) {
  auto t = Temp::start(mem);

  char *filepath_chars = filepath.to_char_array(&assets_temp_allocator);
  FileData file        = read_entire_file(filepath_chars, &assets_temp_allocator);
  Mesh mesh            = load_fmesh(file, mem);
  VertexBuffer buf     = upload_vertex_buffer(mesh);
  buf.asset_id         = asset_id;
  return buf;
}

struct Assets {
  FreeList<VertexBuffer> meshes;
  FreeList<RenderTarget> render_targets;
  FreeList<Texture> textures;
  FreeList<Material> materials;
  FreeList<Shader> shaders;
  FreeList<FileData> font_files;
  FreeList<KeyedAnimation> keyed_animations;

  std::map<std::pair<int, int>, Font> fonts;

  void init() {
    meshes.init(&assets_allocator, 1024);
    render_targets.init(&assets_allocator, 64);
    textures.init(&assets_allocator, 1024);
    materials.init(&assets_allocator, 1024);
    shaders.init(&assets_allocator, 1024);
    font_files.init(&assets_allocator, 32);
    keyed_animations.init(&assets_allocator, 256);
  }

  void load(const char *filename, StackAllocator *main_mem) {
    Temp temp = Temp::start(&assets_temp_allocator);

    FileData file    = read_entire_file(filename, &assets_temp_allocator);
    YAML::Dict *root = YAML::deserialize(String(file.data, file.length), &assets_temp_allocator)
                           ->as_dict()
                           ->get("assets")
                           ->as_dict();

    YAML::List *in_meshes = root->get("meshes")->as_list();
    for (int i = 0; i < in_meshes->len; i++) {
      YAML::Dict *in_mesh = in_meshes->get(i)->as_dict();
      int id      = atoi(in_mesh->get("id")->as_literal().to_char_array(&assets_temp_allocator));
      String path = in_mesh->get("path")->as_literal();

      VertexBuffer mesh = load_and_upload_mesh(path, id, assets_memory);
      mesh.asset_id     = id;
      mesh.asset_name   = String::copy(path, &assets_allocator);
      meshes.emplace(mesh, id);
    }
    if (auto in_render_targets_val = root->get("render_targets")) {
      YAML::List *in_render_targets = in_render_targets_val->as_list();
      for (int i = 0; i < in_render_targets->len; i++) {
        YAML::Dict *in_render_target = in_render_targets->get(i)->as_dict();
        int id =
            atoi(in_render_target->get("id")->as_literal().to_char_array(&assets_temp_allocator));

        String color_format_string = in_render_target->get("color_format")->as_literal();
        String depth_format_string = in_render_target->get("depth_format")->as_literal();
        TextureFormat color_format = texture_format_from_string(color_format_string);
        TextureFormat depth_format = texture_format_from_string(depth_format_string);
        int width                  = atoi(
            in_render_target->get("width")->as_literal().to_char_array(&assets_temp_allocator));
        int height = atoi(
            in_render_target->get("height")->as_literal().to_char_array(&assets_temp_allocator));

        RenderTarget target = RenderTarget(width, height, color_format, depth_format);
        target.asset_id     = id;
        render_targets.emplace(target, id);
      }
    }
    YAML::List *in_textures = root->get("textures")->as_list();
    for (int i = 0; i < in_textures->len; i++) {
      YAML::Dict *in_texture = in_textures->get(i)->as_dict();
      int id = atoi(in_texture->get("id")->as_literal().to_char_array(&assets_temp_allocator));

      Texture texture;
      if (YAML::Value *path_val = in_texture->get("path")) {
        String path          = path_val->as_literal();
        String format_string = in_texture->get("format")->as_literal();
        TextureFormat format = texture_format_from_string(format_string);
        texture              = load_and_upload_texture(path, format, assets_memory);
      } else if (YAML::Value *render_target_val = in_texture->get("render_target")) {
        int render_target_id =
            atoi(render_target_val->as_literal().to_char_array(&assets_temp_allocator));
        texture = render_targets.data[render_target_id].value.color_tex;
      } else {
        assert(false);
      }

      textures.emplace(texture, id);
    }
    YAML::List *in_materials = root->get("materials")->as_list();
    for (int i = 0; i < in_materials->len; i++) {
      YAML::Dict *in_material = in_materials->get(i)->as_dict();
      int id = atoi(in_material->get("id")->as_literal().to_char_array(&assets_temp_allocator));

      int num_parameters = 0;
      if (auto num_parameters_val = in_material->get("num_parameters")) {
        num_parameters =
            atoi(num_parameters_val->as_literal().to_char_array(&assets_temp_allocator));
      }

      YAML::List *texture_refs = in_material->get("textures")->as_list();
      Material material = Material::allocate(texture_refs->len, num_parameters, &assets_allocator);
      material.asset_id = id;
      for (int tex_i = 0; tex_i < texture_refs->len; tex_i++) {
        int texture_ref_id =
            atoi(texture_refs->get(tex_i)->as_literal().to_char_array(&assets_temp_allocator));
        material.textures[tex_i] = textures.data[texture_ref_id].value;
      }

      materials.emplace(material, id);
    }
    if (auto in_shaders_val = root->get("shaders")) {
      YAML::List *in_shaders = in_shaders_val->as_list();
      for (int i = 0; i < in_shaders->len; i++) {
        YAML::Dict *in_shader = in_shaders->get(i)->as_dict();
        int id = atoi(in_shader->get("id")->as_literal().to_char_array(&assets_temp_allocator));
        String name      = in_shader->get("name")->as_literal();
        String vert_path = in_shader->get("vert")->as_literal();
        String frag_path = in_shader->get("frag")->as_literal();

        auto vert_src = read_entire_file(vert_path.to_char_array(&assets_temp_allocator),
                                         &assets_temp_allocator);
        auto frag_src = read_entire_file(frag_path.to_char_array(&assets_temp_allocator),
                                         &assets_temp_allocator);

        Shader shader   = create_shader({vert_src.data, (uint16_t)vert_src.length},
                                      {frag_src.data, (uint16_t)frag_src.length},
                                      name.to_char_array(&assets_temp_allocator));
        shader.asset_id = id;
        shaders.emplace(shader, id);
      }
    }
    if (auto in_fonts_val = root->get("fonts")) {
      YAML::List *in_fonts = in_fonts_val->as_list();
      for (int i = 0; i < in_fonts->len; i++) {
        YAML::Dict *in_font = in_fonts->get(i)->as_dict();
        int id      = atoi(in_font->get("id")->as_literal().to_char_array(&assets_temp_allocator));
        String path = in_font->get("path")->as_literal();

        FileData file =
            read_entire_file(path.to_char_array(&assets_temp_allocator), &assets_allocator);
        font_files.emplace(file, id);
      }
    }
  }

  void load(const char *filename) {
    Temp tmp = Temp::start(&assets_temp_allocator);

    FileData file    = read_entire_file(filename, tmp);
    YAML::Dict *root = YAML::deserialize(String(file.data, file.length), tmp)->as_dict();

    if (auto in_keyed_animations_val = root->get("keyed_animations")) {
      YAML::List *in_keyed_animations = in_keyed_animations_val->as_list();
      for (u32 i = 0; i < in_keyed_animations->len; i++) {
        YAML::Dict *in_keyed_animation = in_keyed_animations->get(i)->as_dict();

        KeyedAnimation ka(0);

        ka.asset_id = atoi(in_keyed_animation->get("asset_id")->as_literal().to_char_array(tmp));
        ka.asset_name =
            String::copy(in_keyed_animation->get("asset_name")->as_literal(), &assets_allocator);
        ka.fps = atoi(in_keyed_animation->get("fps")->as_literal().to_char_array(tmp));

        ka.start_frame = atoi(in_keyed_animation->get("start_frame")->as_literal().to_char_array(tmp));
        ka.end_frame = atoi(in_keyed_animation->get("end_frame")->as_literal().to_char_array(tmp));

        auto tracks_in = in_keyed_animation->get("tracks")->as_list();
        for (u32 track_i = 0; track_i < tracks_in->len; track_i++) {
          auto track_in = tracks_in->get(track_i)->as_dict();

          KeyedAnimationTrack track;

          track.entity_id = atoi(track_in->get("entity_id")->as_literal().to_char_array(tmp));

          auto keys_in = track_in->get("keys")->as_list();
          for (u32 key_i = 0; key_i < keys_in->len; key_i++) {
            auto key_in = keys_in->get(key_i)->as_dict();

            KeyedAnimationTrack::Key key;

            YAML::Dict *in_transform = key_in->get("transform")->as_dict();
            YAML::Dict *in_position  = in_transform->get("position")->as_dict();
            key.transform.position.x = atof(in_position->get("x")->as_literal().to_char_array(tmp));
            key.transform.position.y = atof(in_position->get("y")->as_literal().to_char_array(tmp));
            key.transform.position.z = atof(in_position->get("z")->as_literal().to_char_array(tmp));
            YAML::Dict *in_rotation  = in_transform->get("rotation")->as_dict();
            key.transform.rotation.x = atof(in_rotation->get("x")->as_literal().to_char_array(tmp));
            key.transform.rotation.y = atof(in_rotation->get("y")->as_literal().to_char_array(tmp));
            key.transform.rotation.z = atof(in_rotation->get("z")->as_literal().to_char_array(tmp));
            YAML::Dict *in_scale     = in_transform->get("scale")->as_dict();
            key.transform.scale.x    = atof(in_scale->get("x")->as_literal().to_char_array(tmp));
            key.transform.scale.y    = atof(in_scale->get("y")->as_literal().to_char_array(tmp));
            key.transform.scale.z    = atof(in_scale->get("z")->as_literal().to_char_array(tmp));

            key.interpolation_type = (KeyedAnimationTrack::Key::InterpolationType)atoi(
                key_in->get("interpolation_type")->as_literal().to_char_array(tmp));
            key.frame = atoi(key_in->get("frame")->as_literal().to_char_array(tmp));

            track.keys.push_back(key);
          }

          ka.tracks.push_back(track);
        }

        keyed_animations.emplace(ka, ka.asset_id);
      }
    }
  }

  void save(String filepath) {
    auto tmp = Temp::start(&assets_temp_allocator);

    auto assets               = YAML::new_dict(tmp);
    auto keyed_animations_out = YAML::new_list(tmp);

    for (u32 i = 0; i < keyed_animations.size; i++) {
      if (keyed_animations.data[i].assigned) {
        KeyedAnimation *ka = &keyed_animations.data[i].value;

        auto keyed_animation_out = YAML::new_dict(tmp);

        keyed_animation_out->push_back(
            "asset_id", YAML::new_literal(String::from(ka->asset_id, tmp), tmp), tmp);
        keyed_animation_out->push_back("asset_name", YAML::new_literal(ka->asset_name, tmp), tmp);
        keyed_animation_out->push_back("fps", YAML::new_literal(String::from(ka->fps, tmp), tmp),
                                       tmp);
                                       
        keyed_animation_out->push_back("start_frame", YAML::new_literal(String::from(ka->start_frame, tmp), tmp),
                                       tmp);
        keyed_animation_out->push_back("end_frame", YAML::new_literal(String::from(ka->end_frame, tmp), tmp),
                                       tmp);

        auto tracks_out = YAML::new_list(tmp);
        for (u32 track_i = 0; track_i < ka->tracks.count; track_i++) {
          KeyedAnimationTrack *track = &ka->tracks[track_i];

          auto track_out = YAML::new_dict(tmp);
          track_out->push_back("entity_id",
                               YAML::new_literal(String::from(track->entity_id, tmp), tmp), tmp);

          auto keys_out = YAML::new_list(tmp);
          for (u32 key_i = 0; key_i < track->keys.count; key_i++) {
            KeyedAnimationTrack::Key *key = &track->keys[key_i];

            auto key_out = YAML::new_dict(tmp);

            YAML::Dict *transform_yaml = YAML::new_dict(tmp);
            YAML::Dict *position_yaml  = YAML::new_dict(tmp);
            position_yaml->push_back(
                "x", YAML::new_literal(String::from(key->transform.position.x, tmp), tmp), tmp);
            position_yaml->push_back(
                "y", YAML::new_literal(String::from(key->transform.position.y, tmp), tmp), tmp);
            position_yaml->push_back(
                "z", YAML::new_literal(String::from(key->transform.position.z, tmp), tmp), tmp);
            YAML::Dict *rotation_yaml = YAML::new_dict(tmp);
            rotation_yaml->push_back(
                "x", YAML::new_literal(String::from(key->transform.rotation.x, tmp), tmp), tmp);
            rotation_yaml->push_back(
                "y", YAML::new_literal(String::from(key->transform.rotation.y, tmp), tmp), tmp);
            rotation_yaml->push_back(
                "z", YAML::new_literal(String::from(key->transform.rotation.z, tmp), tmp), tmp);
            YAML::Dict *scale_yaml = YAML::new_dict(tmp);
            scale_yaml->push_back(
                "x", YAML::new_literal(String::from(key->transform.scale.x, tmp), tmp), tmp);
            scale_yaml->push_back(
                "y", YAML::new_literal(String::from(key->transform.scale.y, tmp), tmp), tmp);
            scale_yaml->push_back(
                "z", YAML::new_literal(String::from(key->transform.scale.z, tmp), tmp), tmp);
            transform_yaml->push_back("position", position_yaml, tmp);
            transform_yaml->push_back("rotation", rotation_yaml, tmp);
            transform_yaml->push_back("scale", scale_yaml, tmp);
            key_out->push_back("transform", transform_yaml, tmp);

            key_out->push_back(
                "interpolation_type",
                YAML::new_literal(String::from((u32)key->interpolation_type, tmp), tmp), tmp);
            key_out->push_back("frame", YAML::new_literal(String::from(key->frame, tmp), tmp), tmp);

            keys_out->push_back(key_out, tmp);
          }
          track_out->push_back("keys", keys_out, tmp);

          tracks_out->push_back(track_out, tmp);
        }
        keyed_animation_out->push_back("tracks", tracks_out, tmp);

        keyed_animations_out->push_back(keyed_animation_out, tmp);
      }
    }

    assets->push_back("keyed_animations", keyed_animations_out, tmp);

    String out;
    out.data = tmp.allocator->next;
    YAML::serialize(assets, tmp, 0, false);
    out.len = tmp.allocator->next - out.data;

    write_file(filepath.to_char_array(tmp), out);
  }

  Font *get_font(int font_id, int size) {
    if (fonts.count({font_id, size}) == 0) {
      fonts.emplace(std::pair(font_id, size),
                    load_font(font_files.data[font_id].value, size, &assets_temp_allocator));
    }
    return &fonts[{font_id, size}];
  }

  KeyedAnimation *create_keyed_animation(String folder, String name = {}) {
    u32 i                = keyed_animations.push_back({30});
    KeyedAnimation *anim = &keyed_animations.data[i].value;

    String path;
    if (name.len == 0) {
      auto name_str = i32_to_allocated_string<16>(i);
      name          = name_str;
    }
    anim->asset_id   = i;
    anim->asset_name = filepath_concat(folder, name, &assets_allocator);

    return anim;
  }

  //TODO: maybe assets ids should include asset type
  KeyedAnimation *get_keyed_animation(String name) {
    for (int i = 0; i < keyed_animations.size; i++) {
      if (strcmp(name, keyed_animations.data[i].value.asset_name)) return &keyed_animations.data[i].value;
    }

    assert(false);
    return nullptr;
  }
};