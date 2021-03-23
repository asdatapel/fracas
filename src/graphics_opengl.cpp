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
#include "shader.hpp"

RenderTarget main_target;

unsigned int temp_fbo;
unsigned int temp_rbo;

unsigned int basic_shader;
unsigned int textured_shader;
unsigned int textured_mapped_shader;
unsigned int threed_shader;
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

static void check_shader_error(unsigned int shader, const char *name)
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
    check_shader_error(vertexShader, path);

    strcpy_s(path + folder_len, sizeof("/frag.gl"), "/frag.gl");
    auto frag_shader_filename = path;
    auto frag_shader_file = read_entire_file(frag_shader_filename);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frag_shader_file.data, NULL);
    glCompileShader(fragmentShader);
    check_shader_error(fragmentShader, path);

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

static RenderTarget init_graphics(uint32_t width, uint32_t height)
{
    glViewport(0, 0, width, height);
    glClearColor(powf(.392f, 2.2f), powf(.584f, 2.2f), powf(.929f, 2.2f), 0.0f);

    main_target = {width, height};
    main_target.gl_fbo = 0;

    glGenFramebuffers(1, &temp_fbo);
    glGenRenderbuffers(1, &temp_rbo);

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glGenVertexArrays(1, &screen_quad_vao);
    glGenBuffers(1, &screen_quad_vbo);
    glBindVertexArray(screen_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, screen_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_quad_verts), &screen_quad_verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    glGenVertexArrays(1, &cube_vao);
    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW);
    glBindVertexArray(cube_vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    basic_shader = create_shader_program("resources/shaders/basic");
    textured_shader = create_shader_program("resources/shaders/textured");
    textured_mapped_shader = create_shader_program("resources/shaders/font_atlas");
    threed_shader = create_shader_program("resources/shaders/threed");
    bar_shader = create_shader_program("resources/shaders/bar");
    rect_to_cubemap_shader = create_shader_program("resources/shaders/rect_to_cubemap");
    cubemap_shader = create_shader_program("resources/shaders/cubemap");
    irradiance_shader = create_shader_program("resources/shaders/irradiance");
    env_filter_shader = create_shader_program("resources/shaders/env_filter");
    brdf_lut_shader = create_shader_program("resources/shaders/brdf_lut");

    return main_target;
}

static void clear_backbuffer()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

static Texture to_single_channel_texture(uint8_t *data, int width, int height, bool mipmaps)
{
    Texture tex;
    tex.width = width;
    tex.height = height;

    glGenTextures(1, &tex.gl_reference);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RED, GL_UNSIGNED_BYTE, data);

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

static Texture new_tex(int width, int height, bool mipmaps = false)
{
    Texture tex;
    tex.width = width;
    tex.height = height;

    glGenTextures(1, &tex.gl_reference);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, 0);
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

void gen_mips(Texture tex)
{
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);
    glGenerateMipmap(GL_TEXTURE_2D);
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

    for (int i = 0; i < mesh.components_count; i++)
    {
        Component *c = mesh.components + i;
        glVertexAttribPointer(i, c->size, GL_FLOAT, GL_FALSE, c->stride * sizeof(float), (void *)(c->offset * sizeof(float)));
        glEnableVertexAttribArray(i);
    }

    return ret;
}

RenderTarget new_render_target(uint32_t width, uint32_t height, bool depth = false)
{
    RenderTarget target{width, height};
    glGenFramebuffers(1, &target.gl_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, target.gl_fbo);
    // glGenRenderbuffers(1, &target.gl_depth_buffer);
    // glBindRenderbuffer(GL_RENDERBUFFER, target.gl_depth_buffer);
    // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, temp_rbo);
    
    target.color_tex = new_tex(width, height, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.color_tex.gl_reference, 0);

    return target;
}

void bind(RenderTarget target)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target.gl_fbo);
    glViewport(0, 0, target.width, target.height);
}

static void bind_shader(Shader shader)
{
    glUseProgram(shader.shader_handle);
}

static void bind_camera(Shader shader, Camera camera)
{
    glUniformMatrix4fv(shader.uniform_handles[(int)Shader::UniformId::VIEW], 1, GL_FALSE, &camera.view[0][0]);
    glUniformMatrix4fv(shader.uniform_handles[(int)Shader::UniformId::PROJECTION], 1, GL_FALSE, &camera.perspective[0][0]);
    glUniform3f(shader.uniform_handles[(int)Shader::UniformId::CAMERA_POSITION], camera.pos_x, camera.pos_y, camera.pos_z);
}

static void bind_mat4(Shader shader, Shader::UniformId uniform_id, glm::mat4 mat)
{
    glUniformMatrix4fv(shader.uniform_handles[(int)uniform_id], 1, GL_FALSE, &mat[0][0]);
}

static void bind_cubemap_texture(Shader shader, Shader::UniformId uniform_id, Texture texture)
{
    glUniform1i(shader.uniform_handles[(int)uniform_id], shader.tex_units[(int)uniform_id]);
    glActiveTexture(GL_TEXTURE0 + shader.tex_units[(int)uniform_id]);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture.gl_reference);
}

static void bind_texture(Shader shader, Shader::UniformId uniform_id, Texture texture)
{
    glUniform1i(shader.uniform_handles[(int)uniform_id], shader.tex_units[(int)uniform_id]);
    glActiveTexture(GL_TEXTURE0 + shader.tex_units[(int)uniform_id]);
    glBindTexture(GL_TEXTURE_2D, texture.gl_reference);
}

static void bind_material(Shader shader, Material material)
{
    for (int i = 0; i < material.num_textures; i++)
    {
        glUniform1i(shader.uniform_handles[(int)material.uniform_ids[i]], shader.tex_units[(int)material.uniform_ids[i]]);
        glActiveTexture(GL_TEXTURE0 + shader.tex_units[(int)material.uniform_ids[i]]);
        glBindTexture(GL_TEXTURE_2D, material.textures[i].gl_reference);
    }
}

static void draw(RenderTarget target, Shader shader, VertexBuffer buf)
{
    static float t = 0.0f;
    t += 0.01f;
    glUniform1f(shader.uniform_handles[(int)Shader::UniformId::T], t);

    glBindVertexArray(buf.vao);
    glDrawArrays(GL_TRIANGLES, 0, buf.vert_count);
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

    glUniform1i(glGetUniformLocation(cubemap_shader, "env_map"), 0);
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
    glUniform1i(glGetUniformLocation(irradiance_shader, "env_map"), 0);
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
    glUniform1i(glGetUniformLocation(env_filter_shader, "env_map"), 0);
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
    auto new_float_tex = [](int width, int height, bool mipmaps = false) {
        Texture tex;
        tex.width = width;
        tex.height = height;

        glGenTextures(1, &tex.gl_reference);
        glBindTexture(GL_TEXTURE_2D, tex.gl_reference);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);

        return tex;
    };

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

// static void draw_threed(RenderTarget target, Camera camera, glm::mat4 model, VertexBuffer buf, Material material, Texture cubemap, Texture env_map, Texture brdf_lut)
// {
//     glUseProgram(threed_shader);

//     unsigned int t_uniform = glGetUniformLocation(threed_shader, "t");
//     unsigned int model_uniform = glGetUniformLocation(threed_shader, "model");
//     unsigned int view_uniform = glGetUniformLocation(threed_shader, "view");
//     unsigned int projection_uniform = glGetUniformLocation(threed_shader, "projection");
//     unsigned int camera_position_uniform = glGetUniformLocation(threed_shader, "camera_position");
//     unsigned int albedo_tex_uniform = glGetUniformLocation(threed_shader, "albedo_texture");
//     unsigned int normal_tex_uniform = glGetUniformLocation(threed_shader, "normal_texture");
//     unsigned int metal_tex_uniform = glGetUniformLocation(threed_shader, "metal_texture");
//     unsigned int roughness_tex_uniform = glGetUniformLocation(threed_shader, "roughness_texture");
//     unsigned int irradiance_uniform = glGetUniformLocation(threed_shader, "irradiance");
//     unsigned int env_uniform = glGetUniformLocation(threed_shader, "env_map");
//     unsigned int brdf_uniform = glGetUniformLocation(threed_shader, "brdf");

//     static float t = 0.0f;
//     t += 0.01f;
//     glUniform1f(t_uniform, t);

//     glUniformMatrix4fv(model_uniform, 1, GL_FALSE, &model[0][0]);
//     glUniformMatrix4fv(view_uniform, 1, GL_FALSE, &camera.view[0][0]);
//     glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, &camera.perspective[0][0]);
//     glUniform3f(camera_position_uniform, camera.pos_x, camera.pos_y, camera.pos_z);

//     glUniform1i(albedo_tex_uniform, 0);
//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_2D, material.albedo.gl_reference);
//     glUniform1i(normal_tex_uniform, 1);
//     glActiveTexture(GL_TEXTURE0 + 1);
//     glBindTexture(GL_TEXTURE_2D, material.normal.gl_reference);
//     glUniform1i(metal_tex_uniform, 2);
//     glActiveTexture(GL_TEXTURE0 + 2);
//     glBindTexture(GL_TEXTURE_2D, material.metal.gl_reference);
//     glUniform1i(roughness_tex_uniform, 3);
//     glActiveTexture(GL_TEXTURE0 + 3);
//     glBindTexture(GL_TEXTURE_2D, material.roughness.gl_reference);
//     glUniform1i(irradiance_uniform, 4);
//     glActiveTexture(GL_TEXTURE0 + 4);
//     glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.gl_reference);
//     glUniform1i(env_uniform, 5);
//     glActiveTexture(GL_TEXTURE0 + 5);
//     glBindTexture(GL_TEXTURE_CUBE_MAP, env_map.gl_reference);
//     glUniform1i(brdf_uniform, 6);
//     glActiveTexture(GL_TEXTURE0 + 6);
//     glBindTexture(GL_TEXTURE_2D, brdf_lut.gl_reference);

//     glBindVertexArray(buf.vao);
//     glDrawArrays(GL_TRIANGLES, 0, buf.vert_count);
// }

// static void draw_bar(RenderTarget target, Camera camera, glm::mat4 model, VertexBuffer buf, Material material, Texture cubemap, Texture env_map, Texture brdf_lut, Texture text)
// {
//     glUseProgram(bar_shader);

//     unsigned int t_uniform = glGetUniformLocation(bar_shader, "t");
//     unsigned int model_uniform = glGetUniformLocation(bar_shader, "model");
//     unsigned int view_uniform = glGetUniformLocation(bar_shader, "view");
//     unsigned int projection_uniform = glGetUniformLocation(bar_shader, "projection");
//     unsigned int camera_position_uniform = glGetUniformLocation(bar_shader, "camera_position");

//     unsigned int albedo_tex_uniform = glGetUniformLocation(bar_shader, "albedo_texture");
//     unsigned int normal_tex_uniform = glGetUniformLocation(bar_shader, "normal_texture");
//     unsigned int metal_tex_uniform = glGetUniformLocation(bar_shader, "metal_texture");
//     unsigned int roughness_tex_uniform = glGetUniformLocation(bar_shader, "roughness_texture");
//     unsigned int text_tex_uniform = glGetUniformLocation(bar_shader, "overlay_texture");
//     unsigned int irradiance_uniform = glGetUniformLocation(bar_shader, "irradiance");
//     unsigned int env_uniform = glGetUniformLocation(bar_shader, "env_map");
//     unsigned int brdf_uniform = glGetUniformLocation(bar_shader, "brdf");

//     static float t = 0.0f;
//     t += 0.01f;

//     glUniform1f(t_uniform, t);
//     glUniformMatrix4fv(model_uniform, 1, GL_FALSE, &model[0][0]);
//     glUniformMatrix4fv(view_uniform, 1, GL_FALSE, &camera.view[0][0]);
//     glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, &camera.perspective[0][0]);
//     glUniform3f(camera_position_uniform, camera.pos_x, camera.pos_y, camera.pos_z);

//     glUniform1i(albedo_tex_uniform, 0);
//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_2D, material.albedo.gl_reference);
//     glUniform1i(normal_tex_uniform, 1);
//     glActiveTexture(GL_TEXTURE0 + 1);
//     glBindTexture(GL_TEXTURE_2D, material.normal.gl_reference);
//     glUniform1i(metal_tex_uniform, 2);
//     glActiveTexture(GL_TEXTURE0 + 2);
//     glBindTexture(GL_TEXTURE_2D, material.metal.gl_reference);
//     glUniform1i(roughness_tex_uniform, 3);
//     glActiveTexture(GL_TEXTURE0 + 3);
//     glBindTexture(GL_TEXTURE_2D, material.roughness.gl_reference);
//     glUniform1i(irradiance_uniform, 4);
//     glActiveTexture(GL_TEXTURE0 + 4);
//     glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.gl_reference);
//     glUniform1i(env_uniform, 5);
//     glActiveTexture(GL_TEXTURE0 + 5);
//     glBindTexture(GL_TEXTURE_CUBE_MAP, env_map.gl_reference);
//     glUniform1i(brdf_uniform, 6);
//     glActiveTexture(GL_TEXTURE0 + 6);
//     glBindTexture(GL_TEXTURE_2D, brdf_lut.gl_reference);
//     glUniform1i(text_tex_uniform, 7);
//     glActiveTexture(GL_TEXTURE0 + 7);
//     glBindTexture(GL_TEXTURE_2D, text.gl_reference);

//     glBindVertexArray(buf.vao);
//     glDrawArrays(GL_TRIANGLES, 0, buf.vert_count);
// }

static void draw_rect(RenderTarget target, Rect rect, Color color)
{
    glUseProgram(basic_shader);

    unsigned int resolution_uniform = glGetUniformLocation(basic_shader, "resolution");
    unsigned int pos_uniform = glGetUniformLocation(basic_shader, "pos");
    unsigned int scale_uniform = glGetUniformLocation(basic_shader, "scale");
    unsigned int color_uniform = glGetUniformLocation(basic_shader, "color");

    glUniform2i(resolution_uniform, target.width, target.height);
    glUniform2f(pos_uniform, rect.x, rect.y);
    glUniform2f(scale_uniform, rect.width, rect.height);
    glUniform4f(color_uniform, color.r, color.g, color.b, color.a);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void draw_textured_rect(RenderTarget target, Rect rect, Color color, Texture tex)
{
    glUseProgram(textured_shader);

    unsigned int resolution_uniform = glGetUniformLocation(textured_shader, "resolution");
    unsigned int pos_uniform = glGetUniformLocation(textured_shader, "pos");
    unsigned int scale_uniform = glGetUniformLocation(textured_shader, "scale");
    unsigned int color_uniform = glGetUniformLocation(textured_shader, "color");

    glUniform2i(resolution_uniform, target.width, target.height);
    glUniform2f(pos_uniform, rect.x, rect.y);
    glUniform2f(scale_uniform, rect.width, rect.height);
    glUniform4f(color_uniform, color.r, color.g, color.b, color.a);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void draw_textured_mapped_rect(RenderTarget target, Rect rect, Rect uv, Texture tex)
{
    Shader shader = load_shader(textured_mapped_shader);
    bind_shader(shader);

    int resolution_uniform = glGetUniformLocation(textured_mapped_shader, "resolution");
    int pos_uniform = glGetUniformLocation(textured_mapped_shader, "pos");
    int scale_uniform = glGetUniformLocation(textured_mapped_shader, "scale");
    int uv_uniform = glGetUniformLocation(textured_mapped_shader, "uv");

    glUniform2i(resolution_uniform, target.width, target.height);
    glUniform2f(pos_uniform, rect.x, rect.y);
    glUniform2f(scale_uniform, rect.width, rect.height);
    glUniform4f(uv_uniform, uv.x, uv.y, uv.width, uv.height);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
