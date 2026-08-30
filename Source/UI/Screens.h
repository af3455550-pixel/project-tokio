#pragma once
// Game screens (§51): title, options, save slots, boss ranking (§55),
// game over, level victory, ending, pause. Hand-drawn paper-card look.
#include "Rendering/Renderer2D.h"
#include "Save/SaveSystem.h"
#include "UI/BitmapFont.h"
#include <functional>
#include <string>
#include <vector>

namespace ink {

// End-credits roll (data-driven from Assets/Data/credits.json, §62).
struct CreditEntry {
    std::string heading; // non-empty = section header
    std::string role;
    std::string name;
};
struct CreditsData {
    std::string title = "INKBOUND";
    std::string subtitle = "THE LAST REEL";
    std::vector<CreditEntry> roll;
};

struct RankResult {
    std::string bossName;
    double time = 0.0;
    int damageTaken = 0;
    int parries = 0;
    int maxCombo = 0;
    int specials = 0;
    long long score = 0;
    std::string rank; // S+ S A B C D
    bool noHit = false;
};

class Menu {
public:
    void SetItems(std::vector<std::string> items) {
        items_ = std::move(items);
        cursor_ = 0;
    }
    const std::vector<std::string>& Items() const { return items_; }
    int Cursor() const { return cursor_; }
    void Move(int dir) {
        if (items_.empty())
            return;
        cursor_ = (cursor_ + dir + static_cast<int>(items_.size())) %
                  static_cast<int>(items_.size());
    }
    void Select() {
        if (onSelect_ && cursor_ >= 0 && cursor_ < static_cast<int>(items_.size()))
            onSelect_(cursor_);
    }
    void SetCursor(int i) {
        if (i >= 0 && i < static_cast<int>(items_.size()))
            cursor_ = i;
    }
    void SetOnSelect(std::function<void(int)> f) { onSelect_ = std::move(f); }

    // Paper card rendering.
    void Draw(Renderer2D& r, const BitmapFont& font, int viewW, int viewH, const std::string& title,
              int scale = 1) const;

private:
    std::vector<std::string> items_;
    int cursor_ = 0;
    std::function<void(int)> onSelect_;
};

struct RankScreenData {
    RankResult result;
    double t = 0.0; // for the stamp animation
};

class Screens {
public:
    // Title
    void DrawTitle(Renderer2D& r, const BitmapFont& font, int viewW, int viewH, double timeSec,
                   const Menu& menu, int slotMode);
    // Options
    void DrawOptions(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                     const GameProgress& progress);
    // Slots
    void DrawSlots(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                   const std::string& title, const Menu& menu,
                   const std::string& slot0, const std::string& slot1, const std::string& slot2);
    // Rank
    void DrawRank(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                  const RankScreenData& d);
    // Game over / victory / ending
    void DrawGameOver(Renderer2D& r, const BitmapFont& font, int viewW, int viewH, double t);
    void DrawVictory(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                     const std::string& levelName, double time, int coins, int taken, int total);
    void DrawEnding(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                    const std::string& kind, double t);
    // Scrolling credit roll (descends the screen). t is seconds since the roll
    // began. Returns nothing; use CreditsRollSeconds for total duration.
    void DrawCredits(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                     const CreditsData& c, double t);
    static double CreditsRollSeconds(const CreditsData& c, int viewH);
    void DrawPause(Renderer2D& r, const BitmapFont& font, int viewW, int viewH, const Menu& menu);
    void DrawBossIntro(Renderer2D& r, const BitmapFont& font, int viewW, int viewH,
                       const std::string& bossName);

private:
    void Card(Renderer2D& r, const BitmapFont& font, int viewW, int viewH, double w, double h,
              const std::string& title, int scale = 1);
};

} // namespace ink
