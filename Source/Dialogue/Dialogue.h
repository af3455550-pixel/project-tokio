#pragma once
// NPC dialogue (§38, §60): data-driven lines + a typewriter playback player.
// Pure logic; the app draws the box (UI).
#include <algorithm>
#include <utility>
#include <string>
#include <vector>

namespace ink {

struct DialogueLine {
    std::string speaker;
    std::string text;
};

class DialogueBook {
public:
    bool LoadJson(const std::string& json, std::string* err = nullptr);
    const std::vector<DialogueLine>* Lines(const std::string& npcId) const;

private:
    std::vector<std::pair<std::string, std::vector<DialogueLine>>> entries_;
};

class DialoguePlayer {
public:
    static constexpr double kCharsPerSec = 30.0;

    void Start(const std::vector<DialogueLine>& lines) {
        lines_ = lines;
        index_ = 0;
        revealT_ = 0.0;
        done_ = lines_.empty();
    }
    void Update(double dt) {
        if (done_)
            return;
        revealT_ += dt * kCharsPerSec;
    }
    void Advance() {
        if (done_)
            return;
        // first press finishes the reveal, second press moves on
        if (revealT_ < Current().text.size()) {
            revealT_ = Current().text.size() + 1.0;
            return;
        }
        ++index_;
        revealT_ = 0.0;
        if (index_ >= static_cast<int>(lines_.size()))
            done_ = true;
    }
    void SkipAll() {
        index_ = static_cast<int>(lines_.size());
        done_ = true;
    }
    bool Done() const { return done_; }
    int Index() const { return index_; }
    int Count() const { return static_cast<int>(lines_.size()); }
    const DialogueLine& Current() const { return lines_[index_]; }
    double RevealProgress() const {
        const DialogueLine& l = Current();
        if (l.text.empty())
            return 1.0;
        return std::min(1.0, revealT_ / static_cast<double>(l.text.size()));
    }
    std::string VisibleText() const {
        const DialogueLine& l = Current();
        int n = static_cast<int>(std::min(revealT_, static_cast<double>(l.text.size())));
        return l.text.substr(0, n);
    }

private:
    std::vector<DialogueLine> lines_;
    int index_ = 0;
    double revealT_ = 0.0;
    bool done_ = true;
};

} // namespace ink
