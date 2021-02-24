#include "stdio.h"

#include <glad/glad.h>

#include "graphics.hpp"
#include "platform.hpp"
#include "math.hpp"

unsigned int basic_vao;
unsigned int textured_vao;
static unsigned int vbo;

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
static void check_error_program(unsigned int program, const char *vert, const char *frag)
{
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("Shader program '%s + %s' linking failed: %s\n", vert, frag, infoLog);
    }
}

unsigned int create_shader_program(const char *vert, const char *frag)
{
    auto vert_shader_file = read_entire_file(vert);
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vert_shader_file.data, NULL);
    glCompileShader(vertexShader);
    check_error_shader(vertexShader, vert);

    auto frag_shader_filename = frag;
    auto frag_shader_file = read_entire_file(frag_shader_filename);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frag_shader_file.data, NULL);
    glCompileShader(fragmentShader);
    check_error_shader(fragmentShader, frag);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    check_error_program(shaderProgram, vert, frag);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

static void init_graphics()
{
    glViewport(0, 0, 1920, 1080);
    glClearColor(powf(.392f, 2.2f), powf(.584f, 2.2f), powf(.929f, 2.2f), 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_FRAMEBUFFER_SRGB);

    basic_shader = create_shader_program("resources/shaders/vert.gl", "resources/shaders/frag.gl");
    basic_shader_resolution_uniform = glGetUniformLocation(basic_shader, "resolution");
    basic_shader_pos_uniform = glGetUniformLocation(basic_shader, "pos");
    basic_shader_scale_uniform = glGetUniformLocation(basic_shader, "scale");
    basic_shader_color_uniform = glGetUniformLocation(basic_shader, "color");

    textured_shader = create_shader_program("resources/shaders/textured_vert.gl", "resources/shaders/textured_frag.gl");
    textured_shader_resolution_uniform = glGetUniformLocation(textured_shader, "resolution");
    textured_shader_pos_uniform = glGetUniformLocation(textured_shader, "pos");
    textured_shader_scale_uniform = glGetUniformLocation(textured_shader, "scale");
    textured_shader_color_uniform = glGetUniformLocation(textured_shader, "color");

    textured_mapped_shader = create_shader_program("resources/shaders/font_atlas_vert.gl", "resources/shaders/font_atlas_frag.gl");
    textured_mapped_shader_resolution_uniform = glGetUniformLocation(textured_mapped_shader, "resolution");
    textured_mapped_shader_pos_uniform = glGetUniformLocation(textured_mapped_shader, "pos");
    textured_mapped_shader_scale_uniform = glGetUniformLocation(textured_mapped_shader, "scale");
    textured_mapped_shader_uv_uniform = glGetUniformLocation(textured_mapped_shader, "uv");

    // square
    float verts[] = {
        0.f,
        0.f,
        1.f,
        1.f,
        0.f,
        1.f,
        0.f,
        0.f,
        1.f,
        0.f,
        1.f,
        1.f,
        // uvs
        0.f,
        0.f,
        1.f,
        1.f,
        0.f,
        1.f,
        0.f,
        0.f,
        1.f,
        0.f,
        1.f,
        1.f,
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glGenVertexArrays(1, &basic_vao);
    glBindVertexArray(basic_vao);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &textured_vao);
    glBindVertexArray(textured_vao);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)(12 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

static void clear_backbuffer()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

static void draw_rect(RenderTarget target, Rect rect, Color color)
{
    glUseProgram(basic_shader);
    glBindVertexArray(basic_vao);
    glUniform2i(basic_shader_resolution_uniform, target.width, target.height);
    glUniform2f(basic_shader_pos_uniform, rect.x, rect.y);
    glUniform2f(basic_shader_scale_uniform, rect.width, rect.height);
    glUniform4f(basic_shader_color_uniform, color.r, color.g, color.b, color.a);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void draw_textured_rect(RenderTarget target, Rect rect, Color color, Texture tex)
{
    glUseProgram(textured_shader);
    glBindVertexArray(textured_vao);
    glUniform2i(textured_shader_resolution_uniform, target.width, target.height);
    glUniform2f(textured_shader_pos_uniform, rect.x, rect.y);
    glUniform2f(textured_shader_scale_uniform, rect.width, rect.height);
    glUniform4f(textured_shader_color_uniform, color.r, color.g, color.b, color.a);

    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void draw_textured_mapped_rect(RenderTarget target, Rect rect, Rect uv, Texture tex)
{
    glUseProgram(textured_mapped_shader);
    glBindVertexArray(textured_vao);
    glUniform2i(textured_mapped_shader_resolution_uniform, target.width, target.height);
    glUniform2f(textured_mapped_shader_pos_uniform, rect.x, rect.y);
    glUniform2f(textured_mapped_shader_scale_uniform, rect.width, rect.height);
    glUniform4f(textured_mapped_shader_uv_uniform, uv.x, uv.y, uv.width, uv.height);

    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static Texture to_texture(Bitmap bitmap, bool smooth)
{
    Texture tex;
    tex.data = bitmap;

    glGenTextures(1, &tex.gl_reference);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, bitmap.width, bitmap.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)bitmap.data);
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}