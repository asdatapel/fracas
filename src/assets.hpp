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

struct Temp
{
    StackAllocator *allocator;
    char *data = nullptr;

    Temp(Memory mem)
    {
        allocator = mem.temp;
        data = mem.temp->next;
    }
    Temp(StackAllocator *alloc)
    {
        allocator = alloc;
        data = alloc->next;
    }
    ~Temp()
    {
        allocator->free(data);
    }

    static Temp start(Memory mem)
    {
        return Temp(mem);
    }
};

struct Collection
{
    std::vector<Entity> entities;
};

struct Assets
{
    struct TextureDefinition
    {
        String filename;
        TextureFormat format;
    };
    const std::array<TextureDefinition, StandardPbrMaterial::N> STANDARD_MATERIAL_FILES = {
        TextureDefinition{"\\diffuse.bmp", TextureFormat::SRGB8_ALPHA8},
        TextureDefinition{"\\normal.bmp", TextureFormat::RGBA8},
        TextureDefinition{"\\metal.bmp", TextureFormat::SRGB8_ALPHA8},
        TextureDefinition{"\\roughness.bmp", TextureFormat::SRGB8_ALPHA8},
        TextureDefinition{"\\emit.bmp", TextureFormat::SRGB8_ALPHA8},
        TextureDefinition{"\\ao.bmp", TextureFormat::SRGB8_ALPHA8},
    };

    // TODO don't use a map, somehow
    std::map<String, VertexBuffer> meshes;
    std::map<String, Texture> textures;
    std::map<String, Collection> collections;

    std::vector<Entity> entities;
    std::unordered_map<std::string, int> entity_names;

    std::array<Texture, 8> bar_num_textures;

    FileData font_files[FONT_ASSET_COUNT];
    VertexBuffer load_mesh(String filepath, Memory mem)
    {
        if (meshes.count(filepath) > 0)
        {
            return meshes[filepath];
        }

        auto t = Temp::start(mem);

        char *filepath_chars = filepath.to_char_array(mem.temp);
        FileData file = read_entire_file(filepath_chars, mem.allocator);
        Mesh mesh = load_fmesh(file, mem);
        VertexBuffer buf = upload_vertex_buffer(mesh);
        meshes[filepath] = buf;

        return buf;
    }

    Texture load_texture(String filepath, TextureFormat format, Memory mem)
    {
        if (textures.count(filepath) > 0)
        {
            return textures[filepath];
        }

        auto t = Temp::start(mem);

        char *filepath_chars = filepath.to_char_array(mem.temp);
        FileData file = read_entire_file(filepath_chars, mem.allocator);
        if (!file.length)
        {
            // TODO return default texture
            return Texture{};
        }

        Bitmap bmp = parse_bitmap(file, mem.temp);
        Texture2D tex(bmp.width, bmp.height, format, true);
        tex.upload((uint8_t *)bmp.data, true);
        textures[filepath] = tex;

        return tex;
    }

    void init(Memory mem)
    {
        load_scene("resources\\scenes\\test\\",
                   "set.fobj",
                   mem);

        for (int i = 0; i < FONT_ASSET_COUNT; i++)
        {
            font_files[i] = read_entire_file(FONT_FILES[i], mem.allocator);
        }

        const String bar_numbers[] = {
            "resources/images/bar_numbers/num_1.bmp",
            "resources/images/bar_numbers/num_2.bmp",
            "resources/images/bar_numbers/num_3.bmp",
            "resources/images/bar_numbers/num_4.bmp",
            "resources/images/bar_numbers/num_5.bmp",
            "resources/images/bar_numbers/num_6.bmp",
            "resources/images/bar_numbers/num_7.bmp",
            "resources/images/bar_numbers/num_8.bmp",
        };

        for (int i = 0; i < bar_num_textures.size(); i++)
        {
            bar_num_textures[i] = load_texture(bar_numbers[i], TextureFormat::RGBA8, mem);
        }
    }

    Collection *load_collection(String folder, String filename, Memory mem)
    {
        auto dict_to_vec = [](YamlDictElem *dict, Memory mem)
        {
            Vec3f vec;
            vec.x = atof(((YamlLiteral *)dict->get("x"))->value.to_char_array(mem.temp));
            vec.y = atof(((YamlLiteral *)dict->get("y"))->value.to_char_array(mem.temp));
            vec.z = atof(((YamlLiteral *)dict->get("z"))->value.to_char_array(mem.temp));

            return vec;
        };

        auto t = Temp::start(mem);

        String filepath = folder.concat(filename, mem.temp);
        if (collections.count(filepath) > 0)
        {
            return &collections[filepath];
        }

        FileData file = read_entire_file(filepath.to_char_array(mem.temp), mem.temp);
        assert(file.length > 0);

        Collection collection;

        YamlDictElem *main = YamlParser(file).parse(mem.temp);
        YamlListElem *objects = (YamlListElem *)main->get("objects");
        YamlListElem *obj_elem = objects;
        while (obj_elem)
        {
            Entity e = {};

            YamlDictElem *obj = (YamlDictElem *)obj_elem->value;
            String name = ((YamlLiteral *)obj->get("name"))->value;
            String type = ((YamlLiteral *)obj->get("type"))->value;
            if (strcmp("MESH", type))
            {
                String data = ((YamlLiteral *)obj->get("data"))->value;
                VertexBuffer buf = load_mesh(folder.concat(data, mem.temp), mem);

                StandardPbrMaterial *material = (StandardPbrMaterial *)mem.allocator->alloc(sizeof(StandardPbrMaterial));
                new (material) StandardPbrMaterial();
                for (int i = 0; i < STANDARD_MATERIAL_FILES.size(); i++)
                {
                    String material_name = ((YamlLiteral *)obj->get("material"))->value;
                    String texture_path = folder.concat(material_name, mem.temp).concat(STANDARD_MATERIAL_FILES[i].filename, mem.temp);
                    material->textures[i] = load_texture(texture_path, STANDARD_MATERIAL_FILES[i].format, mem);
                }

                e.type = EntityType::MESH;
                e.vert_buffer = buf;
                e.material = material;
                e.shader = &threed_shader;
            }
            else if (strcmp("LIGHT", type))
            {
                YamlDictElem *light = (YamlDictElem *)obj->get("light");
                YamlDictElem *color = (YamlDictElem *)light->get("color");

                e.type = EntityType::LIGHT;
                e.spot_light.color = dict_to_vec(color, mem);
                e.spot_light.inner_angle = atof(((YamlLiteral *)light->get("inner_angle"))->value.to_char_array(mem.temp));
                e.spot_light.outer_angle = atof(((YamlLiteral *)light->get("outer_angle"))->value.to_char_array(mem.temp));
            }

            YamlDictElem *position = (YamlDictElem *)obj->get("position");
            YamlDictElem *rotation = (YamlDictElem *)obj->get("rotation");
            YamlDictElem *scale = (YamlDictElem *)obj->get("scale");
            e.transform.position = dict_to_vec(position, mem);
            e.transform.rotation = dict_to_vec(rotation, mem);
            e.transform.scale = dict_to_vec(scale, mem);

            e.debug_tag.name = string_to_allocated_string<64>(name);
            collection.entities.push_back(e);

            obj_elem = obj_elem->next;
        }

        collections[filepath] = collection;
        return &collections[filepath];
    }

    void load_scene(String folder, String filename, Memory mem)
    {
        auto dict_to_vec = [](YamlDictElem *dict, Memory mem)
        {
            Vec3f vec;
            vec.x = atof(((YamlLiteral *)dict->get("x"))->value.to_char_array(mem.temp));
            vec.y = atof(((YamlLiteral *)dict->get("y"))->value.to_char_array(mem.temp));
            vec.z = atof(((YamlLiteral *)dict->get("z"))->value.to_char_array(mem.temp));

            return vec;
        };

        auto string_to_string = [](String mine)
        {
            return std::string(mine.data, mine.len);
        };

        char *filepath = folder.concat(filename, mem.temp).to_char_array(mem.temp);
        FileData file = read_entire_file(filepath, mem.temp);

        YamlDictElem *main = YamlParser(file).parse(mem.temp);
        YamlListElem *objects = (YamlListElem *)main->get("objects");

        YamlListElem *obj_elem = objects;
        while (obj_elem)
        {
            YamlDictElem *obj = (YamlDictElem *)obj_elem->value;
            String name = ((YamlLiteral *)obj->get("name"))->value;
            String type = ((YamlLiteral *)obj->get("type"))->value;
            if (strcmp("COLLECTION", type))
            {
                String collection_name = ((YamlLiteral *)obj->get("collection"))->value;
                String collection_folder = folder.concat(collection_name, mem.temp).concat("\\", mem.temp);
                Collection *collection = load_collection(collection_folder, collection_name.concat(".fobj", mem.temp), mem);

                YamlDictElem *position_str = (YamlDictElem *)obj->get("position");
                YamlDictElem *rotation_str = (YamlDictElem *)obj->get("rotation");
                YamlDictElem *scale_str = (YamlDictElem *)obj->get("scale");
                Vec3f position = dict_to_vec(position_str, mem);
                Vec3f rotation = dict_to_vec(rotation_str, mem);
                Vec3f scale = dict_to_vec(scale_str, mem);

                for (int i = 0; i < collection->entities.size(); i++)
                {
                    Entity source_e = collection->entities[i];
                    Transform source_transform = source_e.transform;
                    glm::vec3 e_position = glm::vec3{position.x, position.y, position.z} +
                                           glm::rotate(glm::quat(glm::vec3{rotation.x, rotation.y, rotation.z}), glm::vec3{source_transform.position.x, source_transform.position.y, source_transform.position.z});
                    glm::vec3 e_rotation = glm::vec3{rotation.x, rotation.y, rotation.z} + glm::vec3{source_transform.rotation.x, source_transform.rotation.y, source_transform.rotation.z};
                    glm::vec3 e_scale = glm::vec3{scale.x, scale.y, scale.z} * glm::vec3{source_transform.scale.x, source_transform.scale.y, source_transform.scale.z};

                    Entity new_entity = source_e;
                    new_entity.transform.position = {e_position.x, e_position.y, e_position.z};
                    new_entity.transform.rotation = {e_rotation.x, e_rotation.y, e_rotation.z};
                    new_entity.transform.scale = {e_scale.x, e_scale.y, e_scale.z};

                    entity_names[string_to_string(name)] = entities.size();
                    new_entity.debug_tag.name = string_to_allocated_string<64>(name.concat(source_e.debug_tag.name, mem.temp));
                    entities.push_back(new_entity);
                }
            }
            else
            {
                Entity e = {};
                if (strcmp("MESH", type))
                {
                    String data = ((YamlLiteral *)obj->get("data"))->value;
                    VertexBuffer buf = load_mesh(folder.concat(data, mem.temp), mem);

                    StandardPbrMaterial *material = (StandardPbrMaterial *)mem.allocator->alloc(sizeof(StandardPbrMaterial));
                    new (material) StandardPbrMaterial();
                    for (int i = 0; i < STANDARD_MATERIAL_FILES.size(); i++)
                    {
                        String material_name = ((YamlLiteral *)obj->get("material"))->value;
                        String texture_path = folder.concat(material_name, mem.temp).concat(STANDARD_MATERIAL_FILES[i].filename, mem.temp);
                        material->textures[i] = load_texture(texture_path, STANDARD_MATERIAL_FILES[i].format, mem);
                    }

                    e.type = EntityType::MESH;
                    e.vert_buffer = buf;
                    e.material = material;
                    e.shader = &threed_shader;
                }
                else if (strcmp("LIGHT", type))
                {
                    YamlDictElem *light = (YamlDictElem *)obj->get("light");
                    YamlDictElem *color = (YamlDictElem *)light->get("color");

                    e.type = EntityType::LIGHT;
                    e.spot_light.color = dict_to_vec(color, mem);
                    e.spot_light.inner_angle = atof(((YamlLiteral *)light->get("inner_angle"))->value.to_char_array(mem.temp));
                    e.spot_light.outer_angle = atof(((YamlLiteral *)light->get("outer_angle"))->value.to_char_array(mem.temp));
                }

                YamlDictElem *position = (YamlDictElem *)obj->get("position");
                YamlDictElem *rotation = (YamlDictElem *)obj->get("rotation");
                YamlDictElem *scale = (YamlDictElem *)obj->get("scale");
                e.transform.position = dict_to_vec(position, mem);
                e.transform.rotation = dict_to_vec(rotation, mem);
                e.transform.scale = dict_to_vec(scale, mem);
                e.debug_tag.name = string_to_allocated_string<64>(name);
                entities.push_back(e);
            }

            obj_elem = obj_elem->next;
        }
    }
};

struct Assets2
{
    FreeList<VertexBuffer> meshes;
    FreeList<RenderTarget> render_targets;
    FreeList<Texture> textures;
    FreeList<Material> materials;
    FreeList<Shader> shaders;

    std::map<std::string, int> named_meshes;

    void load(const char *filename, Memory mem)
    {
        meshes.init(mem.allocator, 1024);
        render_targets.init(mem.allocator, 64);
        textures.init(mem.allocator, 1024);
        materials.init(mem.allocator, 1024);
        shaders.init(mem.allocator, 1024);

        FileData file = read_entire_file(filename, mem.temp);
        YAML::Dict *root = YAML::deserialize(String(file.data, file.length), mem.temp)->as_dict()->get("assets")->as_dict();

        YAML::List *in_meshes = root->get("meshes")->as_list();
        for (int i = 0; i < in_meshes->len; i++)
        {
            YAML::Dict *in_mesh = in_meshes->get(i)->as_dict();
            int id = atoi(in_mesh->get("id")->as_literal().to_char_array(mem.temp));
            String name = in_mesh->get("name")->as_literal();
            String path = in_mesh->get("path")->as_literal();

            VertexBuffer mesh = load_and_upload_mesh(path, mem);
            meshes.emplace(mesh, id);
            named_meshes.emplace(std::string(name.data, name.len), id);
        }
        YAML::List *in_render_targets = root->get("render_targets")->as_list();
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
            render_targets.emplace(target, id);
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
            int num_parameters = atoi(in_material->get("num_parameters")->as_literal().to_char_array(mem.temp));
            YAML::List *texture_refs = in_material->get("textures")->as_list();

            Material material = Material::allocate(texture_refs->len, num_parameters, mem.allocator);
            for (int tex_i = 0; tex_i < texture_refs->len; tex_i++)
            {
                int texture_ref_id = atoi(texture_refs->get(tex_i)->as_literal().to_char_array(mem.temp));
                material.textures[tex_i] = textures.data[texture_ref_id].value;
            }

            materials.emplace(material, id);
        }
        YAML::List *in_shaders = root->get("shaders")->as_list();
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
            shaders.emplace(shader, id);
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

    VertexBuffer load_and_upload_mesh(String filepath, Memory mem)
    {
        auto t = Temp::start(mem);

        char *filepath_chars = filepath.to_char_array(mem.temp);
        FileData file = read_entire_file(filepath_chars, mem.allocator);
        Mesh mesh = load_fmesh(file, mem);
        VertexBuffer buf = upload_vertex_buffer(mesh);
        return buf;
    }
};