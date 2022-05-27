#pragma once

#include "../graphics/bloomer.hpp"
#include "../scene/scene.hpp"
#include "scene_renderer.hpp"

const i32 WIDTH = 1920;
const i32 HEIGHT = 1080;

struct Compositor {
  Scene *scene;

  RefArray<ViewLayer> view_layers;
  RefArray<RenderTarget> layer_targets;
  RenderTarget final_target;

  Bloomer bloomer;

  void init(RefArray<ViewLayer> view_layers, Scene *scene, Assets *assets, Memory mem)
  {
    this->view_layers = view_layers;
    this->scene = scene;

    layer_targets.len = view_layers.len;
    layer_targets.data = (RenderTarget*) mem.allocator->alloc(layer_targets.len * sizeof(RenderTarget));

    // these parameters should be configured and read from a file
    final_target = RenderTarget(WIDTH, HEIGHT, TextureFormat::RGB8, TextureFormat::DEPTH24);
    layer_targets[0] = RenderTarget(WIDTH, HEIGHT, TextureFormat::RGBA16F, TextureFormat::DEPTH24);
    for (i32 i = 1; i < view_layers.len; i++) {
      layer_targets[1] = RenderTarget(WIDTH, HEIGHT, TextureFormat::RGBA16F, TextureFormat::DEPTH24);
    }

    bloomer = Bloomer(WIDTH, HEIGHT);
  }

  void render(Camera *debug_camera, Assets *assets, Vec3f debug_camera_pos, float exposure = 1.f)
  {
    // TODO handle target resize

    render_scene(scene, &view_layers[0], layer_targets[0], debug_camera, debug_camera_pos, 0, assets);
    for (i32 i = 1; i < view_layers.len; i++) {
      if (view_layers[i].visible) {
        render_scene(scene, &view_layers[i], layer_targets[i], nullptr, debug_camera_pos, i, assets);
        
        layer_targets[0].bind();
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        draw_textured_rect(layer_targets[0], {0, 0, WIDTH, HEIGHT}, {}, layer_targets[i].color_tex);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
      }
    }

    bloomer.do_bloom(layer_targets[0]);

    bind_shader(tonemap_shader);
    bind_1f(tonemap_shader, UniformId::EXPOSURE, exposure);
    bind_texture(tonemap_shader, UniformId::BASE, layer_targets[0].color_tex);
    bind_texture(tonemap_shader, UniformId::BLOOM, bloomer.get_final().color_tex);

    final_target.bind();
    glDisable(GL_DEPTH_TEST);
    draw_rect();
    glEnable(GL_DEPTH_TEST);

    final_target.color_tex.gen_mipmaps();
  }
};