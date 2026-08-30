#pragma once
// Data-driven weapons (§15). WeaponDef + ProjectileDef are plain data loaded
// from JSON; the firing/homing/pierce/ricochet/explosion *behaviors* live in
// Gameplay/Projectile. Adding a new weapon = new JSON entry, no code change.
#include <string>
#include <vector>

class Json;

namespace ink {

struct ProjectileDef {
    std::string id = "ink";
    std::string vfx = "ink";
    double speed = 440.0;
    double gravity = 0.0;
    double life = 1.5;
    double radius = 4.0;
    double damage = 1.0;
    double knockback = 140.0;
    int pierce = 0;      // extra enemies it can pass through
    int ricochet = 0;    // wall bounces
    double homingTurn = 0.0; // deg/s toward nearest enemy
    double homingMax = 40.0; // max deviation from original direction
    bool explosive = false;
    double explodeRadius = 46.0;
    double explodeDamage = 1.0;
    bool parryable = true; // enemy projectiles: can the player parry them
    double spreadDeg = 0.0;
    int count = 1;         // projectiles per shot (spread)
    int score = 10;
};

struct WeaponDef {
    std::string id = "ink_blaster";
    std::string name = "Ink Blaster";
    double fireCooldown = 0.15;
    bool hasCharged = true;
    double chargeTime = 0.45;
    ProjectileDef shot;
    ProjectileDef charged;
    int energyOnHit = 2;
};

class WeaponBook {
public:
    bool LoadJson(const std::string& json, std::string* err = nullptr);
    const WeaponDef* Get(const std::string& id) const;
    const WeaponDef& Default() const;
    const std::vector<WeaponDef>& All() const { return weapons_; }
    const std::vector<std::string>& Names() const { return names_; }

private:
    std::vector<WeaponDef> weapons_;
    std::vector<std::string> names_;
    int defaultIdx_ = 0;
};

void LoadProjectileDef(const Json& j, ProjectileDef& out);

} // namespace ink
