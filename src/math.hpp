#pragma once

#include <cmath>

#define PI 3.14159265359

float clamp(float d, float min, float max)
{
  const float t = d < min ? min : d;
  return t > max ? max : t;
}
i32 clamp(i32 d, i32 min, i32 max)
{
  const i32 t = d < min ? min : d;
  return t > max ? max : t;
}

struct Color {
  float r, g, b, a;
};

// http://marcocorvi.altervista.org/games/imgpr/rgb-hsl.htm
Color rgb_to_hsl(Color in)
{
  float h, s, l;

  // Find min and max values of R, B, G, say Xmin and Xmax
  float max = fmaxf(in.r, fmaxf(in.g, in.b));
  float min = fminf(in.r, fminf(in.g, in.b));
  l         = (max + min) / 2;

  // If Xmax and Xmin are equal, S is defined to be 0, and H is undefined but in programs usually
  // written as 0
  if (max == min) {
    s = 0;
    h = 0;
  }

  if (max == min) {
    s = 0.f;
  }
  // If L < 1/2, S=(Xmax - Xmin)/(Xmax + Xmin)
  else if (l < 0.5f) {
    s = (max - min) / (max + min);
  }
  // Else, S=(Xmax - Xmin)/(2 - Xmax - Xmin)
  else {
    s = (max - min) / (2 - max - min);
  }

  if (max == min) {
    h = 0.f;
  }
  // If R=Xmax, H = (G-B)/(Xmax - Xmin)
  else if (in.r == max) {
    h = (in.g - in.b) / (max - min);
  }
  // If G=Xmax, H = 2 + (B-R)/(Xmax - Xmin)
  else if (in.g == max) {
    h = 2 + (in.b - in.r) / (max - min);
  }
  // If B=Xmax, H = 4 + (R-G)/(Xmax - Xmin)
  else if (in.b == max) {
    h = 4 + (in.r - in.g) / (max - min);
  }

  // If H < 0 set H = H + 6.
  if (h < 0) {
    h = h + 6;
  }
  // Notice that H ranges from 0 to 6. RGB space is a cube, and HSL space is a double hexacone,
  // where L is the principal diagonal of the RGB cube. Thus corners of the RGB cube; red, yellow,
  // green, cyan, blue, and magenta, become the vertices of the HSL hexagon. Then the value 0-6 for
  // H tells you which section of the hexgon you are in. H is most commonly given as in degrees, so
  // to convert H = H*60.0 (If H is negative, add 360 to complete the conversion.)
  h = h * 60.f;

  return {h, s, l, in.a};
}

Color hsl_to_rgb(Color in)
{
  // https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
  float h = in.r / 360.f;
  float s = in.g;
  float l = in.b;

  Color out = {l, l, l, in.a};
  if (s != 0) {
    auto hue2rgb = [](float p, float q, float t) {
      if (t < 0) t += 1;
      if (t > 1) t -= 1;
      if (t < 1 / 6.f) return p + (q - p) * 6 * t;
      if (t < 1 / 2.f) return q;
      if (t < 2 / 3.f) return p + (q - p) * (2 / 3.f - t) * 6;
      return p;
    };

    float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
    float p = 2 * l - q;
    out.r   = hue2rgb(p, q, h + 1 / 3.f);
    out.g   = hue2rgb(p, q, h);
    out.b   = hue2rgb(p, q, h - 1 / 3.f);
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

struct Vec2B8 {
  bool x, y;
};

struct Vec2i {
  uint8_t x, y;
};

struct Vec4i {
  uint8_t x, y, z, w;
};

struct Vec2f {
  union {
    struct {
      float x, y;
    };
    float values[2];
  };

  float len() { return sqrt(x * x + y * y); }
  float &operator[](i32 i) { return values[i]; }
};
inline Vec2f operator+(const Vec2f &lhs, const Vec2f &rhs)
{
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}
inline Vec2f operator-(const Vec2f &lhs, const Vec2f &rhs)
{
  return {lhs.x - rhs.x, lhs.y - rhs.y};
}
inline Vec2f operator-(const Vec2f &v)
{
  return {-v.x, -v.y};
}
inline void operator+=(Vec2f &lhs, const Vec2f &rhs) { lhs = lhs + rhs; }
inline void operator-=(Vec2f &lhs, const Vec2f &rhs) { lhs = lhs - rhs; }
inline Vec2f operator*(const Vec2f &lhs, const Vec2f &rhs)
{
  return {lhs.x * rhs.x, lhs.y * rhs.y};
}
inline Vec2f operator*(const Vec2f &lhs, const float &rhs) { return lhs * Vec2f{rhs, rhs}; }
inline Vec2f operator*(const float &lhs, const Vec2f &rhs) { return rhs * lhs; }
inline Vec2f operator/(const Vec2f &lhs, const Vec2f &rhs)
{
  return {lhs.x / rhs.x, lhs.y / rhs.y};
}
inline Vec2f operator/(const Vec2f &lhs, const float &rhs) { return lhs / Vec2f{rhs, rhs}; }
inline Vec2f operator/(const float &lhs, const Vec2f &rhs) { return rhs / lhs; }
inline float dot(const Vec2f &lhs, const Vec2f &rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }
Vec2f min(Vec2f a, Vec2f b) { return {fminf(a.x, b.x), fminf(a.y, b.y)}; }
Vec2f max(Vec2f a, Vec2f b) { return {fmaxf(a.x, b.x), fmaxf(a.y, b.y)}; }

inline Vec2B8 operator<(const Vec2f &lhs, const Vec2f &rhs)
{
  return {lhs.x < rhs.x, lhs.y < rhs.y};
}
inline Vec2B8 operator>(const Vec2f &lhs, const Vec2f &rhs)
{
  return {lhs.x > rhs.x, lhs.y > rhs.y};
}

Vec2f normalize(Vec2f v)
{
  float len = sqrt(v.x * v.x + v.y * v.y);
  return {v.x / len, v.y / len};
}
Vec2f clamp(Vec2f val, Vec2f min, Vec2f max)
{
  return {clamp(val.x, min.x, max.x), clamp(val.y, min.y, max.y)};
}

struct Vec3f {
  union {
    struct {
      float x, y, z;
    };
    float values[3];
  };

  Vec2f xy() { return {x, y}; }
  float len() { return sqrt(x * x + y * y + z * z); }
  float &operator[](i32 i) { return values[i]; }
};
inline Vec3f operator+(const Vec3f &lhs, const Vec3f &rhs)
{
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}
inline Vec3f operator-(const Vec3f &lhs, const Vec3f &rhs)
{
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}
inline Vec3f operator*(const Vec3f &lhs, const Vec3f &rhs)
{
  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}
inline Vec3f operator*(const Vec3f &lhs, const float &rhs) { return lhs * Vec3f{rhs, rhs, rhs}; }
inline Vec3f operator*(const float &lhs, const Vec3f &rhs) { return rhs * lhs; }
inline Vec3f operator/(const Vec3f &lhs, const Vec3f &rhs)
{
  return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}
inline Vec3f operator/(const float &lhs, const Vec3f &rhs)
{
  return {lhs / rhs.x, lhs / rhs.y, lhs / rhs.z};
}
inline Vec3f operator/(const Vec3f &lhs, const float &rhs)
{
  return {lhs.x / rhs, lhs.y / rhs, lhs.z / rhs};
}
inline float dot(const Vec3f &lhs, const Vec3f &rhs)
{
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}
inline Vec3f cross(const Vec3f &lhs, const Vec3f &rhs)
{
  return {
      lhs.y * rhs.z - lhs.z * rhs.y,
      lhs.z * rhs.x - lhs.x * rhs.z,
      lhs.x * rhs.y - lhs.y * rhs.x,
  };
}
Vec3f normalize(Vec3f v)
{
  float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  return {v.x / len, v.y / len, v.z / len};
}
Vec3f min(Vec3f a, Vec3f b) { return {fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z)}; }
Vec3f max(Vec3f a, Vec3f b) { return {fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z)}; }

struct Vec3i {
  i32 x, y, z;

  float len() { return sqrt(x * x + y * y + z * z); }
};
inline Vec3i operator+(const Vec3i &lhs, const Vec3i &rhs)
{
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}
inline Vec3i operator-(const Vec3i &lhs, const Vec3i &rhs)
{
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}
inline Vec3i operator*(const Vec3i &lhs, const Vec3i &rhs)
{
  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}
inline Vec3i operator*(const Vec3i &lhs, const i32 &rhs) { return lhs * Vec3i{rhs, rhs, rhs}; }
inline Vec3i operator*(const i32 &lhs, const Vec3i &rhs) { return rhs * lhs; }
inline Vec3i operator/(const Vec3i &lhs, const Vec3i &rhs)
{
  return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}
inline i32 dot(const Vec3i &lhs, const Vec3i &rhs)
{
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}
inline Vec3i cross(const Vec3i &lhs, const Vec3i &rhs)
{
  return {
      lhs.y * rhs.z - lhs.z * rhs.y,
      lhs.z * rhs.x - lhs.x * rhs.z,
      lhs.x * rhs.y - lhs.y * rhs.x,
  };
}
Vec3f normalize(Vec3i v)
{
  float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  return {v.x / len, v.y / len, v.z / len};
}
Vec3i clamp(Vec3i val, Vec3i min, Vec3i max)
{
  return {clamp(val.x, min.x, max.x), clamp(val.y, min.y, max.y), clamp(val.z, min.z, max.z)};
}

struct Vec4f {
  float x, y, z, w;
};

struct Rect {
  float x = 0, y = 0;
  float width = 0, height = 0;

  Rect() {}
  Rect(float x, float y, float width, float height)
  {
    this->x      = x;
    this->y      = y;
    this->width  = width;
    this->height = height;
  }
  Rect(Vec2f pos, Vec2f size)
  {
    this->x      = pos.x;
    this->y      = pos.y;
    this->width  = size.x;
    this->height = size.y;
  }

  void set_bottom(float down) { height = down - y; }
  void set_right(float right) { width = right - x; }

  Vec2f xy() { return {x, y}; }
  Vec2f span() { return {width, height}; }

  static Rect from_ends(Vec2f a, Vec2f b)
  {
    return Rect(Vec2f{std::min(a.x, b.x), std::min(a.y, b.y)},
                Vec2f{fabs(b.x - a.x), fabs(b.y - a.y)});
  }
};
inline bool operator==(const Rect &lhs, const Rect &rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width && lhs.height == rhs.height;
}
inline bool operator!=(const Rect &lhs, const Rect &rhs) { return !(lhs == rhs); }
bool in_rect(Vec2f point, Rect rect, Rect mask = {})
{
  if (mask.width == 0 || mask.height == 0) {
    mask = rect;
  }

  return point.x > rect.x && point.x < rect.x + rect.width && point.y > rect.y &&
         point.y < rect.y + rect.height && point.x > mask.x && point.x < mask.x + mask.width &&
         point.y > mask.y && point.y < mask.y + mask.height;
}

struct Transform {
  Vec3f position;
  Vec3f rotation;
  Vec3f scale;
};

// TODO I should implement my own
inline Vec3f operator*(const Transform &lhs, const Vec3f &rhs)
{
  glm::vec3 rot(lhs.rotation.x, lhs.rotation.y, lhs.rotation.z);
  glm::vec3 pos(lhs.position.x, lhs.position.y, lhs.position.z);
  glm::vec3 scale(lhs.scale.x, lhs.scale.y, lhs.scale.z);
  glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.f), scale) *
                    glm::toMat4(glm::quat(rot));
  glm::vec4 result = model * glm::vec4(rhs.x, rhs.y, rhs.z, 1);
  return {result.x, result.y, result.z};
}

float lerp(float a, float b, float t) { return (1 - t) * a + t * b; }

Vec2f lerp(Vec2f a, Vec2f b, float t) { return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)}; }
Vec3f lerp(Vec3f a, Vec3f b, float t)
{
  return {lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)};
}

struct AABB {
  Vec3f min = {100000, 100000, 100000};
  Vec3f max = {-100000, -100000, -100000};
};