#pragma once

#include <array>

#include "../animation.hpp"
#include "../assets.hpp"
#include "entity.hpp"

struct Scene
{
    FreeList<Entity> entities;
    int active_camera_id = -1;

    Texture unfiltered_cubemap;
    StandardPbrEnvMaterial env_mat;
    RenderTarget target;

    bool render_planar = false;
    Entity *planar_entity = nullptr;
    RenderTarget planar_target;

    bool visible = false;
    bool cubemap_visible = true;
    
    // sequence stuff
    f32 sequence_t = 0.f;
    bool playing_sequence = false;
    KeyedAnimation *current_sequence = nullptr;

    struct EntityTransform {
        EntityId id;
        Transform transform;
    };
    DynamicArray<EntityTransform, 128> saved_transforms;

    Entity *get(int id);
    void init(Memory mem, TextureFormat texture_format = TextureFormat::RGB16F);
    void load(const char *filename, Assets *assets, Memory mem);
    void serialize(const char *filename, Assets *assets, StackAllocator *alloc);
    void update(float timestep);
    void render(Camera *editor_camera, Vec3f editor_camera_pos);

    void set_planar_target(RenderTarget target);

    void render_entities(Camera *camera, Vec3f camera_postion);

    // sequence stuff
    void set_sequence(KeyedAnimation *seq);
    void play_sequence();
    void stop_sequence();
    void set_t(float t);
    u32 get_frame();
    void set_frame(u32 frame);
    void apply_keyed_animation(KeyedAnimation *keyed_anim, f32 t);
    void apply_keyed_animation(KeyedAnimation *keyed_anim, i32 frame);
};

#include "scene.cpp"