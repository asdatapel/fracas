#pragma once

#include <array>

#include "../animation.hpp"

struct Scene
{
    Texture unfiltered_cubemap;
    StandardPbrEnvMaterial env_mat;

    FreeList<Entity> entities;

    // TODO this is temporary until we can connect entities by id
    std::unordered_map<std::string, int> entity_names;

    struct Bar
    {
        int entity_id;
        Texture num_tex;
        RenderTarget target;
        AllocatedMaterial<StandardPbrMaterial::N + 1> material;

        float animation_t = 0.f;
    };
    std::array<Bar, 8> bars;

    RenderTarget score_targets[3];
    AllocatedMaterial<StandardPbrMaterial::N + 1> score_materials[3];

    int floor_id;
    Camera flipped_camera;
    RenderTarget floor_target;
    AllocatedMaterial<StandardPbrMaterial::N + 1> floor_material;

    int uv_sphere_id;
    int icosahedron_id;
    int brick_id;

    Font font;

    void init(Memory mem);
    void load(Assets *assets, Memory mem);
    void update_and_draw(RenderTarget backbuffer, InputState *input, Camera *camera);
    
    Entity *get(int i)
    {
        return entities.data[i].assigned ? &entities.data[i].value : nullptr;
    }

    Animation anim;
};

#include "scene.cpp"