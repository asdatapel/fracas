#pragma once

#include "shader.hpp"

struct Material
{
    int num_textures;
    Shader::UniformId *uniform_ids;
    Texture *textures;
};

struct StandardPbrMaterial : Material
{
    static const int N = 4;
    static inline Shader::UniformId uniform_id_array[N] = {
        Shader::UniformId::ALBEDO_TEXTURE,
        Shader::UniformId::NORMAL_TEXTURE,
        Shader::UniformId::METAL_TEXTURE,
        Shader::UniformId::ROUGHNESS_TEXTURE,
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
    static inline Shader::UniformId uniform_id_array[N] = {
        Shader::UniformId::IRRADIANCE,
        Shader::UniformId::ENV_MAP,
        Shader::UniformId::BRDF,
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