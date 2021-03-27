#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "graphics.hpp"
#include "platform.hpp"



struct Camera
{
    glm::mat4 view;
    glm::mat4 perspective;

    float x_rot = 0.f;
    float y_rot = 0.f;
    float last_mouse_x = -1;
    float last_mouse_y = -1;

    float pos_x = 0.f;
    float pos_y = 0.f;
    float pos_z = 1.f;

    void update(RenderTarget target, InputState *input)
    {
        if (input->mouse_left)
        {
            if (last_mouse_x > -1)
            {
                float diff_x = input->mouse_x - last_mouse_x;
                float diff_y = input->mouse_y - last_mouse_y;

                x_rot -= diff_x * 0.1f;
                y_rot += diff_y * 0.1f;

                if (y_rot >= 89.0f)
                    y_rot = 89.0f;
                if (y_rot < -89.0f)
                    y_rot = -89.0f;
            }
            last_mouse_x = input->mouse_x;
            last_mouse_y = input->mouse_y;
        }
        else
        {
            last_mouse_x = -1;
            last_mouse_y = -1;
        }
        float dir_x = cos(glm::radians(x_rot)) * cos(glm::radians(y_rot));
        float dir_y = sin(glm::radians(y_rot));
        float dir_z = sin(glm::radians(x_rot)) * cos(glm::radians(y_rot));

        if (input->keys[(int)Keys::W])
        {
            pos_x += dir_x * 0.01f;
            pos_y += dir_y * 0.01f;
            pos_z += dir_z * 0.01f;
        }
        if (input->keys[(int)Keys::S])
        {
            pos_x -= dir_x * 0.01f;
            pos_y -= dir_y * 0.01f;
            pos_z -= dir_z * 0.01f;
        }
        if (input->keys[(int)Keys::A])
        {

            pos_x -= (dir_y * 0 - dir_z * 1.f) * 0.01f;
            pos_y -= (dir_z * 0 - dir_x * 0.f) * 0.01f;
            pos_z -= (dir_x * 1.f - dir_y * 0.f) * 0.01f;
        }
        if (input->keys[(int)Keys::D])
        {

            pos_x += (dir_y * 0 - dir_z * 1.f) * 0.01f;
            pos_y += (dir_z * 0 - dir_x * 0.f) * 0.01f;
            pos_z += (dir_x * 1.f - dir_y * 0.f) * 0.01f;
        }

        view = glm::lookAt(glm::vec3{pos_x, pos_y, pos_z}, glm::vec3{pos_x, pos_y, pos_z} + glm::vec3{dir_x, dir_y, dir_z}, {0.f, 1.f, 0.f});
        perspective = glm::perspective(glm::radians(90.f), (float)target.width / (float)target.height, 0.01f, 100.0f);
    }
};

void bind_camera(Shader shader, Camera camera)
{
    glUniformMatrix4fv(shader.uniform_handles[(int)UniformId::VIEW], 1, GL_FALSE, &camera.view[0][0]);
    glUniformMatrix4fv(shader.uniform_handles[(int)UniformId::PROJECTION], 1, GL_FALSE, &camera.perspective[0][0]);
    glUniform3f(shader.uniform_handles[(int)UniformId::CAMERA_POSITION], camera.pos_x, camera.pos_y, camera.pos_z);
}