#pragma once

#include "../animation.hpp"
#include "../camera.hpp"
#include "../common.hpp"
#include "../math.hpp"
#include "../material.hpp"
#include "../util.hpp"

enum struct EntityType
{
    UNKNOWN,
    MESH,
    LIGHT,
    CAMERA,
    SPLINE,
};
String to_string(EntityType type)
{
    switch (type)
    {
    case (EntityType::UNKNOWN):
        return "UNKNOWN";
        break;
    case (EntityType::MESH):
        return "MESH";
        break;
    case (EntityType::LIGHT):
        return "LIGHT";
        break;
    case (EntityType::CAMERA):
        return "CAMERA";
        break;
    case (EntityType::SPLINE):
        return "SPLINE";
        break;
    }
}
EntityType entity_type_from_string(String type)
{
    if (strcmp(type, "MESH"))
        return EntityType::MESH;
    if (strcmp(type, "LIGHT"))
        return EntityType::LIGHT;
    if (strcmp(type, "CAMERA"))
        return EntityType::CAMERA;
    if (strcmp(type, "SPLINE"))
        return EntityType::SPLINE;

    return EntityType::UNKNOWN;
}

struct DebugTag
{
    AllocatedString<32> name;
};

struct SpotLightDef
{
    Vec3f color;
    float outer_angle;
    float inner_angle;
};

struct Spline3
{
    Array<Vec3f, 4> points;
};

struct Entity
{
    EntityType type = EntityType::UNKNOWN;
    DebugTag debug_tag;

    Transform transform;

    VertexBuffer vert_buffer;
    Material *material = nullptr;
    Shader *shader = nullptr;

    SpotLightDef spot_light;

    Camera camera;

    Spline3 spline;

    Animation *animation = nullptr;
};
