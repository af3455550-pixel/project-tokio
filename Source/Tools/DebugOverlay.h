#pragma once
// Developer tooling (§48): on-screen HUD with FPS, entity state, hitboxes,
// toggles for god mode / hitbox display / event trace.
#include "UI/BitmapFont.h"
#include <string>

namespace ink {

class Renderer2D;
struct DebugState {
    double fps = 0.0;
    double frameMs = 0.0;
    int simEntities = 0;
    int projectiles = 0;
    int particles = 0;
    std::string playerState;
    int playerHp = 0;
    int bossState = -1;
    int bossHp = 0;
    double playerX = 0, playerY = 0;
    double camX = 0, camY = 0;
    bool godMode = false;
    bool showHitboxes = false;
    bool showEvents = false;
    std::string lastEvent;
};

class DebugOverlay {
public:
    void Update(double fps, double frameMs);
    void Draw(Renderer2D& r, BitmapFont& font, const DebugState& s,
              int viewW, int viewH);
    void DrawHitboxes(Renderer2D& r, const DebugState& s);
    void SetGodMode(bool on) { state_.godMode = on; }
    void ToggleHitboxes() { state_.showHitboxes = !state_.showHitboxes; }
    void LogEvent(const std::string& e) {
        state_.lastEvent = e;
        state_.showEvents = true;
    }
    const DebugState& State() const { return state_; }
    DebugState& State() { return state_; }

private:
    DebugState state_;
};

} // namespace ink
