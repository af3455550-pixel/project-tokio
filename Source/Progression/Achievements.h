#pragma once
// Achievements (§58): 50+ roster target; the slice wires 26 real ones.
// Conditions are evaluated off GameEvents + a small counter snapshot, so new
// condition types are cheap to add.
#include "Gameplay/GameEvent.h"
#include <string>
#include <vector>

namespace ink {

// Counters the Game maintains for the run / session.
struct MetaCounters {
    int kills = 0;
    int parries = 0;
    int coins = 0;
    int supers = 0;
    int deaths = 0;
    int questsDone = 0;
    int weaponsOwned = 1;
    int films = 0;
    int frames = 0;
    int stamps = 0;
    int charmsOwned = 0;
    std::string levelId;
    std::string lastBossId;
    std::string lastBossRank;
    double lastBossTime = 0.0;
    bool lastBossNoHit = false;
    double lastLevelTime = 0.0;
    bool lastLevelNoHit = false;
    int levelClears = 0;
};

struct AchievementDef {
    // condition types
    static constexpr int C_FIRST_BLOOD = 0;
    static constexpr int C_KILL_TYPE = 1;   // target = enemy id, amount = n
    static constexpr int C_TOTAL_KILLS = 2;
    static constexpr int C_PARRIES = 3;
    static constexpr int C_COINS = 4;
    static constexpr int C_SUPER = 5;
    static constexpr int C_DEATHS = 6;
    static constexpr int C_BOSS_DEFEAT = 7; // target = boss id
    static constexpr int C_BOSS_RANK = 8;   // target = "bossId|rank"
    static constexpr int C_BOSS_NOHIT = 9;
    static constexpr int C_BOSS_TIME = 10;  // target = boss id, amount = seconds
    static constexpr int C_LEVEL_TIME = 11; // target = level id, amount = seconds
    static constexpr int C_LEVEL_NOHIT = 12;
    static constexpr int C_QUESTS = 13;
    static constexpr int C_WEAPONS = 14;
    static constexpr int C_CHARM = 15;      // equip any charm
    static constexpr int C_COLLECT_TYPE = 16; // target = film|frame|stamp, amount = n
    static constexpr int C_LEVEL_CLEAR = 17;

    std::string id, name, desc;
    bool secret = false;
    int type = C_FIRST_BLOOD;
    std::string target;
    int amount = 1;
};

class AchievementBook {
public:
    bool LoadJson(const std::string& json, std::string* err = nullptr);
    const std::vector<AchievementDef>& All() const { return defs_; }
    const AchievementDef* Def(const std::string& id) const;

    void OnEvent(const GameEvent& e, const MetaCounters& c);
    bool IsUnlocked(const std::string& id) const {
        return std::find(unlocked_.begin(), unlocked_.end(), id) != unlocked_.end();
    }
    void Unlock(const std::string& id) {
        if (!IsUnlocked(id))
            unlocked_.push_back(id);
    }
    void SetUnlocked(const std::vector<std::string>& ids) {
        unlocked_ = ids;
    }
    const std::vector<std::string>& Unlocked() const { return unlocked_; }

private:
    std::vector<AchievementDef> defs_;
    std::vector<std::string> unlocked_;
};

} // namespace ink
