#pragma once
// Tile-based AABB collision. Movement is swept axis-by-axis against a
// solid-query interface (the Level implements it; tests use fakes).
// All gameplay-critical movement goes through here, so it is unit-testable
// without SDL (§72).
#include "Core/Math.h"

namespace ink {

class ISolidQuery {
public:
    virtual ~ISolidQuery() = default;
    virtual int TileSize() const = 0;
    virtual int WidthCells() const = 0;
    virtual int HeightCells() const = 0;
    // Tile classification. Out-of-bounds is treated as empty except the
    // top (sky) which is also empty; sides/bottom are solid for boss arenas
    // via explicit tiles.
    virtual bool IsSolidCell(int cx, int cy) const = 0;
    virtual bool IsOnewayCell(int cx, int cy) const = 0;
    virtual bool IsHazardCell(int cx, int cy) const = 0;
};

struct MoveResult {
    Vec2 pos;          // resolved position (top-left)
    Vec2 vel;          // post-collision velocity
    bool onGround = false;
    bool onCeiling = false;
    bool hitWall = false;
    double groundTileTop = 0.0; // world y of the surface we landed on (for carry)
};

// Move an AABB by vel*dt against the query. allowDrop ignores one-way
// platforms this step (drop-through).
MoveResult MoveEntity(Vec2 pos, Vec2 vel, const Rect& size, double dt, const ISolidQuery& q,
                      bool allowDrop = false);

// True if the rect overlaps any solid tile.
bool OverlapsSolid(const Rect& box, const ISolidQuery& q);

// True if the rect overlaps any hazard tile.
bool OverlapsHazard(const Rect& box, const ISolidQuery& q);

// Cheap line-of-sight: samples the segment; solid tile blocks.
bool RaycastLOS(const Vec2& a, const Vec2& b, const ISolidQuery& q);

} // namespace ink
