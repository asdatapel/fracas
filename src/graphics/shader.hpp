#pragma once

#include <glad/glad.h>

#include <uniforms.hpp>
#include "../asset.hpp"
#include "graphics.hpp"

struct Shader : Asset {
  i32 shader_handle;
  i32 uniform_handles[UNIFORM_COUNT];
  i32 tex_units[UNIFORM_COUNT];

  i32 material_offset = 3;
  i32 pbr_texture_offset = 2;
  i32 reflections_texture_offset = 0;
  b8 shadows_enabled = false; 
  i32 shadow_texture_offset = 9; 

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

// struct ShaderDef {
//   String file;

//   enum struct Type {
//     FLOAT,
//     INT,
//   };
//   struct Component {
//     String name;
//     Type type;
//     i32 size;
//   };
//   RefArray<Component> components;

//   bool env_map;
//   bool lit;
//   bool shadows;
//   bool armature;
// };

// ShaderDef threed_shader_def() {
//   ShaderDef def;

//   def.components.len = 3;
//   def.components.data = (ShaderDef::Component *)malloc(sizeof(ShaderDef::Component ) * def.components.len);
//   def.components[0].name = "pos";
//   def.components[0].type = ShaderDef::Type::FLOAT;
//   def.components[0].size = 3;
//   def.components[1].name = "uv";
//   def.components[1].type = ShaderDef::Type::FLOAT;
//   def.components[1].size = 3;
//   def.components[2].name = "normal";
//   def.components[2].type = ShaderDef::Type::FLOAT;
//   def.components[2].size = 3;


//   def.env_map = true;
//   def.lit = true;
//   def.shadows = true;
//   def.armature = false;
// }

// struct Shader2 {
//   void init_gpu_resource()
//   {
//     compile_shader();

//     glVertexAttribFormat(0, shader_def.components[0].size, GL_FLOAT, GL_FALSE, 0);
//     glVertexAttribBinding(0, 0);
//     glVertexAttribFormat(1, shader_def.components[1].size, GL_FLOAT, GL_FALSE, 0);
//     glVertexAttribBinding(1, 1);
//     glVertexAttribFormat(2, shader_def.components[2].size, GL_FLOAT, GL_FALSE, 0);
//     glVertexAttribBinding(2, 2);

//     glBindVertexBuffer(i, buffer, component.offset, component.stride);

//     create VAO with all formats each attribute should have a different binding point then each
//         shader with unique vertex attributes should have a way of selecting from the source mesh
//             putting them in the correct binding point
//         ? ? ? does this mean it can't be interleaved? no because stride is set when binding buffer
//   }
// };