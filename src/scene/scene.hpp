#pragma once

#include <array>

#include "../animation.hpp"
#include "../assets.hpp"
#include "../scripts.hpp"
#include "entity.hpp"

struct Scene {
  FreeList<Entity> entities;
  Scripts scripts;

   // sequence stuff
  f32 sequence_t                   = 0.f;
  bool playing_sequence            = false;
  KeyedAnimation *current_sequence = nullptr;

  struct EntityTransform {
    EntityId id;
    Transform transform;
  };
  DynamicArray<EntityTransform, 128> saved_transforms;

  Entity *get(int id);
  void init(Memory mem);
  void load(String filename, Assets *assets, Memory mem);
  void serialize(const char *filename, Assets *assets, StackAllocator *alloc);
  void update(float timestep);

  void set_planar_target(RenderTarget target);

  // sequence stuff
  void set_sequence(KeyedAnimation *seq);
  void play_sequence();
  void stop_sequence();
  bool is_sequence_finished();
  void set_t(f32 t);
  u32 get_frame();
  void set_frame(u32 frame);
  void apply_keyed_animation(KeyedAnimation *keyed_anim, f32 t);
  void apply_keyed_animation(KeyedAnimation *keyed_anim, i32 frame);
};

#include "scene.cpp"