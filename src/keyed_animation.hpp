#pragma once

#include "util.hpp"

#include "scene/entity.hpp"

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
  Array<Key, 16> keys;

  void add_key(Transform transform, i32 frame, Key::InterpolationType interpolation_type) {
    i32 pos = 0;
    while (pos < keys.len) {
      if (keys[pos].frame == frame) {
        keys[pos].transform          = transform;
        keys[pos].interpolation_type = interpolation_type;
        return;
      }
      if (keys[pos].frame > frame) {
        break;
      }
      pos++;
    }

    keys.append({});
    for (i32 i = keys.len - 1; i > pos; i--) {
      keys[i] = keys[i - 1];
    }
    keys[pos].transform          = transform;
    keys[pos].frame              = frame;
    keys[pos].interpolation_type = interpolation_type;
  }

  Transform eval(f32 t, i32 fps) {
    if (keys.len == 0) return {};

    f32 frame = t * fps;

    i32 base_key_i = 0;
    while (base_key_i < keys.len) {
      if (base_key_i == keys.len - 1 || keys[base_key_i + 1].frame >= frame) {
        break;
      }
      base_key_i++;
    }

    Key base_key = keys[base_key_i];

    // default to constant interpolation
    Transform transform = base_key.transform;

    if (base_key_i + 1 < keys.len) {
      Key target_key    = keys[base_key_i + 1];
      f32 lerp_distance = target_key.frame - base_key.frame;

      if (lerp_distance != 0) {
        f32 lerp_t = (frame - base_key.frame) / lerp_distance;

        if (base_key.interpolation_type == Key::InterpolationType::LINEAR) {
          transform.position =
              lerp(base_key.transform.position, target_key.transform.position, lerp_t);
          transform.rotation =
              lerp(base_key.transform.rotation, target_key.transform.rotation, lerp_t);
          transform.scale = lerp(base_key.transform.scale, target_key.transform.scale, lerp_t);
        } else if (base_key.interpolation_type == Key::InterpolationType::SMOOTHSTEP) {
          f32 smoothstep_t = clamp(lerp_t * lerp_t * (3.0 - 2.0 * lerp_t), 0.0, 1.0);
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
          Key key_3 = base_key_i + 2 < keys.len ? keys[base_key_i + 2] : target_key;

          Spline3 pos_spline   = {{key_0.transform.position, key_1.transform.position,
                                 key_2.transform.position, key_3.transform.position}};
          Spline3 rot_spline   = {{key_0.transform.rotation, key_1.transform.rotation,
                                 key_2.transform.rotation, key_3.transform.rotation}};
          Spline3 scale_spline = {{key_0.transform.rotation, key_1.transform.rotation,
                                   key_2.transform.rotation, key_3.transform.rotation}};

          transform.position = catmull_rom(lerp_t, pos_spline);
          transform.rotation = catmull_rom(lerp_t, rot_spline);
          transform.scale    = catmull_rom(lerp_t, scale_spline);
        }
      }
    }

    return transform;
  }
};

struct KeyedAnimation {
  u32 fps = 30;
  Array<KeyedAnimationTrack, 16> tracks;

  void add_track(EntityId entity_id) {
    for (i32 i = 0; i < tracks.len; i++) {
      if (tracks[i].entity_id == entity_id) return;
    }

    KeyedAnimationTrack track;
    track.entity_id = entity_id;
    tracks.append(track);
  }

  void apply(Scene *scene, f32 t) {
    for (u32 i = 0; i < tracks.len; i++) {
      KeyedAnimationTrack &track = tracks[i];
      Entity *entity             = scene->get(track.entity_id);
      
      entity->transform          = track.eval(t, fps);
    }
  }
  void apply(Scene *scene, i32 frame) {
    i32 t = frame / fps;
    apply(scene, t);
  }

  Transform eval(u32 track_i, float t) {
    assert(track_i < tracks.len);
    return tracks[track_i].eval(t, fps);
  }
};