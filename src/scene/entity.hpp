#pragma once

#include "../math.hpp"

struct Entity
{
    VertexBuffer vert_buffer;
    Material *material;

    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
};

