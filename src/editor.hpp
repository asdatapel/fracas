#pragma once

#include "scene/scene.hpp"

void debug_draw_line(RenderTarget target, Vec2f start, Vec2f end, Color color = {.5, .2, .8, 1}, float thickness = 3)
{
    Vec2f l = {end.x - start.x, end.y - start.y};
    Vec2f cross = normalize({-l.y, l.x});
    cross = {cross.x * thickness, cross.y * thickness};

    Vec2f v1 = {start.x - cross.x, start.y - cross.y},
          v2 = {start.x + cross.x, start.y + cross.y},
          v3 = {end.x + cross.x, end.y + cross.y},
          v4 = {end.x - cross.x, end.y - cross.y};

    debug_draw_immediate(target, v1, v2, v3, v4, color);
}

float catmull_rom(float t, float v0, float v1, float v2, float v3)
{
    float t2 = t * t;
    float t3 = t2 * t;

    return 0.5 * ((2 * v1) +
                  (-v0 + v2) * t +
                  (2 * v0 - 5 * v1 + 4 * v2 - v3) * t2 +
                  (-v0 + 3 * v1 - 3 * v2 + v3) * t3);
}
float catmull_rom_tangent(float t, float v0, float v1, float v2, float v3)
{
    float t2 = t * t;
    float t3 = t2 * t;

    return 0.5 * ((-v0 + v2) +
                  (2 * v0 - 5 * v1 + 4 * v2 - v3) * 2 * t +
                  (-v0 + 3 * v1 - 3 * v2 + v3) * 3 * t2);
}

std::array<Vec2f, 10> control_points = {
    {
        {100, 1070},
        {150, 1070},
        {200, 1070},
        {250, 1070},
        {300, 1070},
        {350, 1070},
        {400, 1070},
        {1000, 1070},
        {1200, 1070},
        {1500, 1070},
    }};
std::array<float, 10> control_point_velocity = {
    {
        0.1,
        0.1,
        0.1,
        0.1,
        0.1,
        0.1,
        0.1,
        0.1,
        0.1,
        0.1,
    }};

Vec2f spline_node(int i, Vec2f pos, InputState *input)
{
    static int selected = -1;

    if (selected == i)
    {
        pos.x += input->mouse_x - input->prev_mouse_x;
        pos.y += input->mouse_y - input->prev_mouse_y;
    }

    Rect rect = {pos.x - 4, pos.y - 4, 8, 8};
    bool hot = in_rect(Vec2f{input->mouse_x, input->mouse_y}, rect);
    if (input->mouse_down_event && hot)
    {
        selected = i;
    }
    else
    {
        if (selected == i && input->mouse_up_event)
        {
            selected = -1;
        }
    }

    if (pos.x < 5)
        pos.x = 5;
    if (pos.x > 1915)
        pos.x = 1915;
    if (pos.y < 5)
        pos.y = 5;
    if (pos.y > 1075)
        pos.y = 1075;

    if (hot)
    {
        control_point_velocity[i] += input->scrollwheel_count / 10;
        if (control_point_velocity[i] <= 0.001f)
        {
            control_point_velocity[i] = 0.001f;
        }
        printf("velocity[%d]: %f\n", i, control_point_velocity[i]);
    }

    return pos;
}

float anim_t = 1;
Vec2f anim_position;

void splines(RenderTarget target, InputState *input, Memory mem)
{

    debug_begin_immediate();

    static float debug_t = 0;
    debug_t += 0.01f;
    debug_draw_line(target, {700, 130}, {1500, 500 + 500 * sinf(debug_t)}, {.5, .2, .8, 1}, 1);
    debug_draw_line(target, {700, 130}, {1500, 500 + 500 * sinf(debug_t + 1)}, {.5, .2, .8, 1}, 2);
    debug_draw_line(target, {700, 130}, {1500, 500 + 500 * sinf(debug_t + 2)}, {.5, .2, .8, 1}, 2.5);
    debug_draw_line(target, {700, 130}, {1500, 500 + 500 * sinf(debug_t + 3)}, {.5, .2, .8, 1}, 3);
    debug_draw_line(target, {700, 130}, {1500, 500 + 500 * sinf(debug_t + 4)}, {.5, .2, .8, 1}, 3.5);

    Vec2f line_start = control_points[1];
    for (int p_i = 1; p_i < control_points.size() - 2; p_i++)
    {
        Vec2f p0 = control_points[p_i - 1],
              p1 = control_points[p_i],
              p2 = control_points[p_i + 1],
              p3 = control_points[p_i + 2];
        for (int i = 1; i <= 100; i++)
        {
            float t = i / 100.f;
            Vec2f line_end = {
                catmull_rom(t, p0.x, p1.x, p2.x, p3.x),
                catmull_rom(t, p0.y, p1.y, p2.y, p3.y)};
            debug_draw_line(target, line_start, line_end, {.4, .8, .1, 1}, 3);
            line_start = line_end;

            // if ((i % 5) == 0)
            // {
            //     Vec2f tangent = normalize({catmull_rom_tangent(t, p0.x, p1.x, p2.x, p3.x),
            //                                catmull_rom_tangent(t, p0.y, p1.y, p2.y, p3.y)});
            //     Vec2f cross = {-tangent.y, tangent.x};
            //     Vec2f cross_p1 = {line_end.x - cross.x * 20, line_end.y - cross.y * 20};
            //     Vec2f cross_p2 = {line_end.x + cross.x * 20, line_end.y + cross.y * 20};

            //     debug_draw_line(target, cross_p1, cross_p2, {.4, .2, .9, 1}, 1.5);
            // }
        }
    }

    float t = 1;
    while (t < control_points.size() - 2)
    {
        int anim_point = (int)t;
        Vec2f p0 = control_points[anim_point - 1],
              p1 = control_points[anim_point],
              p2 = control_points[anim_point + 1],
              p3 = control_points[anim_point + 2];

        Vec2f anim_pos = {
            catmull_rom(t - anim_point, p0.x, p1.x, p2.x, p3.x),
            catmull_rom(t - anim_point, p0.y, p1.y, p2.y, p3.y)};
        Vec2f tangent = {
            catmull_rom_tangent(t - anim_point, p0.x, p1.x, p2.x, p3.x),
            catmull_rom_tangent(t - anim_point, p0.y, p1.y, p2.y, p3.y)};

        float vel_factor = (1 - (t - anim_point)) * control_point_velocity[anim_point] +
                           (t - anim_point) * control_point_velocity[anim_point + 1];
        t += 1 / sqrt(tangent.x * tangent.x + tangent.y * tangent.y) * 100 * vel_factor;

        Vec2f cross = normalize({-tangent.y, tangent.x});
        Vec2f cross_p1 = {anim_pos.x - cross.x * 20, anim_pos.y - cross.y * 20};
        Vec2f cross_p2 = {anim_pos.x + cross.x * 20, anim_pos.y + cross.y * 20};

        debug_draw_line(target, cross_p1, cross_p2, {.4, .2, .9, 1}, 1.5);
    }

    for (int i = 0; i < control_points.size(); i++)
    {
        Vec2f p = control_points[i] = spline_node(i, control_points[i], input);
        draw_rect(target, {p.x - 4, p.y - 4, 8, 8}, {1, .1, .1, 1});
    }

    {
        int anim_point = ((int)anim_t);
        Vec2f p0 = control_points[anim_point - 1],
              p1 = control_points[anim_point],
              p2 = control_points[anim_point + 1],
              p3 = control_points[anim_point + 2];
        Vec2f anim_pos = {
            catmull_rom(anim_t - anim_point, p0.x, p1.x, p2.x, p3.x),
            catmull_rom(anim_t - anim_point, p0.y, p1.y, p2.y, p3.y)};
        Vec2f anim_tangent = {
            catmull_rom_tangent(anim_t - anim_point, p0.x, p1.x, p2.x, p3.x),
            catmull_rom_tangent(anim_t - anim_point, p0.y, p1.y, p2.y, p3.y)};

        float vel_factor = (1 - (anim_t - anim_point)) * control_point_velocity[anim_point] +
                           (anim_t - anim_point) * control_point_velocity[anim_point + 1];
        anim_t += 1 / sqrt(anim_tangent.x * anim_tangent.x + anim_tangent.y * anim_tangent.y) * vel_factor;
        if (anim_t > control_points.size() - 2)
            anim_t = 1;

        draw_rect(target, {anim_pos.x - 4, anim_pos.y - 4, 8, 8}, {1, 1, .1, 1});

        anim_position = anim_pos;
    }

    debug_end_immediate();
}

struct Editor
{
    EditorCamera debug_camera;
    Entity *selected_camera = nullptr;

    void update_and_draw(Scene *scene, RenderTarget backbuffer, InputState *input, Memory mem)
    {
        if (!imm_does_gui_have_focus())
        {
            if (!selected_camera)
                debug_camera.update(backbuffer, input);
        }
        if (selected_camera)
        {
            selected_camera->transform.position.z = anim_position.x / 70;
            selected_camera->transform.position.y = (1080-anim_position.y) / 100 + 7;
        }
        scene->update_and_draw(backbuffer, input, selected_camera ? &selected_camera->camera : &debug_camera);
        debug_ui(scene, backbuffer, input, mem);
        
        splines(backbuffer, input, mem);
    }

    void debug_ui(Scene *scene, RenderTarget target, InputState *input, Memory mem)
    {
        imm_begin(target, input);

        imm_window(String::from("Entities"), {0, 0, 300, 750});
        Entity *selected = nullptr;
        for (int i = 0; i < scene->entities.size; i++)
        {
            if (scene->entities.data[i].assigned)
            {
                Entity &e = scene->entities.data[i].value;
                if (imm_list_item((ImmId)i + 1, e.debug_tag.name))
                {
                    selected = &e;
                }
            }
        }

        if (selected)
        {
            imm_window(String::from("Deets"), {target.width - 300.f, target.height - 400.f, 300, 400});
            imm_textbox(&selected->debug_tag.name);

            imm_label(String::from("Position"));
            imm_num_input(&selected->transform.position.x);
            imm_num_input(&selected->transform.position.y);
            imm_num_input(&selected->transform.position.z);
            imm_label(String::from("Rotation"));
            imm_num_input(&selected->transform.rotation.x);
            imm_num_input(&selected->transform.rotation.y);
            imm_num_input(&selected->transform.rotation.z);
        }

        if (imm_button(String::from("New Camera")))
        {
            Entity new_e;
            new_e.type = EntityType::CAMERA;
            new_e.transform.position.x = (float)(rand() % 10000) / 10000 * 60 - 30;
            new_e.transform.position.z = (float)(rand() % 10000) / 10000 * 60 - 30;
            new_e.transform.position.y = (float)(rand() % 10000) / 10000 * 20;
            new_e.transform.scale.x = 1;
            new_e.transform.scale.z = 1;
            new_e.transform.scale.y = 1;
            scene->entities.push_back(new_e);
        }
        if (imm_button(String::from("Set Camera")))
        {
            selected_camera = selected->type == EntityType::CAMERA ? selected : nullptr;
        }
        if (imm_button(String::from("Reset Camera")))
        {
            selected_camera = nullptr;
        }

        imm_end();
    }
};