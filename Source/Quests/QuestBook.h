#pragma once
// Optional quests (§39): data-driven objectives tracked off GameEvents.
// Rewards (coins / charms / weapons) are applied by the Game on completion.
#include "Core/Event.h"
#include "Gameplay/GameEvent.h"
#include <string>
#include <vector>

namespace ink {

struct QuestObjective {
    enum Type { Coins, Kills, Parries, BossDefeat, Collect } type = Coins;
    std::string target; // enemy id / boss id / collectible type; empty = any
    int count = 1;
};

struct QuestDef {
    std::string id, title, desc;
    QuestObjective objective;
    int rewardCoins = 0;
    std::string rewardCharm;
    std::string rewardWeapon;
};

struct QuestState {
    std::string id;
    int progress = 0;
    bool active = false;
    bool done = false;
};

class QuestBook {
public:
    bool LoadJson(const std::string& json, std::string* err = nullptr);
    const QuestDef* Def(const std::string& id) const;
    std::vector<QuestState>& States() { return states_; }
    const std::vector<QuestState>& States() const { return states_; }
    int ActiveCount() const;

    void StartQuest(const std::string& id);
    void ResetForLevel();
    void OnEvent(const GameEvent& e);
    void SetEvents(Event<GameEvent>* ev) { events_ = ev; }
    bool IsDone(const std::string& id) const;

private:
    void AddProgress(const QuestDef& q, const GameEvent& e, int amount);
    std::vector<QuestDef> defs_;
    std::vector<QuestState> states_;
    Event<GameEvent>* events_ = nullptr;
};

} // namespace ink
