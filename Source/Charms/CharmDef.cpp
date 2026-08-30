#include "Charms/CharmDef.h"
#include "Core/Json.h"

namespace ink {

static CharmMods ModsFromJson(const Json* j) {
    CharmMods m;
    if (!j)
        return m;
    auto d = [&](const char* k, double* dst) {
        const Json* v = j->Find(k);
        if (v)
            *dst = v->AsNumber(*dst);
    };
    auto i = [&](const char* k, int* dst) {
        const Json* v = j->Find(k);
        if (v)
            *dst = v->AsInt(*dst);
    };
    auto b = [&](const char* k, bool* dst) {
        const Json* v = j->Find(k);
        if (v)
            *dst = v->AsBool(*dst);
    };
    i("maxHpAdd", &m.maxHpAdd);
    d("moveSpeedMul", &m.moveSpeedMul);
    d("jumpMul", &m.jumpMul);
    d("damageMul", &m.damageMul);
    d("dmgTakenMul", &m.dmgTakenMul);
    d("dashCooldownMul", &m.dashCooldownMul);
    d("energyGainMul", &m.energyGainMul);
    d("parryWindowMul", &m.parryWindowMul);
    d("invulnMul", &m.invulnMul);
    i("airDashAdd", &m.airDashAdd);
    d("coinMul", &m.coinMul);
    b("reviveOnce", &m.reviveOnce);
    return m;
}

bool CharmBook::LoadJson(const std::string& json, std::string* err) {
    charms_.clear();
    Json root = Json::Parse(json, err);
    const Json* arr = root.IsArray() ? &root : root.Find("charms");
    if (!arr || !arr->IsArray()) {
        if (err)
            *err = "charms.json: expected array of charms";
        return false;
    }
    for (const auto& jc : arr->Items()) {
        CharmDef c;
        c.id = jc.Find("id") ? jc.Find("id")->AsString() : "charm";
        c.name = jc.Find("name") ? jc.Find("name")->AsString() : c.id;
        c.description = jc.Find("desc") ? jc.Find("desc")->AsString() : "";
        c.mods = ModsFromJson(jc.Find("mods"));
        charms_.push_back(std::move(c));
    }
    return !charms_.empty();
}

const CharmDef* CharmBook::Get(const std::string& id) const {
    for (const auto& c : charms_)
        if (c.id == id)
            return &c;
    return nullptr;
}

PlayerStats CharmBook::Apply(const PlayerStats& base, const std::vector<const CharmDef*>& active) {
    PlayerStats s = base;
    s.maxHp = base.maxHp;
    for (const auto* c : active) {
        s.maxHp += c->mods.maxHpAdd;
        s.walkSpeed *= c->mods.moveSpeedMul;
        s.runSpeed *= c->mods.moveSpeedMul;
        s.jumpVel *= c->mods.jumpMul;
        s.damageMul *= c->mods.damageMul;
        s.dmgTakenMul *= c->mods.dmgTakenMul;
        s.dashCooldown *= c->mods.dashCooldownMul;
        s.energyGainMul *= c->mods.energyGainMul;
        s.parryActiveFrom = s.parryActiveFrom / c->mods.parryWindowMul;
        s.parryActiveTo = s.parryActiveTo / c->mods.parryWindowMul;
        s.invulnTime *= c->mods.invulnMul;
        s.airDashCount += c->mods.airDashAdd;
        s.coinMul *= c->mods.coinMul;
    }
    s.maxHp = std::max(1, s.maxHp);
    s.airDashCount = std::max(0, s.airDashCount);
    return s;
}

} // namespace ink
