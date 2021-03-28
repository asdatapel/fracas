#pragma once

#include "stb/stb_truetype.hpp"

#include "graphics.hpp"
#include "platform.hpp"
#include "util.hpp"

const int NUM_CHARS_IN_FONT = 128;

struct Character
{
  Rect shape;
  Rect uv;
  float advance;
};

struct Font
{
  float font_size_px;

  float ascent;
  float descent;
  float baseline;

  Texture atlas;
  Character characters[NUM_CHARS_IN_FONT];
};

Font load_font(const char *filename, float size)
{
  Font font;
  font.font_size_px = size;

  FileData font_file = read_entire_file(filename);

  stbtt_fontinfo stb_font;
  stbtt_InitFont(&stb_font, (unsigned char *)font_file.data, 0);
  float stb_scale = stbtt_ScaleForPixelHeight(&stb_font, size);

  int ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&stb_font, &ascent, &descent, &lineGap);
  font.ascent = ascent * stb_scale;
  font.descent = descent * stb_scale;

  int x0, y0, x1, y1;
  stbtt_GetFontBoundingBox(&stb_font, &x0, &y0, &x1, &y1);
  font.baseline = ( -y0) * stb_scale;

  unsigned char *bitmap = (unsigned char *)malloc(2048 * 2048);
  int bitmap_width = 2048, bitmap_height = 2048;
  stbtt_bakedchar cdata[96];
  int success = stbtt_BakeFontBitmap((unsigned char *)font_file.data, 0, size, bitmap, bitmap_width, bitmap_height, 32, 96, cdata); // no guarantee this fits!
  font.atlas = to_single_channel_texture(bitmap, bitmap_width, bitmap_height, true);

  for (int i = 32; i < NUM_CHARS_IN_FONT; i++)
  {
    float x = 0;
    float y = 0;
    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(cdata, bitmap_width, bitmap_height, i - 32, &x, &y, &q, 1);

    font.characters[i].shape = {q.x0, q.y0, q.x1 - q.x0, q.y1 - q.y0};
    font.characters[i].uv = {q.s0, q.t1, q.s1 - q.s0, q.t0 - q.t1};
    font.characters[i].advance = x;
  }

  free(bitmap);
  free_file(font_file);

  return font;
}

float get_text_width(const Font &font, String text, float scale = 1.f)
{
  float width = 0;
  for (int i = 0; i < text.len; i++)
  {
    width += font.characters[text.data[i]].advance;
  }
  return width * scale;
}
float get_single_line_height(const Font &font, String text, float scale = 1.f)
{
  float height = 0;
  for (int i = 0; i < text.len; i++)
  {
    if (font.characters[text.data[i]].shape.height > height)
      height = font.characters[text.data[i]].shape.height;
  }
  return height * scale;
}

void draw_text(const Font &font, RenderTarget target, String text, float x, float y, float h_scale, float v_scale)
{
  float baseline = font.baseline * v_scale;
  for (int i = 0; i < text.len; i++)
  {
    Character c = font.characters[text.data[i]];
    Rect shape_rect = {x + c.shape.x, y + baseline + (v_scale * c.shape.y), h_scale * c.shape.width, v_scale * c.shape.height};
    x += c.advance * h_scale;
    draw_textured_mapped_rect(target, shape_rect, c.uv, font.atlas);
  }
}

void draw_centered_text(const Font &font, RenderTarget target, String text, Rect sub_target, float scale, float aspect_ratio)
{
  float text_width = get_text_width(font, text, scale) / target.width;
  float text_height = get_single_line_height(font, text, scale * aspect_ratio) / target.height;

  float oversize = sub_target.width / text_width;
  if (oversize < 1.f)
  {
    scale *= oversize;
    text_width *= oversize;
    text_height *= oversize;
  }

  float x_start = sub_target.x + ((sub_target.width - text_width) / 2.f);
  float y_start = sub_target.y + ((sub_target.height - text_height) / 2.f);

  draw_text(font, target, text, x_start * target.width, y_start * target.height, scale, scale * aspect_ratio);
}