#include "UI/Hud.h"
#include <cmath>

namespace ink {

void Hud::AddToast(const std::string& title, const std::string& text) {
    toasts_.push_back({title, text, 0.0, 3.5});
    if (toasts_.size() > 4)
        toasts_.erase(toasts_.begin());
}

void Hud::Update(double dt) {
    for (auto& t : toasts_)
        t.t += dt;
    while (!toasts_.empty() && toasts_.back().t > toasts_.back().dur)
        toasts_.pop_back();
}

namespace {
// Ink heart, 11 wide x 8 tall (11-bit rows, leftmost = bit 10).
static const uint16_t kHeart[8] = {0x1B0, 0x3B0, 0x7F0, 0x3F0, 0x1F0, 0x0C0, 0x080, 0x000};
static void DrawHeart(Renderer2D& r, const BitmapFont& font, double x, double y, int scale, bool full,
                      bool flash) {
    (void)font;
    uint32_t fill = flash ? 0xFFFFFFFF : 0xFFD24A5C;
    uint32_t empty = 0xFF6A3340;
    for (int row = 0; row < 8; ++row) {
        uint16_t bits = kHeart[row];
        for (int col = 0; col < 11; ++col) {
            if (!(bits & (0x400u >> col)))
                continue;
            r.RectFillScreen({x + col * scale, y + row * scale, scale, scale},
                             full ? fill : empty, 1.0);
        }
    }
}
} // namespace

void Hud::Draw(Renderer2D& r, const BitmapFont& font, double fontScale, const HudState& s,
               int viewW, int viewH) {
    int sc = std::max(1, static_cast<int>(1.0 * fontScale));
    (void)viewH;
    // ---- HP hearts (top-left)
    double x = 14, y = 12;
    for (int i = 0; i < std::max(1, s.maxHp); ++i)
        DrawHeart(r, font, x + i * 26, y, 3, i < s.hp, s.hurtFlash > 0.0);

    // ---- Super meter (below hearts): inkwell bar
    x = 14;
    y = 44;
    r.RectFillScreen({x, y, 70, 12}, 0xFF2A2438, 1.0);
    r.RectOutlineScreen({x, y, 70, 12}, 0xFFE8DCC8, 1.0);
    double frac = s.energy / 100.0;
    if (frac > 0.0)
        r.RectFillScreen({x + 2, y + 2, 66 * frac, 8},
                         s.superActive ? 0xFFFFFFFF : 0xFF7FD4FF, 1.0);
    if (s.superReady && !s.superActive) {
        double pulse = 0.6 + 0.4 * std::sin(s.superT * 8.0);
        r.RectOutlineScreen({x - 2, y - 2, 74, 16}, 0xFFFFE08A, pulse);
        font.Draw(r, x + 76, y + 1, "SUPER READY - L", sc, 0xFFFFE08A, 0xFF201A2E, true);
    }

    // ---- Weapon (below meter)
    font.Draw(r, x, y + 22, s.weaponName, sc, 0xFFE8DCC8, 0xFF201A2E, true);

    // ---- Coins (top-right)
    {
        int w = font.Measure("COINS 0000", sc);
        double cx = viewW - 14 - w;
        r.RectFillScreen({cx - 14, 14, 10, 10}, 0xFFFFE08A, 1.0);
        r.RectFillScreen({cx - 12, 16, 6, 6}, 0xFFC8922E, 1.0);
        font.Draw(r, cx, 14, "COINS " + std::to_string(s.coins), sc, 0xFFFFE08A, 0xFF201A2E, true);
    }

    // ---- Objective (top-right under coins)
    if (!s.objective.empty()) {
        int w = font.Measure(s.objective, sc);
        font.Draw(r, viewW - 14 - w, 40, s.objective, sc, 0xFFBFE3FF, 0xFF201A2E, true);
    }

    // ---- Banner (center, fading)
    if (!s.banner.empty() && s.bannerT > 0.0) {
        double a = std::min(1.0, s.bannerT / 0.6);
        font.DrawCenter(r, {0, 60, viewW, 40}, s.banner, std::max(1, sc * 2), 0xFFFFFFFF);
        (void)a;
    }

    // ---- Combo
    if (s.combo >= 3) {
        font.Draw(r, 14, 84, "COMBO x" + std::to_string(s.combo), sc + 1, 0xFFFFE08A,
                  0xFF201A2E, true);
    }

    // ---- Boss bar (bottom center)
    if (s.bossActive) {
        int bw = std::min(480, viewW - 80);
        double bx = (viewW - bw) * 0.5;
        double by = viewH - 34;
        font.DrawCenter(r, {bx, by - 16, bw, 12}, s.bossName, sc, 0xFFFFFFFF);
        r.RectFillScreen({bx, by, bw, 10}, 0xFF2A2438, 1.0);
        r.RectOutlineScreen({bx, by, bw, 10}, 0xFFE8DCC8, 1.0);
        if (s.bossHpf > 0.0)
            r.RectFillScreen({bx + 2, by + 2, (bw - 4) * s.bossHpf, 6}, 0xFFC8452E, 1.0);
        // phase pips
        for (int i = 0; i < s.bossPhaseCount; ++i) {
            double px = bx + bw + 10 + i * 10;
            r.RectFillScreen({px, by + 1, 6, 6}, i <= s.bossPhase ? 0xFFFFE08A : 0xFF5A4A5C, 1.0);
        }
    }

    // ---- Toasts (top-right stack)
    double ty = 64;
    for (const auto& t : toasts_) {
        double in = std::min(1.0, t.t / 0.25);
        double out = std::min(1.0, (t.dur - t.t) / 0.5);
        double a = std::min(in, out);
        double tw = 300.0;
        double tx = viewW - tw - 14 + (1.0 - in) * 40.0;
        r.RectFillScreen({tx, ty, tw, 34}, 0xFFF2E9D8, 0.92 * a);
        r.RectOutlineScreen({tx, ty, tw, 34}, 0xFF2A2438, 1.0);
        font.Draw(r, tx + 10, ty + 5, t.title, sc, 0xFF8A4B2E, 0, false);
        font.Draw(r, tx + 10, ty + 19, t.text, sc, 0xFF2A2438, 0, false);
        ty += 40;
    }
}

void Hud::DrawDialogue(Renderer2D& r, const BitmapFont& font, const std::string& speaker,
                       const std::string& visibleText, bool done, int viewW, int viewH) {
    double bw = std::min(680.0, viewW - 40.0);
    double bx = (viewW - bw) * 0.5;
    double by = viewH - 118;
    r.RectFillScreen({bx, by, bw, 96}, 0xFFF2E9D8, 0.95);
    r.RectOutlineScreen({bx, by, bw, 96}, 0xFF2A2438, 1.0);
    r.RectOutlineScreen({bx + 4, by + 4, bw - 8, 88}, 0xFF2A2438, 1.0);
    // speaker tab
    int sw = font.Measure(speaker, 1);
    r.RectFillScreen({bx + 14, by - 12, sw + 16, 20}, 0xFF2A2438, 1.0);
    font.Draw(r, bx + 22, by - 8, speaker, 1, 0xFFFFE08A, 0, false);
    // text
    font.Draw(r, bx + 18, by + 20, visibleText, 1, 0xFF2A2438, 0, false);
    if (done) {
        double blink = 0.5 + 0.5 * std::sin(by * 0.1);
        font.Draw(r, bx + bw - 30, by + 74, "OK", 1, 0xFF8A4B2E, 0, false);
        (void)blink;
    }
}

} // namespace ink
