#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "graphics/graphics.hpp"
#include "platform.hpp"

struct Camera {
  glm::mat4 view;
  glm::mat4 perspective;

  static constexpr float fov = glm::radians(45.f);

  void update_from_transform(RenderTarget target, Transform transform)
  {
    glm::vec3 dir = glm::rotate(
        glm::quat(glm::vec3{transform.rotation.x, transform.rotation.y, transform.rotation.z}),
        glm::vec3(0, 0, -1));

    view = glm::lookAt(
        glm::vec3{transform.position.x, transform.position.y, transform.position.z},
        glm::vec3{transform.position.x, transform.position.y, transform.position.z} + dir,
        {0.f, 1.f, 0.f});
    perspective = glm::perspective(fov, (float)target.width / (float)target.height, 0.01f, 100.0f);
  }
};

struct EditorCamera : Camera {
  float pos_x = 0.f;
  float pos_y = 4.f;
  float pos_z = 4.f;

  float x_rot = -90.f;
  float y_rot = 0.f;

  Vec3f get_dir()
  {
    return {cos(glm::radians(x_rot)) * cos(glm::radians(y_rot)), sin(glm::radians(y_rot)),
            sin(glm::radians(x_rot)) * cos(glm::radians(y_rot))};
  }

  void update(RenderTarget target, InputState *input)
  {
    if (input->mouse_buttons[(int)MouseButton::LEFT]) {
      x_rot -= input->mouse_pos_delta.x * 0.1f;
      y_rot += input->mouse_pos_delta.y * 0.1f;

      if (y_rot >= 89.0f) y_rot = 89.0f;
      if (y_rot < -89.0f) y_rot = -89.0f;
    }

    float dir_x = cos(glm::radians(x_rot)) * cos(glm::radians(y_rot));
    float dir_y = sin(glm::radians(y_rot));
    float dir_z = sin(glm::radians(x_rot)) * cos(glm::radians(y_rot));

    float speed_f = 0.05f;

    if (input->keys[(int)Keys::W]) {
      pos_x += dir_x * speed_f;
      pos_y += dir_y * speed_f;
      pos_z += dir_z * speed_f;
    }
    if (input->keys[(int)Keys::S]) {
      pos_x -= dir_x * speed_f;
      pos_y -= dir_y * speed_f;
      pos_z -= dir_z * speed_f;
    }
    if (input->keys[(int)Keys::A]) {
      pos_x -= (dir_y * 0 - dir_z * 1.f) * speed_f;
      pos_y -= (dir_z * 0 - dir_x * 0.f) * speed_f;
      pos_z -= (dir_x * 1.f - dir_y * 0.f) * speed_f;
    }
    if (input->keys[(int)Keys::D]) {
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

    view        = glm::lookAt(glm::vec3{pos_x, pos_y, pos_z},
                       glm::vec3{pos_x, pos_y, pos_z} + glm::vec3{dir_x, dir_y, dir_z},
                       {0.f, 1.f, 0.f});
    perspective = glm::perspective(glm::radians(45.f), (float)target.width / (float)target.height,
                                   0.01f, 10000.0f);
  }
};

void bind_camera(Shader shader, Camera camera, Vec3f camera_pos)
{
  glUniformMatrix4fv(shader.uniform_handles[(int)UniformId::VIEW], 1, GL_FALSE, &camera.view[0][0]);
  glUniformMatrix4fv(shader.uniform_handles[(int)UniformId::PROJECTION], 1, GL_FALSE,
                     &camera.perspective[0][0]);
  glUniform3f(shader.uniform_handles[(int)UniformId::CAMERA_POSITION], camera_pos.x, camera_pos.y,
              camera_pos.z);
}

Vec3f screen_to_world(Rect target_rect, Camera *camera, Vec2f screen_pos, float z = 0.f)
{
  Vec3f p              = {screen_pos.x, screen_pos.y, 0};
  glm::vec4 gl_screen  = {(p.x - target_rect.x) / (target_rect.width / 2.f) - 1,
                         -(p.y - target_rect.y) / (target_rect.height / 2.f) + 1, z, 1.f};
  glm::vec4 unprojects = glm::inverse(camera->perspective * camera->view) * gl_screen;
  unprojects /= unprojects.w;

  return {unprojects.x, unprojects.y, unprojects.z};
}