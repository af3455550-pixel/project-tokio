#include "Tools/DebugOverlay.h"
#include "Rendering/Renderer2D.h"
#include <cstdio>

namespace ink {

void DebugOverlay::Update(double fps, double frameMs) {
    state_.fps = fps;
    state_.frameMs = frameMs;
}

void DebugOverlay::Draw(Renderer2D& r, BitmapFont& font, const DebugState& s, int viewW,
                        int viewH) {
    (void)viewW;
    char line[96];
    std::snprintf(line, sizeof(line), "FPS %.0f  MS %.1f  EN %d  PR %d  PT %d", s.fps,
                  s.frameMs, s.simEntities, s.projectiles, s.particles);
    font.Draw(r, 10, viewH - 52, line, 1, 0xFF9FFFB0, 0, false);
    std::snprintf(line, sizeof(line), "MIL %s  HP %d  POS %.0f,%.0f", s.playerState.c_str(),
                  s.playerHp, s.playerX, s.playerY);
    font.Draw(r, 10, viewH - 38, line, 1, 0xFF9FFFB0, 0, false);
    std::snprintf(line, sizeof(line), "CAM %.0f,%.0f%s%s%s", s.camX, s.camY,
                  s.godMode ? "  GOD" : "", s.showHitboxes ? "  BOX" : "",
                  s.bossState >= 0 ? "  BOSS" : "");
    font.Draw(r, 10, viewH - 24, line, 1, 0xFF9FFFB0, 0, false);
    if (!s.lastEvent.empty())
        font.Draw(r, 10, viewH - 10, "EVT " + s.lastEvent, 1, 0xFF9FD4FF, 0, false);
}

void DebugOverlay::DrawHitboxes(Renderer2D& r, const DebugState& s) {
    (void)r;
    (void)s;
}

} // namespace ink
