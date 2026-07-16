#pragma once
// Hand-drawn HUD (§52): ink hearts, super meter (inkwell), weapon, coins,
// boss bar, combo, toasts, dialogue box. Deliberately small — gameplay is
// the screen.
#include "Rendering/Renderer2D.h"
#include "UI/BitmapFont.h"
#include <string>
#include <vector>

namespace ink {

struct HudState {
    int hp = 3, maxHp = 3;
    int energy = 0;
    bool superReady = false;
    bool superActive = false;
    double superT = 0.0;
    std::string weaponName;
    int coins = 0;
    int combo = 0;
    double hurtFlash = 0.0;
    // boss
    bool bossActive = false;
    std::string bossName;
    double bossHpf = 1.0;
    int bossPhase = 0;
    int bossPhaseCount = 1;
    // objective / level banner
    std::string banner;
    double bannerT = 0.0;
    std::string objective;
};

struct Toast {
    std::string title;
    std::string text;
    double t = 0.0;
    double dur = 3.5;
};

class Hud {
public:
    void AddToast(const std::string& title, const std::string& text);

    void Update(double dt);
    void Draw(Renderer2D& r, const BitmapFont& font, double fontScale, const HudState& s,
              int viewW, int viewH);

    // Dialogue (§38)
    void DrawDialogue(Renderer2D& r, const BitmapFont& font, const std::string& speaker,
                      const std::string& visibleText, bool done, int viewW, int viewH);

private:
    std::vector<Toast> toasts_;
};

} // namespace ink
