#pragma once
// Bullets for both player and enemies. Behaviors (homing, pierce, ricochet,
// explosive, parryable) are data-driven via ProjectileDef (§15).
#include "Gameplay/Entity.h"
#include "Weapons/WeaponDef.h"
namespace ink { struct SimContext; }

namespace ink {

struct Projectile {
    Entity e;
    ProjectileDef def;
    bool fromPlayer = true;
    int hitsLeft = 1;
    int ricochetsLeft = 0;
    double life = 1.5;
    Vec2 originVelDir{1.0, 0.0};
    bool dead = false;

    void InitFromDef(const ProjectileDef& d, bool fromPlayer, Vec2 pos, Vec2 dir);
    void Update(double dt, SimContext& ctx);

    // Damage application helpers used by both directions.
    void Explode(SimContext& ctx);
    Rect Area() const {
        double r = def.radius;
        return {e.pos.x - r, e.pos.y - r, r * 2.0, r * 2.0};
    }
};

} // namespace ink
