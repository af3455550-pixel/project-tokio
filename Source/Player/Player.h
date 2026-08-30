#pragma once
// MILO INKWHISTLE (§11). The player controller: movement with coyote time,
// jump buffer, input buffer, variable jump, ground/air dash, dash cancel,
// crouch, drop-through, parry window, charged shots, super meter (§12-14, §17,
// §18). Pure simulation: all feedback (SFX/VFX/shake) is emitted as events or
// particles; the app renders it.
#include "Gameplay/Entity.h"
#include "Player/PlayerStats.h"
#include "Weapons/WeaponDef.h"
namespace ink { struct SimContext; }

namespace ink {

struct PlayerInput {
    float moveX = 0.0f; // -1..1
    bool jumpHeld = false;
    bool jumpPressed = false;
    bool crouch = false;
    bool crouchPressed = false;
    bool dashPressed = false;
    bool attackHeld = false;
    bool attackPressed = false;
    bool parryPressed = false;
    bool specialPressed = false;
    bool dropThrough = false;
};

enum class PlayerState {
    Idle, Run, Jump, Fall, Crouch, Dash, AirDash, Attack, Parry, Hurt, Dead, Super
};

const char* PlayerStateName(PlayerState s);

struct Projectile;

class Player : public Actor {
public:
    static constexpr int kStandH = 22;
    static constexpr int kCrouchH = 13;

    void Init(int id, const PlayerStats& stats);
    const PlayerStats& Stats() const { return stats_; }
    // Charm-folded stats; keeps current HP (clamped to the new max).
    void SetStats(const PlayerStats& s);
    void SetWeapon(const WeaponDef* w) { weapon_ = w; }
    const WeaponDef* Weapon() const { return weapon_; }

    void Update(double dt, SimContext& ctx, const PlayerInput& in);

    // Combat.
    // Returns true when the projectile was parried (and consumed).
    bool TryParryHit(Projectile& p, SimContext& ctx);
    // Returns true when a melee touch was parried.
    bool TryParryMelee(SimContext& ctx);
    bool IsParryActive() const;
    void Damage(int dmg, Vec2 fromPos, SimContext& ctx, bool hazard);
    void Kill(SimContext& ctx);
    void Respawn(const Vec2& pos, SimContext& ctx);
    void SetReviveAvailable(bool b) {
        reviveAvailable_ = b;
        reviveUsed_ = false;
    }

    void AddEnergy(int n);
    int Energy() const { return energy_; }
    bool SuperReady() const { return energy_ >= 100 && !superActive_; }
    bool SuperActive() const { return superActive_; }

    int Combo() const { return combo_; }
    void AddCombo() {
        ++combo_;
        comboT_ = 2.0;
        if (combo_ > maxCombo_)
            maxCombo_ = combo_;
    }
    int MaxCombo() const { return maxCombo_; }
    int Deaths() const { return deaths_; }

    PlayerState State() const { return st_; }
    double StateT() const { return stT_; }
    bool IsAirborne() const { return airborne_; }
    bool OnGround() const { return onGround_; }
    bool Crouching() const { return st_ == PlayerState::Crouch; }
    bool FacingRight() const { return facingRight; }
    int DashChargesLeft() const { return airDashUsed_ < stats_.airDashCount ? 1 : 0; }

private:
    SimContext* ctx_ = nullptr; // cached for event emission inside helpers
    PlayerStats stats_;
    const WeaponDef* weapon_ = nullptr;
    int hp_ = 3;
    int energy_ = 0;
    PlayerState st_ = PlayerState::Idle;
    double stT_ = 0.0;
    bool airborne_ = false;
    bool onGround_ = false;
    double coyoteT_ = 0.0;
    double jumpBufT_ = 0.0;
    bool jumpCut_ = false;
    int airDashUsed_ = 0;
    double dashCdT_ = 0.0;
    double dashDir_ = 1.0;
    double attackQueuedT_ = 0.0;
    double chargeT_ = 0.0;
    bool fired_ = false;
    double fireCdT_ = 0.0;
    double dropIgnore_ = 0.0;
    bool superActive_ = false;
    double superT_ = 0.0;
    bool superFired_ = false;
    int combo_ = 0;
    double comboT_ = 0.0;
    int maxCombo_ = 0;
    int deaths_ = 0;
    bool reviveAvailable_ = false;
    bool reviveUsed_ = false;
    double lastParryT_ = -10.0;
    int nextProjId_ = 1;

    void EnterState(PlayerState s);
    void FireProjectile(SimContext& ctx, bool charged);
    void DoSuper(SimContext& ctx);
    void SetCrouch(bool crouch);
};

} // namespace ink
