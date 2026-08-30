#pragma once
// Enemy framework: data-driven stats (EnemyDef from JSON) + AIBrain state
// machine (§21) + per-type behavior (virtuals). 50+ enemy roster target;
// the slice ships four archetypes: melee / flying / ranged / swarm.
#include "AI/AIBrain.h"
#include "Animation/AnimRef.h"
#include "Gameplay/Entity.h"
#include "Weapons/WeaponDef.h"
namespace ink { struct SimContext; }
#include <memory>
#include <string>

namespace ink {

struct EnemyDef {
    std::string id;
    std::string name;
    std::string sprite; // atlas base name
    int hp = 3;
    double w = 16.0, h = 16.0;
    double speed = 70.0;
    double attackRange = 60.0;
    int attackDamage = 1;
    double detectRange = 240.0;
    double reactTime = 0.25;
    double attackCooldown = 1.6;
    double recoverTime = 0.7;
    double stunDuration = 1.0;
    bool flying = false;
    bool ranged = false;
    bool swarm = false;
    bool canRetreat = false;
    int score = 50;
    int energyOnKill = 8;
    double coinChance = 0.35;

    BrainConfig Brain() const {
        BrainConfig c;
        c.detectRange = detectRange;
        c.attackRange = attackRange;
        c.reactTime = reactTime;
        c.attackCooldown = attackCooldown;
        c.recoverTime = recoverTime;
        c.canRetreat = canRetreat;
        return c;
    }
};

class Enemy : public Actor {
public:
    const EnemyDef* def = nullptr;
    AIBrain brain;
    AnimRef anim;
    double attackCdT = 0.0;
    bool attackActive = false; // dangerous part of the attack (parryable)
    double attackT = 0.0;
    double deadT = 0.0;
    Vec2 homePos{0, 0};
    double bobT = 0.0;

    virtual ~Enemy() = default;

    static std::unique_ptr<Enemy> Create(const std::string& type);
    void Init(const EnemyDef& d, Vec2 pos, int id);

    void Update(double dt, SimContext& ctx);
    void TakeDamage(double dmg, Vec2 knock, SimContext& ctx);

    const char* StateName() const { return BrainStateName(brain.Get()); }
    bool IsDead() const { return !alive; }
    void OnKill(SimContext& ctx);

    // Art
    virtual std::string ArtName() const = 0;
    virtual int ArtFrame() const { return anim.frame; }

    ProjectileDef MakeEnemyShot() const; // shared "quill/crow" shot shape

protected:
    virtual void OnStateEnter(BrainState s, SimContext& ctx) {}
    virtual void UpdateAI(double dt, SimContext& ctx) = 0;
    void ContactDamage(SimContext& ctx);
    void FinishAttack(SimContext& ctx);
    Vec2 PlayerDir(SimContext& ctx) const;
    double PlayerDist(SimContext& ctx) const;
    bool PlayerLOS(SimContext& ctx) const;

    double patrolDir_ = 1.0;
    double patrolWait_ = 0.0;
    bool lungeStarted_ = false;
};

} // namespace ink
