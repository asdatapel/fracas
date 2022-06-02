#pragma once

#include <glad/glad.h>

#include <uniforms.hpp>
#include "graphics.hpp"

struct ComputeShader{
  GLuint shader_handle;
  i32 uniform_handles[UNIFORM_COUNT];
  i32 tex_units[UNIFORM_COUNT];
};