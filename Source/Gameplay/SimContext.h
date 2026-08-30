#pragma once
// The per-frame simulation context handed to every system. Owning pointers
// (set by the app or a test harness) keep the sim decoupled from the app and
// free of global state (§5).
#include "Core/Event.h"
#include "Core/Math.h"
#include "Core/Rng.h"
#include "Gameplay/GameEvent.h"
#include "VFX/Particles.h"
#include <vector>

namespace ink {

struct Projectile;
class Level;
class Player;
class Enemy;
class Boss;

struct SimContext {
    Level* level = nullptr;
    Player* player = nullptr;
    std::vector<Enemy*>* enemies = nullptr; // live enemy instances
    Boss* boss = nullptr;                    // active boss (nullptr when absent)
    Rng* rng = nullptr;
    Event<GameEvent>* events = nullptr;
    ParticleSystem* particles = nullptr;
    std::vector<Projectile>* projectiles = nullptr;

    double time = 0.0;
    int frame = 0;
    double timescale = 1.0;
    bool godMode = false;

    void Emit(const GameEvent& e) {
        if (events)
            events->Emit(e);
    }
    ParticleSystem& Particles() { return *particles; }
    std::vector<Projectile>& Projectiles() { return *projectiles; }
    Rng& GetRng() { return *rng; }
    std::vector<Enemy*>& Enemies() { return *enemies; }
};

} // namespace ink
