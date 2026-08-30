#pragma once
// Reusable boss framework (§22): intro, phases, telegraphs, arena, defeat.
// Parameters (HP, phase thresholds, music layers) are data-driven (bosses.json);
// attack patterns are virtual (per-boss code). 18+ boss roster target.
#include "Animation/AnimRef.h"
#include "Gameplay/Entity.h"
namespace ink { struct SimContext; }
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace ink {

struct BossPhaseDef {
    double hpFrac = 0.66; // phase starts when HP falls to/below this fraction
    int musicLayer = 1;
    std::string label;
};

struct BossDef {
    std::string id;
    std::string name;
    std::string sprite;
    std::string musicId;
    int maxHp = 100;
    double w = 48.0, h = 60.0;
    int score = 1000;
    int energyOnKill = 40;
    double introTime = 2.6;
    double defeatTime = 2.4;
    std::vector<BossPhaseDef> phases;
    bool miniBoss = false;
};

class Boss : public Actor {
public:
    const BossDef* def = nullptr;
    AnimRef anim;
    int phase = 0;
    bool inIntro = false;
    bool inDefeat = false;
    double introT = 0.0;
    double defeatT = 0.0;
    double transitionT = 0.0;
    double damageDealtToPlayer = 0.0;
    double fightTime = 0.0; // starts when intro ends

    virtual ~Boss() = default;

    static std::unique_ptr<Boss> Create(const std::string& id);
    void Init(const BossDef& d, Vec2 pos, int id);
    void StartIntro(SimContext& ctx);

    void Update(double dt, SimContext& ctx);
    void TakeDamage(double dmg, Vec2 knock, SimContext& ctx);

    bool Vulnerable() const { return !inIntro && !inDefeat && transitionT <= 0.0; }
    double Hpf() const { return def ? static_cast<double>(hp) / static_cast<double>(def->maxHp) : 0.0; }
    const char* PhaseLabel() const;
    int PhaseCount() const { return def ? std::max(1, static_cast<int>(def->phases.size())) : 1; }

    // Telegraph zones the app draws as warnings (§26).
    virtual std::vector<Rect> TellZones() const { return {}; }

    // Art
    virtual std::string ArtName() const = 0;
    virtual int ArtFrame() const { return anim.frame; }

protected:
    virtual void UpdateCombat(double dt, SimContext& ctx) = 0;
    void PhaseCheck(SimContext& ctx);
    void PhaseUp(SimContext& ctx);
    void StartDefeat(SimContext& ctx);

    Vec2 spawnPos_;
    Vec2 introFrom_;
    int nextShotId_ = 500000;
};

class BossBook {
public:
    bool LoadJson(const std::string& json, std::string* err = nullptr);
    const BossDef* Get(const std::string& id) const;
    const std::vector<BossDef>& All() const { return defs_; }

private:
    std::vector<BossDef> defs_;
};

} // namespace ink
