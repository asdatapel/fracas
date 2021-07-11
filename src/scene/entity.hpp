#pragma once

#include "../math.hpp"

struct SpotLightDef
{
    Vec3f color;
    float outer_angle;
    float inner_angle;
};

struct Entity
{
    VertexBuffer vert_buffer;
    Material *material = nullptr;
    Shader *shader = nullptr;

    SpotLightDef spot_light;

    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
};

