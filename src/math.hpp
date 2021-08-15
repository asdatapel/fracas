#pragma once

#include <cmath>

struct Color
{
    float r, g, b, a;
};

// http://marcocorvi.altervista.org/games/imgpr/rgb-hsl.htm
Color rgb_to_hsl(Color in)
{
    float h, s, l;

    // Find min and max values of R, B, G, say Xmin and Xmax
    float max = fmaxf(in.r, fmaxf(in.g, in.b));
    float min = fminf(in.r, fminf(in.g, in.b));
    l = (max + min) / 2;

    // If Xmax and Xmin are equal, S is defined to be 0, and H is undefined but in programs usually written as 0
    if (max == min)
    {
        s = 0;
        h = 0;
    }

    // If L < 1/2, S=(Xmax - Xmin)/(Xmax + Xmin)
    if (l < 0.5f)
    {
        s = (max - min) / (max + min);
    }
    // Else, S=(Xmax - Xmin)/(2 - Xmax - Xmin)
    else
    {
        s = (max - min) / (2 - max - min);
    }

    // If R=Xmax, H = (G-B)/(Xmax - Xmin)
    if (in.r == max)
    {
        h = (in.g - in.b) / (max - min);
    }
    // If G=Xmax, H = 2 + (B-R)/(Xmax - Xmin)
    else if (in.g == max)
    {
        h = 2 + (in.b - in.r) / (max - min);
    }
    // If B=Xmax, H = 4 + (R-G)/(Xmax - Xmin)
    else if (in.b == max)
    {
        h = 4 + (in.r - in.g) / (max - min);
    }

    // If H < 0 set H = H + 6.
    if (h < 0)
    {
        h = h + 6;
    }
    // Notice that H ranges from 0 to 6. RGB space is a cube, and HSL space is a double hexacone, where L is the principal diagonal of the RGB cube. Thus corners of the RGB cube; red, yellow, green, cyan, blue, and magenta, become the vertices of the HSL hexagon. Then the value 0-6 for H tells you which section of the hexgon you are in. H is most commonly given as in degrees, so to convert H = H*60.0 (If H is negative, add 360 to complete the conversion.)
    h = h * 60.f;

    return {h, s, l, in.a};
}

Color hsl_to_rgb(Color in)
{
    // http://marcocorvi.altervista.org/games/imgpr/rgb-hsl.htm
    // float r, g, b;
    // // If S=0, define R, G, and B all to L
    // if (in.g == 0)
    // {
    //     return {in.b, in.b, in.b, in.a};
    // }

    // float temp1, temp2;
    // // If L < 1/2, temp2=L*(1+S)
    // if (in.b < 0.5f)
    // {
    //     temp2 = in.b * (1 + in.g);
    // }
    // // Else, temp2=L+S - L*S
    // else
    // {
    //     temp2 = (in.b + in.g) - (in.b * in.g);
    // }

    // // Let temp1 = 2 * L - temp2
    // temp1 = 2 * in.b - temp2;

    // // Convert H to the range 0-1
    // in.r = in.r / 360.f;

    // // For each of R, G, B, compute another temporary value, temp3, as follows:
    // // for R, temp3=H+1/3; if temp3 > 1, temp3 = temp3 - 1
    // // for G, temp3=H
    // // for B, temp3=H-1/3; if temp3 < 0, temp3 = temp3 + 1
    // // For each of R, G, B, do the following test:
    // // If temp3 < 1/6, color=temp1+(temp2-temp1)*6*temp3
    // // Else if temp3 < 1/2, color=temp2
    // // Else if temp3 < 2/3, color=temp1+(temp2-temp1)*(2/3 - temp3)*6
    // // Else color=temp1
    // auto do_something = [=](float temp3) {
    //     if (temp3 < 1 / 6.f)
    //         return temp1 + (temp2 - temp1) * 6 * temp3;
    //     if (temp3 < 1 / 2.f)
    //         return temp2;
    //     if (temp3 < 2 / 3.f)
    //         return temp1 + (temp2 - temp1) * (2 / 3 - temp3) * 6;
    //     return temp1;
    // };
    // float r_temp3 = in.r + (1 / 3.f);
    // if (r_temp3 > 1)
    //     r_temp3 = r_temp3 - 1;
    // float g_temp3 = in.r;
    // float b_temp3 = in.r - (1 / 3.f);
    // if (b_temp3 < 0)
    //     b_temp3 = b_temp3 + 1;

    // return {do_something(r_temp3),
    //         do_something(g_temp3),
    //         do_something(b_temp3),
    //         in.a};

    //https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
    float h = in.r / 360.f;
    float s = in.g;
    float l = in.b;

    Color out = {l, l, l, in.a};
    if (s != 0)
    {
        auto hue2rgb = [](float p, float q, float t)
        {
            if (t < 0)
                t += 1;
            if (t > 1)
                t -= 1;
            if (t < 1 / 6.f)
                return p + (q - p) * 6 * t;
            if (t < 1 / 2.f)
                return q;
            if (t < 2 / 3.f)
                return p + (q - p) * (2 / 3.f - t) * 6;
            return p;
        };

        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        out.r = hue2rgb(p, q, h + 1 / 3.f);
        out.g = hue2rgb(p, q, h);
        out.b = hue2rgb(p, q, h - 1 / 3.f);
    }

    return out;
}

Color lighten(Color in, float val)
{
    auto hsl = rgb_to_hsl(in);
    return hsl_to_rgb({hsl.r, hsl.g, hsl.b + val, hsl.a});
}

Color darken(Color in, float val)
{
    auto hsl = rgb_to_hsl(in);
    return hsl_to_rgb({hsl.r, hsl.g, hsl.b - val, hsl.a});
}

struct Vec4i
{
    uint8_t x, y, z, w;
};

struct Vec2f
{
    float x, y;
};

struct Vec3f
{
    float x, y, z;

    Vec2f xy()
    {
        return {x, y};
    }
};

struct Rect
{
    float x, y;
    float width, height;
};

Vec2f normalize(Vec2f v)
{
    float len = sqrt(v.x * v.x + v.y * v.y);
    return {v.x / len, v.y / len};
}

Vec3f normalize(Vec3f v)
{
    float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return {v.x / len, v.y / len, v.z / len};
}

bool in_rect(Vec2f point, Rect rect, Rect mask = {})
{
    if (mask.width == 0 || mask.height == 0)
    {
        mask = rect;
    }

    return point.x > rect.x &&
           point.x < rect.x + rect.width &&
           point.y > rect.y &&
           point.y < rect.y + rect.height &&
           point.x > mask.x &&
           point.x < mask.x + mask.width &&
           point.y > mask.y &&
           point.y < mask.y + mask.height;
}

struct Transform
{
    Vec3f position;
    Vec3f rotation;
    Vec3f scale;
};