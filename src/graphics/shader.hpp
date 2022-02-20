#pragma once

#include <glad/glad.h>

#include <uniforms.hpp>
#include "../asset.hpp"
#include "graphics.hpp"

struct Shader : Asset {
  int shader_handle;
  int uniform_handles[UNIFORM_COUNT];
  int tex_units[UNIFORM_COUNT];
};

Shader load_shader(unsigned int handle)
{
  Shader s;
  s.shader_handle = handle;

  int num_textures = 0;
  for (int i = 0; i < UNIFORM_COUNT; i++) {
    s.uniform_handles[i] = glGetUniformLocation(s.shader_handle, UNIFORM_DEFINITIONS[i].name);
    if (UNIFORM_DEFINITIONS[i].is_texture) {
      if (s.uniform_handles[i] != -1) {
        s.tex_units[i] = num_textures;
        num_textures++;
      }
    }
  }

  return s;
}