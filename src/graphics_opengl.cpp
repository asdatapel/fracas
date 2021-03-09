#include "stdio.h"

#include <glad/glad.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

#include "graphics.hpp"
#include "platform.hpp"
#include "math.hpp"
#include "mesh.hpp"

unsigned int temp_fbo;
unsigned int temp_rbo;

unsigned int basic_shader;
static int basic_shader_resolution_uniform;
static int basic_shader_pos_uniform;
static int basic_shader_scale_uniform;
static int basic_shader_color_uniform;

unsigned int textured_shader;
static int textured_shader_resolution_uniform;
static int textured_shader_pos_uniform;
static int textured_shader_scale_uniform;
static int textured_shader_color_uniform;

unsigned int textured_mapped_shader;
static int textured_mapped_shader_resolution_uniform;
static int textured_mapped_shader_pos_uniform;
static int textured_mapped_shader_scale_uniform;
static int textured_mapped_shader_uv_uniform;

unsigned int threed_shader;
static int threed_shader_t_uniform;

unsigned int bar_shader;

unsigned int rect_to_cubemap_shader;
unsigned int cubemap_shader;
unsigned int irradiance_shader;
unsigned int env_filter_shader;
unsigned int brdf_lut_shader;

unsigned int screen_quad_vao;
unsigned int screen_quad_vbo;
float screen_quad_verts[] = {
    -1.0f, 1.0f, 0.0f, 1.0f,  //
    -1.0f, -1.0f, 0.0f, 0.0f, //
    1.0f, 1.0f, 1.0f, 1.0f,   //
    1.0f, -1.0f, 1.0f, 0.0f,  //
};

unsigned int cube_vao;
unsigned int cube_vbo;
float cube_verts[] = {
    // back face
    -1.0f, -1.0f, -1.0f, // bottom-left
    1.0f, -1.0f, -1.0f,  // bottom-right
    1.0f, 1.0f, -1.0f,   // top-right
    1.0f, 1.0f, -1.0f,   // top-right
    -1.0f, 1.0f, -1.0f,  // top-left
    -1.0f, -1.0f, -1.0f, // bottom-left
    // front face
    -1.0f, -1.0f, 1.0f, // bottom-left
    1.0f, 1.0f, 1.0f,   // top-right
    1.0f, -1.0f, 1.0f,  // bottom-right
    1.0f, 1.0f, 1.0f,   // top-right
    -1.0f, -1.0f, 1.0f, // bottom-left
    -1.0f, 1.0f, 1.0f,  // top-left
    // left face
    -1.0f, 1.0f, 1.0f,   // top-right
    -1.0f, -1.0f, -1.0f, // bottom-left
    -1.0f, 1.0f, -1.0f,  // top-left
    -1.0f, -1.0f, -1.0f, // bottom-left
    -1.0f, 1.0f, 1.0f,   // top-right
    -1.0f, -1.0f, 1.0f,  // bottom-right
    // right face
    1.0f, 1.0f, 1.0f,   // top-left
    1.0f, 1.0f, -1.0f,  // top-right
    1.0f, -1.0f, -1.0f, // bottom-right
    1.0f, -1.0f, -1.0f, // bottom-right
    1.0f, -1.0f, 1.0f,  // bottom-left
    1.0f, 1.0f, 1.0f,   // top-left
    // bottom face
    -1.0f, -1.0f, -1.0f, // top-right
    1.0f, -1.0f, 1.0f,   // bottom-left
    1.0f, -1.0f, -1.0f,  // top-left
    1.0f, -1.0f, 1.0f,   // bottom-left
    -1.0f, -1.0f, -1.0f, // top-right
    -1.0f, -1.0f, 1.0f,  // bottom-right
    // top face
    -1.0f, 1.0f, -1.0f, // top-left
    1.0f, 1.0f, -1.0f,  // top-right
    1.0f, 1.0f, 1.0f,   // bottom-right
    1.0f, 1.0f, 1.0f,   // bottom-right
    -1.0f, 1.0f, 1.0f,  // bottom-left
    -1.0f, 1.0f, -1.0f, // top-left
};

static void check_error_shader(unsigned int shader, const char *name)
{
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Shader '%s' compilation failed: %s\n", name, infoLog);
    }
}
static void check_error_program(unsigned int program, const char *folder)
{
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("Shader program '%s' linking failed: %s\n", folder, infoLog);
    }
}

unsigned int create_shader_program(const char *folder)
{
    int folder_len = strlen(folder);
    char path[1024];
    strcpy_s(path, folder);

    strcpy_s(path + folder_len, sizeof("/vert.gl"), "/vert.gl");
    auto vert_shader_file = read_entire_file(path);
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vert_shader_file.data, NULL);
    glCompileShader(vertexShader);
    check_error_shader(vertexShader, path);

    strcpy_s(path + folder_len, sizeof("/frag.gl"), "/frag.gl");
    auto frag_shader_filename = path;
    auto frag_shader_file = read_entire_file(frag_shader_filename);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frag_shader_file.data, NULL);
    glCompileShader(fragmentShader);
    check_error_shader(fragmentShader, path);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    check_error_program(shaderProgram, folder);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    free(vert_shader_file.data);
    free(frag_shader_file.data);

    return shaderProgram;
}

static void init_graphics(RenderTarget target)
{
    glViewport(0, 0, target.width, target.height);
    glClearColor(powf(.392f, 2.2f), powf(.584f, 2.2f), powf(.929f, 2.2f), 1.0f);

    glGenFramebuffers(1, &temp_fbo);
    glGenRenderbuffers(1, &temp_rbo);

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    basic_shader = create_shader_program("resources/shaders/basic");
    basic_shader_resolution_uniform = glGetUniformLocation(basic_shader, "resolution");
    basic_shader_pos_uniform = glGetUniformLocation(basic_shader, "pos");
    basic_shader_scale_uniform = glGetUniformLocation(basic_shader, "scale");
    basic_shader_color_uniform = glGetUniformLocation(basic_shader, "color");

    textured_shader = create_shader_program("resources/shaders/textured");
    textured_shader_resolution_uniform = glGetUniformLocation(textured_shader, "resolution");
    textured_shader_pos_uniform = glGetUniformLocation(textured_shader, "pos");
    textured_shader_scale_uniform = glGetUniformLocation(textured_shader, "scale");
    textured_shader_color_uniform = glGetUniformLocation(textured_shader, "color");

    textured_mapped_shader = create_shader_program("resources/shaders/font_atlas");
    textured_mapped_shader_resolution_uniform = glGetUniformLocation(textured_mapped_shader, "resolution");
    textured_mapped_shader_pos_uniform = glGetUniformLocation(textured_mapped_shader, "pos");
    textured_mapped_shader_scale_uniform = glGetUniformLocation(textured_mapped_shader, "scale");
    textured_mapped_shader_uv_uniform = glGetUniformLocation(textured_mapped_shader, "uv");

    glGenVertexArrays(1, &screen_quad_vao);
    glGenBuffers(1, &screen_quad_vbo);
    glBindVertexArray(screen_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, screen_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_quad_verts), &screen_quad_verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    threed_shader = create_shader_program("resources/shaders/threed");
    threed_shader_t_uniform = glGetUniformLocation(threed_shader, "t");

    bar_shader = create_shader_program("resources/shaders/bar");

    rect_to_cubemap_shader = create_shader_program("resources/shaders/rect_to_cubemap");
    cubemap_shader = create_shader_program("resources/shaders/cubemap");
    irradiance_shader = create_shader_program("resources/shaders/irradiance");
    env_filter_shader = create_shader_program("resources/shaders/env_filter");
    brdf_lut_shader = create_shader_program("resources/shaders/brdf_lut");

    glGenVertexArrays(1, &cube_vao);
    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW);
    glBindVertexArray(cube_vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void clear_backbuffer()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void draw_threed(RenderTarget target, Camera camera, glm::mat4 model, VertexBuffer buf, Material material, Texture cubemap, Texture env_map, Texture brdf_lut)
{
    glUseProgram(threed_shader);

    static float t = 0.0f;
    t += 0.01f;
    glUniform1f(threed_shader_t_uniform, t);

    glUniformMatrix4fv(glGetUniformLocation(threed_shader, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(threed_shader, "view"), 1, GL_FALSE, &camera.view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(threed_shader, "projection"), 1, GL_FALSE, &camera.perspective[0][0]);
    glUniform3f(glGetUniformLocation(threed_shader, "cam_pos"), camera.pos_x, camera.pos_y, camera.pos_z);

    glUniform1i(glGetUniformLocation(threed_shader, "albedo_tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material.albedo.gl_reference);
    glUniform1i(glGetUniformLocation(threed_shader, "normal_tex"), 1);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, material.normal.gl_reference);
    glUniform1i(glGetUniformLocation(threed_shader, "metal_tex"), 2);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, material.metal.gl_reference);
    glUniform1i(glGetUniformLocation(threed_shader, "roughness_tex"), 3);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, material.roughness.gl_reference);
    glUniform1i(glGetUniformLocation(threed_shader, "irradiance"), 4);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.gl_reference);
    glUniform1i(glGetUniformLocation(threed_shader, "env"), 5);
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_map.gl_reference);
    glUniform1i(glGetUniformLocation(threed_shader, "brdf"), 6);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_2D, brdf_lut.gl_reference);

    glBindVertexArray(buf.vao);
    glDrawArrays(GL_TRIANGLES, 0, buf.vert_count);
}

static void draw_bar(RenderTarget target, Camera camera, glm::mat4 model, VertexBuffer buf, Material material, Texture cubemap, Texture env_map, Texture brdf_lut, Texture text)
{
    glUseProgram(bar_shader);

    static float t = 0.0f;
    t += 0.01f;
    glUniform1f(glGetUniformLocation(bar_shader, "t"), t);

    glUniformMatrix4fv(glGetUniformLocation(bar_shader, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(bar_shader, "view"), 1, GL_FALSE, &camera.view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(bar_shader, "projection"), 1, GL_FALSE, &camera.perspective[0][0]);
    glUniform3f(glGetUniformLocation(bar_shader, "cam_pos"), camera.pos_x, camera.pos_y, camera.pos_z);

    glUniform1i(glGetUniformLocation(bar_shader, "albedo_tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material.albedo.gl_reference);
    glUniform1i(glGetUniformLocation(bar_shader, "normal_tex"), 1);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, material.normal.gl_reference);
    glUniform1i(glGetUniformLocation(bar_shader, "metal_tex"), 2);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, material.metal.gl_reference);
    glUniform1i(glGetUniformLocation(bar_shader, "roughness_tex"), 3);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, material.roughness.gl_reference);
    glUniform1i(glGetUniformLocation(bar_shader, "irradiance"), 4);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.gl_reference);
    glUniform1i(glGetUniformLocation(bar_shader, "env"), 5);
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_map.gl_reference);
    glUniform1i(glGetUniformLocation(bar_shader, "brdf"), 6);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_2D, brdf_lut.gl_reference);
    glUniform1i(glGetUniformLocation(bar_shader, "text_tex"), 7);
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_2D, text.gl_reference);

    glBindVertexArray(buf.vao);
    glDrawArrays(GL_TRIANGLES, 0, buf.vert_count);
}

static void draw_rect(RenderTarget target, Rect rect, Color color)
{
    glUseProgram(basic_shader);
    glUniform2i(basic_shader_resolution_uniform, target.width, target.height);
    glUniform2f(basic_shader_pos_uniform, rect.x, rect.y);
    glUniform2f(basic_shader_scale_uniform, rect.width, rect.height);
    glUniform4f(basic_shader_color_uniform, color.r, color.g, color.b, color.a);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void draw_textured_rect(RenderTarget target, Rect rect, Color color, Texture tex)
{
    glUseProgram(textured_shader);
    glUniform2i(textured_shader_resolution_uniform, target.width, target.height);
    glUniform2f(textured_shader_pos_uniform, rect.x, rect.y);
    glUniform2f(textured_shader_scale_uniform, rect.width, rect.height);
    glUniform4f(textured_shader_color_uniform, color.r, color.g, color.b, color.a);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void draw_textured_mapped_rect(RenderTarget target, Rect rect, Rect uv, Texture tex)
{
    glUseProgram(textured_mapped_shader);
    glUniform2i(textured_mapped_shader_resolution_uniform, target.width, target.height);
    glUniform2f(textured_mapped_shader_pos_uniform, rect.x, rect.y);
    glUniform2f(textured_mapped_shader_scale_uniform, rect.width, rect.height);
    glUniform4f(textured_mapped_shader_uv_uniform, uv.x, uv.y, uv.width, uv.height);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static Texture to_texture(Bitmap bitmap, bool mipmaps)
{
    Texture tex;
    tex.width = bitmap.width;
    tex.height = bitmap.height;

    glGenTextures(1, &tex.gl_reference);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, bitmap.width, bitmap.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)bitmap.data);

    if (mipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);

    return tex;
}

static Texture to_texture(float *data, int width, int height)
{
    Texture tex;
    tex.width = width;
    tex.height = height;

    glGenTextures(1, &tex.gl_reference);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    return tex;
}

static Texture new_float_tex(int width, int height, bool mipmaps = false)
{
    Texture tex;
    tex.width = width;
    tex.height = height;

    glGenTextures(1, &tex.gl_reference);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

    return tex;
}

static Texture new_cubemap_tex(int square_width, bool mipmaps = false)
{
    Texture tex;
    tex.width = square_width;
    tex.height = square_width;

    glGenTextures(1, &tex.gl_reference);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex.gl_reference);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, square_width, square_width, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

    if (mipmaps)
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    return tex;
}

void gen_cubemap_mips(Texture cubemap)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.gl_reference);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

VertexBuffer upload_vertex_buffer(Mesh mesh)
{
    VertexBuffer ret;
    ret.size = mesh.buf_size;
    ret.vert_count = mesh.verts;

    glGenVertexArrays(1, &ret.vao);
    glBindVertexArray(ret.vao);
    glGenBuffers(1, &ret.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ret.vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh.buf_size, mesh.data, GL_STATIC_DRAW);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
    // glEnableVertexAttribArray(2);
    for (int i = 0; i < mesh.components_count; i++)
    {
        Component *c = mesh.components + i;
        glVertexAttribPointer(i, c->size, GL_FLOAT, GL_FALSE, c->stride * sizeof(float), (void *)(c->offset * sizeof(float)));
        glEnableVertexAttribArray(i);
    }

    return ret;
}

Texture hdri_to_cubemap(Texture hdri, int size)
{
    Texture target = new_cubemap_tex(size, true);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    glUseProgram(rect_to_cubemap_shader);
    glUniformMatrix4fv(glGetUniformLocation(rect_to_cubemap_shader, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
    glUniform1i(glGetUniformLocation(rect_to_cubemap_shader, "equirectangularMap"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdri.gl_reference);

    glBindFramebuffer(GL_FRAMEBUFFER, temp_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, temp_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, target.width, target.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, temp_rbo);

    glViewport(0, 0, target.width, target.height);
    glBindFramebuffer(GL_FRAMEBUFFER, temp_fbo);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(glGetUniformLocation(rect_to_cubemap_shader, "view"), 1, GL_FALSE, &captureViews[i][0][0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, target.gl_reference, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(cube_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glViewport(0, 0, 1920, 1080);

    gen_cubemap_mips(target);
    return target;
}

void draw_cubemap(Texture tex, Camera camera)
{
    glDepthFunc(GL_LEQUAL);

    glUseProgram(cubemap_shader);
    glUniformMatrix4fv(glGetUniformLocation(cubemap_shader, "view"), 1, GL_FALSE, &camera.view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(cubemap_shader, "projection"), 1, GL_FALSE, &camera.perspective[0][0]);

    glUniform1i(glGetUniformLocation(cubemap_shader, "environmentMap"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex.gl_reference);

    glBindVertexArray(cube_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthFunc(GL_LESS);
}

Texture convolve_irradiance_map(Texture src, int size)
{
    Texture target = new_cubemap_tex(size);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    glBindFramebuffer(GL_FRAMEBUFFER, temp_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, temp_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, target.width, target.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, temp_rbo);

    glUseProgram(irradiance_shader);
    glUniformMatrix4fv(glGetUniformLocation(irradiance_shader, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
    glUniform1i(glGetUniformLocation(irradiance_shader, "environmentMap"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, src.gl_reference);

    glViewport(0, 0, target.width, target.height);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(glGetUniformLocation(irradiance_shader, "view"), 1, GL_FALSE, &captureViews[i][0][0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, target.gl_reference, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(cube_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glViewport(0, 0, 1920, 1080);

    return target;
}

Texture filter_env_map(Texture src, int size)
{
    Texture ret = new_cubemap_tex(size, true);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    glUseProgram(env_filter_shader);
    glUniformMatrix4fv(glGetUniformLocation(env_filter_shader, "projection"), 1, GL_FALSE, &captureProjection[0][0]);
    glUniform1i(glGetUniformLocation(env_filter_shader, "environmentMap"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, src.gl_reference);

    glBindFramebuffer(GL_FRAMEBUFFER, temp_fbo);
    unsigned int maxMipLevels = 9; // assuming 512x512 texture
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = size * powf(0.5, mip);
        unsigned int mipHeight = size * powf(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, temp_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        glUniform1f(glGetUniformLocation(env_filter_shader, "roughness"), roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(glGetUniformLocation(env_filter_shader, "view"), 1, GL_FALSE, &captureViews[i][0][0]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ret.gl_reference, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(cube_vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glViewport(0, 0, 1920, 1080);

    return ret;
}

Texture generate_brdf_lut(int size)
{
    Texture target = new_float_tex(size, size, false);

    glBindFramebuffer(GL_FRAMEBUFFER, temp_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, temp_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, target.width, target.height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.gl_reference, 0);

    glViewport(0, 0, target.width, target.height);
    glUseProgram(brdf_lut_shader);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glViewport(0, 0, 1920, 1080);
    
    return target;
}