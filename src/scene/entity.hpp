#pragma once

#include "../util.hpp"
#include "../math.hpp"

enum struct EntityType
{
    UNKNOWN,
    MESH,
    LIGHT,
    CAMERA,
};

struct Transform
{
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
};

struct SpotLightDef
{
    Vec3f color;
    float outer_angle;
    float inner_angle;
};

struct Entity
{
    EntityType type = EntityType::UNKNOWN;

    VertexBuffer vert_buffer;
    Material *material = nullptr;
    Shader *shader = nullptr;

    Transform transform;

    SpotLightDef spot_light;
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

    T *push_back(T &value)
    {
        Element *current = next;
        if (!current)
        {
            return nullptr;
        }

        next = current->next;

        current->value = value;
        current->assigned = true;

        return &current->value;
    }

    void free(Element *elem)
    {
        elem->assigned = false;
        elem->next = nullptr;
        last->next = elem;
        last = elem;
    }
};
