#pragma once
// Pooled hand-drawn-style particles (§45). Pure logic: the simulation owns a
// ParticleSystem (tests count spawns); the app renders Particles::Renders().
#include "Core/Math.h"
#include <cstdint>
#include <vector>

namespace ink {

struct Particle {
    Vec2 pos;
    Vec2 vel;
    double life = 0.0;
    double maxLife = 0.5;
    double size = 4.0;
    double sizeEnd = 1.0;
    uint32_t color = 0xFFFFFFFF;
    double gravity = 0.0;
    bool additive = false;
    bool alive = false;
};

struct ParticleRender {
    double x, y, w, h;
    uint32_t color;
    double alpha;
    bool additive;
};

struct BurstConfig {
    int count = 8;
    double speedMin = 40.0;
    double speedMax = 140.0;
    double lifeMin = 0.25;
    double lifeMax = 0.6;
    double size = 4.0;
    double sizeEnd = 1.0;
    uint32_t color = 0xFF202030;
    double gravity = 300.0;
    double spreadDeg = 360.0;
    double directionDeg = 0.0;
    bool additive = false;
};

class ParticleSystem {
public:
    static constexpr int kMax = 1024;
    ParticleSystem();

    void Spawn(const Particle& p);
    void Burst(const Vec2& pos, const BurstConfig& cfg);
    // Convenience presets (art direction, §45):
    void InkSplash(const Vec2& pos, uint32_t color, int n = 8, double speed = 150.0);
    void Dust(const Vec2& pos, int n = 4);
    void Sparks(const Vec2& pos, uint32_t color, int n = 10);
    void Stars(const Vec2& pos, uint32_t color, int n = 6);
    void Smoke(const Vec2& pos, int n = 5);
    void PaperBits(const Vec2& pos, int n = 8);
    void Trail(const Vec2& pos, uint32_t color);
    void Shockwave(const Vec2& pos, uint32_t color);

    void Update(double dt);
    void Clear();
    int AliveCount() const;

    // Built each Update; the renderer blits these.
    const std::vector<ParticleRender>& Renders() const { return renders_; }

private:
    std::vector<Particle> pool_;
    std::vector<ParticleRender> renders_;
    int freeIdx_ = 0;
};

} // namespace ink
