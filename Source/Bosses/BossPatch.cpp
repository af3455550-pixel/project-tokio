#include "Bosses/BossPatch.h"
#include "Gameplay/Level.h"
#include "Gameplay/Projectile.h"
#include "Gameplay/SimContext.h"
#include "Player/Player.h"
#include <cmath>

namespace ink {

void Patch::UpdateCombat(double dt, SimContext& ctx) {
    if (current_ != None) {
        UpdatePattern(dt, ctx);
        return;
    }
    cdT_ -= dt;
    if (ctx.player)
        facingRight = ctx.player->Center().x < Center().x;
    vel.x *= std::max(0.0, 1.0 - 2.5 * dt);
    if (cdT_ <= 0.0)
        PickPattern(ctx);
}

void Patch::PickPattern(SimContext& ctx) {
    current_ = ctx.GetRng().chance(0.5) ? Needle : Swing;
    patT_ = 0.0;
    ctx.Emit({Evt::HitStop, id, 30, Center(), "pattern"});
}

void Patch::UpdatePattern(double dt, SimContext& ctx) {
    patT_ += dt;
    switch (current_) {
    case Needle: {
        vel.x *= std::max(0.0, 1.0 - 8.0 * dt);
        if (patT_ >= 0.5 && patT_ - dt < 0.5) {
            // Throw a small arc of parryable pins.
            int count = (phase >= 1) ? 3 : 2;
            ProjectileDef pin;
            pin.id = "pin";
            pin.speed = 300.0;
            pin.gravity = 420.0;
            pin.life = 2.6;
            pin.radius = 3.5;
            pin.damage = 1;
            pin.knockback = 180.0;
            pin.parryable = true;
            pin.vfx = "pin";
            pin.score = 0;
            Vec2 toPlayer = ctx.player ? (ctx.player->Center() - Center()).Normalized() : Vec2{-1.0, -0.3};
            double base = std::atan2(toPlayer.y, toPlayer.x);
            for (int i = 0; i < count; ++i) {
                double a = base + (i - (count - 1) / 2.0) * 0.22 - 0.25;
                Projectile p;
                p.e.id = nextShotId_++;
                p.InitFromDef(pin, false, {pos.x + w * 0.5, pos.y + h * 0.4}, {std::cos(a), std::sin(a)});
                ctx.Projectiles().push_back(std::move(p));
            }
            ctx.Particles().Sparks({pos.x + w * 0.5, pos.y + h * 0.4}, 0xFF9FD08A, 4);
        }
        if (patT_ >= 0.9) {
            current_ = None;
            cdT_ = std::max(0.35, 0.7 - 0.2 * phase);
        }
        break;
    }
    case Swing: {
        if (patT_ < 0.4) {
            vel.x = (facingRight ? -1.0 : 1.0) * 140.0; // close in
        } else if (patT_ < 0.95) {
            vel.x *= std::max(0.0, 1.0 - 10.0 * dt); // telegraph
        } else if (patT_ - dt < 0.95) {
            // swing frame
            Rect arc = facingRight ? Rect{pos.x + w - 4.0, pos.y + h - 34.0, 52.0, 34.0}
                                   : Rect{pos.x - 48.0, pos.y + h - 34.0, 52.0, 34.0};
            ctx.Emit({Evt::Shake, id, 4, arc.Center(), "swing"});
            ctx.Particles().Dust(arc.Center(), 5);
            if (ctx.player && arc.Overlaps(ctx.player->Box()))
                ctx.player->Damage(1, Center(), ctx, /*hazard=*/false);
        }
        if (patT_ >= 1.2) {
            current_ = None;
            cdT_ = std::max(0.4, 0.8 - 0.25 * phase);
        }
        break;
    }
    default:
        current_ = None;
        break;
    }
}

std::vector<Rect> Patch::TellZones() const {
    std::vector<Rect> out;
    if (current_ == Swing && patT_ >= 0.4 && patT_ < 1.2) {
        out.push_back(facingRight ? Rect{pos.x + w - 4.0, pos.y + h - 34.0, 52.0, 34.0}
                                  : Rect{pos.x - 48.0, pos.y + h - 34.0, 52.0, 34.0});
    }
    return out;
}

std::string Patch::ArtName() const {
    if (inDefeat)
        return "patch_defeat";
    if (transitionT > 0.0)
        return "patch_phase";
    switch (current_) {
    case Needle: return patT_ < 0.5 ? "patch_needle" : "patch_idle";
    case Swing: return patT_ < 0.95 ? "patch_raise" : "patch_slam";
    default: return "patch_idle";
    }
}

int Patch::ArtFrame() const { return static_cast<int>(fightTime * 4.0) % 2 == 0 ? 0 : 1; }

} // namespace ink
