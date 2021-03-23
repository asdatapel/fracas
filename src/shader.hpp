#include <glad/glad.h>

#include "graphics.hpp"

struct Shader
{
    enum struct UniformId
    {
        T,
        CAMERA_POSITION,
        ALBEDO_TEXTURE,
        NORMAL_TEXTURE,
        METAL_TEXTURE,
        ROUGHNESS_TEXTURE,
        OVERLAY_TEXTURE,
        IRRADIANCE,
        ENV_MAP,
        BRDF,
        MODEL,
        VIEW,
        PROJECTION,
        COLOR,
        RESOLUTION,
        POS,
        SCALE,
        ROUGHNESS,
        TEX,
        UV,
        EQUIRECTANGULAR_MAP,

        INVALID,
    };
    const static int UNIFORM_COUNT = (int)UniformId::INVALID;

    struct UniformDefinition
    {
        char name[50];
        bool is_texture;
    };
    constexpr static UniformDefinition UNIFORM_DEFINITIONS[UNIFORM_COUNT] = {
        {"t", false},
        {"camera_position", false},
        {"albedo_texture", true},
        {"normal_texture", true},
        {"metal_texture", true},
        {"roughness_texture", true},
        {"overlay_texture", true},
        {"irradiance", true},
        {"env_map", true},
        {"brdf", true},
        {"model", false},
        {"view", false},
        {"projection", false},
        {"color", false},
        {"resolution", false},
        {"pos", false},
        {"scale", false},
        {"roughness", false},
        {"tex", true},
        {"uv", false},
        {"equirectangular_map", true},
    };

    unsigned int shader_handle;
    unsigned int uniform_handles[UNIFORM_COUNT];
    unsigned int tex_units[UNIFORM_COUNT];
};

Shader load_shader(unsigned int handle)
{
    Shader s;
    s.shader_handle = handle;

    int num_textures = 0;
    for (int i = 0; i < Shader::UNIFORM_COUNT; i++)
    {
        s.uniform_handles[i] = glGetUniformLocation(s.shader_handle, s.UNIFORM_DEFINITIONS[i].name);
        if (s.UNIFORM_DEFINITIONS[i].is_texture)
        {
            if (s.uniform_handles[i] != -1)
            {
                glUniform1i(s.uniform_handles[i], num_textures);
                s.tex_units[i] = num_textures;
                num_textures++;
            }
        }
    }

    return s;
}

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

struct UniformData
{
    int num_uniforms;
    Shader::UniformId *uniform_ids;
    void **data;
};

struct CameraUniformData : UniformData
{
    Shader::UniformId uniform_id_array[3] = {
        Shader::UniformId::PROJECTION,
        Shader::UniformId::VIEW,
        Shader::UniformId::CAMERA_POSITION,
    };
    void *data_array[3];
    CameraUniformData()
    {
        num_uniforms = 3;
        data = data_array;
        uniform_ids = uniform_id_array;
    }
};