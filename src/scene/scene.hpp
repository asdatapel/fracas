#pragma once

#include <array>

#include "../animation.hpp"

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

    Entity *get(int id);
    void init(Memory mem, TextureFormat texture_format = TextureFormat::RGB16F);
    void load(const char *filename, Assets *assets, Memory mem);
    void update_and_draw(Camera *editor_camera);

    void set_planar_target(RenderTarget target);

    void render_entities(Camera *camera);
};

#include "scene.cpp"