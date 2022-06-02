#pragma once

#include "../scene/entity.hpp"
#include "../scene/scene.hpp"
#include "../util/timer.hpp"
#include "irradiance_volume.hpp"
#include "tracing.hpp"
#include "view_layer.hpp"

struct PlanarProbe {
  Vec3f position;
  RenderTarget target;

  glm::mat4 pv_matrix;
};

struct Renderer {
  Cubemap sky_cubemap;
  Material sky;

  RenderTarget shadow_map;
  Camera shadow_camera;

  PlanarProbe planar_probe;

  IrradianceVolume irradiance_volume;
  RenderTarget irradiance_probe_target;
  RenderTarget irradiance_convolution_target;
  Cubemap tmp_probe_texture;

  float sky_t                        = 12;
  float directional_light_distance   = 50;
  float directional_light_brightness = 120;

  float shadow_offset_factor = 2;
  float shadow_offset_units  = 0;

  Texture2D rt_tex;
  Texture2D rt_gpu_tex;

  void init(Scene *scene, ViewLayer *view_layer);
  void bake_probes(Scene *scene, ViewLayer *view_layer);
  void raytrace(Camera *camera, Vec3f camera_pos);
  void raytrace_threaded(Camera *camera, Vec3f camera_pos);
  void upload_bvh();
  void raytrace_gpu(Camera *camera, Vec3f camera_pos);
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
          bind_texture(shader, shader.pbr_texture_offset, view_layer->env_map->env_mat.textures[2]);

          if (shader.asset_id == 0) {
            bind_material(shader, view_layer->env_map->env_mat, shader.reflections_texture_offset);
            bind_texture(shader, shader.reflections_texture_offset + 1,
                         renderer.irradiance_volume.cubemaps);
          } else {
            bind_material(shader, view_layer->env_map->env_mat, shader.reflections_texture_offset);
          }

          if (shader.asset_id == 1) {
            glEnable(GL_BLEND);
          } else {
            glDisable(GL_BLEND);
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

void render_shadow_map(Scene *scene, ViewLayer *view_layer, EntityId light_id, i32 shadow_map_index,
                       RenderTarget target, Camera *camera)
{
  Entity *light = scene->get(light_id);

  Camera light_camera;
  light_camera.fov = light->spot_light.outer_angle * 2;
  light_camera.update_from_transform_perspective(target, light->transform);
  renderer.shadow_camera = light_camera;
  renderer.shadow_camera.update_from_transform_perspective(target, light->transform);

  const int ATLAS_DIM   = 2;
  const float ELEM_SIZE = 1.f / ATLAS_DIM;
  i32 atlas_position_x  = shadow_map_index % ATLAS_DIM;
  i32 atlas_position_y  = shadow_map_index / ATLAS_DIM;
  glViewport(target.width * atlas_position_x * ELEM_SIZE,
             target.height * atlas_position_y * ELEM_SIZE, target.width / ATLAS_DIM,
             target.height / ATLAS_DIM);

  render_scene_shadow_entities(scene, view_layer, target, &light_camera, light->transform.position);
}

void render_directional_shadow_map(Scene *scene, ViewLayer *view_layer, i32 shadow_map_index,
                                   DirectionalLight light, RenderTarget target)
{
  glm::vec3 light_position = -light.direction * renderer.directional_light_distance;

  Camera light_camera;
  light_camera.fov = 40;
  light_camera.update_orthographic(target, light_position, light.direction);
  renderer.shadow_camera = light_camera;
  renderer.shadow_camera.update_orthographic(target, light_position, light.direction);

  const int ATLAS_DIM   = 2;
  const float ELEM_SIZE = 1.f / ATLAS_DIM;
  i32 atlas_position_x  = shadow_map_index % ATLAS_DIM;
  i32 atlas_position_y  = shadow_map_index / ATLAS_DIM;
  glViewport(target.width * atlas_position_x * ELEM_SIZE,
             target.height * atlas_position_y * ELEM_SIZE, target.width / ATLAS_DIM,
             target.height / ATLAS_DIM);

  render_scene_shadow_entities(scene, view_layer, target, &light_camera, {});
}

void render_scene(Scene *scene, ViewLayer *view_layer, RenderTarget target, Camera *editor_camera,
                  Vec3f editor_camera_pos, i32 layer_index, Assets *assets)
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
        e.camera.update_from_transform_perspective(target, e.transform);
      }
    }
  }

  renderer.init(scene, view_layer);

  renderer.raytrace_gpu(camera, camera_pos);

  {
    draw_sky(renderer.sky_cubemap, renderer.sky_t);
    renderer.sky_cubemap.gen_mipmaps();
    static RenderTarget temp_target(0, 0, TextureFormat::NONE, TextureFormat::NONE);

    static Cubemap irradiance_map(32, 32, TextureFormat::RGB16F, true);
    static Cubemap env_map(512, 512, TextureFormat::RGB16F, true);
    convolve_irradiance_map(temp_target, renderer.sky_cubemap, irradiance_map);
    filter_env_map(temp_target, renderer.sky_cubemap, env_map);

    view_layer->env_map->unfiltered_cubemap  = renderer.sky_cubemap;
    view_layer->env_map->env_mat.textures[0] = irradiance_map;
    view_layer->env_map->env_mat.textures[1] = env_map;
  }

  {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(renderer.shadow_offset_factor, renderer.shadow_offset_units);

    renderer.shadow_map.bind();
    renderer.shadow_map.clear();
    LightUniformBlock all_lights;

    all_lights.directional_light.color.x     = renderer.directional_light_brightness;
    all_lights.directional_light.color.y     = renderer.directional_light_brightness;
    all_lights.directional_light.color.z     = renderer.directional_light_brightness;
    all_lights.directional_light.direction.x = -cos(renderer.sky_t / 10);
    all_lights.directional_light.direction.y = -sin(renderer.sky_t / 10);
    all_lights.directional_light.direction.z = 0;

    all_lights.num_lights = 0;
    for (int i = 0; i < scene->entities.size && all_lights.num_lights < MAX_LIGHTS; i++) {
      if (scene->entities.data[i].assigned) {
        Entity &e = scene->entities.data[i].value;
        if (e.type == EntityType::LIGHT && (e.view_layer_mask & view_layer->visiblity_mask)) {
          SpotLight light;

          if (i == 304) {
            light.shadow_map_index = i - 304;
            render_shadow_map(scene, view_layer, i, i - 304, renderer.shadow_map, camera);
            light.lightspace_mat = renderer.shadow_camera.projection * renderer.shadow_camera.view;
          }

          light.position = {e.transform.position.x, e.transform.position.y, e.transform.position.z,
                            0};
          light.direction =
              glm::rotate(glm::quat(glm::vec3{e.transform.rotation.x, e.transform.rotation.y,
                                              e.transform.rotation.z}),
                          glm::vec4(0, -1, 0, 0));
          light.color       = {e.spot_light.color.x, e.spot_light.color.y, e.spot_light.color.z, 0};
          light.outer_angle = e.spot_light.outer_angle;
          light.inner_angle = e.spot_light.inner_angle;
          all_lights.spot_lights[all_lights.num_lights] = light;
          all_lights.num_lights++;
        }
      }
    }

    render_directional_shadow_map(scene, view_layer, 3, all_lights.directional_light,
                                  renderer.shadow_map);
    glDisable(GL_POLYGON_OFFSET_FILL);
    all_lights.directional_light.shadow_map_index = 3;
    all_lights.directional_light.lightspace_mat =
        renderer.shadow_camera.projection * renderer.shadow_camera.view;
    upload_lights(all_lights);
  }

  // {
  //   float planar_position_y = renderer.planar_probe.position.y;

  //   // http://khayyam.kaplinski.com/2011/09/reflective-water-with-glsl-part-i.html
  //   glm::mat4 reflection_mat = {1, 0,  0, 0,                       //
  //                               0, -1, 0, -2 * planar_position_y,  //
  //                               0, 0,  1, 0,                       //
  //                               0, 0,  0, 1};

  //   Vec3f flipped_camera_pos = {camera_pos.x, (2 * planar_position_y) - camera_pos.y,
  //   camera_pos.z}; Camera flipped_camera    = *camera; flipped_camera.view      = reflection_mat
  //   * camera->view * reflection_mat; renderer.planar_probe.pv_matrix = flipped_camera.projection
  //   * flipped_camera.view;

  //   renderer.planar_probe.target.bind();
  //   renderer.planar_probe.target.clear();
  //   render_scene_entities(scene, view_layer, renderer.planar_probe.target, &flipped_camera,
  //   flipped_camera_pos);
  // }

  target.bind();
  target.clear();
  render_scene_entities(scene, view_layer, target, camera, camera_pos);

  {
    bind_shader(probe_debug_shader);
    bind_camera(probe_debug_shader, *camera, camera_pos);
    bind_texture(probe_debug_shader, 0, renderer.irradiance_volume.cubemaps);
    for (i32 i = 0; i < renderer.irradiance_volume.probes.count; i++) {
      RadianceProbe probe = renderer.irradiance_volume.probes[i];
      glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(probe.position.x, probe.position.y,
                                                                 probe.position.z)) *
                        glm::scale(glm::mat4(1.f), glm::vec3(.2, .2, .2));
      bind_mat4(probe_debug_shader, UniformId::MODEL, model);

      VertexBuffer vb = assets->vertex_buffers.data[304].value;
      draw(target, probe_debug_shader, vb);
    }
}

  if (view_layer->cubemap_visible) {
    bind_shader(cubemap_shader);
    bind_mat4(cubemap_shader, UniformId::PROJECTION, camera->projection);
    bind_mat4(cubemap_shader, UniformId::VIEW, camera->view);
    bind_texture(cubemap_shader, UniformId::ENV_MAP, view_layer->env_map->unfiltered_cubemap);
    draw_cubemap();
  }
}

const i32 RT_WIDTH  = 1920;
const i32 RT_HEIGHT = 1080;
const i32 RT_RATIO  = RT_WIDTH / RT_HEIGHT;
float hdr_image[RT_WIDTH * RT_HEIGHT * 3];
u8 image[RT_WIDTH * RT_HEIGHT * 3];

void Renderer::init(Scene *scene, ViewLayer *view_layer)
{
  static bool initted = false;
  if (!initted) {
    initted = true;

    sky_cubemap = Cubemap(512, 512, TextureFormat::RGB16F, true);
    draw_sky(sky_cubemap);
    sky_cubemap.gen_mipmaps();
    RenderTarget temp_target(0, 0, TextureFormat::NONE, TextureFormat::NONE);
    sky = create_env_mat(temp_target, sky_cubemap);

    irradiance_volume       = create_irradiance_volume(&assets_allocator);
    irradiance_probe_target = RenderTarget(128, 128, TextureFormat::NONE, TextureFormat::DEPTH24);
    irradiance_convolution_target =
        RenderTarget(128, 128, TextureFormat::NONE, TextureFormat::DEPTH24);
    tmp_probe_texture = Cubemap(128, 128, TextureFormat::RGB16F);

    shadow_map = RenderTarget(4096, 4096, TextureFormat::NONE, TextureFormat::DEPTH24);

    planar_probe.target   = RenderTarget(1280, 720, TextureFormat::RGB16F, TextureFormat::DEPTH24);
    planar_probe.position = {0, 0, 0};

    rt_tex = Texture2D(RT_WIDTH, RT_HEIGHT, TextureFormat::RGB8, true);
    rt_gpu_tex = Texture2D(RT_WIDTH, RT_HEIGHT, TextureFormat::RGBA32, true);
    create_bvh_threaded(scene);
    upload_bvh();
  }
}

void Renderer::bake_probes(Scene *scene, ViewLayer *view_layer)
{
  struct CubemapOrientation {
    Vec3f dir;
    Vec3f up;
  };
  static CubemapOrientation cubemap_orientations[] = {
      {{1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},   //
      {{-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},  //
      {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},    //
      {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},  //
      {{0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},   //
      {{0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0}},   //
  };

  for (i32 i = 0; i < irradiance_volume.probes.count; i++) {
    RadianceProbe *p = &irradiance_volume.probes[i];

    for (i32 face_i = 0; face_i < 6; face_i++) {
      auto o        = cubemap_orientations[face_i];
      Camera camera = Camera::lookat_perspective(p->position, p->position + o.dir, o.up, 1.5708, 1);
      irradiance_probe_target.change_color_target(tmp_probe_texture.get_face(face_i), 0);

      irradiance_probe_target.bind();
      irradiance_probe_target.clear({0, 0, 0, 0});
      render_scene_entities(scene, view_layer, irradiance_probe_target, &camera, p->position);

      bind_shader(cubemap_shader);
      bind_mat4(cubemap_shader, UniformId::PROJECTION, camera.projection);
      bind_mat4(cubemap_shader, UniformId::VIEW, camera.view);
      bind_texture(cubemap_shader, UniformId::ENV_MAP, view_layer->env_map->unfiltered_cubemap);
      draw_cubemap();
    }
    tmp_probe_texture.gen_mipmaps();

    bind_shader(irradiance_shader);
    bind_texture(irradiance_shader, UniformId::ENV_MAP, tmp_probe_texture);
    for (i32 face_i = 0; face_i < 6; face_i++) {
      auto o        = cubemap_orientations[face_i];
      Camera camera = Camera::lookat_perspective({0, 0, 0}, o.dir, o.up, 90, 1);
      bind_camera(irradiance_shader, camera, p->position);

      irradiance_convolution_target.change_color_target(irradiance_volume.cubemaps, p->flat_index,
                                                        face_i);

      irradiance_convolution_target.bind();
      irradiance_convolution_target.clear();
      draw_cube();
    }
  }
  irradiance_volume.cubemaps.gen_mipmaps();
}

void Renderer::raytrace(Camera *camera, Vec3f camera_pos)
{
  Timer timer;

  float min_t = 1e30;
  float max_t = -1e30;

  glm::mat4 inverse_camera = glm::inverse(camera->view);
  for (i32 y = 0; y < RT_HEIGHT; y++) {
    for (i32 x = 0; x < RT_WIDTH; x++) {
      Vec3f ray_origin = camera_pos;

      Vec3f pixel_ndc     = {((float)x / RT_WIDTH) * 2.f - 1, ((float)y / RT_HEIGHT) * 2.f - 1, -1};
      glm::vec3 world_pos = inverse_camera * glm::vec4(pixel_ndc.x, pixel_ndc.y, pixel_ndc.z, 1);
      Vec3f pixel_pos     = {world_pos.x, world_pos.y, world_pos.z};
      Vec3f ray_dir       = normalize(pixel_pos - camera_pos);

      float t = traverse_bvh_threaded(ray_origin, ray_dir);
      min_t   = fminf(min_t, t);
      max_t   = fmaxf(max_t, t);

      hdr_image[(RT_WIDTH * y + x) * 3]     = t;
      hdr_image[(RT_WIDTH * y + x) * 3 + 1] = t;
      hdr_image[(RT_WIDTH * y + x) * 3 + 2] = t;
    }
  }

  for (i32 y = 0; y < RT_HEIGHT; y++) {
    for (i32 x = 0; x < RT_WIDTH; x++) {
      float hdr                         = hdr_image[(RT_WIDTH * y + x) * 3];
      float normalized                  = (hdr - min_t) / (max_t - min_t);
      image[(RT_WIDTH * y + x) * 3]     = normalized * 255;
      image[(RT_WIDTH * y + x) * 3 + 1] = normalized * 255;
      image[(RT_WIDTH * y + x) * 3 + 2] = normalized * 255;
    }
  }

  rt_tex.upload(image, true);

  timer.print_ms();
}

template <typename T, size_t SIZE>
struct ThreadSafeFiniteJobQueue {
  std::mutex lock;
  T elements[SIZE];
  i32 head           = 0;
  i32 count          = 0;
  std::atomic<i32> remaining_jobs = 0;

  const static size_t MAX_COUNT = SIZE;

  i32 push(T elem)
  {
    lock.lock();
    i32 pos       = (head + count) % SIZE;
    elements[pos] = elem;
    assert(count < SIZE);
    count++;
    remaining_jobs++;
    lock.unlock();

    return pos;
  }

  b8 pop(T *elem)
  {
    b8 exists = false;

    lock.lock();
    if (count > 0) {
      i32 pos = head % SIZE;
      *elem   = elements[pos];

      head++;
      count--;
      exists = true;
    }
    lock.unlock();

    return exists;
  }

  void worker_thread()
  {
    while (true) {
      T job;
      if (pop(&job)) {
        job.do_work();
        remaining_jobs--;
      }

      if (remaining_jobs == 0) {
        return;
      }
    }
  }
};
struct TraverseBvhJob {
  Camera *camera;
  Vec3f ray_origin;
  i32 x, y;

  void do_work()
  {
    glm::mat4 inverse_camera = glm::inverse(camera->view);
    Vec3f pixel_ndc     = {((float)x / RT_WIDTH) * 2.f - 1, ((float)y / RT_HEIGHT) * 2.f - 1, -1};
    glm::vec3 world_pos = inverse_camera * glm::vec4(pixel_ndc.x, pixel_ndc.y, pixel_ndc.z, 1);
    Vec3f pixel_pos     = {world_pos.x, world_pos.y, world_pos.z};
    Vec3f ray_dir       = normalize(pixel_pos - ray_origin);

    float t = traverse_bvh_threaded(ray_origin, ray_dir);

    hdr_image[(RT_WIDTH * y + x) * 3]     = t;
    hdr_image[(RT_WIDTH * y + x) * 3 + 1] = t;
    hdr_image[(RT_WIDTH * y + x) * 3 + 2] = t;
  }
};
ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000> bvh_traversal_queue;

void Renderer::raytrace_threaded(Camera *camera, Vec3f camera_pos)
{
  Timer timer;

  for (i32 y = 0; y < RT_HEIGHT; y++) {
    for (i32 x = 0; x < RT_WIDTH; x++) {
      TraverseBvhJob job;
      job.camera     = camera;
      job.ray_origin = camera_pos;
      job.x          = x;
      job.y          = y;

      bvh_traversal_queue.push(job);
    }
  }

  std::thread t0(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t1(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t2(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t3(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t4(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t5(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t6(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t7(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t8(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t9(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t10(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t11(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t12(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t13(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t14(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t15(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t16(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t17(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t18(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);
  std::thread t19(&ThreadSafeFiniteJobQueue<TraverseBvhJob, 3000000>::worker_thread, &bvh_traversal_queue);

  t0.join();
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  t6.join();
  t7.join();
  t8.join();
  t9.join();
  t10.join();
  t11.join();
  t12.join();
  t13.join();
  t14.join();
  t15.join();
  t16.join();
  t17.join();
  t18.join();
  t19.join();

  for (i32 y = 0; y < RT_HEIGHT; y++) {
    for (i32 x = 0; x < RT_WIDTH; x++) {
      float hdr                         = hdr_image[(RT_WIDTH * y + x) * 3];
      float normalized                  = hdr / 20.f;
      image[(RT_WIDTH * y + x) * 3]     = normalized * 255;
      image[(RT_WIDTH * y + x) * 3 + 1] = normalized * 255;
      image[(RT_WIDTH * y + x) * 3 + 2] = normalized * 255;
    }
  }

  rt_tex.upload(image, true);

  timer.print_ms();
}

void Renderer::upload_bvh(){
  
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvh_ssbo);

  for (i32 i = 0; i < triangles.size(); i++) {
    int buf_index = 48 * i;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index, 16,
                    &triangles[i].verts[0]);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index + 16, 16,
                    &triangles[i].verts[1]);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index + 32, 16,
                    &triangles[i].verts[2]);

  }
  
  for (i32 i = 0; i < threaded_nodes.count; i++) {
    int buf_index = 48 * 5000000 + 48 * i;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index, 16,
                    &threaded_nodes[i].bounds.min);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index + 16, 16,
                    &threaded_nodes[i].bounds.max);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index + 28, 4,
                    &threaded_nodes[i].child_0);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index + 32, 4,
                    &threaded_nodes[i].child_1);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index + 36, 4,
                    &threaded_nodes[i].triangle_i_start);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index + 40, 4,
                    &threaded_nodes[i].triangle_count);
  }
}
void Renderer::raytrace_gpu(Camera *camera, Vec3f camera_pos){
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvh_ssbo);

  int buf_index = 48 * 5000000 + 48 * 20000000;
  glm::mat4 inverse_camera = glm::inverse(camera->view);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index, 64,
                  glm::value_ptr(inverse_camera));
  glm::vec3 glm_camera_pos = {camera_pos.x, camera_pos.y, camera_pos.z};
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, buf_index + 64, 16,
                  glm::value_ptr(glm_camera_pos));

  //// 
  glBindImageTexture(0, rt_gpu_tex.gl_ref, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glUseProgram(rtshadow_compute_shader.shader_handle);
  glDispatchCompute((GLuint)rt_gpu_tex.width / 8, (GLuint)rt_gpu_tex.height / 4, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  rt_gpu_tex.gen_mipmaps();
}