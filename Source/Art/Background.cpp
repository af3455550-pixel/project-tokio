#include "Art/Background.h"
#include "Rendering/Camera2D.h"
#include "Core/Math.h"

namespace ink {

namespace {
// Deterministic pseudo-random from an integer seed (cheap per-call).
int Hash(int n) {
    n = (n << 13) ^ n;
    return (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
}
} // namespace

void DrawMeadowsBackground(Renderer2D& r, const Camera2D& cam, double t,
                           int viewW, int viewH) {
    const double px = cam.Pos().x; // camera world x drives parallax

    // --- Sky: warm dawn gradient bands ---
    const uint32_t sky[6] = {0xFF9FC7E8, 0xFFB3D2EA, 0xFFC9DDE9,
                             0xFFE2D9C8, 0xFFF2D9A8, 0xFFF7E2AE};
    int band = viewH / 6;
    for (int i = 0; i < 6; ++i)
        r.RectFillScreen({0, i * band, viewW, band + 2}, sky[i]);

    // --- Sun: soft double disc, fixed on screen ---
    const int sunX = viewW - 190, sunY = 110;
    r.RectFillScreen({sunX - 54, sunY - 54, 108, 108}, 0x33FFD98A);
    r.RectFillScreen({sunX - 40, sunY - 40, 80, 80}, 0x55FFD98A);
    r.RectFillScreen({sunX - 28, sunY - 28, 56, 56}, 0xFFF2CE6B);
    r.RectFillScreen({sunX - 20, sunY - 20, 40, 40}, 0xFFF7E2AE);

    // --- Drifting clouds: procedural puffs, parallax 0.15 ---
    const double cloudScroll = px * 0.15 + t * 6.0;
    for (int i = 0; i < 7; ++i) {
        int span = viewW + 260;
        int cx = static_cast<int>(Hash(i * 31 + 7) % span - 130 - cloudScroll);
        cx = ((cx % span) + span) % span - 130;
        int cy = 50 + (Hash(i * 17 + 3) % 120);
        int s = 1 + (i % 2);
        r.RectFillScreen({cx, cy, 64 * s, 18 * s}, 0xFFFBF6EC);
        r.RectFillScreen({cx + 14 * s, cy - 10 * s, 34 * s, 16 * s}, 0xFFFBF6EC);
        r.RectFillScreen({cx + 34 * s, cy, 30 * s, 16 * s}, 0xFFFBF6EC);
    }

    // --- Distant hills: two sine silhouettes, parallax 0.2/0.3 ---
    for (int layer = 0; layer < 2; ++layer) {
        const double f = layer == 0 ? 0.2 : 0.3;
        const uint32_t col = layer == 0 ? 0xFF8FAF9A : 0xFF7A9E7C;
        const double base = viewH * (layer == 0 ? 0.62 : 0.7);
        const double amp = layer == 0 ? 26.0 : 38.0;
        for (int sx = 0; sx < viewW; sx += 4) {
            double wx = px * f + sx;
            double h = amp * (0.6 + 0.4 * std::sin(wx * 0.004 + layer * 2.1));
            r.RectFillScreen({sx, static_cast<int>(base - h), 4,
                              viewH - static_cast<int>(base - h)},
                             col);
        }
    }

    // --- Windmill on the ridge, parallax 0.45 ---
    {
        double mx = viewW * 0.18 - (px * 0.45) * 0.05;
        int tx = static_cast<int>(mx), ty = static_cast<int>(viewH * 0.7 - 120);
        r.RectFillScreen({tx - 10, ty, 20, 120}, 0xFFB58F63);
        r.RectFillScreen({tx - 12, ty - 6, 24, 10}, 0xFF8A6B45);
        // Blades: 4 arms rotating (3px-thick via offset lines).
        for (int i = 0; i < 4; ++i) {
            double a = t * 0.9 + i * 1.5707963;
            int len = 44;
            int ex = static_cast<int>(std::cos(a) * len);
            int ey = static_cast<int>(std::sin(a) * len);
            for (int o = -1; o <= 1; ++o)
                r.LineScreen(Vec2(tx, ty + 4 + o), Vec2(tx + ex, ty + 4 + ey + o),
                             0xFFE8DCC4);
        }
        r.RectFillScreen({tx - 4, ty, 8, 8}, 0xFF3A3153);
    }

    // --- Mid grass band with swaying tufts, parallax 0.7 ---
    const double gbase = viewH * 0.86;
    r.RectFillScreen({0, static_cast<int>(gbase), viewW, viewH - static_cast<int>(gbase)},
                     0xFF5F7F38);
    for (int i = 0; i < 40; ++i) {
        int span = viewW + 40;
        int cx = (Hash(i * 13 + 5) % span) - static_cast<int>(px * 0.7);
        cx = ((cx % span) + span) % span - 20;
        int cy = static_cast<int>(gbase) + (Hash(i * 7 + 1) % 24);
        int sw = static_cast<int>(std::sin(t * 2.0 + i) * 2.0);
        r.LineScreen(Vec2(cx, cy), Vec2(cx + sw, cy - 10 - (i % 8)), 0xFF74923A);
        r.LineScreen(Vec2(cx + 1, cy), Vec2(cx + sw + 1, cy - 10 - (i % 8)), 0xFF74923A);
    }
    // Flowers: a few warm dots
    for (int i = 0; i < 14; ++i) {
        int span = viewW + 40;
        int cx = (Hash(i * 29 + 11) % span) - static_cast<int>(px * 0.7);
        cx = ((cx % span) + span) % span - 20;
        int cy = static_cast<int>(gbase) + 10 + (Hash(i * 11 + 2) % 30);
        uint32_t col = i % 3 == 0 ? 0xFFF2CE6B : (i % 3 == 1 ? 0xFFE8A0A8 : 0xFFF5F2EA);
        r.RectFillScreen({cx, cy, 3, 3}, col);
    }

    // --- Foreground floating seeds / dust, parallax 1.3 (drawn after world) ---
    for (int i = 0; i < 24; ++i) {
        double sp = 12.0 + (i % 5) * 6.0;
        double spanW = viewW + 60.0;
        double x = (Hash(i * 3 + 9) % 3000) + t * sp - px * 1.3;
        x = std::fmod(x, spanW);
        if (x < 0.0)
            x += spanW;
        x -= 30.0;
        double y = (Hash(i * 5 + 13) % 800) + std::sin(t * 1.4 + i) * 18.0
                   - (px * 1.3) * 0.02;
        double spanY = static_cast<double>(viewH);
        y = std::fmod(y, spanY);
        if (y < 0.0)
            y += spanY;
        r.RectFillScreen({static_cast<int>(x), static_cast<int>(y), 2, 2}, 0x88FBF6EC);
    }

    // --- Vignette: dark film-edge corners ---
    r.RectFillScreen({0, 0, viewW, 6}, 0x4417131F);
    r.RectFillScreen({0, viewH - 6, viewW, 6}, 0x4417131F);
    r.RectFillScreen({0, 0, 6, viewH}, 0x4417131F);
    r.RectFillScreen({viewW - 6, 0, 6, viewH}, 0x4417131F);
}

} // namespace ink
