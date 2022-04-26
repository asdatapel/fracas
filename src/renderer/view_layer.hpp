#pragma once

#include "../scene/entity.hpp"

struct ViewLayer {
  int active_camera_id = -1;

  u32 visiblity_mask = 1;
  
  EnvMap *env_map;
  
  bool visible         = false;
  bool cubemap_visible = true;
};