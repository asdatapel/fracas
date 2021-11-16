#pragma once

#include "asset.hpp"
#include "graphics/shader.hpp"

struct Parameter
{
    UniformId uniform_id;
    float value;
};

struct Material : Asset
{
    int num_textures;
    Texture *textures;

    int num_parameters = 0;
    Parameter *parameters;

    static Material allocate(int num_textures, int num_parameters, StackAllocator *allocator)
    {
        Material material;
        material.textures = (Texture *)allocator->alloc(sizeof(Texture) * num_textures);
        material.num_textures = num_textures;
        material.num_parameters = num_parameters;
        material.parameters = (Parameter *)allocator->alloc(sizeof(Parameter) * num_parameters);
        return material;
    }
};

struct StandardPbrMaterial : Material
{
    static const int N = 6;
    Texture texture_array[N];
    StandardPbrMaterial()
    {
        num_textures = N;
        textures = texture_array;
    }
    void operator=(const StandardPbrMaterial &other)
    {
        memcpy(textures, other.textures, N * sizeof(Texture));
    }
};

template <size_t N>
struct AllocatedMaterial : Material
{
    Array<Texture, N> texture_array;

    AllocatedMaterial()
    {
        num_textures = 0;
        textures = texture_array.arr;
        uniform_ids = uniform_id_array.arr;
    }

    void operator=(const AllocatedMaterial &other)
    {
        num_textures = other.num_textures;
        texture_array = other.texture_array;
    }

    void append(Texture tex, UniformId uniform_id)
    {
        assert(num_textures < N);
        texture_array.append(tex);
        num_textures++;
    }

    static AllocatedMaterial from(const Material *other)
    {
        assert(other->num_textures <= N);

        AllocatedMaterial<N> m;
        memcpy(m.texture_array.arr, other->textures, other->num_textures * sizeof(Texture));
        memcpy(m.uniform_id_array.arr, other->uniform_ids, other->num_textures * sizeof(UniformId));
        m.texture_array.len = other->num_textures;
        m.num_textures = other->num_textures;
        return m;
    }
};

struct StandardPbrEnvMaterial : Material
{
    static const int N = 3;
    Texture texture_array[N];
    StandardPbrEnvMaterial()
    {
        num_textures = N;
        textures = texture_array;
    }
    void operator=(const StandardPbrEnvMaterial &other)
    {
        memcpy(textures, other.textures, N * sizeof(Texture));
    }
};

void bind_material(Shader shader, Material material, int texture_slot_offset = 0)
{
    for (int i = 0; i < material.num_textures; i++)
    {
        bind_texture(shader, texture_slot_offset + i, material.textures[i]);
    }
    for (int i = 0; i < material.num_parameters; i++)
    {
        bind_1f(shader, material.parameters[i].uniform_id, material.parameters[i].value);
    }
}