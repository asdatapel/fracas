#pragma once

#include "../scene/entity.hpp"
#include "../scene/scene.hpp"
#include "view_layer.hpp"

struct PlanarProbe {
  Vec3f position;
  RenderTarget target;

  glm::mat4 pv_matrix;
};

struct Renderer {
  RenderTarget shadow_map;
  Camera shadow_camera;

  PlanarProbe planar_probe;

  void init()
  {
    static bool initted = false;
    if (!initted) {
      initted = true;

      shadow_map = RenderTarget(1024, 1024, TextureFormat::NONE, TextureFormat::DEPTH24);

      planar_probe.target = RenderTarget(1280, 720, TextureFormat::RGB16F, TextureFormat::DEPTH24);
      planar_probe.position = {0, 0, 0};
    }
  }
};
static Renderer renderer;

void render_scene_entities(Scene *scene, ViewLayer *view_layer, RenderTarget target, Camera *camera,
                           Vec3f camera_postion)
{
  for (int i = 0; i < scene->entities.size; i++) {
    if (scene->entities.data[i].assigned) {
      Entity &e = scene->entities.data[i].value;
      if (e.view_layer_mask & view_layer->visiblity_mask) {
        if (e.type == EntityType::MESH) {
          glm::vec3 rot(e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z);
          glm::vec3 pos(e.transform.position.x, e.transform.position.y, e.transform.position.z);
          glm::vec3 scale(e.transform.scale.x, e.transform.scale.y, e.transform.scale.z);
          glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                            glm::scale(glm::mat4(1.f), scale) * glm::toMat4(glm::quat(rot));

          Shader shader = *e.shader;
          bind_shader(shader);
          bind_camera(shader, *camera, camera_postion);
          bind_mat4(shader, UniformId::MODEL, model);
          bind_material(shader, *e.material, shader.material_offset);
          
          if (shader.asset_id == 2) {
            bind_mat4(shader, UniformId::REFLECTED_PROJECTION, renderer.planar_probe.pv_matrix);
            bind_texture(shader, shader.reflections_texture_offset, renderer.planar_probe.target.color_tex);
          } else {
            bind_material(shader, view_layer->env_map->env_mat, shader.reflections_texture_offset);
          }

          if (e.animation) {
            for (int i = 0; i < e.animation->final_mats.size(); i++) {
              // TODO shouldn't be querying location every time
              std::string uniform_name =
                  std::string("bone_transforms[") + std::to_string(i) + std::string("]");
              int handle =
                  glGetUniformLocation(threed_skinning_shader.shader_handle, uniform_name.c_str());
              glm::mat4 transform = e.animation->final_mats[i];
              glUniformMatrix4fv(handle, 1, false, &transform[0][0]);
            }
          }

          if (shader.shadows_enabled) {
            bind_mat4(shader, UniformId::SHADOW_CASTER_MAT,
                      renderer.shadow_camera.perspective * renderer.shadow_camera.view);
            bind_texture(shader, shader.shadow_texture_offset, renderer.shadow_map.depth_tex);
          }

          draw(target, shader, e.vert_buffer);
        }
      }
    }
  }
}

void render_scene_shadow_entities(Scene *scene, ViewLayer *view_layer, RenderTarget target,
                                  Camera *camera, Vec3f camera_postion)
{
  for (int i = 0; i < scene->entities.size; i++) {
    if (scene->entities.data[i].assigned) {
      Entity &e = scene->entities.data[i].value;
      if (e.view_layer_mask & view_layer->visiblity_mask) {
        if (e.type == EntityType::MESH) {
          glm::vec3 rot(e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z);
          glm::vec3 pos(e.transform.position.x, e.transform.position.y, e.transform.position.z);
          glm::vec3 scale(e.transform.scale.x, e.transform.scale.y, e.transform.scale.z);
          glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                            glm::scale(glm::mat4(1.f), scale) * glm::toMat4(glm::quat(rot));

          Shader shader = shadow_shader;
          bind_shader(shader);
          bind_camera(shader, *camera, camera_postion);
          bind_mat4(shader, UniformId::MODEL, model);
          draw(target, shader, e.vert_buffer);
        }
      }
    }
  }
}

void render_shadow_map(Scene *scene, ViewLayer *view_layer, EntityId light_id, RenderTarget target,
                       Camera *camera)
{
  Entity *light = scene->get(light_id);

  Camera light_camera;
  light_camera.fov = light->spot_light.outer_angle * 2;
  light_camera.update_from_transform(target, light->transform);
  renderer.shadow_camera = light_camera;
  renderer.shadow_camera.update_from_transform(target, light->transform);

  target.clear();
  target.bind();
  render_scene_shadow_entities(scene, view_layer, target, &light_camera, light->transform.position);
}

void render_scene(Scene *scene, ViewLayer *view_layer, RenderTarget target, Camera *editor_camera,
                  Vec3f editor_camera_pos, i32 layer_index)
{
  Camera *camera;
  Vec3f camera_pos;
  if (editor_camera) {
    camera     = editor_camera;
    camera_pos = editor_camera_pos;
  } else {
    if (view_layer->active_camera_id < 0) {
      for (int i = 0; i < scene->entities.size; i++) {
        if (scene->entities.data[i].assigned) {
          Entity &e = scene->entities.data[i].value;
          if (e.type == EntityType::CAMERA && (e.view_layer_mask & view_layer->visiblity_mask)) {
            view_layer->active_camera_id = i;
            break;
          }
        }
      }
    }
    if (view_layer->active_camera_id < 0) {
      return;  // cant draw without camera
    }
    camera     = &scene->entities.data[view_layer->active_camera_id].value.camera;
    camera_pos = scene->entities.data[view_layer->active_camera_id].value.transform.position;
  }

  for (int i = 0; i < scene->entities.size; i++) {
    if (scene->entities.data[i].assigned) {
      Entity &e = scene->entities.data[i].value;
      if (e.type == EntityType::CAMERA && (e.view_layer_mask & view_layer->visiblity_mask)) {
        e.camera.update_from_transform(target, e.transform);
      }
    }
  }

  LightUniformBlock all_lights;
  all_lights.num_lights = 0;
  for (int i = 0; i < scene->entities.size && all_lights.num_lights < MAX_LIGHTS; i++) {
    if (scene->entities.data[i].assigned) {
      Entity &e = scene->entities.data[i].value;
      if (e.type == EntityType::LIGHT && (e.view_layer_mask & view_layer->visiblity_mask)) {
        SpotLight light;
        light.position = {e.transform.position.x, e.transform.position.y, e.transform.position.z, 0};
        light.direction =
            glm::rotate(glm::quat(glm::vec3{e.transform.rotation.x, e.transform.rotation.y,
                                            e.transform.rotation.z}),
                        glm::vec4(0, -1, 0, 0));
        light.color = {e.spot_light.color.x, e.spot_light.color.y, e.spot_light.color.z, 0};
        light.outer_angle                             = e.spot_light.outer_angle;
        light.inner_angle                             = e.spot_light.inner_angle;
        all_lights.spot_lights[all_lights.num_lights] = light;
        all_lights.num_lights++;
      }
    }
  }
  upload_lights(all_lights);

  renderer.init();
  if (layer_index == 0) {
    EntityId light_id = 82;
    render_shadow_map(scene, view_layer, light_id, renderer.shadow_map, camera);
  }

  float planar_position_y = renderer.planar_probe.position.y;

  // http://khayyam.kaplinski.com/2011/09/reflective-water-with-glsl-part-i.html
  glm::mat4 reflection_mat = {1, 0,  0, 0,                       //
                              0, -1, 0, -2 * planar_position_y,  //
                              0, 0,  1, 0,                       //
                              0, 0,  0, 1};

  Vec3f flipped_camera_pos = {camera_pos.x, (2 * planar_position_y) - camera_pos.y, camera_pos.z};
  Camera flipped_camera    = *camera;
  flipped_camera.view      = reflection_mat * camera->view * reflection_mat;
  renderer.planar_probe.pv_matrix = flipped_camera.perspective * flipped_camera.view;

  renderer.planar_probe.target.bind();
  renderer.planar_probe.target.clear();
  render_scene_entities(scene, view_layer, renderer.planar_probe.target, &flipped_camera, flipped_camera_pos);

  target.bind();
  target.clear();
  render_scene_entities(scene, view_layer, target, camera, camera_pos);

  if (view_layer->cubemap_visible) {
    bind_shader(cubemap_shader);
    bind_mat4(cubemap_shader, UniformId::PROJECTION, camera->perspective);
    bind_mat4(cubemap_shader, UniformId::VIEW, camera->view);
    bind_texture(cubemap_shader, UniformId::ENV_MAP, view_layer->env_map->unfiltered_cubemap);
    draw_cubemap();
  }
}
