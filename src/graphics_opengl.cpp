#include "stdio.h"

#include <glad/glad.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

#include "graphics.hpp"
#include "platform.hpp"
#include "material.hpp"
#include "math.hpp"
#include "shader.hpp"

RenderTarget main_target;

unsigned int temp_fbo;
unsigned int temp_rbo;

Shader basic_shader;
Shader textured_shader;
Shader textured_mapped_shader;
Shader threed_shader;
Shader bar_shader;
Shader rect_to_cubemap_shader;
Shader cubemap_shader;
Shader irradiance_shader;
Shader env_filter_shader;
Shader brdf_lut_shader;

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

RenderTarget init_graphics(uint32_t width, uint32_t height)
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

    basic_shader = load_shader(create_shader_program("resources/shaders/basic"));
    textured_shader = load_shader(create_shader_program("resources/shaders/textured"));
    textured_mapped_shader = load_shader(create_shader_program("resources/shaders/font_atlas"));
    threed_shader = load_shader(create_shader_program("resources2/shaders/threed"));
    bar_shader = load_shader(create_shader_program("resources2/shaders/threed_with_overlay"));
    rect_to_cubemap_shader = load_shader(create_shader_program("resources/shaders/rect_to_cubemap"));
    cubemap_shader = load_shader(create_shader_program("resources/shaders/cubemap"));
    irradiance_shader = load_shader(create_shader_program("resources/shaders/irradiance"));
    env_filter_shader = load_shader(create_shader_program("resources/shaders/env_filter"));
    brdf_lut_shader = load_shader(create_shader_program("resources/shaders/brdf_lut"));

    return main_target;
}

void clear_backbuffer()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

Texture to_texture(Bitmap bitmap, bool mipmaps)
{
    Texture tex;
    tex.type = Texture::Type::_2D;
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

Texture to_single_channel_texture(uint8_t *data, int width, int height, bool mipmaps)
{
    Texture tex;
    tex.type = Texture::Type::_2D;
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
    tex.type = Texture::Type::_2D;
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
    tex.type = Texture::Type::_2D;
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
    tex.type = Texture::Type::CUBEMAP;
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
    
    target.color_tex = new_tex(width, height, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.color_tex.gl_reference, 0);

    return target;
}

void bind(RenderTarget target)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target.gl_fbo);
    glViewport(0, 0, target.width, target.height);
}

void bind_shader(Shader shader)
{
    glUseProgram(shader.shader_handle);
}

void bind_camera(Shader shader, Camera camera)
{
    glUniformMatrix4fv(shader.uniform_handles[(int)UniformId::VIEW], 1, GL_FALSE, &camera.view[0][0]);
    glUniformMatrix4fv(shader.uniform_handles[(int)UniformId::PROJECTION], 1, GL_FALSE, &camera.perspective[0][0]);
    glUniform3f(shader.uniform_handles[(int)UniformId::CAMERA_POSITION], camera.pos_x, camera.pos_y, camera.pos_z);
}

void bind_1f(Shader shader, UniformId uniform_id, float val)
{
    glUniform1f(shader.uniform_handles[(int)uniform_id], val);
}

void bind_2i(Shader shader, UniformId uniform_id, int i1, int i2)
{
    glUniform2i(shader.uniform_handles[(int)uniform_id], i1, i2);
}

void bind_2f(Shader shader, UniformId uniform_id, float f1, float f2)
{
    glUniform2f(shader.uniform_handles[(int)uniform_id], f1, f2);
}

void bind_4f(Shader shader, UniformId uniform_id, float f1, float f2, float f3, float f4)
{
    glUniform4f(shader.uniform_handles[(int)uniform_id], f1, f2, f3, f4);
}

void bind_mat4(Shader shader, UniformId uniform_id, glm::mat4 mat)
{
    glUniformMatrix4fv(shader.uniform_handles[(int)uniform_id], 1, GL_FALSE, &mat[0][0]);
}

void bind_texture(Shader shader, UniformId uniform_id, Texture texture)
{
    GLenum gl_tex_type = GL_TEXTURE_2D;
    switch(texture.type){
        case(Texture::Type::_2D):
            gl_tex_type = GL_TEXTURE_2D;
            break;
        case(Texture::Type::CUBEMAP):
            gl_tex_type = GL_TEXTURE_CUBE_MAP;
            break;
    }
    glUniform1i(shader.uniform_handles[(int)uniform_id], shader.tex_units[(int)uniform_id]);
    glActiveTexture(GL_TEXTURE0 + shader.tex_units[(int)uniform_id]);
    glBindTexture(gl_tex_type, texture.gl_reference);
}

void bind_material(Shader shader, Material material)
{
    for (int i = 0; i < material.num_textures; i++)
    {
        bind_texture(shader, material.uniform_ids[i], material.textures[i]);
    }
}

void draw(RenderTarget target, Shader shader, VertexBuffer buf)
{
    static float t = 0.0f;
    t += 0.01f;
    glUniform1f(shader.uniform_handles[(int)UniformId::T], t);

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

    bind_shader(rect_to_cubemap_shader);
    bind_mat4(rect_to_cubemap_shader, UniformId::PROJECTION, captureProjection);
    bind_texture(rect_to_cubemap_shader, UniformId::EQUIRECTANGULAR_MAP, hdri);

    glBindFramebuffer(GL_FRAMEBUFFER, temp_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, temp_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, target.width, target.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, temp_rbo);

    glViewport(0, 0, target.width, target.height);
    glBindFramebuffer(GL_FRAMEBUFFER, temp_fbo);
    for (unsigned int i = 0; i < 6; ++i)
    {
        bind_mat4(rect_to_cubemap_shader, UniformId::VIEW, captureViews[i]);
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

    bind_shader(cubemap_shader);
    
    bind_mat4(cubemap_shader, UniformId::PROJECTION, camera.perspective);
    bind_mat4(cubemap_shader, UniformId::VIEW, camera.view);

    bind_texture(cubemap_shader, UniformId::ENV_MAP, tex);

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

    bind_shader(irradiance_shader);
    bind_mat4(irradiance_shader, UniformId::PROJECTION, captureProjection);
    bind_texture(irradiance_shader, UniformId::ENV_MAP, src);

    glViewport(0, 0, target.width, target.height);
    for (unsigned int i = 0; i < 6; ++i)
    {
        bind_mat4(irradiance_shader, UniformId::VIEW, captureViews[i]);
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

    bind_shader(env_filter_shader);
    bind_mat4(env_filter_shader, UniformId::PROJECTION, captureProjection);
    bind_texture(env_filter_shader, UniformId::ENV_MAP, src);

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
        bind_1f(env_filter_shader, UniformId::ROUGHNESS, roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            bind_mat4(env_filter_shader, UniformId::VIEW, captureViews[i]);
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bind_shader(brdf_lut_shader);
    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glViewport(0, 0, 1920, 1080);

    return target;
}

void draw_rect(RenderTarget target, Rect rect, Color color)
{
    bind_shader(basic_shader);

    bind_2i(basic_shader, UniformId::RESOLUTION, target.width, target.height);
    bind_2f(basic_shader, UniformId::POS,rect.x, rect.y);
    bind_2f(basic_shader, UniformId::SCALE, rect.width, rect.height);
    bind_4f(basic_shader, UniformId::COLOR, color.r, color.g, color.b, color.a);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_textured_rect(RenderTarget target, Rect rect, Color color, Texture tex)
{
    bind_shader(textured_shader);

    bind_2i(textured_shader, UniformId::RESOLUTION, target.width, target.height);
    bind_2f(textured_shader, UniformId::POS,rect.x, rect.y);
    bind_2f(textured_shader, UniformId::SCALE, rect.width, rect.height);
    bind_4f(textured_shader, UniformId::COLOR, color.r, color.g, color.b, color.a);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.gl_reference);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_textured_mapped_rect(RenderTarget target, Rect rect, Rect uv, Texture tex)
{
    bind_shader(textured_mapped_shader);

    bind_2i(textured_mapped_shader, UniformId::RESOLUTION, target.width, target.height);
    bind_2f(textured_mapped_shader, UniformId::POS,rect.x, rect.y);
    bind_2f(textured_mapped_shader, UniformId::SCALE, rect.width, rect.height);
    bind_4f(textured_mapped_shader, UniformId::UV, uv.x, uv.y, uv.width, uv.height);


    bind_texture(textured_mapped_shader, UniformId::TEX, tex);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
