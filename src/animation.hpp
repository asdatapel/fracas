#pragma once

#include <glm/gtx/matrix_decompose.hpp>

glm::quat lerp_quat(glm::quat a, glm::quat b, f32 t) { return (1 - t) * a + t * b; }

struct SRT {
  Vec3f scale;
  glm::quat rotation;
  Vec3f translation;

  glm::mat4 to_mat() {
    return glm::translate(glm::mat4(1.0), {translation.x, translation.y, translation.z}) *
           glm::toMat4(rotation) *
           glm::scale(glm::mat4(1.0), {scale.x, scale.y, scale.z});
  }
};

SRT lerp(SRT a, SRT b, f32 t) {
  Vec3f pos = {
      lerp(a.translation.x, b.translation.x, t),
      lerp(a.translation.y, b.translation.y, t),
      lerp(a.translation.z, b.translation.z, t),
  };
  glm::quat rot = lerp_quat(a.rotation, b.rotation, t);
  Vec3f scale   = {
      lerp(a.scale.x, b.scale.x, t),
      lerp(a.scale.y, b.scale.y, t),
      lerp(a.scale.z, b.scale.z, t),
  };

  return {scale, rot, pos};
}

// a - b
SRT diff(SRT a, SRT b) {
  SRT d;
  d.scale = a.scale - b.scale;
  d.rotation = a.rotation * glm::inverse(b.rotation);
  d.translation = a.translation - b.translation;
  return d;
}

struct Bone {
  i32 parent_index = -1;

  glm::mat4 offset;

  // TODO: this should be stored as an SRT
  glm::mat4 default_transform;
  SRT get_default_transform()
  {
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(default_transform, scale, rotation, translation, skew, perspective);

    Vec3f p = {translation.x, translation.y, translation.z};
    Vec3f s = {scale.x, scale.y, scale.z};
    return {s, rotation, p};
  }
};

struct Skeleton {
  RefArray<Bone> bones;
};

struct BoneTrack {
  Bone *bone;

  RefArray<Vec3f> position_keys;
  RefArray<glm::quat> rotation_keys;
  RefArray<Vec3f> scale_keys;

  SRT interpolate(f32 frame) {
    i32 frame_a   = (i32)frame;
    i32 frame_b   = frame_a + 1;
    f32 frame_t = frame - frame_a;

    if (frame_a >= position_keys.len) {
      return bone->get_default_transform();
    }

    if (frame_b >= position_keys.len) {
      frame_b = 0;
    }

    Vec3f pos = {
        lerp(position_keys[frame_a].x, position_keys[frame_b].x, frame_t),
        lerp(position_keys[frame_a].y, position_keys[frame_b].y, frame_t),
        lerp(position_keys[frame_a].z, position_keys[frame_b].z, frame_t),
    };
    glm::quat rot = lerp_quat(rotation_keys[frame_a], rotation_keys[frame_b], frame_t);
    Vec3f scale   = {
        lerp(scale_keys[frame_a].x, scale_keys[frame_b].x, frame_t),
        lerp(scale_keys[frame_a].y, scale_keys[frame_b].y, frame_t),
        lerp(scale_keys[frame_a].z, scale_keys[frame_b].z, frame_t),
    };

    return {scale, rot, pos};
  }
};

struct Pose {
  Skeleton *skeleton;
  std::vector<SRT> local_transforms;

  std::vector<glm::mat4> global_transforms;
  std::vector<glm::mat4> final_mats;

  void calculate_final_mats()
  {
    global_transforms.resize(skeleton->bones.len);
    final_mats.resize(skeleton->bones.len);

    for (i32 i = 0; i < skeleton->bones.len; i++) {
      glm::mat4 parent_transform = glm::mat4(1.f);
      if (skeleton->bones[i].parent_index >= 0) {
        parent_transform = global_transforms[skeleton->bones[i].parent_index];
      }

      glm::mat4 local = local_transforms[i].to_mat();

      global_transforms[i] = parent_transform * local;
      final_mats[i]        = global_transforms[i] * skeleton->bones[i].offset;//global_transforms[i] * skeleton->bones[i].offset;
    }
  }
};

struct SkeletalAnimation {
  Skeleton *skeleton;
  RefArray<BoneTrack> tracks;

  Pose eval(f32 frame) {
    Pose pose;
    pose.skeleton = skeleton;
    for (int i = 0; i < skeleton->bones.len; i++) {
      pose.local_transforms.push_back(tracks[i].interpolate(frame));
    }

    return pose;
  }

  // TODO: should not be calculated on the fly, the track should be diffed already
  Pose eval_as_additive(f32 frame, b8 against_first_frame = false) {
    Pose pose;
    pose.skeleton = skeleton;

    for (int i = 0; i < skeleton->bones.len; i++) {
      SRT base = against_first_frame ? tracks[i].interpolate(0)
                                     : skeleton->bones[i].get_default_transform();

      pose.local_transforms.push_back(diff(tracks[i].interpolate(frame), base));
    }

    return pose;
  }
};

Pose additive_blend(Pose *p1, Pose *p2, f32 weight) {
  Pose pose;
  pose.skeleton = p1->skeleton;

  for (i32 i = 0; i < p1->local_transforms.size(); i++) {
    SRT srt;
    srt.scale = p1->local_transforms[i].scale + lerp({}, p2->local_transforms[i].scale, weight);
    srt.rotation = lerp_quat(glm::quat({0.f, 0.f, 0.f}), p2->local_transforms[i].rotation, weight) * p1->local_transforms[i].rotation;
    srt.translation = p1->local_transforms[i].translation + lerp({}, p2->local_transforms[i].translation, weight);
    pose.local_transforms.push_back(srt);
  }

  return pose;
}

SkeletalAnimation parse_animation(FileData file, Memory mem)
{
  char *data = file.data;

  Skeleton *skeleton = (Skeleton *)mem.allocator->alloc(sizeof(Skeleton));
  skeleton->bones.len = *(i32 *)data;
  data += sizeof(i32);
  skeleton->bones.data = (Bone *)mem.allocator->alloc(sizeof(Bone) * skeleton->bones.len);

  SkeletalAnimation animation;
  animation.skeleton = skeleton;
  animation.tracks.len = skeleton->bones.len;
  animation.tracks.data = (BoneTrack *)mem.allocator->alloc(sizeof(BoneTrack) * animation.tracks.len);

  for (i32 bone_index = 0; bone_index < skeleton->bones.len; bone_index++) {
    Bone *b = &skeleton->bones[bone_index];

    b->parent_index = *(i32 *)data;
    data += sizeof(i32);

    memcpy(&b->offset[0][0], data, sizeof(f32) * 16);
    data += sizeof(f32) * 16;
    memcpy(&b->default_transform[0][0], data, sizeof(f32) * 16);
    data += sizeof(f32) * 16;

    BoneTrack *track = &animation.tracks[bone_index];
    track->bone = b;

    track->position_keys.len = *(i32 *)data;
    data += sizeof(i32);
    track->position_keys.data = (Vec3f *)mem.allocator->alloc(sizeof(Vec3f) * track->position_keys.len);
    memcpy(track->position_keys.data, data, sizeof(Vec3f) * track->position_keys.len);
    data += sizeof(Vec3f) * track->position_keys.len;

    track->rotation_keys.len = *(i32 *)data;
    data += sizeof(i32);
    track->rotation_keys.data =
        (glm::quat *)mem.allocator->alloc(sizeof(glm::quat) * track->rotation_keys.len);
    memcpy(track->rotation_keys.data, data, sizeof(glm::quat) * track->rotation_keys.len);
    data += sizeof(glm::quat) * track->rotation_keys.len;

    track->scale_keys.len = *(i32 *)data;
    data += sizeof(i32);
    track->scale_keys.data = (Vec3f *)mem.allocator->alloc(sizeof(Vec3f) * track->scale_keys.len);
    memcpy(track->scale_keys.data, data, sizeof(Vec3f) * track->scale_keys.len);
    data += sizeof(Vec3f) * track->scale_keys.len;
  }

  return animation;
}