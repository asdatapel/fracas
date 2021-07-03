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
#include "graphics/framebuffer.hpp"

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
Shader tonemap_shader;
Shader blur_shader;

struct Bitmap
{
    int width, height;
    Vec4i *data;
};

struct VertexBuffer
{
    unsigned int vao;
    unsigned int vbo;
    int size;
    int vert_count;
};

RenderTarget init_graphics(RenderTarget target);

void bind_shader(Shader shader);
void bind_1f(Shader shader, UniformId uniform_id, float val);
void bind_1i(Shader shader, UniformId uniform_id, int val);
void bind_2i(Shader shader, UniformId uniform_id, int i1, int i2);
void bind_2f(Shader shader, UniformId uniform_id, float f1, float f2);
void bind_4f(Shader shader, UniformId uniform_id, float f1, float f2, float f3, float f4);
void bind_mat4(Shader shader, UniformId uniform_id, glm::mat4 mat);
void bind_texture(Shader shader, UniformId uniform_id, Texture texture);

void draw(RenderTarget target, Shader shader, VertexBuffer buf);
void draw_rect();
void draw_rect(RenderTarget target, Rect rect, Color color);
void draw_textured_rect(RenderTarget target, Rect rect, Color color, Texture tex);
void draw_textured_mapped_rect(RenderTarget target, Rect rect, Rect uv, Texture tex);
void draw_cubemap();

Texture hdri_to_cubemap(RenderTarget target, Texture hdri, int size);
Texture convolve_irradiance_map(RenderTarget target, Texture src, int size);
Texture filter_env_map(RenderTarget target, Texture src, int size);
VertexBuffer upload_vertex_buffer(Mesh mesh);

const int MAX_LIGHTS = 10;
struct PointLight
{
    glm::vec3 position;
    glm::vec3 color;

    const static int SIZE = 32;
};
struct SpotLight
{
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float inner_angle;
    float outer_angle;
    
    const static int SIZE = 64;
};
struct LightUniformBlock
{
    SpotLight spot_lights[MAX_LIGHTS];
    uint32_t num_lights;

    const static int SIZE = (SpotLight::SIZE * 10) + 4;
};
void update_lights(LightUniformBlock lights);

static Bitmap parse_bitmap(FileData file_data, StackAllocator *allocator)
{
    assert(file_data.length > 14 + 40); // Header + InfoHeader
    uint32_t pixels_offset = *(uint32_t *)(file_data.data + 10);

    Bitmap bitmap;
    bitmap.width = *(uint32_t *)(file_data.data + 18);
    bitmap.height = *(uint32_t *)(file_data.data + 22);

    bitmap.data = (Vec4i *)allocator->alloc(sizeof(Vec4i) * bitmap.width * bitmap.height);
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