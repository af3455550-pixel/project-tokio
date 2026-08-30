#include "UI/Screens.h"
#include <cmath>
#include <cstdio>

namespace ink {

void Menu::Draw(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                const std::string& title, int scale) const {
    double w = std::min(420.0, viewW * 0.8);
    double itemH = 30.0 * scale;
    double h = 70.0 + itemH * items_.size();
    double x = (viewW - w) * 0.5;
    double y = (viewH - h) * 0.5;
    // paper card
    r.RectFillScreen({x, y, w, h}, 0xFFF2E9D8, 0.96);
    r.RectOutlineScreen({x, y, w, h}, 0xFF2A2438, 1.0);
    r.RectOutlineScreen({x + 5, y + 5, w - 10, h - 10}, 0xFF2A2438, 1.0);
    // corner notches (hand-cut look)
    r.RectFillScreen({x + 2, y + 2, 8, 3}, 0xFF2A2438, 1.0);
    r.RectFillScreen({x + w - 10, y + h - 5, 8, 3}, 0xFF2A2438, 1.0);
    font.DrawCenter(r, {x, y + 14, w, 24}, title, scale + 1, 0xFF2A2438);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        double iy = y + 52 + i * itemH;
        if (static_cast<int>(i) == cursor_) {
            r.RectFillScreen({x + 10, iy - 2, w - 20, itemH - 4}, 0xFF2A2438, 0.16);
            font.Draw(r, x + 20, iy + 4, ">", 1, 0xFF8A4B2E, 0, false);
            font.Draw(r, x + 34, iy + 4, items_[i], scale, 0xFF2A2438, 0, false);
        } else {
            font.Draw(r, x + 34, iy + 4, items_[i], scale, 0xFF5A4A5C, 0, false);
        }
    }
}

void Screens::Card(Renderer2D& r, const BitmapFont& font, int viewW, int viewH, double w,
                   double h, const std::string& title, int scale) {
    double x = (viewW - w) * 0.5;
    double y = (viewH - h) * 0.5;
    r.RectFillScreen({x, y, w, h}, 0xFFF2E9D8, 0.97);
    r.RectOutlineScreen({x, y, w, h}, 0xFF2A2438, 1.0);
    r.RectOutlineScreen({x + 5, y + 5, w - 10, h - 10}, 0xFF2A2438, 1.0);
    if (!title.empty())
        font.DrawCenter(r, {x, y + 16, w, 24}, title, scale + 1, 0xFF2A2438);
    (void)font;
}

void Screens::DrawTitle(Renderer2D& r, const BitmapFont& font, int viewW, int viewH, double t,
                        const Menu& menu, int slotMode) {
    (void)slotMode;
    // film strip borders
    r.RectFillScreen({0, 0, viewW, 14}, 0xFF17131F, 1.0);
    r.RectFillScreen({0, viewH - 14, viewW, 14}, 0xFF17131F, 1.0);
    for (int i = 0; i < viewW / 34; ++i) {
        double off = std::fmod(t * 40.0, 34.0);
        r.RectFillScreen({i * 34 + off - 34, 3, 16, 8}, 0xFF3A3348, 1.0);
        r.RectFillScreen({i * 34 - off + 34, viewH - 11, 16, 8}, 0xFF3A3348, 1.0);
    }
    // logo with ink splash
    double lx = viewW * 0.5;
    double ly = viewH * 0.24;
    r.RectFillScreen({lx - 210, ly - 34, 420, 92}, 0xFF17131F, 0.85);
    font.DrawCenter(r, {lx - 220, ly - 30, 440, 56}, "INKBOUND", 6, 0xFFFFE08A);
    font.DrawCenter(r, {lx - 220, ly + 26, 440, 20}, "T H E   L A S T   R E E L", 2, 0xFFBFE3FF);
    // tagline
    font.DrawCenter(r, {0, viewH * 0.46, viewW, 16},
                    "A LIVING-INK ACTION PLATFORMER - ORIGINAL IP", 1, 0xFF8A7B6C);
    menu.Draw(r, font, viewW, viewH, "MAIN MENU");
    font.Draw(r, 10, viewH - 30, "V0.1 VERTICAL SLICE", 1, 0xFF6A5B4C, 0, false);
}

void Screens::DrawOptions(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                          const GameProgress& p) {
    Card(r, font, viewW, viewH, 460, 340, "OPTIONS");
    auto val = [](double v) {
        char b[16];
        std::snprintf(b, sizeof(b), "%d%%", static_cast<int>(v * 100.0));
        return std::string(b);
    };
    std::string rows[10];
    rows[0] = "MASTER VOLUME " + val(p.settings.masterVol);
    rows[1] = "MUSIC VOLUME " + val(p.settings.musicVol);
    rows[2] = "SFX VOLUME " + val(p.settings.sfxVol);
    rows[3] = "DIFFICULTY " + p.settings.difficulty;
    rows[4] = "FILM EFFECTS " + val(p.settings.filmFx);
    rows[5] = "REDUCED FLASHING " + std::string(p.settings.reducedFlash ? "ON" : "OFF");
    rows[6] = "REDUCED SHAKE " + std::string(p.settings.reducedShake ? "ON" : "OFF");
    rows[7] = "HIGH CONTRAST " + std::string(p.settings.highContrast ? "ON" : "OFF");
    rows[8] = "VIBRATION ON";
    rows[9] = "REMAP KEYS: SELECT, THEN PRESS A KEY";
    for (int i = 0; i < 10; ++i)
        font.Draw(r, (viewW - 400) * 0.5 + 30, 60 + i * 28, rows[i], 1, 0xFF2A2438, 0, false);
    font.Draw(r, (viewW - 400) * 0.5 + 30, 344, "UP/DOWN SELECT - LEFT/RIGHT CHANGE - ESC BACK", 1,
              0xFF8A7B6C, 0, false);
}

void Screens::DrawSlots(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                        const std::string& title, const Menu& menu, const std::string& slot0,
                        const std::string& slot1, const std::string& slot2) {
    Card(r, font, viewW, viewH, 480, 260, title);
    const char* slots[3] = {slot0.c_str(), slot1.c_str(), slot2.c_str()};
    for (int i = 0; i < 3; ++i) {
        double y = 70 + i * 56;
        double x = (viewW - 400) * 0.5 + 20;
        bool cur = (menu.Cursor() == i);
        if (cur)
            r.RectFillScreen({x - 6, y - 4, 396, 44}, 0xFF2A2438, 0.14);
        r.RectOutlineScreen({x, y, 380, 36}, 0xFF2A2438, 1.0);
        font.Draw(r, x + 12, y + 10, "SLOT " + std::to_string(i + 1), 1, 0xFF2A2438, 0, false);
        font.Draw(r, x + 110, y + 10, slots[i], 1,
                  std::string(slots[i]).empty() ? 0xFF8A7B6C : 0xFF5A4A5C, 0, false);
    }
}

void Screens::DrawRank(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                       const RankScreenData& d) {
    Card(r, font, viewW, viewH, 520, 380, "BOSS DEFEATED");
    const RankResult& res = d.result;
    font.DrawCenter(r, {40, 56, 440, 24}, res.bossName, 2, 0xFF8A4B2E);
    char line[64];
    std::snprintf(line, sizeof(line), "TIME        %02d:%05.2f", static_cast<int>(res.time / 60.0),
                  res.time - static_cast<int>(res.time / 60.0) * 60.0);
    font.Draw(r, 60, 100, line, 1, 0xFF2A2438, 0, false);
    std::snprintf(line, sizeof(line), "DAMAGE      %d", res.damageTaken);
    font.Draw(r, 60, 128, line, 1, 0xFF2A2438, 0, false);
    std::snprintf(line, sizeof(line), "PARRIES     %d", res.parries);
    font.Draw(r, 60, 156, line, 1, 0xFF2A2438, 0, false);
    std::snprintf(line, sizeof(line), "MAX COMBO   x%d", res.maxCombo);
    font.Draw(r, 60, 184, line, 1, 0xFF2A2438, 0, false);
    std::snprintf(line, sizeof(line), "SUPER       %d", res.specials);
    font.Draw(r, 60, 212, line, 1, 0xFF2A2438, 0, false);
    std::snprintf(line, sizeof(line), "SCORE       %lld", res.score);
    font.Draw(r, 60, 240, line, 1, 0xFF2A2438, 0, false);
    if (res.noHit)
        font.Draw(r, 300, 268, "* NO HIT *", 1, 0xFFC8452E, 0, false);
    // rank stamp: scales in
    double t = std::min(1.0, d.t / 0.5);
    double s = 1.0 + (1.0 - t) * 2.0;
    int scale = static_cast<int>(3.5 * s);
    double cx = (viewW - 120) * 0.5;
    double cy = 250;
    r.RectOutlineScreen({cx - 30, cy - 26, 64, 64}, 0xFFC8452E, 2.0);
    font.DrawCenter(r, {cx - 30, cy - 26, 64, 64}, res.rank, scale, 0xFFC8452E);
    font.Draw(r, cx - 30, cy + 40, "RANK", 1, 0xFF8A7B6C, 0, false);
    font.Draw(r, 60, 330, "PRESS ENTER TO CONTINUE", 1, 0xFF8A7B6C, 0, false);
}

void Screens::DrawGameOver(Renderer2D& r, const BitmapFont& font, int viewW, int viewH, double t) {
    r.RectFillScreen({0, 0, viewW, viewH}, 0xFF17131F, std::min(1.0, t / 0.8) * 0.75);
    double shake = t < 0.4 ? 3.0 : 0.0;
    double ox = std::sin(t * 40.0) * shake;
    font.DrawCenter(r, {ox, viewH * 0.34, viewW - ox, 40}, "THE REEL TEARS", 4, 0xFFD24A5C);
    font.DrawCenter(r, {0, viewH * 0.5, viewW, 16}, "BUT THE INK REMEMBERS...", 1, 0xFFBFE3FF);
    font.DrawCenter(r, {0, viewH * 0.62, viewW, 16}, "E - TRY AGAIN      ESC - MAIN MENU", 1,
                    0xFFE8DCC8);
}

void Screens::DrawVictory(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                          const std::string& levelName, double time, int coins, int taken,
                          int total) {
    r.RectFillScreen({0, 0, viewW, viewH}, 0xFF17131F, 0.55);
    Card(r, font, viewW, viewH, 460, 260, "LEVEL COMPLETE");
    font.DrawCenter(r, {40, 60, 380, 20}, levelName, 1, 0xFF5A4A5C);
    char line[64];
    std::snprintf(line, sizeof(line), "TIME   %02d:%05.2f", static_cast<int>(time / 60.0),
                  time - static_cast<int>(time / 60.0) * 60.0);
    font.Draw(r, 60, 104, line, 1, 0xFF2A2438, 0, false);
    std::snprintf(line, sizeof(line), "COINS  %d", coins);
    font.Draw(r, 60, 132, line, 1, 0xFF2A2438, 0, false);
    std::snprintf(line, sizeof(line), "FOUND  %d/%d", taken, total);
    font.Draw(r, 60, 160, line, 1, 0xFF2A2438, 0, false);
    font.Draw(r, 60, 200, "PRESS ENTER FOR THE NEXT REEL", 1, 0xFF8A7B6C, 0, false);
}

void Screens::DrawEnding(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                         const std::string& kind, double t) {
    (void)t;
    r.RectFillScreen({0, 0, viewW, viewH}, 0xFF17131F, 0.85);
    std::string title = kind == "true" ? "THE TRUE ENDING" : (kind == "secret" ? "SECRET ENDING"
                                                                                : "THE REEL ENDURES");
    font.DrawCenter(r, {0, viewH * 0.2, viewW, 30}, title, 3, 0xFFFFE08A);
    const char* body[6] = {
        "THE MASTER FRAMES TURN ONE LAST TIME.",
        "INK FLOWS AGAIN THROUGH WHISPERING MEADOWS,",
        "CARNIVAL, BAY, CITY, KINGDOM AND FRAME.",
        "THE DIRECTOR WATCHES - AND, AT LAST, WINKS.",
        "",
        "LIVING INK NEVER TRULY DRIES.",
    };
    for (int i = 0; i < 6; ++i)
        font.DrawCenter(r, {0, viewH * 0.38 + i * 22, viewW, 16}, body[i], 1, 0xFFBFE3FF);
    font.DrawCenter(r, {0, viewH * 0.85, viewW, 16}, "PRESS ENTER TO RETURN TO THE REEL", 1,
                    0xFFE8DCC8);
}

namespace {
constexpr double kCreditSpeed = 36.0;   // px/second, descending
constexpr double kCreditStartY = 120.0; // where the first line appears
struct CreditLine {
    std::string text;
    int scale;
    uint32_t color;
    double h; // pitch to the next line
};
std::vector<CreditLine> CreditLines(const CreditsData& c) {
    std::vector<CreditLine> L;
    L.push_back({c.title, 4, 0xFFFFE08A, 46.0});
    L.push_back({c.subtitle, 2, 0xFFBFE3FF, 32.0});
    L.push_back({"", 1, 0, 26.0});
    for (const auto& e : c.roll) {
        if (!e.heading.empty())
            L.push_back({e.heading, 2, 0xFF7FD4FF, 60.0});
        else {
            L.push_back({e.role, 1, 0xFF9A8FA8, 16.0});
            L.push_back({e.name, 2, 0xFFF6E7C8, 46.0});
        }
    }
    L.push_back({"", 1, 0, 46.0});
    L.push_back({"OBRIGADO POR JOGAR", 2, 0xFFFFE08A, 28.0});
    L.push_back({"INKBOUND (C) 2026 - A REEL ORIGIN STORY", 1, 0xFF9A8FA8, 16.0});
    return L;
}
} // namespace

void Screens::DrawCredits(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                          const CreditsData& c, double t) {
    r.RectFillScreen({0, 0, viewW, viewH}, 0xFF120E18);
    const auto L = CreditLines(c);
    double before = 0.0;
    for (const auto& l : L) {
        double y = kCreditStartY + t * kCreditSpeed - before;
        before += l.h;
        if (l.text.empty() || y < -20.0 || y > viewH + 20.0)
            continue;
        font.DrawCenter(r, {0, y, viewW, 7 * l.scale}, l.text, l.scale, l.color);
    }
}

double Screens::CreditsRollSeconds(const CreditsData& c, int viewH) {
    double total = 0.0;
    for (const auto& l : CreditLines(c))
        total += l.h;
    return (viewH - kCreditStartY + total) / kCreditSpeed + 1.5;
}

void Screens::DrawPause(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                        const Menu& menu) {
    r.RectFillScreen({0, 0, viewW, viewH}, 0xFF17131F, 0.5);
    menu.Draw(r, font, viewW, viewH, "PAUSED");
}

void Screens::DrawBossIntro(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                            const std::string& bossName) {
    r.RectFillScreen({0, 0, viewW, 70}, 0xFF17131F, 0.9);
    r.RectFillScreen({0, viewH - 70, viewW, 70}, 0xFF17131F, 0.9);
    font.DrawCenter(r, {0, viewH * 0.4, viewW, 20}, "A HARVEST IS COMING...", 1, 0xFFBFE3FF);
    font.DrawCenter(r, {0, viewH * 0.46, viewW, 30}, bossName, 3, 0xFFD24A5C);
}

} // namespace ink
