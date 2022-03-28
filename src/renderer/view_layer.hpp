#pragma once

#include "../scene/entity.hpp"

struct ViewLayer {
  int active_camera_id = -1;

  u32 visiblity_mask = 1;
  
  EnvMap *env_map;
  
  bool render_planar    = false;
  Entity *planar_entity = nullptr;
  RenderTarget planar_target = {};
  
  bool visible         = false;
  bool cubemap_visible = true;
};