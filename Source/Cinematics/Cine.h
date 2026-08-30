#pragma once
// Short, punchy cinematic runner (§59): timed cards, letterboxing, shakes.
// The sim owns timing; the app renders cards/letterbox.
#include <string>
#include <vector>

namespace ink {

struct CineCard {
    double at = 0.0;
    double dur = 1.5;
    std::string text;
    bool letterbox = true;
};

struct CineDef {
    std::string id;
    std::vector<CineCard> cards;
    double total = 0.0;
};

class Cine {
public:
    void Start(const CineDef& def) {
        def_ = &def;
        t_ = 0.0;
        active_ = true;
        skipQueued_ = false;
    }
    void Skip() { skipQueued_ = true; }
    bool Active() const { return active_; }
    bool CanSkip() const { return active_ && !skipQueued_; }

    void Update(double dt) {
        if (!active_)
            return;
        t_ += dt;
        if (skipQueued_ || t_ >= def_->total)
            active_ = false;
    }
    double T() const { return t_; }
    double Letterbox() const {
        if (!active_)
            return 0.0;
        for (const auto& c : def_->cards)
            if (c.letterbox && t_ >= c.at && t_ < c.at + c.dur)
                return 1.0;
        return 0.0;
    }
    const std::string* Card() const {
        if (!active_)
            return nullptr;
        for (const auto& c : def_->cards)
            if (!c.text.empty() && t_ >= c.at && t_ < c.at + c.dur)
                return &c.text;
        return nullptr;
    }

    // Convenience builders.
    static CineDef TextCards(const std::string& id, std::vector<std::string> texts,
                             double durPerCard = 1.8);

private:
    const CineDef* def_ = nullptr;
    double t_ = 0.0;
    bool active_ = false;
    bool skipQueued_ = false;
};

} // namespace ink
