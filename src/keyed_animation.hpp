#pragma once

#include "util.hpp"

#include "global_allocators.hpp"
#include "scene/entity.hpp"
#include "spline.hpp"

template <typename T, u32 INITIAL_CAPACITY = 2>
struct DynamicArray {
  T *elements  = nullptr;
  u32 count    = 0;
  u32 capacity = INITIAL_CAPACITY;

  StackAllocator *allocator;

  DynamicArray() = default;
  DynamicArray(StackAllocator *allocator)
  {
    this->allocator = allocator;
    elements        = (T *)allocator->alloc(sizeof(T) * capacity);
  }

  void resize(u32 new_size)
  {
    T *new_elements = (T *)allocator->resize((char *)elements, sizeof(T) * new_size);

    if (new_elements != elements) {
      u32 to_copy_n = std::min(capacity, new_size);
      memcpy(new_elements, elements, to_copy_n * sizeof(T));
    }

    capacity = new_size;
    elements = new_elements;
  }

  T &push_back(T val)
  {
    if (count >= capacity) {
      resize(capacity * 2);
    }
    elements[count] = val;
    count++;
    return elements[count - 1];
  }

  void remove(u32 i)
  {
    assert(i < count);
    memcpy(elements + i, elements + i + 1, (count - (i + 1)) * sizeof(T));
    count--;
  }

  void clear() { count = 0; }

  T &operator[](u32 i)
  {
    assert(i < count);
    return elements[i];
  }
};

struct KeyedAnimationTrack {
  struct Key {
    enum struct InterpolationType {
      CONSTANT,
      LINEAR,
      SMOOTHSTEP,
      SPLINE,
    };

    Transform transform;
    i32 frame;
    InterpolationType interpolation_type;
  };

  EntityId entity_id;
  DynamicArray<Key> keys;

  KeyedAnimationTrack() : keys(&assets_allocator) {}

  u32 add_key(Transform transform, i32 frame, Key::InterpolationType interpolation_type)
  {
    i32 pos = 0;
    while (pos < keys.count) {
      if (keys[pos].frame == frame) {
        keys[pos].transform          = transform;
        keys[pos].interpolation_type = interpolation_type;
        return pos;
      }
      if (keys[pos].frame > frame) {
        break;
      }
      pos++;
    }

    keys.push_back({});
    for (i32 i = keys.count - 1; i > pos; i--) {
      keys[i] = keys[i - 1];
    }
    keys[pos].transform          = transform;
    keys[pos].frame              = frame;
    keys[pos].interpolation_type = interpolation_type;

    return pos;
  }
  u32 add_key(Key key) { return add_key(key.transform, key.frame, key.interpolation_type); }

  Transform eval(f32 t, i32 fps)
  {
    if (keys.count == 0) return {};

    f32 frame = t * fps;

    i32 base_key_i = 0;
    while (base_key_i < keys.count) {
      if (base_key_i == keys.count - 1 || keys[base_key_i + 1].frame >= frame) {
        break;
      }
      base_key_i++;
    }

    Key base_key = keys[base_key_i];

    // default to constant interpolation
    Transform transform = base_key.transform;

    if (base_key_i + 1 < keys.count) {
      Key target_key    = keys[base_key_i + 1];
      i32 lerp_distance = target_key.frame - base_key.frame;

      if (lerp_distance != 0) {
        f32 lerp_t = (frame - base_key.frame) / lerp_distance;

        if (base_key.interpolation_type == Key::InterpolationType::LINEAR) {
          transform.position =
              lerp(base_key.transform.position, target_key.transform.position, lerp_t);
          transform.rotation =
              lerp(base_key.transform.rotation, target_key.transform.rotation, lerp_t);
          transform.scale = lerp(base_key.transform.scale, target_key.transform.scale, lerp_t);
        } else if (base_key.interpolation_type == Key::InterpolationType::SMOOTHSTEP) {
          f32 smoothstep_t = clamp(lerp_t * lerp_t * (3.0 - 2.0 * lerp_t), 0.f, 1.f);
          transform.position =
              lerp(base_key.transform.position, target_key.transform.position, smoothstep_t);
          transform.rotation =
              lerp(base_key.transform.rotation, target_key.transform.rotation, smoothstep_t);
          transform.scale =
              lerp(base_key.transform.scale, target_key.transform.scale, smoothstep_t);
        } else if (base_key.interpolation_type == Key::InterpolationType::SPLINE) {
          Key key_0 = base_key_i > 0 ? keys[base_key_i - 1] : base_key;
          Key key_1 = base_key;
          Key key_2 = target_key;
          Key key_3 = base_key_i + 2 < keys.count ? keys[base_key_i + 2] : target_key;

          Spline3 pos_spline   = {{key_0.transform.position, key_1.transform.position,
                                 key_2.transform.position, key_3.transform.position}};
          Spline3 rot_spline   = {{key_0.transform.rotation, key_1.transform.rotation,
                                 key_2.transform.rotation, key_3.transform.rotation}};
          Spline3 scale_spline = {{key_0.transform.scale, key_1.transform.scale,
                                   key_2.transform.scale, key_3.transform.scale}};

          transform.position = catmull_rom(lerp_t, pos_spline);
          transform.rotation = catmull_rom(lerp_t, rot_spline);
          transform.scale    = catmull_rom(lerp_t, scale_spline);
        }
      }
    }

    return transform;
  }
};

struct KeyedAnimation : Asset {
  u32 fps         = 30;
  u32 start_frame = 0;
  u32 end_frame   = 100;

  DynamicArray<KeyedAnimationTrack> tracks;

  KeyedAnimation(u32 fps) : fps(fps), tracks(&assets_allocator) {}

  void add_track(EntityId entity_id)
  {
    for (i32 i = 0; i < tracks.count; i++) {
      if (tracks[i].entity_id == entity_id) return;
    }

    KeyedAnimationTrack track;
    track.entity_id = entity_id;
    tracks.push_back(track);
  }

  Transform eval(u32 track_i, f32 t)
  {
    assert(track_i < tracks.count);
    return tracks[track_i].eval(t, fps);
  }
};