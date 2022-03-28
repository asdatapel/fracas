#pragma once

#include <array>
#include <map>
#include <unordered_map>
#include <vector>

#include <stb/stb_image.hpp>

#include "asset.hpp"
#include "font.hpp"
#include "global_allocators.hpp"
#include "graphics/graphics.hpp"
#include "keyed_animation.hpp"
#include "material.hpp"
#include "scene/entity.hpp"
#include "yaml.hpp"

struct EnvMap {
  Texture unfiltered_cubemap;
  Material env_mat;
};

Texture2D load_hdri(String filepath, Memory mem)
{
  auto tmp = Temp::start(mem);

  FileData file        = read_entire_file(filepath, tmp);

  int width, height, components;
  stbi_set_flip_vertically_on_load(true);
  float *hdri =
      stbi_loadf_from_memory((stbi_uc *)file.data, file.length, &width, &height, &components, 0);

  Texture2D tex(width, height, TextureFormat::RGB16F, true);
  tex.upload(hdri, true);

  stbi_image_free(hdri);

  return tex;
}

Material create_env_mat(RenderTarget temp_target, Texture unfiltered_cubemap)
{
  Texture irradiance_map = convolve_irradiance_map(temp_target, unfiltered_cubemap, 32);
  Texture env_map        = filter_env_map(temp_target, unfiltered_cubemap, 512);

  Texture2D brdf_lut = Texture2D(512, 512, TextureFormat::RGB16F, false);
  temp_target.change_color_target(brdf_lut);
  temp_target.clear();
  temp_target.bind();
  bind_shader(brdf_lut_shader);
  draw_rect();

  Material env_mat = Material::allocate(3, 0,  &assets_allocator);
  env_mat.textures[0] = irradiance_map;
  env_mat.textures[1] = env_map;
  env_mat.textures[2] = brdf_lut;

  return env_mat;
}

Texture load_and_upload_texture(String filepath, TextureFormat format, Memory mem)
{
  auto tmp = Temp::start(mem);

  FileData file        = read_entire_file(filepath, tmp);
  if (!file.length) {
    // TODO return default checkerboard texture
    return Texture{};
  }

  Bitmap bmp = parse_bitmap(file, tmp);
  Texture2D tex(bmp.width, bmp.height, format, true);
  tex.upload((uint8_t *)bmp.data, true);
  return tex;
}

VertexBuffer load_and_upload_mesh(String filepath, int asset_id, Memory mem)
{
  auto tmp = Temp::start(mem);

  char *filepath_chars = filepath.to_char_array(tmp);
  FileData file        = read_entire_file(filepath_chars, tmp);
  Mesh mesh            = load_fmesh(file, mem);
  VertexBuffer buf     = upload_vertex_buffer(mesh);
  buf.asset_id         = asset_id;
  return buf;
}

struct Assets {
  FreeList<VertexBuffer> meshes;
  FreeList<RenderTarget> render_targets;
  FreeList<Texture> textures;
  FreeList<EnvMap> env_maps;
  FreeList<Material> materials;
  FreeList<Shader> shaders;
  FreeList<FileData> font_files;
  FreeList<KeyedAnimation> keyed_animations;

  std::map<std::pair<int, int>, Font> fonts;

  void init()
  {
    meshes.init(&assets_allocator, 1024);
    render_targets.init(&assets_allocator, 64);
    textures.init(&assets_allocator, 1024);
    env_maps.init(&assets_allocator, 32);
    materials.init(&assets_allocator, 1024);
    shaders.init(&assets_allocator, 1024);
    font_files.init(&assets_allocator, 32);
    keyed_animations.init(&assets_allocator, 256);
  }

  void save(String filepath)
  {
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

        keyed_animation_out->push_back(
            "start_frame", YAML::new_literal(String::from(ka->start_frame, tmp), tmp), tmp);
        keyed_animation_out->push_back(
            "end_frame", YAML::new_literal(String::from(ka->end_frame, tmp), tmp), tmp);

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

  Font *get_font(int font_id, int size)
  {
    if (fonts.count({font_id, size}) == 0) {
      fonts.emplace(std::pair(font_id, size),
                    load_font(font_files.data[font_id].value, size, &assets_temp_allocator));
    }
    return &fonts[{font_id, size}];
  }

  RenderTarget get_render_target(String name)
  {
    for (int i = 0; i < render_targets.size; i++) {
      if (strcmp(name, render_targets.data[i].value.asset_name))
        return render_targets.data[i].value;
    }

    assert(false);
    return {};
  }

  KeyedAnimation *create_keyed_animation(String folder, String name = {})
  {
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

  // TODO: maybe assets ids should include asset type
  KeyedAnimation *get_keyed_animation(String name)
  {
    for (int i = 0; i < keyed_animations.size; i++) {
      if (strcmp(name, keyed_animations.data[i].value.asset_name))
        return &keyed_animations.data[i].value;
    }

    assert(false);
    return nullptr;
  }
};