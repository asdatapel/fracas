#pragma once

#include "opengl.hpp"

enum struct TextureFormat {
  NONE,
  RED,
  RGB8,
  RGBA8,
  RGBA32,
  SRGB8_ALPHA8,
  SRGB8,
  RGB16F,
  RGBA16F,

  DEPTH24,
  DEPTH32,
};

// [internalformat, format]
std::pair<GLenum, GLenum> format_to_opengl(TextureFormat format)
{
  switch (format) {
    case TextureFormat::NONE:
      return {0, 0};
    case TextureFormat::RED:
      return {GL_R8, GL_RED};
    case TextureFormat::RGB8:
      return {GL_RGB8, GL_RGB};
    case TextureFormat::RGBA8:
      return {GL_RGBA8, GL_RGBA};
    case TextureFormat::RGBA32:
      return {GL_RGBA32F, GL_RGBA};
    case TextureFormat::SRGB8_ALPHA8:
      return {GL_SRGB_ALPHA, GL_RGBA};
    case TextureFormat::SRGB8:
      return {GL_SRGB8, GL_RGB};
    case TextureFormat::RGB16F:
      return {GL_RGB16F, GL_RGB};
    case TextureFormat::RGBA16F:
      return {GL_RGBA16F, GL_RGBA};
    case TextureFormat::DEPTH24:
      return {GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT};
    case TextureFormat::DEPTH32:
      return {GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT};
  }
}

TextureFormat texture_format_from_string(String str)
{
  if (strcmp(str, "SRGB8_ALPHA8")) {
    return TextureFormat::SRGB8_ALPHA8;
  }
  if (strcmp(str, "SRGB8")) {
    return TextureFormat::SRGB8;
  }
  if (strcmp(str, "RED")) {
    return TextureFormat::RED;
  }
  if (strcmp(str, "RGB8")) {
    return TextureFormat::RGB8;
  }
  if (strcmp(str, "RGBA8")) {
    return TextureFormat::RGBA8;
  }
  if (strcmp(str, "RGB16F")) {
    return TextureFormat::RGB16F;
  }
  if (strcmp(str, "RGBA16F")) {
    return TextureFormat::RGBA16F;
  }
  if (strcmp(str, "DEPTH24")) {
    return TextureFormat::DEPTH24;
  }
  if (strcmp(str, "DEPTH32")) {
    return TextureFormat::DEPTH32;
  }
  return TextureFormat::NONE;
}

struct Texture {
  TextureFormat format = TextureFormat::NONE;
  uint32_t width = 0, height = 0;

  uint32_t gl_ref = 0;
  GLenum gl_type  = GL_TEXTURE_2D;

  void gen_mipmaps()
  {
    bind();
    glGenerateMipmap(gl_type);
  }

  void bind() { glBindTexture(gl_type, gl_ref); }
};

struct Texture2D : Texture {
  Texture2D() = default;
  Texture2D(uint32_t width, uint32_t height, TextureFormat format, TextureFormat internal_format, bool want_mipmaps = true)
  {
    this->format = format;
    this->width  = width;
    this->height = height;

    gl_type = GL_TEXTURE_2D;

    glGenTextures(1, &gl_ref);
    bind();

    auto [gl_internalformat, gl_format] = format_to_opengl(format);
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internalformat, width, height, 0, gl_format, GL_UNSIGNED_BYTE,
                 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, want_mipmaps ? GL_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
  Texture2D(uint32_t width, uint32_t height, TextureFormat format, bool want_mipmaps = true)
  {
    this->format = format;
    this->width  = width;
    this->height = height;

    gl_type = GL_TEXTURE_2D;

    glGenTextures(1, &gl_ref);
    bind();

    auto [gl_internalformat, gl_format] = format_to_opengl(format);
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internalformat, width, height, 0, gl_format, GL_UNSIGNED_BYTE,
                 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, want_mipmaps ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (format == TextureFormat::DEPTH24) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
    }
  }

  void upload(void *data, GLenum pixel_data_type, bool want_mipmaps)
  {
    bind();

    auto [gl_internalformat, gl_format] = format_to_opengl(format);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, gl_format, pixel_data_type,
                    (void *)data);

    if (want_mipmaps) {
      gen_mipmaps();
    }
  }

  void upload(uint8_t *data, bool want_mipmaps = false)
  {
    upload(data, GL_UNSIGNED_BYTE, want_mipmaps);
  }

  void upload(float *data, bool want_mipmaps = false) { upload(data, GL_FLOAT, want_mipmaps); }
};

struct Cubemap : public Texture {
  Cubemap() = default;
  Cubemap(uint32_t width, uint32_t height, TextureFormat format, bool want_mipmaps = true)
  {
    this->format = format;
    this->width  = width;
    this->height = height;

    this->gl_type = GL_TEXTURE_CUBE_MAP;

    glGenTextures(1, &gl_ref);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gl_ref);

    auto [gl_internalformat, gl_format] = format_to_opengl(format);
    for (unsigned int i = 0; i < 6; ++i) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, gl_internalformat, width, height, 0,
                   gl_format, GL_UNSIGNED_BYTE, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    want_mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

    if (want_mipmaps) {
      glGenerateMipmap(gl_type);
    }
  }

  Texture get_face(uint32_t i)
  {
    assert(i < 6);
    return {format, width, height, gl_ref, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i};
  }
};

struct CubemapArray : public Texture {
  i32 count = 0;

  CubemapArray() {gl_type = GL_TEXTURE_CUBE_MAP_ARRAY;}

  void init(i32 count, i32 size, TextureFormat format) {
    this->count  = count;
    this->width  = size;
    this->height = size;

    glGenTextures(1, &gl_ref);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, gl_ref);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    auto [gl_internalformat, gl_format] = format_to_opengl(format);
    glTexImage3D(
        GL_TEXTURE_CUBE_MAP_ARRAY, 0, gl_internalformat, size, size, 6 * count, 0,
        gl_format, GL_UNSIGNED_BYTE, nullptr);
  }
};