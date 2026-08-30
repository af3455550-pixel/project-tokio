#pragma once
// Core math: 2D vectors, rects, small helpers. Pure C++, no dependencies.
#include <cmath>
#include <algorithm>

namespace ink {

struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    Vec2() = default;
    // Templated so braced list-init with integers is not a narrowing
    // conversion (GCC: -Wnarrowing; MSVC: hard error C2397).
    template <typename Tx, typename Ty>
    Vec2(Tx x_, Ty y_) : x(static_cast<double>(x_)), y(static_cast<double>(y_)) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(double s) const { return {x * s, y * s}; }
    Vec2 operator/(double s) const { return {x / s, y / s}; }
    Vec2 operator-() const { return {-x, -y}; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(double s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(double s) { x /= s; y /= s; return *this; }

    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2& o) const { return !(*this == o); }

    double Length() const { return std::sqrt(x * x + y * y); }
    double LengthSq() const { return x * x + y * y; }
    Vec2 Normalized() const {
        double l = Length();
        return l > 1e-9 ? Vec2{x / l, y / l} : Vec2{0.0, 0.0};
    }
};

struct Rect {
    double x = 0.0, y = 0.0, w = 0.0, h = 0.0;

    Rect() = default;
    // See Vec2: avoids narrowing diagnostics for integer arguments.
    template <typename Tx, typename Ty, typename Tw, typename Th>
    Rect(Tx x_, Ty y_, Tw w_, Th h_)
        : x(static_cast<double>(x_)), y(static_cast<double>(y_)),
          w(static_cast<double>(w_)), h(static_cast<double>(h_)) {}

    double Left() const { return x; }
    double Right() const { return x + w; }
    double Top() const { return y; }
    double Bottom() const { return y + h; }
    Vec2 Center() const { return {x + w * 0.5, y + h * 0.5}; }

    bool Overlaps(const Rect& o) const {
        return x < o.Right() && o.x < x + w && y < o.Bottom() && o.y < y + h;
    }
    bool Contains(double px, double py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    bool Contains(const Rect& o) const {
        return o.x >= x && o.x + o.w <= x + w && o.y >= y && o.y + o.h <= y + h;
    }
    Rect Union(const Rect& o) const {
        double l = std::min(x, o.x), t = std::min(y, o.y);
        double r = std::max(Right(), o.Right()), b = std::max(Bottom(), o.Bottom());
        return {l, t, r - l, b - t};
    }
    Rect Translated(double dx, double dy) const { return {x + dx, y + dy, w, h}; }
};

inline Vec2 LerpVec(Vec2 a, Vec2 b, double t) { return a + (b - a) * t; }

inline double Lerp(double a, double b, double t) { return a + (b - a) * t; }

inline double Clamp(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline int IClamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline double SmoothStep(double t) { t = Clamp(t, 0.0, 1.0); return t * t * (3.0 - 2.0 * t); }
inline double Sign(double v) { return v > 0.0 ? 1.0 : (v < 0.0 ? -1.0 : 0.0); }
inline double Dist(Vec2 a, Vec2 b) { Vec2 d = a - b; return d.Length(); }

// Frame-accurate timing helper: convert seconds to a fixed-step frame count.
inline int SecToFrames(double s, double fps = 60.0) { return static_cast<int>(s * fps); }

} // namespace ink
