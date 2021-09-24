#pragma once

#include "../util.hpp"
#include "../math.hpp"
#include "../camera.hpp"

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

    VertexBuffer vert_buffer;
    Material *material = nullptr;
    Shader *shader = nullptr;

    Transform transform;

    SpotLightDef spot_light;

    Camera camera;

    Spline3 spline;
};

template <typename T>
struct FreeList
{
    struct Element
    {
        bool assigned;
        union
        {
            T value;
            Element *next;
        };
    };

    Element *data = nullptr;
    int size = 0;
    Element *next = nullptr, *last = nullptr;

    void init(StackAllocator *allocator, int size)
    {
        this->size = size;
        data = (Element *)allocator->alloc(size * sizeof(Element));
        next = data;
        last = next;

        for (int i = 0; i < size; i++)
        {
            free(data + i);
        }
    }

    int push_back(T &value)
    {
        Element *current = next;
        if (!current)
        {
            return -1;
        }

        next = current->next;

        current->value = value;
        current->assigned = true;

        return current - data;
    }

    int get_index()
    {

    }

    void free(Element *elem)
    {
        elem->assigned = false;
        elem->next = nullptr;
        last->next = elem;
        last = elem;
    }
};
