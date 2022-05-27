#pragma once

#include <math.h>
#include <stdint.h>
#include <utility>

#include "opengl.hpp"
#include "texture.hpp"

struct RenderTarget : Asset {
  uint32_t width = 0, height = 0;
  Texture color_tex;
  Texture depth_tex;

  uint32_t gl_fbo = 0;

  RenderTarget() = default;
  RenderTarget(uint32_t width, uint32_t height, uint32_t gl_fbo)
  {
    this->width  = width;
    this->height = height;
    this->gl_fbo = gl_fbo;
  }

  RenderTarget(uint32_t width, uint32_t height, TextureFormat color, TextureFormat depth)
  {
    this->width  = width;
    this->height = height;

    glGenFramebuffers(1, &gl_fbo);
    bind();

    if (color != TextureFormat::NONE) {
      color_tex = Texture2D(width, height, color, true);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.gl_ref,
                             0);
    }
    if (depth != TextureFormat::NONE) {
      depth_tex = Texture2D(width, height, depth, true);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex.gl_ref,
                             0);
    }
  }

  void change_color_target(Texture new_color_tex, uint32_t mip_level = 0)
  {
    // TODO possible leak here, if the original color_tex isn't cleaned up somewhere else
    color_tex = new_color_tex;

    width  = color_tex.width * powf(0.5, mip_level);
    height = color_tex.height * powf(0.5, mip_level);

    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_tex.gl_type,
                           color_tex.gl_ref, mip_level);
  }

  void change_color_target(CubemapArray new_color_tex, i32 layer, i32 face, uint32_t mip_level = 0)
  {
    assert(layer < new_color_tex.count);
    assert(face < 6);

    // TODO possible leak here, if the original color_tex isn't cleaned up somewhere else
    color_tex = new_color_tex;

    width  = color_tex.width * powf(0.5, mip_level);
    height = color_tex.height * powf(0.5, mip_level);

    bind();
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_tex.gl_ref, mip_level,
                              (layer * 6) + face);
  }

  void change_mip_level(uint32_t mip_level)
  {
    width  = color_tex.width * powf(0.5, mip_level);
    height = color_tex.height * powf(0.5, mip_level);

    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_tex.gl_type,
                           color_tex.gl_ref, mip_level);
  }

  void clear(Color color = {.3f, .35f, .4f, 0.f})
  {
    bind();

    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void bind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
    glViewport(0, 0, width, height);
  }

  void destroy() { glDeleteFramebuffers(1, &gl_fbo); }
};
