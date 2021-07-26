#include "stdio.h"

#include <glad/glad.h>

#include "graphics.hpp"
#include "platform.hpp"
#include "math.hpp"

const int LIGHTS_BUFFER_BINDING = 0;

RenderTarget main_target;

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

unsigned int lights_ubo_buffer;

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

    main_target = {width, height, 0};

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
    blurred_colors_shader = load_shader(create_shader_program("resources2/shaders/blurred_colors"));
    threed_shader = load_shader(create_shader_program("resources2/shaders/threed"));
    bar_shader = load_shader(create_shader_program("resources2/shaders/threed_with_overlay"));
    rect_to_cubemap_shader = load_shader(create_shader_program("resources/shaders/rect_to_cubemap"));
    cubemap_shader = load_shader(create_shader_program("resources/shaders/cubemap"));
    irradiance_shader = load_shader(create_shader_program("resources/shaders/irradiance"));
    env_filter_shader = load_shader(create_shader_program("resources/shaders/env_filter"));
    brdf_lut_shader = load_shader(create_shader_program("resources/shaders/brdf_lut"));
    tonemap_shader = load_shader(create_shader_program("resources2/shaders/tonemap"));
    blur_shader = load_shader(create_shader_program("resources2/shaders/blur"));
    threed_with_planar_shader = load_shader(create_shader_program("resources2/shaders/threed_with_planar"));
    threed_with_normals_shader = load_shader(create_shader_program("resources2/shaders/threed_with_overlay_normals"));
    
    // setup uniform buffers
    glGenBuffers(1, &lights_ubo_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo_buffer);
    glBufferData(GL_UNIFORM_BUFFER, LightUniformBlock::SIZE, NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHTS_BUFFER_BINDING, lights_ubo_buffer);

    return main_target;
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

void bind_shader(Shader shader)
{
    glUseProgram(shader.shader_handle);
}

void bind_1f(Shader shader, UniformId uniform_id, float val)
{
    glUniform1f(shader.uniform_handles[(int)uniform_id], val);
}

void bind_1i(Shader shader, UniformId uniform_id, int val)
{
    glUniform1i(shader.uniform_handles[(int)uniform_id], val);
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
    if (shader.uniform_handles[(int)uniform_id] != -1)
    {
        glUniform1i(shader.uniform_handles[(int)uniform_id], shader.tex_units[(int)uniform_id]);
        glActiveTexture(GL_TEXTURE0 + shader.tex_units[(int)uniform_id]);
        texture.bind();
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

void draw_cubemap()
{
    glDepthFunc(GL_LEQUAL);

    glBindVertexArray(cube_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthFunc(GL_LESS);
}

void draw_rect(RenderTarget target, Rect rect, Color color)
{
    bind_shader(basic_shader);

    bind_2i(basic_shader, UniformId::RESOLUTION, target.width, target.height);
    bind_2f(basic_shader, UniformId::POS, rect.x, rect.y);
    bind_2f(basic_shader, UniformId::SCALE, rect.width, rect.height);
    bind_4f(basic_shader, UniformId::COLOR, color.r, color.g, color.b, color.a);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_rect()
{
    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_textured_rect(RenderTarget target, Rect rect, Color color, Texture tex)
{
    bind_shader(textured_shader);

    bind_2i(textured_shader, UniformId::RESOLUTION, target.width, target.height);
    bind_2f(textured_shader, UniformId::POS, rect.x, rect.y);
    bind_2f(textured_shader, UniformId::SCALE, rect.width, rect.height);
    bind_4f(textured_shader, UniformId::COLOR, color.r, color.g, color.b, color.a);

    glActiveTexture(GL_TEXTURE0);
    tex.bind();

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void draw_textured_mapped_rect(RenderTarget target, Rect rect, Rect uv, Texture tex)
{
    bind_shader(textured_mapped_shader);

    bind_2i(textured_mapped_shader, UniformId::RESOLUTION, target.width, target.height);
    bind_2f(textured_mapped_shader, UniformId::POS, rect.x, rect.y);
    bind_2f(textured_mapped_shader, UniformId::SCALE, rect.width, rect.height);
    bind_4f(textured_mapped_shader, UniformId::UV, uv.x, uv.y, uv.width, uv.height);

    bind_texture(textured_mapped_shader, UniformId::TEX, tex);

    glBindVertexArray(screen_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void update_lights(LightUniformBlock lights)
{
    glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo_buffer);
    for (int i = 0; i < lights.num_lights; i++)
    {
        int buf_index = SpotLight::SIZE * i;
        glBufferSubData(GL_UNIFORM_BUFFER, buf_index, 12,
                        glm::value_ptr(lights.spot_lights[i].position));
        glBufferSubData(GL_UNIFORM_BUFFER, buf_index + 16, 12,
                        glm::value_ptr(lights.spot_lights[i].direction));
        glBufferSubData(GL_UNIFORM_BUFFER, buf_index + 32, 12,
                        glm::value_ptr(lights.spot_lights[i].color));
        glBufferSubData(GL_UNIFORM_BUFFER, buf_index + 44, 4,
                        &lights.spot_lights[i].inner_angle);
        glBufferSubData(GL_UNIFORM_BUFFER, buf_index + 48, 4,
                        &lights.spot_lights[i].outer_angle);
    }
    glBufferSubData(GL_UNIFORM_BUFFER, MAX_LIGHTS * SpotLight::SIZE, 4, &lights.num_lights);
};

void render_to_cubemap(RenderTarget target, Shader shader, Cubemap cubemap, uint32_t mip_level = 0)
{
    static const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    static const glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    bind_mat4(shader, UniformId::PROJECTION, captureProjection);

    target.bind();
    for (unsigned int i = 0; i < 6; ++i)
    {
        bind_mat4(shader, UniformId::VIEW, captureViews[i]);
        target.change_color_target(cubemap.get_face(i), mip_level);

        target.clear();

        glBindVertexArray(cube_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

Texture hdri_to_cubemap(RenderTarget target, Texture hdri, int size)
{
    Cubemap cubemap(size, size, TextureFormat::RGB16F, true);

    bind_shader(rect_to_cubemap_shader);
    bind_texture(rect_to_cubemap_shader, UniformId::EQUIRECTANGULAR_MAP, hdri);

    render_to_cubemap(target, rect_to_cubemap_shader, cubemap);
    cubemap.gen_mipmaps();

    return cubemap;
}

Texture convolve_irradiance_map(RenderTarget target, Texture src, int size)
{
    Cubemap cubemap(size, size, TextureFormat::RGB16F, true);

    bind_shader(irradiance_shader);
    bind_texture(irradiance_shader, UniformId::ENV_MAP, src);

    render_to_cubemap(target, irradiance_shader, cubemap);
    cubemap.gen_mipmaps();

    return cubemap;
}

Texture filter_env_map(RenderTarget target, Texture src, int size)
{
    Cubemap cubemap(size, size, TextureFormat::RGB16F, true);

    bind_shader(env_filter_shader);
    bind_texture(env_filter_shader, UniformId::ENV_MAP, src);

    unsigned int max_mip_levels = 9; // assuming 512x512 texture
    for (unsigned int mip = 0; mip < max_mip_levels; ++mip)
    {
        float roughness = (float)mip / (float)(max_mip_levels - 1);
        bind_1f(env_filter_shader, UniformId::ROUGHNESS, roughness);

        render_to_cubemap(target, env_filter_shader, cubemap, mip);
    }

    return cubemap;
}