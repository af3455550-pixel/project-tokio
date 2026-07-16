#include "Core/Math.h"
#include "Test.h"

using namespace ink;

INK_TEST(vec2_arithmetic) {
    Vec2 a{1.0, 2.0}, b{3.0, -1.0};
    Vec2 sum = a + b, diff = a - b;
    INK_REQUIRE_EQ(sum.x, 4.0);
    INK_REQUIRE_EQ(sum.y, 1.0);
    INK_REQUIRE_EQ(diff.x, -2.0);
    Vec2 threeFour{3.0, 4.0};
    INK_REQUIRE_NEAR(threeFour.Length(), 5.0, 1e-12);
    Vec2 n = Vec2{1.0, 1.0}.Normalized();
    INK_REQUIRE_NEAR(n.x, 0.7071067811865476, 1e-12); // 1/sqrt(2)
    double zeroLen = Vec2{}.Normalized().Length();
    INK_REQUIRE_EQ(zeroLen, 0.0);
}

INK_TEST(rect_overlap) {
    Rect a{0.0, 0.0, 10.0, 10.0};
    INK_REQUIRE(a.Overlaps({5.0, 5.0, 10.0, 10.0}));
    INK_REQUIRE(!a.Overlaps({10.0, 0.0, 5.0, 5.0}));  // touching edge: no overlap
    INK_REQUIRE(!a.Overlaps({20.0, 20.0, 5.0, 5.0}));
    INK_REQUIRE(a.Contains(5.0, 5.0));
    INK_REQUIRE(!a.Contains(10.0, 5.0)); // right edge excluded
    Rect u = a.Union({9.0, 9.0, 10.0, 10.0});
    INK_REQUIRE_EQ(u.x, 0.0);
    INK_REQUIRE_EQ(u.Right(), 19.0);
    INK_REQUIRE_EQ(u.Bottom(), 19.0);
}

INK_TEST(clamp) {
    INK_REQUIRE_EQ(IClamp(5, 0, 10), 5);
    INK_REQUIRE_EQ(IClamp(-3, 0, 10), 0);
    INK_REQUIRE_EQ(IClamp(11, 0, 10), 10);
}
