#pragma once

#include <assert.h>
#include <stdint.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

#include "math.hpp"
#include "mesh.hpp"
#include "platform.hpp"
#include "shader.hpp"

// TODO move this to the asset loading system
Shader basic_shader;
Shader textured_shader;
Shader textured_mapped_shader;
Shader blurred_colors_shader;
Shader threed_shader;
Shader bar_shader;
Shader rect_to_cubemap_shader;
Shader cubemap_shader;
Shader irradiance_shader;
Shader env_filter_shader;
Shader brdf_lut_shader;

struct Bitmap
{
    int width, height;
    Vec4i *data;
};

struct Texture
{
    enum struct Type
    {
        _2D,
        CUBEMAP,
    };
    Type type;
    unsigned int gl_reference;
    int width, height;
};

struct RenderTarget
{
    uint32_t width, height;
    unsigned int gl_fbo;
    unsigned int gl_depth_buffer;
    Texture color_tex;
};

struct VertexBuffer
{
    unsigned int vao;
    unsigned int vbo;
    int size;
    int vert_count;
};

RenderTarget init_graphics(RenderTarget target);

Texture to_texture(Bitmap bitmap, bool mipmaps = true);
Texture to_texture(float *data, int width, int height);
void gen_mips(Texture tex);
Texture to_single_channel_texture(uint8_t *data, int width, int height, bool mipmaps);
Texture hdri_to_cubemap(Texture hdri, int size);
Texture convolve_irradiance_map(Texture src, int size);
Texture filter_env_map(Texture src, int size);
Texture generate_brdf_lut(int size);
VertexBuffer upload_vertex_buffer(Mesh mesh);
RenderTarget new_render_target(uint32_t width, uint32_t height, bool depth = false);

void bind(RenderTarget target);
void bind_shader(Shader shader);
void bind_1f(Shader shader, UniformId uniform_id, float val);
void bind_2i(Shader shader, UniformId uniform_id, int i1, int i2);
void bind_2f(Shader shader, UniformId uniform_id, float f1, float f2);
void bind_4f(Shader shader, UniformId uniform_id, float f1, float f2, float f3, float f4);
void bind_mat4(Shader shader, UniformId uniform_id, glm::mat4 mat);
void bind_texture(Shader shader, UniformId uniform_id, Texture texture);

void draw(RenderTarget target, Shader shader, VertexBuffer buf);
void draw_rect(RenderTarget target, Rect rect, Color color);
void draw_textured_rect(RenderTarget target, Rect rect, Color color, Texture tex);
void draw_textured_mapped_rect(RenderTarget target, Rect rect, Rect uv, Texture tex);
void draw_cubemap();

void clear_backbuffer();

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
            // int bitmap_i = (bitmap.width * (bitmap.height - 1)) - (y * bitmap.width) + (x);
            int bitmap_i = (y * bitmap.width) + x;

            Vec4i color;
            color.z = *(file_data.data + file_i);
            color.y = *(file_data.data + file_i + 1);
            color.x = *(file_data.data + file_i + 2);
            color.w = *(file_data.data + file_i + 3);
            bitmap.data[bitmap_i] = color;
        }
    }

    return bitmap;
}

static void free_bitmap(Bitmap bitmap)
{
    free(bitmap.data);
}