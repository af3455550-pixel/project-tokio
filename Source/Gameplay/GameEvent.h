#pragma once
// Single typed event flow for the whole simulation. The app subscribes once
// and maps events to SFX / VFX / camera / music / UI toasts; tests subscribe
// a logger. This is the seam that keeps the sim SDL-free and testable.
#include "Core/Math.h"
#include <string>

namespace ink {

enum class Evt {
    // player
    PlayerHurt,        // amount = damage, pos = hit point
    PlayerHealed,      // amount = hp gained
    PlayerDied,
    PlayerRespawn,
    SuperUsed,
    SuperCharged,      // energy reached 100
    ParrySuccess,      // amount = parry type (0 projectile, 1 melee, 2 chain)
    ComboChanged,      // amount = combo counter
    // enemies / combat
    EnemyKilled,       // id, name
    PlayerHitEnemy,    // id, amount = damage dealt
    ProjectileExplode, // pos
    // pickups
    CoinPicked,        // amount = value
    FilmPicked,        // name = film strip id
    MasterFramePicked, // name = frame id
    StampPicked,       // name = stamp id
    // world
    CheckpointReached, // id
    BossIntro,
    BossPhase,         // id = new phase index
    BossDefeated,
    LevelComplete,
    // meta
    QuestUpdated,      // name = quest id
    QuestCompleted,    // name = quest id
    AchievementUnlocked, // name = achievement id
    NpcTalk,           // name = npc id
    // game feel (§13)
    Shake,             // amount = shake intensity
    HitStop,           // amount = stop duration * 1000
    SlowMo,            // amount = timescale * 100 (0 = normal)
};

struct GameEvent {
    Evt type;
    int id = -1;
    int amount = 0;
    Vec2 pos{0.0, 0.0};
    std::string name;
};

} // namespace ink
