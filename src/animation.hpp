#pragma once

struct Bone
{
    Bone *parent = nullptr;
    glm::mat4 offset;
    glm::mat4 default_transform;

    RefArray<Vec3f> position_keys;
    RefArray<glm::quat> rotation_keys;
    RefArray<Vec3f> scale_keys;

    glm::mat4 global_transform;

    glm::mat4 interpolate(float frame)
    {
        auto lerp = [](float a, float b, float t)
        {
            return (1 - t) * a + t * b;
        };
        auto lerp_quat = [](glm::quat a, glm::quat b, float t)
        {
            return (1 - t) * a + t * b;
        };

        int frame_a = (int)frame;
        int frame_b = frame_a + 1;
        float frame_t = frame - frame_a;

        if (frame_a >= position_keys.len)
        {
            return default_transform;
        }

        if (frame_b >= position_keys.len)
        {
            frame_b = 0;
        }

        Vec3f pos = {
            lerp(position_keys[frame_a].x, position_keys[frame_b].x, frame_t),
            lerp(position_keys[frame_a].y, position_keys[frame_b].y, frame_t),
            lerp(position_keys[frame_a].z, position_keys[frame_b].z, frame_t),
        };
        glm::quat rot = lerp_quat(rotation_keys[frame_a], rotation_keys[frame_b], frame_t);
        Vec3f scale = {
            lerp(scale_keys[frame_a].x, scale_keys[frame_b].x, frame_t),
            lerp(scale_keys[frame_a].y, scale_keys[frame_b].y, frame_t),
            lerp(scale_keys[frame_a].z, scale_keys[frame_b].z, frame_t),
        };

        return glm::translate(glm::mat4(1.0), {pos.x, pos.y, pos.z}) *
               glm::toMat4(rot) * glm::scale(glm::mat4(1.0), {scale.x, scale.y, scale.z});
    }

    int num_frames()
    {
        return position_keys.len;
    }

    glm::mat4 calc_final_matrix(float frame)
    {
        glm::mat4 my_transform = interpolate(frame);

        glm::mat4 parent_transform = glm::mat4(1.f); //glm::inverse(glm::mat4(1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0));
        if (parent)
        {
            parent_transform = parent->global_transform;
        }
        global_transform = parent_transform * my_transform;

        return global_transform * offset;
    }
};

struct Animation
{
    Bone *bones;
    int num_bones;

    // TODO use something less dynamic
    std::vector<glm::mat4> mats;
    void update(float t)
    {
        mats.clear();
        for (int i = 0; i < num_bones; i++)
        {
            float frame = fmod(t, 1.f) * bones[i].num_frames();
            mats.push_back(bones[i].calc_final_matrix(frame));
        }
    }
};

Animation parse_animation(FileData file, Memory mem)
{
    char *data = file.data;

    Animation animation;
    animation.num_bones = *(int *)data;
    data += sizeof(int);
    animation.bones = (Bone *)mem.allocator->alloc(sizeof(Bone) * animation.num_bones);

    for (int bone_index = 0; bone_index < animation.num_bones; bone_index++)
    {
        Bone *b = animation.bones + bone_index;

        int parent = *(int *)data;
        b->parent = parent > -1 ? &animation.bones[parent] : nullptr;
        data += sizeof(int);

        memcpy(&b->offset[0][0], data, sizeof(float) * 16);
        data += sizeof(float) * 16;
        memcpy(&b->default_transform[0][0], data, sizeof(float) * 16);
        data += sizeof(float) * 16;

        b->position_keys.len = *(int *)data;
        data += sizeof(int);
        b->position_keys.data = (Vec3f *)mem.allocator->alloc(sizeof(Vec3f) * b->position_keys.len);
        memcpy(b->position_keys.data, data, sizeof(Vec3f) * b->position_keys.len);
        data += sizeof(Vec3f) * b->position_keys.len;

        b->rotation_keys.len = *(int *)data;
        data += sizeof(int);
        b->rotation_keys.data = (glm::quat *)mem.allocator->alloc(sizeof(glm::quat) * b->rotation_keys.len);
        memcpy(b->rotation_keys.data, data, sizeof(glm::quat) * b->rotation_keys.len);
        data += sizeof(glm::quat) * b->rotation_keys.len;

        b->scale_keys.len = *(int *)data;
        data += sizeof(int);
        b->scale_keys.data = (Vec3f *)mem.allocator->alloc(sizeof(Vec3f) * b->scale_keys.len);
        memcpy(b->scale_keys.data, data, sizeof(Vec3f) * b->scale_keys.len);
        data += sizeof(Vec3f) * b->scale_keys.len;
    }

    return animation;
}