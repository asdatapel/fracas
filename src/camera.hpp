#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "graphics/graphics.hpp"
#include "platform.hpp"

struct Camera
{
    glm::mat4 view;
    glm::mat4 perspective;

    float pos_x = 0.f;
    float pos_y = 0.f;
    float pos_z = 1.f;

    void update_from_transform(RenderTarget target, Transform transform)
    {
        pos_x = transform.position.x;
        pos_y = transform.position.y;
        pos_z = transform.position.z;

        glm::vec3 dir = glm::rotate(
            glm::quat(glm::vec3{transform.rotation.x, transform.rotation.y, transform.rotation.z}),
            glm::vec3(0, 0, -1));

        view = glm::lookAt(glm::vec3{pos_x, pos_y, pos_z}, glm::vec3{pos_x, pos_y, pos_z} + dir, {0.f, 1.f, 0.f});
        perspective = glm::perspective(glm::radians(45.f), (float)target.width / (float)target.height, 0.01f, 100.0f);
    }
};

struct EditorCamera : Camera
{

    float x_rot = 0.f;
    float y_rot = 0.f;

    Vec3f get_dir()
    {
        return {cos(glm::radians(x_rot)) * cos(glm::radians(y_rot)),
                sin(glm::radians(y_rot)),
                sin(glm::radians(x_rot)) * cos(glm::radians(y_rot))};
    }

    void update(RenderTarget target, InputState *input)
    {
        if (input->mouse_left)
        {
            float diff_x = input->mouse_x - input->prev_mouse_x;
            float diff_y = input->mouse_y - input->prev_mouse_y;

            x_rot -= diff_x * 0.1f;
            y_rot += diff_y * 0.1f;

            if (y_rot >= 89.0f)
                y_rot = 89.0f;
            if (y_rot < -89.0f)
                y_rot = -89.0f;
        }

        float dir_x = cos(glm::radians(x_rot)) * cos(glm::radians(y_rot));
        float dir_y = sin(glm::radians(y_rot));
        float dir_z = sin(glm::radians(x_rot)) * cos(glm::radians(y_rot));

        float speed_f = 0.05f;

        if (input->keys[(int)Keys::W])
        {
            pos_x += dir_x * speed_f;
            pos_y += dir_y * speed_f;
            pos_z += dir_z * speed_f;
        }
        if (input->keys[(int)Keys::S])
        {
            pos_x -= dir_x * speed_f;
            pos_y -= dir_y * speed_f;
            pos_z -= dir_z * speed_f;
        }
        if (input->keys[(int)Keys::A])
        {
            pos_x -= (dir_y * 0 - dir_z * 1.f) * speed_f;
            pos_y -= (dir_z * 0 - dir_x * 0.f) * speed_f;
            pos_z -= (dir_x * 1.f - dir_y * 0.f) * speed_f;
        }
        if (input->keys[(int)Keys::D])
        {

            pos_x += (dir_y * 0 - dir_z * 1.f) * speed_f;
            pos_y += (dir_z * 0 - dir_x * 0.f) * speed_f;
            pos_z += (dir_x * 1.f - dir_y * 0.f) * speed_f;
        }

        update_transforms(target);
    }

    void update_transforms(RenderTarget target)
    {
        float dir_x = cos(glm::radians(x_rot)) * cos(glm::radians(y_rot));
        float dir_y = sin(glm::radians(y_rot));
        float dir_z = sin(glm::radians(x_rot)) * cos(glm::radians(y_rot));

        view = glm::lookAt(glm::vec3{pos_x, pos_y, pos_z}, glm::vec3{pos_x, pos_y, pos_z} + glm::vec3{dir_x, dir_y, dir_z}, {0.f, 1.f, 0.f});
        perspective = glm::perspective(glm::radians(45.f), (float)target.width / (float)target.height, 0.01f, 10000.0f);
    }
};

void bind_camera(Shader shader, Camera camera)
{
    glUniformMatrix4fv(shader.uniform_handles[(int)UniformId::VIEW], 1, GL_FALSE, &camera.view[0][0]);
    glUniformMatrix4fv(shader.uniform_handles[(int)UniformId::PROJECTION], 1, GL_FALSE, &camera.perspective[0][0]);
    glUniform3f(shader.uniform_handles[(int)UniformId::CAMERA_POSITION], camera.pos_x, camera.pos_y, camera.pos_z);
}