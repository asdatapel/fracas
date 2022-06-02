#pragma once

#include <assert.h>
#include <stdint.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"

#include "../math.hpp"
#include "../mesh.hpp"
#include "../platform.hpp"
#include "framebuffer.hpp"
#include "compute_shader.hpp"
#include "shader.hpp"

Shader basic_shader;
Shader lines_shader;
Shader textured_shader;
Shader textured_mapped_shader;
Shader single_channel_font_shader;
Shader blurred_colors_shader;
Shader rect_to_cubemap_shader;
Shader cubemap_shader;
Shader irradiance_shader;
Shader env_filter_shader;
Shader brdf_lut_shader;
Shader tonemap_shader;
Shader blur_shader;
Shader add_shader;
Shader twod_shader;
Shader shadow_shader;
Shader sky_shader;
Shader probe_debug_shader;

ComputeShader rtshadow_compute_shader;

// TODO move this to the asset loading system
Shader threed_skinning_shader;

struct Bitmap {
  int width, height;
  Vec4i *data;
};

struct VertexBuffer {
  unsigned int vao;
  unsigned int vbo;
  int size;
  int vert_count;
};

RenderTarget init_graphics(uint32_t width, uint32_t height);

ComputeShader load_compute_shader(const char *filepath);
Shader create_shader(String vert_src, String frag_src, const char *debug_name = "");
void bind_shader(Shader shader);
void bind_1f(Shader shader, UniformId uniform_id, float val);
void bind_1i(Shader shader, UniformId uniform_id, int val);
void bind_2i(Shader shader, UniformId uniform_id, int i1, int i2);
void bind_2f(Shader shader, UniformId uniform_id, float f1, float f2);
void bind_4f(Shader shader, UniformId uniform_id, float f1, float f2, float f3, float f4);
void bind_mat4(Shader shader, UniformId uniform_id, glm::mat4 mat);
void bind_texture(Shader shader, int texture_slot, Texture texture);
void bind_texture(Shader shader, UniformId uniform_id, Texture texture);

void debug_begin_immediate();
void debug_end_immediate();
void debug_draw_immediate(RenderTarget target, Vec2f v1, Vec2f v2, Vec2f v3, Vec2f v4, Color color);
void debug_draw_immediate(RenderTarget target, Vec2f v1, Vec2f v2, Vec2f v3, Color color);
void debug_draw_immediate(RenderTarget target, Rect rect, Color color);
void debug_draw_lines(RenderTarget target, float *lines, int count);

void draw(RenderTarget target, Shader shader, VertexBuffer buf);
void draw_rect();
void draw_cube();
void draw_rect(RenderTarget target, Rect rect, Color color);
void draw_textured_rect(RenderTarget target, Rect rect, Color color, Texture tex);
void draw_textured_mapped_rect(RenderTarget target, Rect rect, Rect uv, Texture tex,
                               Color color = {1, 1, 1, 1});
void draw_single_channel_text(RenderTarget target, Rect rect, Rect uv, Texture tex);
void draw_cubemap();

void start_scissor(RenderTarget target, Rect rect);
void end_scissor();

void draw_sky(Cubemap cubemap, float dir_t = 0.f);

Texture hdri_to_cubemap(RenderTarget target, Texture hdri, int size);
void convolve_irradiance_map(RenderTarget target, Texture src, Cubemap target_cubemap);
void filter_env_map(RenderTarget target, Texture src, Cubemap target_cubemap);
VertexBuffer upload_vertex_buffer(Mesh mesh);

const int MAX_LIGHTS = 50;
struct PointLight {
  glm::vec3 position;
  glm::vec3 color;

  const static int SIZE = 32;
};
struct SpotLight {
  glm::vec4 position;
  glm::vec4 direction;
  glm::vec4 color;
  
  float inner_angle;
  float outer_angle;

  i32 shadow_map_index = -1;
  glm::mat4 lightspace_mat = glm::mat4(1.0);

  const static int SIZE = 128;
};
struct DirectionalLight {
  glm::vec4 direction;
  glm::vec4 color;
  
  i32 shadow_map_index = -1;
  glm::mat4 lightspace_mat = glm::mat4(1.0);

  const static int SIZE = 112;
};
struct LightUniformBlock {
  DirectionalLight directional_light;
  SpotLight spot_lights[MAX_LIGHTS];
  uint32_t num_lights;

  const static int SIZE = DirectionalLight::SIZE + (SpotLight::SIZE * MAX_LIGHTS) + 4;
};
void upload_lights(LightUniformBlock lights);

static Bitmap parse_bitmap(FileData file_data, StackAllocator *allocator)
{
  assert(file_data.length > 14 + 40);  // Header + InfoHeader
  uint32_t pixels_offset = *(uint32_t *)(file_data.data + 10);

  Bitmap bitmap;
  bitmap.width  = *(uint32_t *)(file_data.data + 18);
  bitmap.height = *(uint32_t *)(file_data.data + 22);

  u16 pixel_size = *(u16 *)(file_data.data + 28);

  bitmap.data = (Vec4i *)allocator->alloc(sizeof(Vec4i) * bitmap.width * bitmap.height);
  for (int y = 0; y < bitmap.height; y++) {
    for (int x = 0; x < bitmap.width; x++) {
      int file_i = (((y * bitmap.width) + x) * sizeof(Vec4i)) + pixels_offset;
      // int bitmap_i = (bitmap.width * (bitmap.height - 1)) - (y * bitmap.width) + (x);
      int bitmap_i = (y * bitmap.width) + x;

      Vec4i color;
      color.z               = *(file_data.data + file_i);
      color.y               = *(file_data.data + file_i + 1);
      color.x               = *(file_data.data + file_i + 2);
      color.w               = *(file_data.data + file_i + 3);
      bitmap.data[bitmap_i] = color;
    }
  }

  return bitmap;
}

static void free_bitmap(Bitmap bitmap) { free(bitmap.data); }

GLuint bvh_ssbo;
