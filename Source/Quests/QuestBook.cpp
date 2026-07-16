#include "Quests/QuestBook.h"
#include "Core/Json.h"
#include <algorithm>

namespace ink {

static QuestObjective ObjectiveFromJson(const Json& j) {
    QuestObjective o;
    std::string t = j.Find("type") ? j.Find("type")->AsString("coins") : "coins";
    if (t == "kills")
        o.type = QuestObjective::Kills;
    else if (t == "parries")
        o.type = QuestObjective::Parries;
    else if (t == "boss")
        o.type = QuestObjective::BossDefeat;
    else if (t == "collect")
        o.type = QuestObjective::Collect;
    else
        o.type = QuestObjective::Coins;
    o.target = j.Find("target") ? j.Find("target")->AsString("") : "";
    o.count = j.Find("count") ? j.Find("count")->AsInt(1) : 1;
    return o;
}

bool QuestBook::LoadJson(const std::string& json, std::string* err) {
    defs_.clear();
    Json root = Json::Parse(json, err);
    const Json* arr = root.IsArray() ? &root : root.Find("quests");
    if (!arr || !arr->IsArray()) {
        if (err)
            *err = "quests.json: expected array of quests";
        return false;
    }
    for (const auto& jq : arr->Items()) {
        QuestDef q;
        q.id = jq.Find("id") ? jq.Find("id")->AsString() : "quest";
        q.title = jq.Find("title") ? jq.Find("title")->AsString() : q.id;
        q.desc = jq.Find("desc") ? jq.Find("desc")->AsString() : "";
        q.objective = ObjectiveFromJson(jq.Find("objective") ? *jq.Find("objective") : Json());
        q.rewardCoins = jq.Find("rewardCoins") ? jq.Find("rewardCoins")->AsInt(0) : 0;
        q.rewardCharm = jq.Find("rewardCharm") ? jq.Find("rewardCharm")->AsString("") : "";
        q.rewardWeapon = jq.Find("rewardWeapon") ? jq.Find("rewardWeapon")->AsString("") : "";
        states_.push_back({q.id, 0, false, false});
        defs_.push_back(std::move(q));
    }
    return !defs_.empty();
}

const QuestDef* QuestBook::Def(const std::string& id) const {
    for (const auto& d : defs_)
        if (d.id == id)
            return &d;
    return nullptr;
}

int QuestBook::ActiveCount() const {
    int n = 0;
    for (const auto& s : states_)
        if (s.active && !s.done)
            ++n;
    return n;
}

void QuestBook::StartQuest(const std::string& id) {
    for (auto& s : states_)
        if (s.id == id && !s.active && !s.done)
            s.active = true;
}

void QuestBook::ResetForLevel() {
    // Quests persist across levels (they are not per-level state).
}

void QuestBook::OnEvent(const GameEvent& e) {
    for (auto& s : states_) {
        if (!s.active || s.done)
            continue;
        const QuestDef* q = Def(s.id);
        if (!q)
            continue;
        AddProgress(*q, e, 1);
    }
}

bool QuestBook::IsDone(const std::string& id) const {
    for (const auto& s : states_)
        if (s.id == id)
            return s.done;
    return false;
}

void QuestBook::AddProgress(const QuestDef& q, const GameEvent& e, int amount) {
    bool match = false;
    switch (q.objective.type) {
    case QuestObjective::Coins:
        match = e.type == Evt::CoinPicked;
        break;
    case QuestObjective::Kills:
        match = e.type == Evt::EnemyKilled &&
                (q.objective.target.empty() || e.name == q.objective.target);
        break;
    case QuestObjective::Parries:
        match = e.type == Evt::ParrySuccess;
        break;
    case QuestObjective::BossDefeat:
        match = e.type == Evt::BossDefeated &&
                (q.objective.target.empty() || e.name == q.objective.target);
        break;
    case QuestObjective::Collect:
        match = (e.type == Evt::FilmPicked || e.type == Evt::StampPicked || e.type == Evt::MasterFramePicked) &&
                (q.objective.target.empty() || e.name == q.objective.target);
        break;
    }
    if (!match)
        return;
    for (auto& st : states_) {
        if (st.id != q.id || st.done)
            continue;
        st.progress = std::min(q.objective.count, st.progress + amount);
        if (st.progress >= q.objective.count) {
            st.done = true;
            if (events_)
                events_->Emit({Evt::QuestCompleted, -1, q.rewardCoins, {0, 0}, q.id});
        }
    }
}

} // namespace ink
