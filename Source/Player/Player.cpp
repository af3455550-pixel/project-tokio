#include "Player/Player.h"
#include "Gameplay/Projectile.h"
#include "Gameplay/Level.h"
#include "Gameplay/SimContext.h"
#include "Physics/Collision.h"
#include <cmath>

namespace ink {

void Player::AddEnergy(int n) {
    if (superActive_)
        return;
    int gain = static_cast<int>(n * stats_.energyGainMul);
    int before = energy_;
    energy_ = IClamp(energy_ + gain, 0, 100);
    if (before < 100 && energy_ >= 100 && ctx_)
        ctx_->Emit({Evt::SuperCharged, -1, 1, Center(), ""});
}

const char* PlayerStateName(PlayerState s) {
    switch (s) {
    case PlayerState::Idle: return "IDLE";
    case PlayerState::Run: return "RUN";
    case PlayerState::Jump: return "JUMP";
    case PlayerState::Fall: return "FALL";
    case PlayerState::Crouch: return "CROUCH";
    case PlayerState::Dash: return "DASH";
    case PlayerState::AirDash: return "AIRDASH";
    case PlayerState::Attack: return "ATTACK";
    case PlayerState::Parry: return "PARRY";
    case PlayerState::Hurt: return "HURT";
    case PlayerState::Dead: return "DEAD";
    case PlayerState::Super: return "SUPER";
    }
    return "?";
}

void Player::Init(int id_, const PlayerStats& stats) {
    id = id_;
    stats_ = stats;
    w = 14.0;
    h = kStandH;
    hp_ = stats_.maxHp;
    maxHp = stats_.maxHp;
    hp = hp_;
    alive = true;
    st_ = PlayerState::Idle;
    stT_ = 0.0;
}

void Player::SetStats(const PlayerStats& s) {
    PlayerStats old = stats_;
    stats_ = s;
    maxHp = s.maxHp;
    if (hp_ > maxHp)
        hp_ = maxHp;
    hp = hp_;
    if (old.maxHp != s.maxHp)
        hp = std::min(hp_, maxHp);
}

void Player::SetCrouch(bool crouch) {
    if (crouch) {
        pos.y += h - kCrouchH;
        h = kCrouchH;
    } else {
        pos.y -= h - kStandH;
        h = kStandH;
    }
}

void Player::EnterState(PlayerState s) {
    if (s == st_)
        return;
    bool wasCrouch = (st_ == PlayerState::Crouch);
    st_ = s;
    stT_ = 0.0;
    if (s == PlayerState::Attack) {
        fired_ = false;
        chargeT_ = 0.0;
    }
    if (s == PlayerState::Crouch)
        SetCrouch(true);
    else if (wasCrouch)
        SetCrouch(false);
    if (s == PlayerState::Jump) {
        airborne_ = true;
        jumpCut_ = false;
    }
}

static void ApplyHorizontal(Vec2& vel, double target, double acc) {
    if (std::abs(target) < 0.5)
        target = 0.0;
    if (vel.x < target)
        vel.x = std::min(vel.x + acc, target);
    else
        vel.x = std::max(vel.x - acc, target);
}

void Player::Update(double dt, SimContext& ctx, const PlayerInput& in) {
    ctx_ = &ctx;
    TickTimers(dt);
    if (fireCdT_ > 0.0)
        fireCdT_ -= dt;
    if (dashCdT_ > 0.0)
        dashCdT_ -= dt;
    if (dropIgnore_ > 0.0)
        dropIgnore_ -= dt;
    if (comboT_ > 0.0) {
        comboT_ -= dt;
        if (comboT_ <= 0.0)
            combo_ = 0;
    }
    if (coyoteT_ > 0.0 && !onGround_)
        coyoteT_ -= dt;
    if (jumpBufT_ > 0.0)
        jumpBufT_ -= dt;
    if (attackQueuedT_ > 0.0)
        attackQueuedT_ -= dt;

    if (!alive || st_ == PlayerState::Dead) {
        vel.y += stats_.gravity * dt;
        MoveResult r = MoveEntity(pos, vel, Box(), dt, *ctx.level, false);
        if (r.onGround)
            vel.y *= -0.35;
        pos = r.pos;
        stT_ += dt;
        return;
    }

    if (in.jumpPressed)
        jumpBufT_ = stats_.jumpBufferTime;
    if (in.attackPressed)
        attackQueuedT_ = stats_.inputBufferTime;

    // ---- Super ---------------------------------------------------------
    if (superActive_) {
        superT_ += dt;
        if (!superFired_ && superT_ >= 0.25) {
            superFired_ = true;
            ProjectileDef d;
            d.id = "ink_nova";
            d.speed = 540.0;
            d.damage = 2.0;
            d.radius = 4.0;
            d.life = 0.9;
            d.knockback = 320.0;
            d.parryable = false;
            d.vfx = "nova";
            d.score = 15;
            for (int i = 0; i < 16; ++i) {
                double a = i * (3.14159265 / 8.0);
                Projectile p;
                p.e.id = nextProjId_++;
                p.InitFromDef(d, true, Center(), {std::cos(a), std::sin(a)});
                ctx.Projectiles().push_back(std::move(p));
            }
            ctx.Particles().Shockwave(Center(), 0xFF7FD4FF);
            ctx.Emit({Evt::Shake, id, 7, Center(), "super"});
            ctx.Emit({Evt::HitStop, id, 120, Center(), "super"});
        }
        if (superT_ >= stats_.superTime) {
            superActive_ = false;
            EnterState(airborne_ ? PlayerState::Fall : PlayerState::Idle);
        }
        vel.y += stats_.gravity * 0.3 * dt;
    } else {
        // ---- Actions -----------------------------------------------------
        switch (st_) {
        case PlayerState::Idle:
        case PlayerState::Run: {
            ApplyHorizontal(vel, in.moveX * stats_.runSpeed, stats_.accel * dt);
            if (in.moveX != 0.0f)
                facingRight = in.moveX > 0.0f;
            if (in.crouch) {
                EnterState(PlayerState::Crouch);
                break;
            }
            if (in.dashPressed && dashCdT_ <= 0.0) {
                dashDir_ = (in.moveX != 0.0f) ? Sign(in.moveX) : (facingRight ? 1.0 : -1.0);
                vel = {dashDir_ * stats_.dashSpeed, 0.0};
                ctx.Particles().Dust({pos.x + w * 0.5, pos.y + h});
                EnterState(PlayerState::Dash);
                break;
            }
            if (in.parryPressed) {
                EnterState(PlayerState::Parry);
                break;
            }
            if (in.specialPressed && SuperReady()) {
                energy_ = 0;
                superActive_ = true;
                superT_ = 0.0;
                superFired_ = false;
                invulnT = stats_.superTime;
                vel = {vel.x * 0.4, -260.0};
                airborne_ = true;
                EnterState(PlayerState::Super);
                ctx.Emit({Evt::SuperUsed, id, 0, Center(), "ink_nova"});
                break;
            }
            if (attackQueuedT_ > 0.0) {
                attackQueuedT_ = 0.0;
                EnterState(PlayerState::Attack);
                break;
            }
            if (jumpBufT_ > 0.0 && (onGround_ || coyoteT_ > 0.0)) {
                vel.y = -stats_.jumpVel;
                jumpBufT_ = 0.0;
                coyoteT_ = 0.0;
                jumpCut_ = false;
                airborne_ = true;
                ctx.Particles().Dust({pos.x + w * 0.5, pos.y + h}, 3);
                EnterState(PlayerState::Jump);
                break;
            }
            st_ = (std::abs(vel.x) > 12.0) ? PlayerState::Run : PlayerState::Idle;
            break;
        }
        case PlayerState::Crouch: {
            vel.x *= std::max(0.0, 1.0 - 18.0 * dt);
            if (!in.crouch) {
                EnterState(PlayerState::Idle);
                break;
            }
            if (in.dropThrough)
                dropIgnore_ = 0.25;
            if (in.dashPressed && dashCdT_ <= 0.0) {
                dashDir_ = (in.moveX != 0.0f) ? Sign(in.moveX) : (facingRight ? 1.0 : -1.0);
                vel = {dashDir_ * stats_.dashSpeed, 0.0};
                EnterState(PlayerState::Dash);
                break;
            }
            if (attackQueuedT_ > 0.0) {
                attackQueuedT_ = 0.0;
                EnterState(PlayerState::Attack);
                break;
            }
            if (in.parryPressed)
                EnterState(PlayerState::Parry);
            break;
        }
        case PlayerState::Jump:
        case PlayerState::Fall: {
            if (!in.jumpHeld && vel.y < 0.0 && !jumpCut_) {
                vel.y *= stats_.variableJumpCut;
                jumpCut_ = true;
            }
            ApplyHorizontal(vel, in.moveX * stats_.runSpeed * stats_.airControl,
                            stats_.accel * stats_.airControl * dt);
            if (in.moveX != 0.0f)
                facingRight = in.moveX > 0.0f;
            if (in.dashPressed && dashCdT_ <= 0.0 && airDashUsed_ < stats_.airDashCount) {
                airDashUsed_++;
                dashDir_ = (in.moveX != 0.0f) ? Sign(in.moveX) : (facingRight ? 1.0 : -1.0);
                vel = {dashDir_ * stats_.dashSpeed, vel.y * 0.15};
                ctx.Particles().Dust(Center());
                EnterState(PlayerState::AirDash);
                break;
            }
            if (in.parryPressed) {
                EnterState(PlayerState::Parry);
                break;
            }
            if (attackQueuedT_ > 0.0) {
                attackQueuedT_ = 0.0;
                EnterState(PlayerState::Attack);
                break;
            }
            if (vel.y > 0.0)
                st_ = PlayerState::Fall;
            else if (vel.y < 0.0)
                st_ = PlayerState::Jump;
            break;
        }
        case PlayerState::Dash:
        case PlayerState::AirDash: {
            stT_ += dt;
            vel.x = dashDir_ * stats_.dashSpeed;
            vel.y += stats_.gravity * 0.25 * dt;
            if (attackQueuedT_ > 0.0 || in.attackPressed) { // dash cancel (§12)
                attackQueuedT_ = 0.0;
                EnterState(PlayerState::Attack);
                stT_ = 0.02;
                break;
            }
            if (stT_ >= stats_.dashTime) {
                if (st_ == PlayerState::Dash)
                    dashCdT_ = stats_.dashCooldown;
                EnterState(airborne_ ? PlayerState::Fall : PlayerState::Idle);
            }
            break;
        }
        case PlayerState::Attack: {
            stT_ += dt;
            if (in.attackHeld)
                chargeT_ += dt;
            if (!fired_ && stT_ >= stats_.attackFireT) {
                fired_ = true;
                bool charged = weapon_ && weapon_->hasCharged && chargeT_ >= weapon_->chargeTime;
                FireProjectile(ctx, charged);
            }
            if (airborne_)
                vel.y += stats_.gravity * dt;
            vel.x *= std::max(0.0, 1.0 - 6.0 * dt);
            if (stT_ >= stats_.attackTime) {
                if (attackQueuedT_ > 0.0) { // chain attacks via input buffer
                    attackQueuedT_ = 0.0;
                    stT_ = stats_.attackFireT * 0.5;
                    fired_ = false;
                    chargeT_ = 0.0;
                } else {
                    EnterState(airborne_ ? PlayerState::Fall : PlayerState::Idle);
                }
            }
            break;
        }
        case PlayerState::Parry: {
            stT_ += dt;
            vel.x *= std::max(0.0, 1.0 - 14.0 * dt);
            if (airborne_)
                vel.y += stats_.gravity * dt;
            if (stT_ >= stats_.parryTime)
                EnterState(airborne_ ? PlayerState::Fall : PlayerState::Idle);
            break;
        }
        case PlayerState::Hurt: {
            stT_ += dt;
            vel.x *= std::max(0.0, 1.0 - 3.0 * dt);
            vel.y += stats_.gravity * dt;
            if (stT_ >= 0.45)
                EnterState(airborne_ ? PlayerState::Fall : PlayerState::Idle);
            break;
        }
        case PlayerState::Super:
        case PlayerState::Dead:
            break;
        }
    }

    // ---- Hazards ---------------------------------------------------------
    if (!superActive_ && invulnT <= 0.0 && st_ != PlayerState::Dead && ctx.level) {
        if (ctx.level->HazardOverlaps(Box()))
            Damage(1, Center() + Vec2{0.0, -12.0}, ctx, /*hazard=*/true);
    }

    // ---- Gravity (states not handled above) --------------------------------
    switch (st_) {
    case PlayerState::Idle:
    case PlayerState::Run:
    case PlayerState::Jump:
    case PlayerState::Fall:
    case PlayerState::Crouch:
    case PlayerState::Hurt:
    case PlayerState::Attack:
    case PlayerState::Parry:
    case PlayerState::Dead:
        vel.y += stats_.gravity * dt;
        break;
    default:
        break;
    }
    vel.y = Clamp(vel.y, -2000.0, stats_.maxFall);

    // ---- Physics -----------------------------------------------------------
    const double prevVy = vel.y;
    MoveResult r = MoveEntity(pos, vel, Box(), dt, *ctx.level, dropIgnore_ > 0.0);
    bool wasAirborne = airborne_;
    pos = r.pos;
    vel = r.vel;
    if ((st_ == PlayerState::Dash) && r.hitWall) {
        dashCdT_ = stats_.dashCooldown * 0.5;
        EnterState(airborne_ ? PlayerState::Fall : PlayerState::Idle);
    }
    if (r.onGround) {
        if (wasAirborne && prevVy > 420.0)
            ctx.Particles().Dust({pos.x + w * 0.5, pos.y + h});
        airborne_ = false;
        onGround_ = true;
        coyoteT_ = stats_.coyoteTime;
        airDashUsed_ = 0;
    } else {
        onGround_ = false;
    }

    // Moving platform carry
    if (onGround_ && ctx.level) {
        for (const auto& p : ctx.level->Platforms()) {
            Rect pb{p.pos.x, p.pos.y, 48.0, 8.0};
            Rect feet{pos.x, pos.y + h - 3.0, w, 3.0};
            if (feet.Overlaps(pb)) {
                pos += p.vel;
                break;
            }
        }
    }

    // Level bounds + fall-out safety
    if (ctx.level) {
        const Rect b = ctx.level->Data().bounds;
        pos.x = Clamp(pos.x, b.Left() + 2.0, b.Right() - w - 2.0);
        if (pos.y > b.Bottom() + 64.0) {
            Damage(1, Center(), ctx, /*hazard=*/true);
            if (alive) {
                pos = ctx.level->RespawnPoint();
                vel = {0.0, 0.0};
                airborne_ = false;
                EnterState(PlayerState::Idle);
            }
        }
    }
}

bool Player::IsParryActive() const {
    return st_ == PlayerState::Parry && stT_ >= stats_.parryActiveFrom && stT_ <= stats_.parryActiveTo;
}

bool Player::TryParryHit(Projectile& p, SimContext& ctx) {
    if (!alive || !IsParryActive())
        return false;
    p.dead = true;
    bool chain = (ctx.time - lastParryT_) < 1.5 && lastParryT_ > 0.0;
    lastParryT_ = ctx.time;
    AddEnergy(chain ? 25 : 15);
    ctx.Emit({Evt::ParrySuccess, id, chain ? 2 : 0, p.e.Center(), p.def.id});
    ctx.Emit({Evt::Shake, id, 2, p.e.Center(), "parry"});
    ctx.Emit({Evt::HitStop, id, 70, p.e.Center(), "parry"});
    ctx.Particles().Stars(p.e.Center(), 0xFFFFE08A, 8);
    ctx.Particles().Sparks(p.e.Center(), 0xFFFFFFFF, 6);
    AddCombo();
    AddCombo();
    hitFlashT = 0.12;
    return true;
}

bool Player::TryParryMelee(SimContext& ctx) {
    if (!alive || !IsParryActive())
        return false;
    bool chain = (ctx.time - lastParryT_) < 1.5 && lastParryT_ > 0.0;
    lastParryT_ = ctx.time;
    AddEnergy(chain ? 25 : 15);
    ctx.Emit({Evt::ParrySuccess, id, chain ? 2 : 1, Center(), "melee"});
    ctx.Emit({Evt::Shake, id, 2, Center(), "parry"});
    ctx.Emit({Evt::HitStop, id, 70, Center(), "parry"});
    ctx.Particles().Stars(Center(), 0xFFFFE08A, 8);
    AddCombo();
    AddCombo();
    hitFlashT = 0.12;
    return true;
}

void Player::Damage(int dmg, Vec2 fromPos, SimContext& ctx, bool hazard) {
    ctx_ = &ctx;
    if (!alive || ctx.godMode)
        return;
    if (invulnT > 0.0)
        return;
    if (superActive_)
        return;
    if (!hazard && st_ == PlayerState::Dash && stT_ < stats_.dashIFrames)
        return;

    int final = IClamp(static_cast<int>(std::round(dmg * stats_.dmgTakenMul)), 1, 99);
    hp_ -= final;
    hp = hp_;
    ctx.Emit({Evt::PlayerHurt, id, final, Center(), hazard ? "hazard" : ""});
    ctx.Emit({Evt::Shake, id, hazard ? 4 : 5, Center(), "hurt"});
    combo_ = 0;
    comboT_ = 0.0;
    invulnT = stats_.invulnTime;

    Vec2 away = Center() - fromPos;
    if (away.LengthSq() < 1e-6)
        away = {facingRight ? -1.0 : 1.0, -1.0};
    Vec2 nd = away.Normalized();
    vel.x = (std::abs(nd.x) < 0.3 ? (facingRight ? -1.0 : 1.0) : nd.x) * 260.0;
    vel.y = -240.0;
    airborne_ = true;

    if (hp_ <= 0) {
        if (reviveAvailable_ && !reviveUsed_) {
            reviveUsed_ = true;
            hp_ = 1;
            hp = 1;
            EnterState(PlayerState::Hurt);
            invulnT = 2.0;
            ctx.Emit({Evt::PlayerHealed, id, 1, Center(), "echo_heart"});
            ctx.Particles().Stars(Center(), 0xFF8AFFC1, 14);
            return;
        }
        hp_ = 0;
        hp = 0;
        alive = false;
        EnterState(PlayerState::Dead);
        vel = {0.0, -340.0};
        deaths_++;
        ctx.Emit({Evt::PlayerDied, id, 0, Center(), ""});
        return;
    }
    EnterState(PlayerState::Hurt);
}

void Player::Kill(SimContext& ctx) {
    ctx_ = &ctx;
    if (!alive)
        return;
    hp_ = 0;
    hp = 0;
    alive = false;
    EnterState(PlayerState::Dead);
    vel = {0.0, -340.0};
    deaths_++;
    ctx.Emit({Evt::PlayerDied, id, 0, Center(), ""});
}

void Player::Respawn(const Vec2& pos_, SimContext& ctx) {
    ctx_ = &ctx;
    pos = pos_;
    vel = {0.0, 0.0};
    hp_ = stats_.maxHp;
    hp = hp_;
    alive = true;
    invulnT = 1.5;
    airborne_ = false;
    onGround_ = false;
    energy_ = std::max(energy_, 0);
    EnterState(PlayerState::Idle);
    ctx.Emit({Evt::PlayerRespawn, id, 0, pos, ""});
}

void Player::FireProjectile(SimContext& ctx, bool charged) {
    if (!weapon_)
        return;
    const ProjectileDef& d = charged ? weapon_->charged : weapon_->shot;
    fireCdT_ = weapon_->fireCooldown;
    const double dir = facingRight ? 1.0 : -1.0;
    Vec2 base{dir, 0.0};
    Vec2 origin = Center() + base * 12.0 + Vec2{0.0, -2.0};
    for (int i = 0; i < std::max(1, d.count); ++i) {
        double a = (i - (d.count - 1) / 2.0) * d.spreadDeg * (3.14159265 / 180.0);
        double c = std::cos(a), s = std::sin(a);
        Vec2 rd{base.x * c - base.y * s, base.x * s + base.y * c};
        Projectile p;
        p.e.id = nextProjId_++;
        p.InitFromDef(d, true, origin, rd);
        ctx.Projectiles().push_back(std::move(p));
    }
    ctx.Particles().Sparks(origin + base * 6.0, charged ? 0xFF7FD4FF : 0xFFBFE3FF, charged ? 6 : 3);
}

} // namespace ink
