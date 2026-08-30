#include "Gameplay/Projectile.h"
#include "Gameplay/Level.h"
#include "Gameplay/SimContext.h"
#include "Player/Player.h"
#include "Enemies/Enemy.h"
#include "Bosses/Boss.h"
#include "Physics/Collision.h"
#include <cmath>

namespace ink {

void Projectile::InitFromDef(const ProjectileDef& d, bool fp, Vec2 pos, Vec2 dir) {
    def = d;
    fromPlayer = fp;
    e.id = -1;
    e.pos = pos;
    e.w = d.radius * 2.0;
    e.h = d.radius * 2.0;
    e.facingRight = dir.x >= 0.0;
    e.alive = true;
    dead = false;
    double len = dir.LengthSq() > 1e-9 ? dir.Length() : 1.0;
    Vec2 nd = len > 1e-9 ? dir / len : Vec2{1.0, 0.0};
    e.vel = nd * d.speed;
    originVelDir = nd;
    life = d.life;
    hitsLeft = 1 + d.pierce;
    ricochetsLeft = d.ricochet;
}

void Projectile::Explode(SimContext& ctx) {
    ctx.Emit({Evt::ProjectileExplode, -1, 0, e.Center(), def.id});
    ctx.Particles().InkSplash(e.Center(), 0xFF3A2A18, 14, 220);
    ctx.Particles().Sparks(e.Center(), 0xFFE8B84B, 8);
    if (fromPlayer && def.explodeRadius > 0.0 && ctx.player) {
        double dmg = def.explodeDamage * ctx.player->Stats().damageMul;
        for (Enemy* en : ctx.Enemies()) {
            if (!en || !en->alive)
                continue;
            if (Dist(e.Center(), en->Center()) < def.explodeRadius) {
                en->TakeDamage(dmg, (en->Center() - e.Center()).Normalized() * 220.0, ctx);
            }
        }
        if (ctx.boss && ctx.boss->alive && Dist(e.Center(), ctx.boss->Center()) < def.explodeRadius + 20.0)
            ctx.boss->TakeDamage(dmg, (ctx.boss->Center() - e.Center()).Normalized() * 60.0, ctx);
    }
}

void Projectile::Update(double dt, SimContext& ctx) {
    if (dead)
        return;
    life -= dt;
    if (life <= 0.0) {
        if (def.explosive)
            Explode(ctx);
        dead = true;
        return;
    }

    // Homing (limited, §14 TRACKING SHOT)
    if (def.homingTurn > 0.0 && fromPlayer) {
        Actor* target = nullptr;
        double best = 220.0;
        for (Enemy* en : ctx.Enemies()) {
            if (!en || !en->alive)
                continue;
            double d = Dist(e.Center(), en->Center());
            if (d < best) {
                best = d;
                target = en;
            }
        }
        if (ctx.boss && ctx.boss->alive) {
            double d = Dist(e.Center(), ctx.boss->Center());
            if (d < best)
                target = ctx.boss;
        }
        if (target) {
            double sp = e.vel.Length();
            Vec2 desired = (target->Center() - e.Center()).Normalized() * sp;
            double curAng = std::atan2(e.vel.y, e.vel.x);
            double desAng = std::atan2(desired.y, desired.x);
            double diff = desAng - curAng;
            while (diff > 3.14159265) diff -= 6.2831853;
            while (diff < -3.14159265) diff += 6.2831853;
            double maxTurn = def.homingTurn * (3.14159265 / 180.0) * dt;
            double turn = Clamp(diff, -maxTurn, maxTurn);
            double newAng = curAng + turn;
            double originAng = std::atan2(originVelDir.y, originVelDir.x);
            double devFromOrigin = newAng - originAng;
            while (devFromOrigin > 3.14159265) devFromOrigin -= 6.2831853;
            while (devFromOrigin < -3.14159265) devFromOrigin += 6.2831853;
            double maxDev = def.homingMax * (3.14159265 / 180.0);
            if (std::abs(devFromOrigin) > maxDev)
                newAng = originAng + (devFromOrigin > 0.0 ? maxDev : -maxDev);
            e.vel = {std::cos(newAng) * sp, std::sin(newAng) * sp};
        }
    }

    e.vel.y += def.gravity * dt;
    e.pos += e.vel * dt;

    // Tile collision
    if (ctx.level) {
        const Rect box = Area();
        if (OverlapsSolid(box, *ctx.level)) {
            if (def.explosive) {
                Explode(ctx);
                dead = true;
                return;
            }
            if (ricochetsLeft > 0) {
                // Probe which axis blocked us (nudge back slightly and re-test).
                double px = e.vel.x * 0.01;
                double py = e.vel.y * 0.01;
                Rect bx{box.x + (e.vel.x > 0 ? -px : px), box.y, box.w, box.h};
                if (OverlapsSolid(bx, *ctx.level))
                    e.vel.x = -e.vel.x;
                Rect by{box.x, box.y + (e.vel.y > 0 ? -py : py), box.w, box.h};
                if (OverlapsSolid(by, *ctx.level))
                    e.vel.y = -e.vel.y;
                e.pos += e.vel * 0.02;
                --ricochetsLeft;
                ctx.Particles().Sparks(e.Center(), 0xFFE8B84B, 4);
            } else {
                ctx.Particles().InkSplash(e.Center(), fromPlayer ? 0xFF26213C : 0xFF7A2E1E, 6, 120);
                dead = true;
                return;
            }
        }
    }

    // Entity collisions
    const Rect box = Area();
    if (fromPlayer) {
        for (Enemy* en : ctx.Enemies()) {
            if (!en || !en->alive)
                continue;
            if (!box.Overlaps(en->Box()))
                continue;
            en->TakeDamage(def.damage * ctx.player->Stats().damageMul, e.vel.Normalized() * def.knockback, ctx);
            ctx.Emit({Evt::PlayerHitEnemy, en->id, static_cast<int>(def.damage), en->Center(),
                      en->def ? en->def->id : ""});
            ctx.Particles().InkSplash(en->Center(), 0xFF26213C, 6, 140);
            if (def.explosive) {
                Explode(ctx);
                dead = true;
                return;
            }
            --hitsLeft;
            if (hitsLeft <= 0) {
                dead = true;
                return;
            }
        }
        if (ctx.boss && ctx.boss->alive && box.Overlaps(ctx.boss->Box())) {
            ctx.boss->TakeDamage(def.damage * ctx.player->Stats().damageMul,
                                 e.vel.Normalized() * def.knockback * 0.4, ctx);
            ctx.Emit({Evt::PlayerHitEnemy, ctx.boss->id, static_cast<int>(def.damage),
                      ctx.boss->Center(), ctx.boss->def ? ctx.boss->def->id : ""});
            if (def.explosive) {
                Explode(ctx);
                dead = true;
                return;
            }
            --hitsLeft;
            if (hitsLeft <= 0)
                dead = true;
        }
    } else if (ctx.player && ctx.player->alive && box.Overlaps(ctx.player->Box())) {
        // Enemy projectile: the player can parry it (projectile parry, §16).
        bool parried = ctx.player->TryParryHit(*this, ctx);
        if (!parried)
            ctx.player->Damage(1, e.Center(), ctx, /*hazard=*/false);
        ctx.Particles().InkSplash(e.Center(), 0xFF7A2E1E, 6, 120);
        dead = true;
    }
}

} // namespace ink
