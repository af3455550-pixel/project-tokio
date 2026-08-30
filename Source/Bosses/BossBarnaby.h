#pragma once
// BARNABY THE HARVEST KING (§24) — the Whispering Meadows boss.
// A giant scarecrow who believes the crop of characters must be "harvested".
//   Phase 1: scythe slams, crow spit, hops.
//   Phase 2: + seed rain, root eruptions.
//   Phase 3: harvest storm — faster everything + the final arena slam.
// Every attack is telegraphed (§26): poses, shadows, cracks, creaks.
#include "Bosses/Boss.h"
namespace ink { struct SimContext; }

namespace ink {

class Barnaby : public Boss {
public:
    std::string ArtName() const override;
    int ArtFrame() const override;
    std::vector<Rect> TellZones() const override;

private:
    void UpdateCombat(double dt, SimContext& ctx) override;
    enum Pat { None, Scythe, Crows, Hop, SeedRain, Roots, StormSlam };
    int current_ = None;
    double patT_ = 0.0;
    double cdT_ = 1.0;
    int lastPat_ = -1;
    Vec2 slamDir_{-1, 0};
    // seeds / roots: {x pos, state time}
    struct Zone {
        double x;
        double t;
        int kind; // 0 shadow, 1 seed falling, 2 root up
    };
    std::vector<Zone> zones_;
    int hopPhase_ = 0;
    double hopVelX_ = 0.0;

    void StartPattern(SimContext& ctx);
    void PickPattern(SimContext& ctx);
    void UpdatePattern(double dt, SimContext& ctx);
    void SpawnCrow(SimContext& ctx, double angleOffset);
    void MeleeArc(SimContext& ctx);
};

} // namespace ink
