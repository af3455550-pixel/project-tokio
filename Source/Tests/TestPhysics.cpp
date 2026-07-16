#include "Physics/Collision.h"
#include "Test.h"

using namespace ink;

namespace {
// 8x8 tile world: solid floor on the bottom row, a wall column at x=3,
// a hazard cell at (1,7), a one-way platform at (5,5).
struct FakeGrid : ISolidQuery {
    int W = 8, H = 8, ts = 16;
    int TileSize() const override { return ts; }
    int WidthCells() const override { return W; }
    int HeightCells() const override { return H; }
    bool IsSolidCell(int cx, int cy) const override {
        if (cy == H - 1 || cx == 3)
            return true;
        return false;
    }
    bool IsOnewayCell(int cx, int cy) const override { return cx == 5 && cy == 5; }
    bool IsHazardCell(int cx, int cy) const override { return cx == 1 && cy == H - 1; }
};
} // namespace

INK_TEST(land_on_floor) {
    FakeGrid q;
    // Drop a 10x10 box from mid-air; it must rest on the floor top (y=7*16=112).
    MoveResult r = MoveEntity({64.0, 40.0}, {0.0, 200.0}, {0.0, 0.0, 10.0, 10.0}, 2.0, q);
    INK_REQUIRE(r.onGround);
    INK_REQUIRE_NEAR(r.pos.y, 112.0 - 10.0, 0.5);
}

INK_TEST(hit_wall) {
    FakeGrid q;
    // Wall column at x=3 occupies px 48..63. Coming from the left, the box's
    // right edge (x=40+10=50) must stop flush at 48.
    MoveResult r = MoveEntity({20.0, 100.0}, {300.0, 0.0}, {0.0, 0.0, 10.0, 10.0}, 0.5, q);
    INK_REQUIRE(r.hitWall);
    INK_REQUIRE_NEAR(r.pos.x + 10.0, 48.0, 0.5);
}

INK_TEST(one_way_platform) {
    FakeGrid q;
    // One-way top surface at y=5*16=80, x=80..95.
    // Landing from above:
    MoveResult fromTop = MoveEntity({82.0, 60.0}, {0.0, 100.0}, {0.0, 0.0, 10.0, 10.0}, 2.0, q);
    INK_REQUIRE(fromTop.onGround);
    INK_REQUIRE_NEAR(fromTop.pos.y, 80.0 - 10.0, 0.5);
    // Passing from below must not catch on the platform's underside:
    MoveResult fromBelow = MoveEntity({82.0, 95.0}, {0.0, -100.0}, {0.0, 0.0, 10.0, 10.0}, 0.1, q);
    INK_REQUIRE(!fromBelow.onCeiling);
    INK_REQUIRE_NEAR(fromBelow.pos.y, 85.0, 0.5);
}

INK_TEST(hazard_and_solid_overlap) {
    FakeGrid q;
    INK_REQUIRE(OverlapsSolid({48.0, 0.0, 10.0, 10.0}, q)); // wall column
    INK_REQUIRE(!OverlapsSolid({64.0, 0.0, 10.0, 10.0}, q));
    INK_REQUIRE(OverlapsHazard({8.0, 112.0, 16.0, 10.0}, q)); // hazard floor cell (x 16..32)
    INK_REQUIRE(!OverlapsHazard({64.0, 112.0, 16.0, 10.0}, q));
}

INK_TEST(raycast_los) {
    FakeGrid q;
    INK_REQUIRE(RaycastLOS({64.0, 10.0}, {64.0, 100.0}, q)); // clear column
    INK_REQUIRE(!RaycastLOS({20.0, 10.0}, {60.0, 10.0}, q)); // crosses the wall
}
