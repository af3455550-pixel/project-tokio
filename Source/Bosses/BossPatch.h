#pragma once
// PATCH — the mini-boss scarecrow of the Whispering Meadows (§37).
// A half-finished scarecrow that "practices" Barnaby's scythe and tosses
// needle-pins. Its job is to TEACH the parry: every needle is parryable and
// parrying its swing stuns it long to punish.
#include "Bosses/Boss.h"
namespace ink { struct SimContext; }

namespace ink {

class Patch : public Boss {
public:
    std::string ArtName() const override;
    int ArtFrame() const override;
    std::vector<Rect> TellZones() const override;

private:
    void UpdateCombat(double dt, SimContext& ctx) override;
    enum Pat { None, Needle, Swing };
    int current_ = None;
    double patT_ = 0.0;
    double cdT_ = 0.8;

    void UpdatePattern(double dt, SimContext& ctx);
    void PickPattern(SimContext& ctx);
};

} // namespace ink
