#pragma once

#include <array>
#include <map>
#include <unordered_map>
#include <vector>

#include <asset_definitions.hpp>

#include "font.hpp"
#include "graphics/graphics.hpp"
#include "material.hpp"
#include "yaml_parser.hpp"
#include "scene/entity.hpp"
#include "yaml.hpp"

struct Assets
{
    Memory mem;

    FreeList<VertexBuffer> meshes;
    FreeList<RenderTarget> render_targets;
    FreeList<Texture> textures;
    FreeList<Material> materials;
    FreeList<Shader> shaders;
    FreeList<FileData> font_files;

    std::map<std::pair<int, int>, Font> fonts;

    void load(const char *filename, Memory mem)
    {
        this->mem = mem;

        meshes.init(mem.allocator, 1024);
        render_targets.init(mem.allocator, 64);
        textures.init(mem.allocator, 1024);
        materials.init(mem.allocator, 1024);
        shaders.init(mem.allocator, 1024);
        font_files.init(mem.allocator, 32);

        auto temp = Temp(mem);

        FileData file = read_entire_file(filename, mem.temp);
        YAML::Dict *root = YAML::deserialize(String(file.data, file.length), mem.temp)->as_dict()->get("assets")->as_dict();

        YAML::List *in_meshes = root->get("meshes")->as_list();
        for (int i = 0; i < in_meshes->len; i++)
        {
            YAML::Dict *in_mesh = in_meshes->get(i)->as_dict();
            int id = atoi(in_mesh->get("id")->as_literal().to_char_array(mem.temp));
            String path = in_mesh->get("path")->as_literal();

            VertexBuffer mesh = load_and_upload_mesh(path, id, mem);
            meshes.emplace(mesh, id);
        }
        if (auto in_render_targets_val = root->get("render_targets"))
        {
            YAML::List *in_render_targets = in_render_targets_val->as_list();
            for (int i = 0; i < in_render_targets->len; i++)
            {
                YAML::Dict *in_render_target = in_render_targets->get(i)->as_dict();
                int id = atoi(in_render_target->get("id")->as_literal().to_char_array(mem.temp));

                String color_format_string = in_render_target->get("color_format")->as_literal();
                String depth_format_string = in_render_target->get("depth_format")->as_literal();
                TextureFormat color_format = texture_format_from_string(color_format_string);
                TextureFormat depth_format = texture_format_from_string(depth_format_string);
                int width = atoi(in_render_target->get("width")->as_literal().to_char_array(mem.temp));
                int height = atoi(in_render_target->get("height")->as_literal().to_char_array(mem.temp));

                RenderTarget target = RenderTarget(width, height, color_format, depth_format);
                target.asset_id = id;
                render_targets.emplace(target, id);
            }
        }
        YAML::List *in_textures = root->get("textures")->as_list();
        for (int i = 0; i < in_textures->len; i++)
        {
            YAML::Dict *in_texture = in_textures->get(i)->as_dict();
            int id = atoi(in_texture->get("id")->as_literal().to_char_array(mem.temp));

            Texture texture;
            if (YAML::Value *path_val = in_texture->get("path"))
            {
                String path = path_val->as_literal();
                String format_string = in_texture->get("format")->as_literal();
                TextureFormat format = texture_format_from_string(format_string);
                texture = load_and_upload_texture(path, format, mem);
            }
            else if (YAML::Value *render_target_val = in_texture->get("render_target"))
            {
                int render_target_id = atoi(render_target_val->as_literal().to_char_array(mem.temp));
                texture = render_targets.data[render_target_id].value.color_tex;
            }
            else
            {
                assert(false);
            }

            textures.emplace(texture, id);
        }
        YAML::List *in_materials = root->get("materials")->as_list();
        for (int i = 0; i < in_materials->len; i++)
        {
            YAML::Dict *in_material = in_materials->get(i)->as_dict();
            int id = atoi(in_material->get("id")->as_literal().to_char_array(mem.temp));

            int num_parameters = 0;
            if (auto num_parameters_val = in_material->get("num_parameters"))
            {
                num_parameters = atoi(num_parameters_val->as_literal().to_char_array(mem.temp));
            }

            YAML::List *texture_refs = in_material->get("textures")->as_list();
            Material material = Material::allocate(texture_refs->len, num_parameters, mem.allocator);
            material.asset_id = id;
            for (int tex_i = 0; tex_i < texture_refs->len; tex_i++)
            {
                int texture_ref_id = atoi(texture_refs->get(tex_i)->as_literal().to_char_array(mem.temp));
                material.textures[tex_i] = textures.data[texture_ref_id].value;
            }

            materials.emplace(material, id);
        }
        if (auto in_shaders_val = root->get("shaders"))
        {
            YAML::List *in_shaders = in_shaders_val->as_list();
            for (int i = 0; i < in_shaders->len; i++)
            {
                YAML::Dict *in_shader = in_shaders->get(i)->as_dict();
                int id = atoi(in_shader->get("id")->as_literal().to_char_array(mem.temp));
                String name = in_shader->get("name")->as_literal();
                String vert_path = in_shader->get("vert")->as_literal();
                String frag_path = in_shader->get("frag")->as_literal();

                auto vert_src = read_entire_file(vert_path.to_char_array(mem.temp), mem.temp);
                auto frag_src = read_entire_file(frag_path.to_char_array(mem.temp), mem.temp);

                Shader shader = create_shader({vert_src.data, (uint16_t)vert_src.length}, {frag_src.data, (uint16_t)frag_src.length}, name.to_char_array(mem.temp));
                shader.asset_id = id;
                shaders.emplace(shader, id);
            }
        }

        
        if (auto in_fonts_val = root->get("fonts"))
        {
            YAML::List *in_fonts = in_fonts_val->as_list();
            for (int i = 0; i < in_fonts->len; i++)
            {
                YAML::Dict *in_font = in_fonts->get(i)->as_dict();
                int id = atoi(in_font->get("id")->as_literal().to_char_array(mem.temp));
                String path = in_font->get("path")->as_literal();

                FileData file = read_entire_file(path.to_char_array(mem.temp), mem.allocator);
                font_files.emplace(file, id);
            }
        }
    }

    Texture
    load_and_upload_texture(String filepath, TextureFormat format, Memory mem)
    {
        auto t = Temp::start(mem);

        char *filepath_chars = filepath.to_char_array(mem.temp);
        FileData file = read_entire_file(filepath_chars, mem.allocator);
        if (!file.length)
        {
            // TODO return default checkerboard texture
            return Texture{};
        }

        Bitmap bmp = parse_bitmap(file, mem.temp);
        Texture2D tex(bmp.width, bmp.height, format, true);
        tex.upload((uint8_t *)bmp.data, true);
        return tex;
    }

    VertexBuffer load_and_upload_mesh(String filepath, int asset_id, Memory mem)
    {
        auto t = Temp::start(mem);

        char *filepath_chars = filepath.to_char_array(mem.temp);
        FileData file = read_entire_file(filepath_chars, mem.allocator);
        Mesh mesh = load_fmesh(file, mem);
        VertexBuffer buf = upload_vertex_buffer(mesh);
        buf.asset_id = asset_id;
        return buf;
    }

    Font *get_font(int font_id, int size)
    {
        if (fonts.count({font_id, size}) == 0)
        {
            fonts.emplace(std::pair(font_id, size), load_font(font_files.data[font_id].value, size, mem.temp));
        }
        return &fonts[{font_id, size}];
    }
};