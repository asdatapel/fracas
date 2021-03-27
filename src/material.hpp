#pragma once

#include "shader.hpp"

struct Material
{
    int num_textures;
    const UniformId *uniform_ids;
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