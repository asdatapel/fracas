#pragma once

#include "util.hpp"

#include "scene/entity.hpp"

struct KeyedAnimation
{
    struct Key
    {
        enum struct InterpolationType
        {
            CONSTANT,
            LINEAR,
            SMOOTHSTEP,
            SPLINE,
        };

        Transform transform;
        int frame;
        InterpolationType interpolation_type;
    };

    int fps;
    Array<Key, 16> keys;

    void add_key(Transform transform, int frame, Key::InterpolationType interpolation_type)
    {
        int pos = 0;
        while (pos < keys.len)
        {
            if (keys[pos].frame == frame)
            {
                keys[pos].transform = transform;
                keys[pos].interpolation_type = interpolation_type;
                return;
            }
            if (keys[pos].frame > frame)
            {
                break;
            }
            pos++;
        }

        keys.append({});
        for (int i = pos; i < keys.len - 1; i++)
        {
            keys[i + 1] = keys[i];
        }
        keys[pos].transform = transform;
        keys[pos].frame = frame;
        keys[pos].interpolation_type = interpolation_type;
    }

    Transform eval(float t)
    {
        float frame = t * fps;

        int base_key_i = 0;
        while (base_key_i < keys.len)
        {
            if (base_key_i == keys.len - 1 || keys[base_key_i + 1].frame >= frame)
            {
                break;
            }
            base_key_i++;
        }

        Key base_key = keys[base_key_i];

        // default to constant interpolation
        Transform transform = base_key.transform;

        if (base_key_i + 1 < keys.len)
        {
            Key target_key = keys[base_key_i + 1];
            float lerp_distance = target_key.frame - base_key.frame;

            if (lerp_distance != 0)
            {
                float lerp_t = (frame - base_key.frame) / lerp_distance;

                if (base_key.interpolation_type == Key::InterpolationType::LINEAR)
                {
                    transform.position = lerp(base_key.transform.position, target_key.transform.position, lerp_t);
                    transform.rotation = lerp(base_key.transform.rotation, target_key.transform.rotation, lerp_t);
                    transform.scale = lerp(base_key.transform.scale, target_key.transform.scale, lerp_t);
                }
                else if (base_key.interpolation_type == Key::InterpolationType::SMOOTHSTEP)
                {
                    float smoothstep_t = clamp(lerp_t * lerp_t * (3.0 - 2.0 * lerp_t), 0.0, 1.0);
                    transform.position = lerp(base_key.transform.position, target_key.transform.position, smoothstep_t);
                    transform.rotation = lerp(base_key.transform.rotation, target_key.transform.rotation, smoothstep_t);
                    transform.scale = lerp(base_key.transform.scale, target_key.transform.scale, smoothstep_t);
                }
                else if (base_key.interpolation_type == Key::InterpolationType::SPLINE)
                {
                    Key key_0 = base_key_i > 0 ? keys[base_key_i - 1] : base_key;
                    Key key_1 = base_key;
                    Key key_2 = target_key;
                    Key key_3 = base_key_i + 2 < keys.len ? keys[base_key_i + 2] : target_key;
                    Spline3 pos_spline =
                        {
                            {
                                key_0.transform.position,
                                key_1.transform.position,
                                key_2.transform.position,
                                key_3.transform.position,
                            }};
                    Spline3 rot_spline =
                        {
                            {
                                key_0.transform.rotation,
                                key_1.transform.rotation,
                                key_2.transform.rotation,
                                key_3.transform.rotation,
                            }};
                    transform.position = catmull_rom(lerp_t, pos_spline);
                    transform.rotation = catmull_rom(lerp_t, rot_spline);
                }
            }
        }

        return transform;
    }
};