#pragma once
// Charms (§19): small risk/reward modifiers. Each charm changes the player
// stat block multiplicatively/additively — real trade-offs, no pure upgrades
// (§41). Data-driven: adding a charm = JSON entry.
#include "Player/PlayerStats.h"
#include <string>
#include <vector>

namespace ink {

struct CharmMods {
    int maxHpAdd = 0;
    double moveSpeedMul = 1.0;
    double jumpMul = 1.0;
    double damageMul = 1.0;
    double dmgTakenMul = 1.0;
    double dashCooldownMul = 1.0;
    double energyGainMul = 1.0;
    double parryWindowMul = 1.0;
    double invulnMul = 1.0;
    int airDashAdd = 0;
    double coinMul = 1.0;
    bool reviveOnce = false; // Echo Heart: one free revive per level
};

struct CharmDef {
    std::string id;
    std::string name;
    std::string description;
    CharmMods mods;
};

class CharmBook {
public:
    bool LoadJson(const std::string& json, std::string* err = nullptr);
    const CharmDef* Get(const std::string& id) const;
    const std::vector<CharmDef>& All() const { return charms_; }

    // Fold all active charms into a copy of the base stats.
    static PlayerStats Apply(const PlayerStats& base, const std::vector<const CharmDef*>& active);

private:
    std::vector<CharmDef> charms_;
};

} // namespace ink
