#pragma once

#include "math.hpp"

struct Segment
{
    Vec3f p0, p1, p2, p3;
};

void debug_draw_sline(RenderTarget target, Vec2f start, Vec2f end, Color color = {.5, .2, .8, 1}, float thickness = 3)
{
    Vec2f l = {end.x - start.x, end.y - start.y};
    Vec2f cross = normalize(Vec2f{-l.y, l.x});
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
Vec3f catmull_rom(float t, Spline3 s)
{
    Vec3f ret;
    ret.x = catmull_rom(t, s.points[0].x, s.points[1].x, s.points[2].x, s.points[3].x);
    ret.y = catmull_rom(t, s.points[0].y, s.points[1].y, s.points[2].y, s.points[3].y);
    ret.z = catmull_rom(t, s.points[0].z, s.points[1].z, s.points[2].z, s.points[3].z);

    return ret;
}
float catmull_rom_tangent(float t, float v0, float v1, float v2, float v3)
{
    float t2 = t * t;
    float t3 = t2 * t;

    return 0.5 * ((-v0 + v2) +
                  (2 * v0 - 5 * v1 + 4 * v2 - v3) * 2 * t +
                  (-v0 + 3 * v1 - 3 * v2 + v3) * 3 * t2);
}
float arc_length_at_point(float t, Segment s)
{
    float x_prime = catmull_rom_tangent(t, s.p0.x, s.p1.x, s.p2.x, s.p3.x);
    float y_prime = catmull_rom_tangent(t, s.p0.y, s.p1.y, s.p2.y, s.p3.y);
    float z_prime = catmull_rom_tangent(t, s.p0.z, s.p1.z, s.p2.z, s.p3.z);

    float xx = x_prime * x_prime;
    float yy = y_prime * y_prime;
    float zz = z_prime * z_prime;
    return sqrt(xx + yy + zz);
};

Vec3f to_screen_space(Vec3f p, Camera *camera)
{
    glm::vec4 p_3d = camera->perspective * camera->view * glm::vec4(p.x, p.y, p.z, 1.f);
    if (p_3d.w > 0)
    {
        p_3d /= p_3d.w;
        return Vec3f{(p_3d.x + 1) * 1920 * 0.5f, 1080 - ((p_3d.y + 1) * 1080 * .5f), p_3d.z};
    }
    else
    {
        return Vec3f{-1, -1, -1};
    }
};

float simpsons_rule(Segment s, float t, int num_samples = 4)
{
    float n_halves = (2 * num_samples);
    float h = t / n_halves;

    float tangent_at_0 = arc_length_at_point(0, s);
    float tangent_at_halfs = 0;
    float tangent_at_ends = arc_length_at_point(h * n_halves, s);
    for (int i = 1; i < n_halves; i += 2)
    {
        tangent_at_halfs += arc_length_at_point(h * i, s);
    }
    for (int i = 2; i < n_halves; i += 2)
    {
        tangent_at_ends += 2 * arc_length_at_point(h * i, s);
    }

    return (1 / 3.f) * h * (tangent_at_0 + 4 * tangent_at_halfs + tangent_at_ends);
}

float numerical_integration(Segment s, int samples = 100)
{
    float length = 0;
    for (int i = 0; i < samples; i++)
    {
        float t = (float)i / samples;
        length += (1.f / samples) * arc_length_at_point(t, s);
    }

    return length;
}

float constant_distance_t(Spline3 spline, float t)
{
    auto get_segment = [&](int i)
    {
        return Segment{spline.points[i - 1],
                       spline.points[i],
                       spline.points[i + 1],
                       spline.points[i + 2]};
    };

    float total_length = 0;
    for (int i = 1; i < spline.points.len - 2; i++)
    {
        Segment s = get_segment(i);
        total_length += simpsons_rule(s, 1);
    }

    float desired_distance = t * total_length;

    float current_distance = 0;

    int desired_segment = 1;
    Segment first_segment = get_segment(desired_segment);
    float next_distance = simpsons_rule(first_segment, 1);
    while (current_distance + next_distance < desired_distance)
    {
        current_distance += next_distance;
        desired_segment++;

        Segment s = get_segment(desired_segment);
        next_distance = simpsons_rule(s, 1);
    }

    float remaining_distance = desired_distance - current_distance;
    Segment final_segment = get_segment(desired_segment);
    float last_t = 0.f;
    float next_t = 0.5f;
    while (fabs(next_t - last_t) > 0.001f)
    {
        last_t = next_t;

        float numer = simpsons_rule(final_segment, last_t) - remaining_distance;
        float denom = arc_length_at_point(last_t, final_segment);

        next_t = last_t - (numer / denom);
    }

    return desired_segment + next_t;
}
Vec3f catmull_rom_const_distance(float t, Spline3 &spline)
{
    float const_t = constant_distance_t(spline, t) - 1;
    return catmull_rom(const_t, spline);
}

void draw_spline(Spline3 &spline, RenderTarget target, InputState *input, Memory mem, Camera *camera, bool debug)
{
    debug_begin_immediate();

    if (debug)
    {
        Vec2f line_start_screen = to_screen_space(spline.points[1], camera).xy();
        for (int p_i = 1; p_i < spline.points.len - 2; p_i++)
        {
            Vec3f p0 = spline.points[p_i - 1],
                  p1 = spline.points[p_i],
                  p2 = spline.points[p_i + 1],
                  p3 = spline.points[p_i + 2];
            for (int i = 1; i <= 100; i++)
            {
                float t = i / 100.f;
                Vec3f line_end = {
                    catmull_rom(t, p0.x, p1.x, p2.x, p3.x),
                    catmull_rom(t, p0.y, p1.y, p2.y, p3.y),
                    catmull_rom(t, p0.z, p1.z, p2.z, p3.z)};
                Vec2f line_end_screen = to_screen_space(line_end, camera).xy();

                if (line_start_screen.x >= 0 && line_end_screen.x >= 0)
                {
                    debug_draw_sline(target, line_start_screen, line_end_screen, {.4, .8, .1, 1}, 3);
                }
                line_start_screen = line_end_screen;
            }
        }

        float t = 1;
        while (t < spline.points.len - 2)
        {
            int anim_point = (int)t;
            Vec3f p0 = spline.points[anim_point - 1],
                  p1 = spline.points[anim_point],
                  p2 = spline.points[anim_point + 1],
                  p3 = spline.points[anim_point + 2];

            Vec3f anim_pos = {
                catmull_rom(t - anim_point, p0.x, p1.x, p2.x, p3.x),
                catmull_rom(t - anim_point, p0.y, p1.y, p2.y, p3.y),
                catmull_rom(t - anim_point, p0.z, p1.z, p2.z, p3.z)};
            Vec3f tangent = {
                catmull_rom_tangent(t - anim_point, p0.x, p1.x, p2.x, p3.x),
                catmull_rom_tangent(t - anim_point, p0.y, p1.y, p2.y, p3.y),
                catmull_rom_tangent(t - anim_point, p0.z, p1.z, p2.z, p3.z)};

            t += fmax(0.001, 1 / sqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z));

            Vec3f cross = normalize(Vec3f{tangent.z, tangent.z, -tangent.x - tangent.y});
            Vec3f cross_p1 = {anim_pos.x - cross.x / 10.f, anim_pos.y - cross.y / 10.f, anim_pos.z - cross.z / 10.f};
            Vec3f cross_p2 = {anim_pos.x + cross.x / 10.f, anim_pos.y + cross.y / 10.f, anim_pos.z + cross.z / 10.f};

            Vec3f cross_p1_screen = to_screen_space(cross_p1, camera);
            Vec3f cross_p2_screen = to_screen_space(cross_p2, camera);

            if (cross_p1_screen.z > 0 && cross_p2_screen.z > 0)
                debug_draw_sline(target, cross_p1_screen.xy(), cross_p2_screen.xy(), {.4, .2, .9, 1}, 1.5);
        }
    }

    debug_end_immediate();
}