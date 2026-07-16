#include "Enemies/EnemyTypes.h"
#include "Gameplay/Level.h"
#include "Gameplay/Projectile.h"
#include "Gameplay/SimContext.h"
#include "Player/Player.h"
#include "Physics/Collision.h"
#include <cmath>

namespace ink {

// ---------------------------------------------------------------- SLome ----
void Slome::OnStateEnter(BrainState s, SimContext& ctx) {
    if (s == BrainState::Attack) {
        attackT = 0.0;
        attackActive = false;
        lungeStarted_ = false;
        vel.x = 0.0;
    }
    if ((s == BrainState::Patrol || s == BrainState::Idle) && ctx.rng) {
        if (ctx.rng->chance(0.5))
            patrolDir_ = -patrolDir_;
    }
}

void Slome::UpdateAI(double dt, SimContext& ctx) {
    using BS = BrainState;
    switch (brain.Get()) {
    case BS::Idle:
    case BS::Patrol: {
        vel.x = patrolDir_ * def->speed * 0.4;
        anim.frame = (static_cast<int>(bobT * 6.0) % 2) == 0 ? 0 : 1;
        // Flip at walls or ledges.
        if (ctx.level) {
            Rect ahead = Box().Translated(patrolDir_ > 0 ? w + 2.0 : -w - 2.0, 0.0);
            if (OverlapsSolid(ahead, *ctx.level))
                patrolDir_ = -patrolDir_;
            Vec2 probe = {pos.x + w * 0.5 + patrolDir_ * (w * 0.5 + 4.0), pos.y + h + 8.0};
            if (!OverlapsSolid(Rect{probe.x - 1, probe.y, 2, 2}, *ctx.level))
                patrolDir_ = -patrolDir_;
        }
        break;
    }
    case BS::Notice:
        vel.x *= std::max(0.0, 1.0 - 10.0 * dt);
        break;
    case BS::Chase: {
        double dir = PlayerDir(ctx).x;
        vel.x = dir * def->speed;
        facingRight = dir > 0.0;
        break;
    }
    case BS::Damaged:
        break;
    case BS::Attack: {
        attackT += dt;
        if (attackT < 0.35) {
            // Telegraph: crouch, shake.
            vel.x *= std::max(0.0, 1.0 - 12.0 * dt);
        } else if (attackT < 0.6) {
            if (!lungeStarted_) {
                lungeStarted_ = true;
                attackActive = true;
                double dir = PlayerDir(ctx).x;
                if (std::abs(dir) < 0.1)
                    dir = patrolDir_;
                vel.x = dir * def->speed * 3.4;
                vel.y = -140.0;
                facingRight = dir > 0.0;
                ctx.Particles().Dust({pos.x + w * 0.5, pos.y + h});
            }
        } else {
            FinishAttack(ctx);
        }
        break;
    }
    case BS::Recover:
    case BS::Retreat:
        vel.x = -PlayerDir(ctx).x * def->speed * 0.5;
        break;
    case BS::Stunned:
        vel.x *= std::max(0.0, 1.0 - 8.0 * dt);
        break;
    default:
        break;
    }
}

std::string Slome::ArtName() const {
    using BS = BrainState;
    switch (brain.Get()) {
    case BS::Stunned: return "slome_stun";
    case BS::Notice: return "slome_notice";
    case BS::Attack: return attackT < 0.35 ? "slome_tele" : "slome_lunge";
    case BS::Chase: return "slome_walk";
    default: return (anim.frame >= 1) ? "slome_walk" : "slome_idle";
    }
}

int Slome::ArtFrame() const {
    return (brain.Get() == BrainState::Patrol || brain.Get() == BrainState::Chase) ? (anim.frame >= 1 ? 1 : 0) : 0;
}

// ---------------------------------------------------------------- InkBat ----
void InkBat::OnStateEnter(BrainState s, SimContext& ctx) {
    hoverBaseY_ = pos.y;
    if (s == BrainState::Attack) {
        attackT = 0.0;
        attackActive = false;
        lungeStarted_ = false;
        swoopDir_ = PlayerDir(ctx);
    }
}

void InkBat::UpdateAI(double dt, SimContext& ctx) {
    using BS = BrainState;
    switch (brain.Get()) {
    case BS::Idle:
    case BS::Patrol: {
        // Slow figure-eight around the home point.
        pos.x = homePos.x + std::sin(bobT * 0.9) * 34.0;
        pos.y = hoverBaseY_ + std::sin(bobT * 1.7) * 10.0;
        vel = {0, 0};
        break;
    }
    case BS::Notice:
        vel.x *= std::max(0.0, 1.0 - 6.0 * dt);
        break;
    case BS::Chase: {
        Vec2 pd = PlayerDir(ctx);
        double d = PlayerDist(ctx);
        double speed = def->speed;
        // Keep a comfortable band; strafe vertically.
        Vec2 want;
        if (d > 240.0)
            want = pd * speed;
        else if (d < 120.0)
            want = -pd * speed * 0.7;
        else
            want = {-pd.y * speed * 0.6, pd.x * speed * 0.4};
        want.y += (ctx.player->Center().y - 40.0 - Center().y) * 2.0;
        vel = LerpVec(vel, want, 1.0 - std::exp(-4.0 * dt));
        facingRight = vel.x > 0.0;
        break;
    }
    case BS::Attack: {
        attackT += dt;
        if (attackT < 0.3) {
            // Telegraph: shrink and glow.
            vel *= std::max(0.0, 1.0 - 10.0 * dt);
        } else if (attackT < 0.65) {
            if (!lungeStarted_) {
                lungeStarted_ = true;
                attackActive = true;
                vel = swoopDir_ * def->speed * 4.6;
                ctx.Particles().Sparks(Center(), 0xFF8A6BFF, 5);
            }
        } else {
            FinishAttack(ctx);
        }
        break;
    }
    case BS::Recover:
    case BS::Retreat:
        vel *= std::max(0.0, 1.0 - 3.0 * dt);
        break;
    case BS::Stunned:
        vel.y += 260.0 * dt; // stunned bats drop
        break;
    default:
        break;
    }
}

std::string InkBat::ArtName() const {
    using BS = BrainState;
    switch (brain.Get()) {
    case BS::Stunned: return "inkbat_stun";
    case BS::Notice: return "inkbat_notice";
    case BS::Attack: return attackT < 0.3 ? "inkbat_tele" : "inkbat_dive";
    default: return "inkbat_flap";
    }
}

int InkBat::ArtFrame() const { return (anim.frame >= 1) ? 1 : 0; }

// ----------------------------------------------------------- QuillGunner ----
void QuillGunner::OnStateEnter(BrainState s, SimContext& ctx) {
    if (s == BrainState::Attack) {
        attackT = 0.0;
        attackActive = false;
        lungeStarted_ = false;
        vel.x = 0.0;
    }
}

void QuillGunner::UpdateAI(double dt, SimContext& ctx) {
    using BS = BrainState;
    switch (brain.Get()) {
    case BS::Idle:
    case BS::Patrol: {
        vel.x = patrolDir_ * def->speed * 0.3;
        anim.frame = (static_cast<int>(bobT * 6.0) % 2) == 0 ? 0 : 1;
        if (ctx.level && OverlapsSolid(Box().Translated(patrolDir_ > 0 ? w + 2.0 : -w - 2.0, 0.0), *ctx.level))
            patrolDir_ = -patrolDir_;
        break;
    }
    case BS::Notice:
        vel.x *= std::max(0.0, 1.0 - 10.0 * dt);
        break;
    case BS::Chase: {
        double d = PlayerDist(ctx);
        double dir = PlayerDir(ctx).x;
        facingRight = dir > 0.0;
        if (d < 140.0)
            vel.x = -dir * def->speed;      // back away
        else if (d > 230.0)
            vel.x = dir * def->speed;      // close in
        else
            vel.x *= std::max(0.0, 1.0 - 8.0 * dt);
        break;
    }
    case BS::Attack: {
        attackT += dt;
        if (attackT < 0.4) {
            // Telegraph: raise the quiller.
            vel.x = 0.0;
        } else if (attackT < 0.5) {
            if (!lungeStarted_) {
                lungeStarted_ = true;
                ProjectileDef shot = MakeEnemyShot();
                shot.speed = 320.0;
                int n = ctx.rng && ctx.rng->chance(0.3) ? 2 : 1;
                for (int i = 0; i < n; ++i) {
                    Vec2 dir = {facingRight ? 1.0 : -1.0, (i == 0) ? -0.12 : 0.12};
                    Projectile p;
                    p.e.id = 100000 + id + i;
                    p.InitFromDef(shot, false, Center() + Vec2{(facingRight ? 10.0 : -10.0), -4.0}, dir);
                    ctx.Projectiles().push_back(std::move(p));
                }
                ctx.Particles().Sparks(Center(), 0xFF9FD08A, 5);
            }
        } else {
            FinishAttack(ctx);
        }
        break;
    }
    case BS::Retreat: {
        double dir = PlayerDir(ctx).x;
        vel.x = -dir * def->speed * 1.3;
        facingRight = dir > 0.0;
        break;
    }
    case BS::Recover:
        vel.x *= std::max(0.0, 1.0 - 8.0 * dt);
        break;
    case BS::Stunned:
        vel.x *= std::max(0.0, 1.0 - 8.0 * dt);
        break;
    default:
        break;
    }
}

std::string QuillGunner::ArtName() const {
    using BS = BrainState;
    switch (brain.Get()) {
    case BS::Stunned: return "quillgunner_stun";
    case BS::Notice: return "quillgunner_notice";
    case BS::Attack: return attackT < 0.4 ? "quillgunner_tele" : "quillgunner_shoot";
    case BS::Chase: return "quillgunner_walk";
    default: return (anim.frame >= 1) ? "quillgunner_walk" : "quillgunner_idle";
    }
}

int QuillGunner::ArtFrame() const {
    return (brain.Get() == BrainState::Chase || brain.Get() == BrainState::Patrol) ? (anim.frame >= 1 ? 1 : 0) : 0;
}

// --------------------------------------------------------------- PaperWisp ----
void PaperWisp::OnStateEnter(BrainState s, SimContext& ctx) {
    if (s == BrainState::Attack) {
        attackT = 0.0;
        lungeStarted_ = false;
    }
    (void)ctx;
}

void PaperWisp::UpdateAI(double dt, SimContext& ctx) {
    using BS = BrainState;
    switch (brain.Get()) {
    case BS::Idle:
    case BS::Patrol: {
        pos.x = homePos.x + std::sin(bobT * 2.1 + id * 1.7) * 26.0;
        pos.y = homePos.y + std::cos(bobT * 1.4 + id) * 14.0;
        vel = {0, 0};
        break;
    }
    case BS::Chase: {
        Vec2 pd = PlayerDir(ctx);
        vel = LerpVec(vel, pd * def->speed * 1.1 + Vec2{0, std::sin(bobT * 8.0) * 40.0},
                      1.0 - std::exp(-3.0 * dt));
        facingRight = vel.x > 0.0;
        break;
    }
    case BS::Attack: {
        attackT += dt;
        Vec2 pd = PlayerDir(ctx);
        vel = pd * def->speed * 2.4; // final dive
        attackActive = true;
        if (attackT > 0.4)
            FinishAttack(ctx);
        break;
    }
    case BS::Stunned:
        vel.y += 200.0 * dt;
        break;
    default:
        break;
    }
}

std::string PaperWisp::ArtName() const {
    using BS = BrainState;
    switch (brain.Get()) {
    case BS::Stunned: return "wisp_stun";
    case BS::Attack: return "wisp_dive";
    default: return "wisp_wobble";
    }
}

int PaperWisp::ArtFrame() const { return (anim.frame >= 1) ? 1 : 0; }

} // namespace ink
