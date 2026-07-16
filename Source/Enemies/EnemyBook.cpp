#include "Enemies/EnemyBook.h"
#include "Core/Json.h"

namespace ink {

static EnemyDef DefFromJson(const Json& j) {
    EnemyDef d;
    d.id = j.Find("id") ? j.Find("id")->AsString() : "enemy";
    d.name = j.Find("name") ? j.Find("name")->AsString() : d.id;
    d.sprite = j.Find("sprite") ? j.Find("sprite")->AsString() : d.id;
    auto n = [&](const char* k, double* dst) {
        if (const Json* v = j.Find(k))
            *dst = v->AsNumber(*dst);
    };
    auto i = [&](const char* k, int* dst) {
        if (const Json* v = j.Find(k))
            *dst = v->AsInt(*dst);
    };
    auto b = [&](const char* k, bool* dst) {
        if (const Json* v = j.Find(k))
            *dst = v->AsBool(*dst);
    };
    i("hp", &d.hp);
    n("w", &d.w);
    n("h", &d.h);
    n("speed", &d.speed);
    n("attackRange", &d.attackRange);
    i("attackDamage", &d.attackDamage);
    n("detectRange", &d.detectRange);
    n("reactTime", &d.reactTime);
    n("attackCooldown", &d.attackCooldown);
    n("recoverTime", &d.recoverTime);
    n("stunDuration", &d.stunDuration);
    b("flying", &d.flying);
    b("ranged", &d.ranged);
    b("swarm", &d.swarm);
    b("canRetreat", &d.canRetreat);
    i("score", &d.score);
    i("energyOnKill", &d.energyOnKill);
    n("coinChance", &d.coinChance);
    return d;
}

bool EnemyBook::LoadJson(const std::string& json, std::string* err) {
    defs_.clear();
    Json root = Json::Parse(json, err);
    const Json* arr = root.IsArray() ? &root : root.Find("enemies");
    if (!arr || !arr->IsArray()) {
        if (err)
            *err = "enemies.json: expected array of enemies";
        return false;
    }
    for (const auto& j : arr->Items())
        defs_.push_back(DefFromJson(j));
    return !defs_.empty();
}

const EnemyDef* EnemyBook::Get(const std::string& id) const {
    for (const auto& d : defs_)
        if (d.id == id)
            return &d;
    return nullptr;
}

} // namespace ink
