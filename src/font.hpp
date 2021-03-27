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
  font.baseline = size + y0 * stb_scale;

  unsigned char *bitmap = (unsigned char *)malloc(2048 * 2048);
  int bitmap_width = 2048, bitmap_height = 2048;
  stbtt_bakedchar cdata[96];
  int success = stbtt_BakeFontBitmap((unsigned char *)font_file.data, 0, size, bitmap, bitmap_width, bitmap_height, 32, 96, cdata); // no guarantee this fits!
  font.atlas = to_single_channel_texture(bitmap, bitmap_width, bitmap_height, true);

  for (int i = 32; i < NUM_CHARS_IN_FONT; i++)
  {
    float x = 0;
    float y = font.baseline;
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
  for (int i = 0; i < text.len; i++)
  {
    Character c = font.characters[text.data[i]];
    Rect shape_rect = {x + c.shape.x, y + c.shape.y, h_scale * c.shape.width, v_scale * c.shape.height};
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

// void draw_character(RenderTarget target, Font *font, char c, Rect rect)
// {
//   draw_textured_mapped_rect(target, rect, font->characters[c].uv, font->tex);
// }

// void draw_string(RenderTarget target, Font *font, float x, float y, float scale, String str)
// {
//   for (int i = 0; i < str.len; i++)
//   {
//     char c = str.data[i];

//     Rect rect = {};

//     auto scaled = [scale](float val) { return val * scale; };

//     rect.x = x - scaled(font->characters[c].originX);
//     rect.y = y + (scaled(font->size)) - scaled(font->characters[c].originY);
//     rect.width = scaled(font->characters[c].width);
//     rect.height = scaled(font->characters[c].height);
//     draw_character(target, font, c, rect);

//     x += scaled(font->characters[c].advance);
//   }
// }

// float string_width(Font *font, float scale, String str)
// {
//   auto scaled = [scale](float val) { return val * scale; };
//   float width = 0;
//   for (int i = 0; i < str.len; i++)
//   {
//     char c = str.data[i];
//     width += scaled(font->characters[c].advance);
//   }
//   return width;
// }

// struct ConsolasCharacter
// {
//   int codePoint;
//   float x, y, width, height;
//   int originX, originY;
//   float advance;
// };
// static ConsolasCharacter characters_Consolas[] = {
//     {' ', 407, 143, 12, 12, 6, 6, 17},
//     {'!', 175, 44, 16, 34, -1, 28, 17},
//     {'"', 251, 143, 22, 19, 2, 28, 17},
//     {'#', 183, 78, 29, 32, 5, 26, 17},
//     {'$', 199, 0, 26, 39, 4, 29, 17},
//     {'%', 306, 0, 29, 35, 6, 28, 17},
//     {'&', 362, 0, 29, 34, 5, 28, 17},
//     {'\'', 273, 143, 16, 19, -1, 28, 17},
//     {'(', 37, 0, 21, 42, 1, 29, 17},
//     {')', 15, 0, 22, 42, 2, 29, 17},
//     {'*', 154, 143, 25, 25, 4, 28, 17},
//     {'+', 102, 143, 27, 27, 5, 22, 17},
//     {',', 205, 143, 20, 22, 2, 11, 17},
//     {'-', 385, 143, 22, 14, 2, 15, 17},
//     {'.', 317, 143, 18, 17, 0, 11, 17},
//     {'/', 254, 0, 26, 37, 4, 28, 17},
//     {'0', 219, 44, 27, 33, 5, 27, 17},
//     {'1', 407, 78, 26, 32, 4, 26, 17},
//     {'2', 408, 44, 26, 33, 4, 27, 17},
//     {'3', 0, 78, 25, 33, 4, 27, 17},
//     {'4', 270, 78, 28, 32, 5, 26, 17},
//     {'5', 25, 78, 25, 33, 3, 26, 17},
//     {'6', 246, 44, 27, 33, 4, 26, 17},
//     {'7', 433, 78, 26, 32, 4, 26, 17},
//     {'8', 434, 44, 26, 33, 4, 27, 17},
//     {'9', 273, 44, 27, 33, 5, 27, 17},
//     {':', 0, 143, 17, 28, 0, 22, 17},
//     {';', 73, 78, 20, 33, 2, 22, 17},
//     {'<', 252, 111, 24, 29, 4, 23, 17},
//     {'=', 225, 143, 26, 20, 4, 18, 17},
//     {'>', 227, 111, 25, 29, 3, 23, 17},
//     {'?', 153, 44, 22, 34, 1, 28, 17},
//     {'@', 58, 0, 29, 41, 6, 28, 17},
//     {'A', 93, 78, 30, 32, 6, 26, 17},
//     {'B', 459, 78, 26, 32, 4, 26, 17},
//     {'C', 300, 44, 27, 33, 5, 27, 17},
//     {'D', 326, 78, 27, 32, 4, 26, 17},
//     {'E', 155, 111, 24, 32, 3, 26, 17},
//     {'F', 179, 111, 24, 32, 3, 26, 17},
//     {'G', 327, 44, 27, 33, 5, 27, 17},
//     {'H', 353, 78, 27, 32, 4, 26, 17},
//     {'I', 130, 111, 25, 32, 3, 26, 17},
//     {'J', 50, 78, 23, 33, 3, 26, 17},
//     {'K', 0, 111, 26, 32, 4, 26, 17},
//     {'L', 203, 111, 24, 32, 2, 26, 17},
//     {'M', 298, 78, 28, 32, 5, 26, 17},
//     {'N', 26, 111, 26, 32, 4, 26, 17},
//     {'O', 191, 44, 28, 33, 5, 27, 17},
//     {'P', 52, 111, 26, 32, 4, 26, 17},
//     {'Q', 225, 0, 29, 38, 5, 27, 17},
//     {'R', 78, 111, 26, 32, 3, 26, 17},
//     {'S', 460, 44, 26, 33, 4, 27, 17},
//     {'T', 380, 78, 27, 32, 5, 26, 17},
//     {'U', 354, 44, 27, 33, 4, 26, 17},
//     {'V', 123, 78, 30, 32, 6, 26, 17},
//     {'W', 212, 78, 29, 32, 5, 26, 17},
//     {'X', 241, 78, 29, 32, 6, 26, 17},
//     {'Y', 153, 78, 30, 32, 6, 26, 17},
//     {'Z', 104, 111, 26, 32, 4, 26, 17},
//     {'[', 159, 0, 20, 41, 1, 28, 17},
//     {'\\', 280, 0, 26, 37, 4, 28, 17},
//     {']', 179, 0, 20, 41, 2, 28, 17},
//     {'^', 179, 143, 26, 22, 4, 26, 17},
//     {'_', 355, 143, 30, 14, 6, 2, 17},
//     {'`', 335, 143, 20, 16, 3, 28, 17},
//     {'a', 330, 111, 26, 28, 4, 22, 17},
//     {'b', 447, 0, 26, 34, 4, 28, 17},
//     {'c', 382, 111, 25, 28, 4, 22, 17},
//     {'d', 473, 0, 26, 34, 4, 28, 17},
//     {'e', 356, 111, 26, 28, 4, 22, 17},
//     {'f', 391, 0, 28, 34, 5, 28, 17},
//     {'g', 335, 0, 27, 35, 5, 22, 17},
//     {'h', 78, 44, 25, 34, 4, 28, 17},
//     {'i', 103, 44, 25, 34, 3, 28, 17},
//     {'j', 87, 0, 24, 41, 4, 28, 17},
//     {'k', 0, 44, 26, 34, 3, 28, 17},
//     {'l', 128, 44, 25, 34, 3, 28, 17},
//     {'m', 276, 111, 27, 28, 5, 22, 17},
//     {'n', 407, 111, 25, 28, 4, 22, 17},
//     {'o', 303, 111, 27, 28, 5, 22, 17},
//     {'p', 26, 44, 26, 34, 4, 22, 17},
//     {'q', 52, 44, 26, 34, 4, 22, 17},
//     {'r', 432, 111, 25, 28, 3, 22, 17},
//     {'s', 457, 111, 25, 28, 3, 22, 17},
//     {'t', 381, 44, 27, 33, 5, 26, 17},
//     {'u', 482, 111, 25, 28, 4, 21, 17},
//     {'v', 46, 143, 28, 27, 5, 21, 17},
//     {'w', 17, 143, 29, 27, 6, 21, 17},
//     {'x', 74, 143, 28, 27, 5, 21, 17},
//     {'y', 419, 0, 28, 34, 5, 21, 17},
//     {'z', 129, 143, 25, 27, 4, 21, 17},
//     {'{', 111, 0, 24, 41, 4, 28, 17},
//     {'|', 0, 0, 15, 44, -1, 31, 17},
//     {'}', 135, 0, 24, 41, 3, 28, 17},
//     {'~', 289, 143, 28, 18, 5, 18, 17},
// };

// Font get_font_consolas()
// {
//   Bitmap atlas = parse_bitmap(read_entire_file("resources/fonts/consolas.bmp"));
//   Font font = {32, 0, 0, 507, 171};
//   for (int i = 0; i < sizeof(characters_Consolas) / sizeof(ConsolasCharacter); i++)
//   {
//     auto from = characters_Consolas[i];

//     font.characters[from.codePoint] =
//         {
//             from.width,
//             from.height,
//             {from.x / atlas.width, from.y / atlas.height, from.width / atlas.width, from.height / atlas.height},
//             (float)from.originX,
//             (float)from.originY,
//             from.advance,
//         };
//   }
//   font.tex = to_texture(atlas, true);
//   return font;
// }