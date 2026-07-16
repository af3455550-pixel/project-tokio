#include "Bosses/BossBarnaby.h"
#include "Gameplay/Level.h"
#include "Gameplay/Projectile.h"
#include "Gameplay/SimContext.h"
#include "Player/Player.h"
#include <cmath>

namespace ink {

static ProjectileDef CrowDef() {
    ProjectileDef d;
    d.id = "crow";
    d.speed = 330.0;
    d.gravity = 620.0;
    d.life = 3.2;
    d.radius = 5.0;
    d.damage = 1;
    d.knockback = 220.0;
    d.parryable = true;
    d.vfx = "crow";
    d.score = 0;
    return d;
}

void Barnaby::UpdateCombat(double dt, SimContext& ctx) {
    if (current_ != None) {
        UpdatePattern(dt, ctx);
        return;
    }
    cdT_ -= dt;
    if (ctx.player)
        facingRight = ctx.player->Center().x < Center().x;
    vel.x *= std::max(0.0, 1.0 - 2.0 * dt);
    if (cdT_ <= 0.0)
        PickPattern(ctx);
}

void Barnaby::PickPattern(SimContext& ctx) {
    int options[7];
    int n = 0;
    options[n++] = Scythe;
    options[n++] = Crows;
    options[n++] = Hop;
    if (phase >= 1) {
        options[n++] = SeedRain;
        options[n++] = Roots;
    }
    if (phase >= 2)
        options[n++] = StormSlam;
    // avoid repeating the last pattern when possible
    int pick = None;
    for (int tries = 0; tries < n * 2; ++tries) {
        pick = options[ctx.GetRng().range(0, n - 1)];
        if (pick != lastPat_ || n == 1)
            break;
    }
    lastPat_ = pick;
    current_ = pick;
    patT_ = 0.0;
    zones_.clear();
    if (current_ == SeedRain || current_ == Roots) {
        const double groundY = ctx.level ? ctx.level->Data().bounds.Bottom() : pos.y + h;
        (void)groundY;
        if (ctx.player) {
            int count = (current_ == SeedRain) ? 3 : 2;
            for (int i = 0; i < count; ++i) {
                double off = ctx.GetRng().range(-90, 90) + i * 60;
                zones_.push_back({ctx.player->Center().x + off, 0.0, 0});
            }
        }
    }
    ctx.Emit({Evt::HitStop, id, 40, Center(), "pattern"});
}

void Barnaby::SpawnCrow(SimContext& ctx, double angleOffset) {
    Projectile crow;
    crow.e.id = nextShotId_++;
    Vec2 toPlayer = ctx.player ? (ctx.player->Center() - Center()).Normalized() : Vec2{-1.0, -0.3};
    double a = std::atan2(toPlayer.y, toPlayer.x) + angleOffset;
    crow.InitFromDef(CrowDef(), false, {pos.x + w * 0.5, pos.y + h * 0.35}, {std::cos(a), std::sin(a)});
    ctx.Projectiles().push_back(std::move(crow));
    ctx.Particles().Sparks({pos.x + w * 0.5, pos.y + h * 0.35}, 0xFF4A3B5C, 4);
}

void Barnaby::MeleeArc(SimContext& ctx) {
    if (!ctx.player)
        return;
    Rect arc = facingRight ? Rect{pos.x + w - 6.0, pos.y + h - 46.0, 66.0, 46.0}
                           : Rect{pos.x - 60.0, pos.y + h - 46.0, 66.0, 46.0};
    ctx.Particles().Dust(arc.Center(), 8);
    ctx.Particles().InkSplash(arc.Center(), 0xFF3A2A18, 10, 180);
    ctx.Emit({Evt::Shake, id, 6, arc.Center(), "slam"});
    if (arc.Overlaps(ctx.player->Box()))
        ctx.player->Damage(1, Center(), ctx, /*hazard=*/false);
}

void Barnaby::UpdatePattern(double dt, SimContext& ctx) {
    patT_ += dt;
    switch (current_) {
    case Scythe: {
        if (patT_ < 0.45) {
            vel.x = (facingRight ? -1.0 : 1.0) * 170.0; // step into range
        } else if (patT_ < 1.05) {
            vel.x *= std::max(0.0, 1.0 - 10.0 * dt); // telegraph: raise scythe
        } else if (patT_ < 1.35) {
            if (patT_ - dt >= 1.05)
                MeleeArc(ctx); // slam frame
            vel.x = (facingRight ? 1.0 : -1.0) * 240.0;
        } else {
            current_ = None;
            cdT_ = std::max(0.3, 0.6 - 0.12 * phase);
        }
        break;
    }
    case Crows: {
        vel.x *= std::max(0.0, 1.0 - 8.0 * dt);
        if (patT_ >= 0.5 && patT_ - dt < 0.5) {
            SpawnCrow(ctx, -0.42);
            SpawnCrow(ctx, 0.0);
            SpawnCrow(ctx, 0.42);
        }
        if (patT_ >= 0.95) {
            current_ = None;
            cdT_ = std::max(0.3, 0.55 - 0.12 * phase);
        }
        break;
    }
    case Hop: {
        if (patT_ < 0.4) {
            vel.x *= std::max(0.0, 1.0 - 10.0 * dt); // crouch
        } else if (hopPhase_ == 0 && patT_ >= 0.4) {
            hopPhase_ = 1;
            vel.y = -560.0;
            hopVelX_ = (facingRight ? -1.0 : 1.0) * 240.0;
        } else if (hopPhase_ == 1) {
            vel.x = hopVelX_;
            if (vel.y > 320.0) { // landing
                hopPhase_ = 2;
                ctx.Emit({Evt::Shake, id, 4, Center(), "hopland"});
                ctx.Particles().Dust({pos.x + w * 0.5, pos.y + h}, 8);
                MeleeArc(ctx);
            }
        }
        if (patT_ >= 1.7) {
            current_ = None;
            hopPhase_ = 0;
            cdT_ = std::max(0.3, 0.55 - 0.12 * phase);
        }
        break;
    }
    case SeedRain: {
        vel.x *= std::max(0.0, 1.0 - 8.0 * dt);
        const double groundY = ctx.level ? ctx.level->Data().bounds.Bottom() : pos.y + h;
        bool allDone = true;
        for (auto& z : zones_) {
            z.t += dt;
            if (z.t < 1.9)
                allDone = false;
            if (z.t >= 1.0 && z.t - dt < 1.0) {
                // seed falls
                ProjectileDef seed;
                seed.id = "seed";
                seed.speed = 90.0;
                seed.gravity = 900.0;
                seed.life = 1.6;
                seed.radius = 4.0;
                seed.damage = 1;
                seed.knockback = 160.0;
                seed.parryable = true;
                seed.vfx = "seed";
                seed.score = 0;
                Projectile p;
                p.e.id = nextShotId_++;
                p.InitFromDef(seed, false, {z.x - 4.0, groundY - 300.0}, {0.0, 90.0});
                ctx.Projectiles().push_back(std::move(p));
                ctx.Particles().InkSplash({z.x, groundY}, 0xFF5C7A2E, 5, 120);
            }
        }
        if (allDone) {
            current_ = None;
            cdT_ = 0.5;
        }
        break;
    }
    case Roots: {
        vel.x *= std::max(0.0, 1.0 - 8.0 * dt);
        const double groundY = ctx.level ? ctx.level->Data().bounds.Bottom() : pos.y + h;
        (void)groundY;
        bool allDone = true;
        for (auto& z : zones_) {
            z.t += dt;
            if (z.t < 1.35)
                allDone = false;
            if (z.t >= 0.5 && z.t < 1.05) {
                if (z.t - dt < 0.5)
                    ctx.Emit({Evt::Shake, id, 4, {z.x, groundY}, "root"});
                Rect root{z.x - 13.0, groundY - 46.0, 26.0, 46.0};
                if (ctx.player && root.Overlaps(ctx.player->Box()))
                    ctx.player->Damage(1, root.Center(), ctx, /*hazard=*/false);
            }
        }
        if (allDone) {
            current_ = None;
            cdT_ = 0.5;
        }
        break;
    }
    case StormSlam: {
        if (patT_ < 1.1) {
            vel.x *= std::max(0.0, 1.0 - 10.0 * dt);
            if (patT_ >= 0.9)
                ctx.Emit({Evt::Shake, id, 2, Center(), "stormwarn"});
        } else if (patT_ - dt < 1.1) {
            ctx.Emit({Evt::Shake, id, 10, Center(), "stormslam"});
            ctx.Emit({Evt::HitStop, id, 140, Center(), "stormslam"});
            MeleeArc(ctx);
            for (int i = 0; i < 6; ++i)
                SpawnCrow(ctx, (i - 2.5) * 0.24);
            ctx.Particles().Shockwave(Center(), 0xFF7FD4FF);
        }
        if (patT_ >= 1.6) {
            current_ = None;
            cdT_ = 0.8;
        }
        break;
    }
    default:
        current_ = None;
        break;
    }
}

std::vector<Rect> Barnaby::TellZones() const {
    std::vector<Rect> out;
    const double groundY = pos.y + h;
    if (current_ == Scythe && patT_ >= 0.45 && patT_ < 1.35) {
        out.push_back(facingRight ? Rect{pos.x + w - 6.0, groundY - 46.0, 66.0, 46.0}
                                  : Rect{pos.x - 60.0, groundY - 46.0, 66.0, 46.0});
    }
    if (current_ == SeedRain) {
        for (const auto& z : zones_)
            if (z.t < 1.0)
                out.push_back({z.x - 18.0, groundY - 10.0, 36.0, 10.0});
    }
    if (current_ == Roots) {
        for (const auto& z : zones_)
            if (z.t >= 0.3 && z.t < 1.05)
                out.push_back({z.x - 13.0, groundY - 46.0, 26.0, 46.0});
    }
    return out;
}

std::string Barnaby::ArtName() const {
    if (inDefeat)
        return "barnaby_defeat";
    if (transitionT > 0.0)
        return "barnaby_phase";
    switch (current_) {
    case Scythe: return patT_ < 0.45 ? "barnaby_walk" : (patT_ < 1.05 ? "barnaby_raise" : "barnaby_slam");
    case Crows: return "barnaby_crow";
    case Hop: return patT_ < 0.4 ? "barnaby_crouch" : "barnaby_hop";
    case SeedRain: return "barnaby_seeds";
    case Roots: return "barnaby_roots";
    case StormSlam: return "barnaby_storm";
    default: return "barnaby_idle";
    }
}

int Barnaby::ArtFrame() const {
    return static_cast<int>(fightTime * 4.0) % 2 == 0 ? 0 : 1;
}

} // namespace ink
