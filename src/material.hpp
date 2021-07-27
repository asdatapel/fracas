#pragma once

#include "graphics/shader.hpp"

struct Material
{
    const UniformId *uniform_ids;
    int num_textures;
    Texture *textures;
};

struct StandardPbrMaterial : Material
{
    static const int N = 6;
    static inline UniformId uniform_id_array[N] = {
        UniformId::ALBEDO_TEXTURE,
        UniformId::NORMAL_TEXTURE,
        UniformId::METAL_TEXTURE,
        UniformId::ROUGHNESS_TEXTURE,
        UniformId::EMIT_TEXTURE,
        UniformId::AO_TEXTURE,
    };
    Texture texture_array[N];
    StandardPbrMaterial()
    {
        num_textures = N;
        textures = texture_array;
        uniform_ids = uniform_id_array;
    }
    void operator=(const StandardPbrMaterial &other)
    {
        memcpy(textures, other.textures, N * sizeof(Texture));
    }
};

template <size_t N>
struct AllocatedMaterial : Material
{
    Array<UniformId, N> uniform_id_array;
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
        uniform_id_array = other.uniform_id_array;
    }

    void append(Texture tex, UniformId uniform_id)
    {
        assert(num_textures < N);
        texture_array.append(tex);
        uniform_id_array.append(uniform_id);
        num_textures++;
    }

    static AllocatedMaterial from(const Material *other)
    {
        assert(other->num_textures <= N);

        AllocatedMaterial<N> m;
        memcpy(m.texture_array.arr, other->textures, other->num_textures * sizeof(Texture));
        memcpy(m.uniform_id_array.arr, other->uniform_ids, other->num_textures * sizeof(UniformId));
        m.texture_array.len = other->num_textures;
        m.uniform_id_array.len = other->num_textures;
        m.num_textures = other->num_textures; 
        return m;
    }
};

struct StandardPbrEnvMaterial : Material
{
    static const int N = 3;
    static inline UniformId uniform_id_array[N] = {
        UniformId::IRRADIANCE,
        UniformId::ENV_MAP,
        UniformId::BRDF,
    };
    Texture texture_array[N];
    StandardPbrEnvMaterial()
    {
        num_textures = N;
        textures = texture_array;
        uniform_ids = uniform_id_array;
    }
    void operator=(const StandardPbrEnvMaterial &other)
    {
        memcpy(textures, other.textures, N * sizeof(Texture));
    }
};

void bind_material(Shader shader, Material material)
{
    for (int i = 0; i < material.num_textures; i++)
    {
        bind_texture(shader, material.uniform_ids[i], material.textures[i]);
    }
}