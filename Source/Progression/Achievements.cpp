#include "Progression/Achievements.h"
#include "Core/Json.h"
#include <algorithm>

namespace ink {

static AchievementDef AchFromJson(const Json& j) {
    AchievementDef a;
    a.id = j.Find("id") ? j.Find("id")->AsString() : "ach";
    a.name = j.Find("name") ? j.Find("name")->AsString() : a.id;
    a.desc = j.Find("desc") ? j.Find("desc")->AsString() : "";
    a.secret = j.Find("secret") ? j.Find("secret")->AsBool(false) : false;
    a.type = j.Find("type") ? j.Find("type")->AsInt(AchievementDef::C_FIRST_BLOOD) : AchievementDef::C_FIRST_BLOOD;
    a.target = j.Find("target") ? j.Find("target")->AsString("") : "";
    a.amount = j.Find("amount") ? j.Find("amount")->AsInt(1) : 1;
    return a;
}

bool AchievementBook::LoadJson(const std::string& json, std::string* err) {
    defs_.clear();
    Json root = Json::Parse(json, err);
    const Json* arr = root.IsArray() ? &root : root.Find("achievements");
    if (!arr || !arr->IsArray()) {
        if (err)
            *err = "achievements.json: expected array";
        return false;
    }
    for (const auto& j : arr->Items())
        defs_.push_back(AchFromJson(j));
    return !defs_.empty();
}

const AchievementDef* AchievementBook::Def(const std::string& id) const {
    for (const auto& d : defs_)
        if (d.id == id)
            return &d;
    return nullptr;
}

void AchievementBook::OnEvent(const GameEvent& e, const MetaCounters& c) {
    for (const auto& a : defs_) {
        if (IsUnlocked(a.id))
            continue;
        bool done = false;
        switch (a.type) {
        case AchievementDef::C_FIRST_BLOOD:
            done = (e.type == Evt::EnemyKilled);
            break;
        case AchievementDef::C_KILL_TYPE:
            done = (e.type == Evt::EnemyKilled && e.name == a.target) && c.kills >= a.amount;
            break;
        case AchievementDef::C_TOTAL_KILLS:
            done = c.kills >= a.amount;
            break;
        case AchievementDef::C_PARRIES:
            done = c.parries >= a.amount;
            break;
        case AchievementDef::C_COINS:
            done = c.coins >= a.amount;
            break;
        case AchievementDef::C_SUPER:
            done = c.supers >= a.amount;
            break;
        case AchievementDef::C_DEATHS:
            done = c.deaths >= a.amount;
            break;
        case AchievementDef::C_BOSS_DEFEAT:
            done = (e.type == Evt::BossDefeated) && (a.target.empty() || e.name == a.target);
            break;
        case AchievementDef::C_BOSS_RANK: {
            if (e.type != Evt::BossDefeated)
                break;
            // target = "bossId|rank"
            auto bar = a.target.find('|');
            if (bar == std::string::npos)
                break;
            std::string bid = a.target.substr(0, bar);
            std::string rank = a.target.substr(bar + 1);
            done = (e.name == bid) && (c.lastBossRank == rank);
            break;
        }
        case AchievementDef::C_BOSS_NOHIT:
            done = (e.type == Evt::BossDefeated) && c.lastBossNoHit;
            break;
        case AchievementDef::C_BOSS_TIME:
            done = (e.type == Evt::BossDefeated) && (a.target.empty() || e.name == a.target) &&
                   c.lastBossTime > 0.0 && c.lastBossTime <= a.amount;
            break;
        case AchievementDef::C_LEVEL_TIME:
            done = (e.type == Evt::LevelComplete) && (a.target.empty() || e.name == a.target) &&
                   c.lastLevelTime > 0.0 && c.lastLevelTime <= a.amount;
            break;
        case AchievementDef::C_LEVEL_NOHIT:
            done = (e.type == Evt::LevelComplete) && c.lastLevelNoHit;
            break;
        case AchievementDef::C_QUESTS:
            done = c.questsDone >= a.amount;
            break;
        case AchievementDef::C_WEAPONS:
            done = c.weaponsOwned >= a.amount;
            break;
        case AchievementDef::C_CHARM:
            done = c.charmsOwned >= a.amount;
            break;
        case AchievementDef::C_COLLECT_TYPE:
            done = (a.target == "film" && c.films >= a.amount) ||
                   (a.target == "frame" && c.frames >= a.amount) ||
                   (a.target == "stamp" && c.stamps >= a.amount);
            break;
        case AchievementDef::C_LEVEL_CLEAR:
            done = c.levelClears >= a.amount;
            break;
        default:
            break;
        }
        if (done) {
            Unlock(a.id);
            // The Game diffs Unlocked() after OnEvent to emit toasts (§58).
        }
    }
}

} // namespace ink
