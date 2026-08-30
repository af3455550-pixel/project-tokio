#pragma once
// Generic enemy state machine (§21): IDLE / PATROL / NOTICE / CHASE / ATTACK /
// RECOVER / STUNNED / DAMAGED / RETREAT / DEAD.
// The brain owns state *transitions* (detection, reaction time, cooldowns);
// concrete enemies own *behavior* inside each state (virtuals on Enemy).
#include <string>

namespace ink {

enum class BrainState { Idle, Patrol, Notice, Chase, Attack, Recover, Stunned, Damaged, Retreat, Dead };

const char* BrainStateName(BrainState s);

struct BrainConfig {
    double detectRange = 240.0;
    double attackRange = 70.0;
    double reactTime = 0.25;    // NOTICE duration before acting
    double recoverTime = 0.6;   // RECOVER base duration
    double attackCooldown = 1.4;
    double loseAggroRange = 420.0;
    bool useLOS = true;
    bool canRetreat = false;
    double retreatAtHpf = 0.3; // fraction of HP at which RETREAT triggers
};

struct BrainView {
    double dt = 0.0;
    double distToPlayer = 0.0;
    bool hasLOS = false;
    bool playerAlive = true;
    double hpf = 1.0;           // enemy health fraction
    bool interrupted = false;   // e.g. damaged mid-attack
};

class AIBrain {
    using enum BrainState; // C++20: unscoped access inside the class

public:
    void Reset() { state_ = Idle; t_ = 0.0; prev_ = Idle; }

    BrainState Get() const { return state_; }
    double T() const { return t_; } // time in current state

    bool IsAggro() const {
        return state_ == Notice || state_ == Chase || state_ == Attack || state_ == Retreat;
    }
    bool IsCombatState() const { return state_ == Chase || state_ == Attack; }

    // Called by the enemy when it wants to leave its current state
    // (attack finished, got stunned, ...).
    void Request(BrainState s) { Enter(s); }

    void Stun(double duration) {
        stunDur_ = duration;
        Enter(Stunned);
    }
    void DamageFlash() {
        if (state_ != Damaged && state_ != Stunned && state_ != Dead) {
            prevCombat_ = state_;
            Enter(Damaged);
        }
    }

    // Advance the machine one tick.
    void Tick(const BrainView& v, const BrainConfig& cfg) {
        t_ += v.dt;
        if (state_ == Dead)
            return;
        if (!v.playerAlive) {
            if (state_ != Idle && state_ != Patrol)
                Enter(Idle);
            return;
        }

        switch (state_) {
        case Idle:
        case Patrol:
            if (v.distToPlayer < cfg.detectRange && (!cfg.useLOS || v.hasLOS))
                Enter(Notice);
            break;
        case Notice:
            if (t_ >= cfg.reactTime)
                Enter(v.distToPlayer <= cfg.attackRange ? Attack : Chase);
            break;
        case Chase:
            if (cfg.canRetreat && v.hpf < cfg.retreatAtHpf)
                Enter(Retreat);
            else if (v.distToPlayer > cfg.loseAggroRange && !v.hasLOS)
                Enter(Idle);
            else if (v.distToPlayer <= cfg.attackRange)
                Enter(Attack);
            break;
        case Attack:
            if (v.interrupted)
                Enter(Recover);
            // otherwise the enemy itself requests Recover when its attack
            // animation finishes.
            break;
        case Recover:
            if (t_ >= std::max(cfg.recoverTime, cfg.attackCooldown)) {
                if (cfg.canRetreat && v.hpf < cfg.retreatAtHpf)
                    Enter(Retreat);
                else
                    Enter(v.distToPlayer <= cfg.attackRange ? Attack : Chase);
            }
            break;
        case Stunned:
            if (t_ >= stunDur_)
                Enter(Recover);
            break;
        case Damaged:
            if (t_ >= 0.12)
                Enter(prevCombat_);
            break;
        case Retreat:
            if (v.distToPlayer > cfg.detectRange * 1.5)
                Enter(Idle);
            break;
        case Dead:
            break;
        }
    }

private:
    void Enter(BrainState s) {
        if (s == Dead)
            prev_ = state_;
        if (state_ == Chase || state_ == Attack)
            prevCombat_ = state_;
        state_ = s;
        t_ = 0.0;
    }

    BrainState state_ = Idle;
    BrainState prev_ = Idle;
    BrainState prevCombat_ = Chase;
    double t_ = 0.0;
    double stunDur_ = 1.0;
};

} // namespace ink
