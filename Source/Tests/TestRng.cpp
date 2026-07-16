#include "Core/Rng.h"
#include "Test.h"

using namespace ink;

INK_TEST(rng_determinism) {
    Rng a(1234), b(1234), c(9999);
    for (int i = 0; i < 16; ++i) {
        INK_REQUIRE_EQ(a.next(), b.next());
    }
    Rng d(9999);
    bool anyDiff = false;
    for (int i = 0; i < 16; ++i)
        anyDiff |= (c.next() != d.next());
    INK_REQUIRE(!anyDiff); // same seed -> same stream
}

INK_TEST(rng_ranges) {
    Rng r(7);
    for (int i = 0; i < 2000; ++i) {
        double u = r.unit();
        INK_REQUIRE(u >= 0.0 && u < 1.0);
        int v = r.range(3, 9);
        INK_REQUIRE(v >= 3 && v <= 9);
    }
    INK_REQUIRE_EQ(r.range(5, 5), 5);
    INK_REQUIRE(!r.chance(0.0));
    INK_REQUIRE(r.chance(1.0));
}

INK_TEST(rng_pick) {
    Rng r(42);
    const std::vector<int> v{10, 20, 30};
    bool seen0 = false, seen1 = false, seen2 = false;
    for (int i = 0; i < 300; ++i) {
        const int& x = r.pick(v);
        INK_REQUIRE(x == 10 || x == 20 || x == 30);
        if (x == 10) seen0 = true;
        if (x == 20) seen1 = true;
        if (x == 30) seen2 = true;
    }
    INK_REQUIRE(seen0 && seen1 && seen2);
}
