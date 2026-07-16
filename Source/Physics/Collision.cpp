#include "Physics/Collision.h"
#include <cmath>

namespace ink {

namespace {
constexpr double kEps = 0.001;

bool CellIn(const ISolidQuery& q, int cx, int cy) {
    if (cy < 0) return false; // sky is open
    if (cx < 0 || cx >= q.WidthCells() || cy >= q.HeightCells()) return true; // walls/floor
    return q.IsSolidCell(cx, cy);
}
} // namespace

MoveResult MoveEntity(Vec2 pos, Vec2 vel, const Rect& size, double dt, const ISolidQuery& q,
                      bool allowDrop) {
    MoveResult r;
    const double ts = q.TileSize();
    r.pos = pos;
    r.vel = vel;

    // ---- X axis ---------------------------------------------------------
    double nx = pos.x + vel.x * dt;
    if (vel.x > 0.0) {
        int c0 = static_cast<int>(std::floor(pos.x / ts)) + 1;
        int c1 = static_cast<int>(std::floor((nx + size.w - kEps) / ts));
        int r0 = static_cast<int>(std::floor(pos.y / ts));
        int r1 = static_cast<int>(std::floor((pos.y + size.h - kEps) / ts));
        for (int c = c0; c <= c1 && !r.hitWall; ++c) {
            for (int rr = r0; rr <= r1; ++rr) {
                if (CellIn(q, c, rr)) {
                    nx = c * ts - size.w - kEps;
                    r.hitWall = true;
                    r.vel.x = 0.0;
                    break;
                }
            }
        }
    } else if (vel.x < 0.0) {
        int c0 = static_cast<int>(std::floor((nx + kEps) / ts));
        int c1 = static_cast<int>(std::floor((pos.x + size.w) / ts)) - 1;
        int r0 = static_cast<int>(std::floor(pos.y / ts));
        int r1 = static_cast<int>(std::floor((pos.y + size.h - kEps) / ts));
        for (int c = c0; c >= c1 && !r.hitWall; --c) {
            for (int rr = r0; rr <= r1; ++rr) {
                if (CellIn(q, c, rr)) {
                    nx = (c + 1) * ts + kEps;
                    r.hitWall = true;
                    r.vel.x = 0.0;
                    break;
                }
            }
        }
    }
    r.pos.x = nx;

    // ---- Y axis ---------------------------------------------------------
    double ny = r.pos.y + vel.y * dt;
    const double oldBottom = pos.y + size.h;
    if (vel.y > 0.0) {
        int r0 = static_cast<int>(std::floor(pos.y / ts)) + 1;
        int r1 = static_cast<int>(std::floor((ny + size.h - kEps) / ts));
        int c0 = static_cast<int>(std::floor(r.pos.x / ts));
        int c1 = static_cast<int>(std::floor((r.pos.x + size.w - kEps) / ts));
        for (int rr = r0; rr <= r1 && !r.onGround; ++rr) {
            for (int c = c0; c <= c1; ++c) {
                if (CellIn(q, c, rr)) {
                    ny = rr * ts - size.h - kEps;
                    r.onGround = true;
                    r.groundTileTop = rr * ts;
                    r.vel.y = 0.0;
                    break;
                }
                if (!allowDrop && q.IsOnewayCell(c, rr) && oldBottom <= rr * ts + 1.0) {
                    ny = rr * ts - size.h - kEps;
                    r.onGround = true;
                    r.groundTileTop = rr * ts;
                    r.vel.y = 0.0;
                    break;
                }
            }
        }
    } else if (vel.y < 0.0) {
        int r0 = static_cast<int>(std::floor((ny + kEps) / ts));
        int r1 = static_cast<int>(std::floor((pos.y + size.h) / ts)) - 1;
        int c0 = static_cast<int>(std::floor(r.pos.x / ts));
        int c1 = static_cast<int>(std::floor((r.pos.x + size.w - kEps) / ts));
        for (int rr = r0; rr >= r1 && !r.onCeiling; --rr) {
            for (int c = c0; c <= c1; ++c) {
                if (CellIn(q, c, rr)) {
                    ny = (rr + 1) * ts + kEps;
                    r.onCeiling = true;
                    r.vel.y = 0.0;
                    break;
                }
            }
        }
    }
    r.pos.y = ny;
    return r;
}

bool OverlapsSolid(const Rect& box, const ISolidQuery& q) {
    const double ts = q.TileSize();
    int c0 = static_cast<int>(std::floor(box.Left() / ts));
    int c1 = static_cast<int>(std::floor((box.Right() - kEps) / ts));
    int r0 = static_cast<int>(std::floor(box.Top() / ts));
    int r1 = static_cast<int>(std::floor((box.Bottom() - kEps) / ts));
    for (int c = c0; c <= c1; ++c)
        for (int r = r0; r <= r1; ++r)
            if (CellIn(q, c, r))
                return true;
    return false;
}

bool OverlapsHazard(const Rect& box, const ISolidQuery& q) {
    const double ts = q.TileSize();
    int c0 = static_cast<int>(std::floor(box.Left() / ts));
    int c1 = static_cast<int>(std::floor((box.Right() - kEps) / ts));
    int r0 = static_cast<int>(std::floor(box.Top() / ts));
    int r1 = static_cast<int>(std::floor((box.Bottom() - kEps) / ts));
    for (int c = c0; c <= c1; ++c)
        for (int r = r0; r <= r1; ++r)
            if (c >= 0 && c < q.WidthCells() && r >= 0 && r < q.HeightCells() && q.IsHazardCell(c, r))
                return true;
    return false;
}

bool RaycastLOS(const Vec2& a, const Vec2& b, const ISolidQuery& q) {
    const double d = Dist(a, b);
    if (d < 1e-6)
        return true;
    const int steps = static_cast<int>(d / 8.0) + 1;
    for (int i = 1; i < steps; ++i) {
        double t = static_cast<double>(i) / steps;
        Vec2 p = a + (b - a) * t;
        int c = static_cast<int>(std::floor(p.x / q.TileSize()));
        int r = static_cast<int>(std::floor(p.y / q.TileSize()));
        if (c >= 0 && c < q.WidthCells() && r >= 0 && r < q.HeightCells() && q.IsSolidCell(c, r))
            return false;
    }
    return true;
}

} // namespace ink
