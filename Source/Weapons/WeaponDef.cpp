#include "Weapons/WeaponDef.h"
#include "Core/Json.h"
#include <algorithm>

namespace ink {

void LoadProjectileDef(const Json& j, ProjectileDef& out) {
    auto num = [&](const char* k, double* dst) {
        const Json* v = j.Find(k);
        if (v)
            *dst = v->AsNumber(*dst);
    };
    auto str = [&](const char* k, std::string* dst) {
        const Json* v = j.Find(k);
        if (v && v->IsString())
            *dst = v->AsString();
    };
    auto bl = [&](const char* k, bool* dst) {
        const Json* v = j.Find(k);
        if (v)
            *dst = v->AsBool(*dst);
    };
    auto in = [&](const char* k, int* dst) {
        const Json* v = j.Find(k);
        if (v)
            *dst = v->AsInt(*dst);
    };
    str("id", &out.id);
    str("vfx", &out.vfx);
    num("speed", &out.speed);
    num("gravity", &out.gravity);
    num("life", &out.life);
    num("radius", &out.radius);
    num("damage", &out.damage);
    num("knockback", &out.knockback);
    in("pierce", &out.pierce);
    in("ricochet", &out.ricochet);
    num("homingTurn", &out.homingTurn);
    num("homingMax", &out.homingMax);
    bl("explosive", &out.explosive);
    num("explodeRadius", &out.explodeRadius);
    num("explodeDamage", &out.explodeDamage);
    bl("parryable", &out.parryable);
    num("spreadDeg", &out.spreadDeg);
    in("count", &out.count);
    in("score", &out.score);
}

static WeaponDef MakeBlaster() {
    WeaponDef w;
    w.id = "ink_blaster";
    w.name = "Ink Blaster";
    w.fireCooldown = 0.15;
    w.shot = {};
    w.charged = w.shot;
    w.charged.speed = 640.0;
    w.charged.damage = 2.0;
    w.charged.radius = 6.0;
    w.charged.pierce = 1;
    w.charged.knockback = 260.0;
    w.charged.vfx = "ink_charged";
    w.charged.score = 20;
    return w;
}

bool WeaponBook::LoadJson(const std::string& json, std::string* err) {
    weapons_.clear();
    names_.clear();
    defaultIdx_ = 0;
    Json root = Json::Parse(json, err);
    const Json* arr = root.IsArray() ? &root : root.Find("weapons");
    if (!arr || !arr->IsArray()) {
        if (err)
            *err = "weapons.json: expected array of weapons";
        return false;
    }
    for (const auto& jw : arr->Items()) {
        WeaponDef w = MakeBlaster();
        w.id = jw.Find("id") ? jw.Find("id")->AsString() : "weapon";
        w.name = jw.Find("name") ? jw.Find("name")->AsString() : w.id;
        if (const Json* v = jw.Find("fireCooldown"))
            w.fireCooldown = v->AsNumber(w.fireCooldown);
        if (const Json* v = jw.Find("hasCharged"))
            w.hasCharged = v->AsBool(w.hasCharged);
        if (const Json* v = jw.Find("chargeTime"))
            w.chargeTime = v->AsNumber(w.chargeTime);
        if (const Json* v = jw.Find("energyOnHit"))
            w.energyOnHit = v->AsInt(w.energyOnHit);
        if (const Json* v = jw.Find("shot"))
            LoadProjectileDef(*v, w.shot);
        if (const Json* v = jw.Find("charged"))
            LoadProjectileDef(*v, w.charged);
        weapons_.push_back(std::move(w));
        names_.push_back(weapons_.back().id);
    }
    if (weapons_.empty())
        return false;
    for (int i = 0; i < static_cast<int>(weapons_.size()); ++i)
        if (weapons_[i].id == "ink_blaster")
            defaultIdx_ = i;
    return true;
}

const WeaponDef* WeaponBook::Get(const std::string& id) const {
    for (const auto& w : weapons_)
        if (w.id == id)
            return &w;
    return nullptr;
}

const WeaponDef& WeaponBook::Default() const { return weapons_[defaultIdx_]; }

} // namespace ink
