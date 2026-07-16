#pragma once
// Entity base + damageable Actor. All gameplay bodies derive from these.
#include "Core/Math.h"
#include <cstdint>

namespace ink {

struct Entity {
    int id = -1;
    Vec2 pos; // top-left corner
    Vec2 vel;
    double w = 16.0, h = 16.0;
    bool facingRight = true;
    bool alive = true;
    double timeAlive = 0.0;

    Rect Box() const { return {pos.x, pos.y, w, h}; }
    Vec2 Center() const { return {pos.x + w * 0.5, pos.y + h * 0.5}; }
};

struct Actor : Entity {
    int hp = 1;
    int maxHp = 1;
    double invulnT = 0.0;
    double hitFlashT = 0.0;
    bool canDamagePlayer = true;
    int score = 0;

    void TickTimers(double dt) {
        if (invulnT > 0.0)
            invulnT -= dt;
        if (hitFlashT > 0.0)
            hitFlashT -= dt;
    }
};

} // namespace ink
