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

struct Temp
{
    StackAllocator *allocator;
    char *data = nullptr;

    Temp(Memory mem)
    {
        allocator = mem.temp;
        data = mem.temp->next;
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
        TextureDefinition{String::from("\\diffuse.bmp"), TextureFormat::SRGB8_ALPHA8},
        TextureDefinition{String::from("\\normal.bmp"), TextureFormat::RGBA8},
        TextureDefinition{String::from("\\metal.bmp"), TextureFormat::SRGB8_ALPHA8},
        TextureDefinition{String::from("\\roughness.bmp"), TextureFormat::SRGB8_ALPHA8},
        TextureDefinition{String::from("\\emit.bmp"), TextureFormat::SRGB8_ALPHA8},
        TextureDefinition{String::from("\\ao.bmp"), TextureFormat::SRGB8_ALPHA8},
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
        load_scene(String::from("C:\\Users\\Asda\\Desktop\\test\\"),
                   String::from("set.fobj"),
                   mem);

        for (int i = 0; i < FONT_ASSET_COUNT; i++)
        {
            font_files[i] = read_entire_file(FONT_FILES[i], mem.allocator);
        }

        const String bar_numbers[] = {
            String::from("resources/images/bar_numbers/num_1.bmp"),
            String::from("resources/images/bar_numbers/num_2.bmp"),
            String::from("resources/images/bar_numbers/num_3.bmp"),
            String::from("resources/images/bar_numbers/num_4.bmp"),
            String::from("resources/images/bar_numbers/num_5.bmp"),
            String::from("resources/images/bar_numbers/num_6.bmp"),
            String::from("resources/images/bar_numbers/num_7.bmp"),
            String::from("resources/images/bar_numbers/num_8.bmp"),
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
            vec.x = atof(((YamlLiteral *)dict->get(String::from("x")))->value.to_char_array(mem.temp));
            vec.y = atof(((YamlLiteral *)dict->get(String::from("y")))->value.to_char_array(mem.temp));
            vec.z = atof(((YamlLiteral *)dict->get(String::from("z")))->value.to_char_array(mem.temp));

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
        YamlListElem *objects = (YamlListElem *)main->get(String::from("objects"));
        YamlListElem *obj_elem = objects;
        while (obj_elem)
        {
            Entity e = {};

            YamlDictElem *obj = (YamlDictElem *)obj_elem->value;
            String name = ((YamlLiteral *)obj->get(String::from("name")))->value;
            String type = ((YamlLiteral *)obj->get(String::from("type")))->value;
            if (strcmp(String::from("MESH"), type))
            {
                String data = ((YamlLiteral *)obj->get(String::from("data")))->value;
                VertexBuffer buf = load_mesh(folder.concat(data, mem.temp), mem);

                StandardPbrMaterial *material = (StandardPbrMaterial *)mem.allocator->alloc(sizeof(StandardPbrMaterial));
                new (material) StandardPbrMaterial();
                for (int i = 0; i < STANDARD_MATERIAL_FILES.size(); i++)
                {
                    String material_name = ((YamlLiteral *)obj->get(String::from("material")))->value;
                    String texture_path = folder.concat(material_name, mem.temp).concat(STANDARD_MATERIAL_FILES[i].filename, mem.temp);
                    material->textures[i] = load_texture(texture_path, STANDARD_MATERIAL_FILES[i].format, mem);
                }

                e.type = EntityType::MESH;
                e.vert_buffer = buf;
                e.material = material;
                e.shader = &threed_shader;
            }
            else if (strcmp(String::from("LIGHT"), type))
            {
                YamlDictElem *light = (YamlDictElem *)obj->get(String::from("light"));
                YamlDictElem *color = (YamlDictElem *)light->get(String::from("color"));

                e.type = EntityType::LIGHT;
                e.spot_light.color = dict_to_vec(color, mem);
                e.spot_light.inner_angle = atof(((YamlLiteral *)light->get(String::from("inner_angle")))->value.to_char_array(mem.temp));
                e.spot_light.outer_angle = atof(((YamlLiteral *)light->get(String::from("outer_angle")))->value.to_char_array(mem.temp));
            }

            YamlDictElem *position = (YamlDictElem *)obj->get(String::from("position"));
            YamlDictElem *rotation = (YamlDictElem *)obj->get(String::from("rotation"));
            YamlDictElem *scale = (YamlDictElem *)obj->get(String::from("scale"));
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
            vec.x = atof(((YamlLiteral *)dict->get(String::from("x")))->value.to_char_array(mem.temp));
            vec.y = atof(((YamlLiteral *)dict->get(String::from("y")))->value.to_char_array(mem.temp));
            vec.z = atof(((YamlLiteral *)dict->get(String::from("z")))->value.to_char_array(mem.temp));

            return vec;
        };

        auto string_to_string = [](String mine)
        {
            return std::string(mine.data, mine.len);
        };

        char *filepath = folder.concat(filename, mem.temp).to_char_array(mem.temp);
        FileData file = read_entire_file(filepath, mem.temp);

        YamlDictElem *main = YamlParser(file).parse(mem.temp);
        YamlListElem *objects = (YamlListElem *)main->get(String::from("objects"));

        YamlListElem *obj_elem = objects;
        while (obj_elem)
        {
            YamlDictElem *obj = (YamlDictElem *)obj_elem->value;
            String name = ((YamlLiteral *)obj->get(String::from("name")))->value;
            String type = ((YamlLiteral *)obj->get(String::from("type")))->value;
            if (strcmp(String::from("COLLECTION"), type))
            {
                String collection_name = ((YamlLiteral *)obj->get(String::from("collection")))->value;
                String collection_folder = folder.concat(collection_name, mem.temp).concat(String::from("\\"), mem.temp);
                Collection *collection = load_collection(collection_folder, collection_name.concat(String::from(".fobj"), mem.temp), mem);

                YamlDictElem *position_str = (YamlDictElem *)obj->get(String::from("position"));
                YamlDictElem *rotation_str = (YamlDictElem *)obj->get(String::from("rotation"));
                YamlDictElem *scale_str = (YamlDictElem *)obj->get(String::from("scale"));
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
                if (strcmp(String::from("MESH"), type))
                {
                    String data = ((YamlLiteral *)obj->get(String::from("data")))->value;
                    VertexBuffer buf = load_mesh(folder.concat(data, mem.temp), mem);

                    StandardPbrMaterial *material = (StandardPbrMaterial *)mem.allocator->alloc(sizeof(StandardPbrMaterial));
                    new (material) StandardPbrMaterial();
                    for (int i = 0; i < STANDARD_MATERIAL_FILES.size(); i++)
                    {
                        String material_name = ((YamlLiteral *)obj->get(String::from("material")))->value;
                        String texture_path = folder.concat(material_name, mem.temp).concat(STANDARD_MATERIAL_FILES[i].filename, mem.temp);
                        material->textures[i] = load_texture(texture_path, STANDARD_MATERIAL_FILES[i].format, mem);
                    }

                    e.type = EntityType::MESH;
                    e.vert_buffer = buf;
                    e.material = material;
                    e.shader = &threed_shader;
                }
                else if (strcmp(String::from("LIGHT"), type))
                {
                    YamlDictElem *light = (YamlDictElem *)obj->get(String::from("light"));
                    YamlDictElem *color = (YamlDictElem *)light->get(String::from("color"));

                    e.type = EntityType::LIGHT;
                    e.spot_light.color = dict_to_vec(color, mem);
                    e.spot_light.inner_angle = atof(((YamlLiteral *)light->get(String::from("inner_angle")))->value.to_char_array(mem.temp));
                    e.spot_light.outer_angle = atof(((YamlLiteral *)light->get(String::from("outer_angle")))->value.to_char_array(mem.temp));
                }

                YamlDictElem *position = (YamlDictElem *)obj->get(String::from("position"));
                YamlDictElem *rotation = (YamlDictElem *)obj->get(String::from("rotation"));
                YamlDictElem *scale = (YamlDictElem *)obj->get(String::from("scale"));
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
