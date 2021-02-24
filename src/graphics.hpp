#pragma once

#include <assert.h>
#include <stdint.h>

#include "platform.hpp"
#include "math.hpp"

struct Bitmap {
    int width, height;
    Vec4i *data;
};

struct Texture
{
    Bitmap data;
    unsigned int gl_reference;
};

struct RenderTarget
{
    uint32_t width;
    uint32_t height;
};

void init_graphics();
void clear_backbuffer();
void draw_rect(RenderTarget target, Rect rect, Color color);
Texture to_texture(Bitmap bitmap, bool smooth = true);
void draw_textured_rect(RenderTarget target, Rect rect, Color color, Texture tex);
void draw_textured_mapped_rect(RenderTarget target, Rect rect, Rect uv, Texture tex);

static Bitmap parse_bitmap(FileData file_data)
{
    assert(file_data.length > 14 + 40); // Header + InfoHeader
    uint32_t pixels_offset = *(uint32_t *)(file_data.data + 10);

    Bitmap bitmap;
    bitmap.width = *(uint32_t *)(file_data.data + 18);
    bitmap.height = *(uint32_t *)(file_data.data + 22);

    bitmap.data = (Vec4i *)malloc(sizeof(Vec4i) * bitmap.width * bitmap.height);
    for (int y = 0; y < bitmap.height; y++)
    {
        for (int x = 0; x < bitmap.width; x++)
        {
            int file_i = (((y * bitmap.width) + x) * sizeof(Vec4i)) + pixels_offset;
            int bitmap_i = (bitmap.width * (bitmap.height - 1)) - (y * bitmap.width) + (x);

            Vec4i color;
            color.x = *(file_data.data + file_i);
            color.y = *(file_data.data + file_i + 1);
            color.z = *(file_data.data + file_i + 2);
            color.w = *(file_data.data + file_i + 3);
            bitmap.data[bitmap_i] = color;
        }
    }

    return bitmap;
}