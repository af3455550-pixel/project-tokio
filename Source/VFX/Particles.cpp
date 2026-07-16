#include "VFX/Particles.h"
#include "Core/Rng.h"
#include <cmath>

namespace ink {

ParticleSystem::ParticleSystem() { pool_.resize(kMax); }

void ParticleSystem::Spawn(const Particle& p) {
    for (int n = 0; n < kMax; ++n) {
        int i = (freeIdx_ + n) % kMax;
        if (!pool_[i].alive) {
            pool_[i] = p;
            pool_[i].alive = true;
            freeIdx_ = (i + 1) % kMax;
            return;
        }
    }
    // Pool full: recycle the oldest (lowest remaining life) is overkill; overwrite.
    pool_[freeIdx_] = p;
    pool_[freeIdx_].alive = true;
    freeIdx_ = (freeIdx_ + 1) % kMax;
}

void ParticleSystem::Burst(const Vec2& pos, const BurstConfig& cfg) {
    static thread_local Rng rng(12345);
    for (int i = 0; i < cfg.count; ++i) {
        double ang = (cfg.directionDeg + (rng.unit() - 0.5) * cfg.spreadDeg) * (3.14159265358979 / 180.0);
        double sp = cfg.speedMin + rng.unit() * (cfg.speedMax - cfg.speedMin);
        Particle p;
        p.pos = pos;
        p.vel = {std::cos(ang) * sp, std::sin(ang) * sp};
        p.maxLife = cfg.lifeMin + rng.unit() * (cfg.lifeMax - cfg.lifeMin);
        p.life = p.maxLife;
        p.size = cfg.size;
        p.sizeEnd = cfg.sizeEnd;
        p.color = cfg.color;
        p.gravity = cfg.gravity;
        p.additive = cfg.additive;
        Spawn(p);
    }
}

void ParticleSystem::InkSplash(const Vec2& pos, uint32_t color, int n, double speed) {
    BurstConfig c;
    c.count = n;
    c.speedMin = speed * 0.4;
    c.speedMax = speed;
    c.lifeMin = 0.3;
    c.lifeMax = 0.7;
    c.size = 3.5;
    c.sizeEnd = 0.5;
    c.color = color;
    c.gravity = 620.0;
    Burst(pos, c);
}

void ParticleSystem::Dust(const Vec2& pos, int n) {
    BurstConfig c;
    c.count = n;
    c.speedMin = 20;
    c.speedMax = 70;
    c.lifeMin = 0.2;
    c.lifeMax = 0.45;
    c.size = 3.0;
    c.sizeEnd = 1.5;
    c.color = 0xFFD8C9A3;
    c.gravity = -40.0;
    c.directionDeg = 180.0;
    c.spreadDeg = 160.0;
    Burst(pos, c);
}

void ParticleSystem::Sparks(const Vec2& pos, uint32_t color, int n) {
    BurstConfig c;
    c.count = n;
    c.speedMin = 120;
    c.speedMax = 320;
    c.lifeMin = 0.12;
    c.lifeMax = 0.3;
    c.size = 2.5;
    c.sizeEnd = 1.0;
    c.color = color;
    c.gravity = 500.0;
    c.additive = true;
    Burst(pos, c);
}

void ParticleSystem::Stars(const Vec2& pos, uint32_t color, int n) {
    BurstConfig c;
    c.count = n;
    c.speedMin = 60;
    c.speedMax = 220;
    c.lifeMin = 0.3;
    c.lifeMax = 0.6;
    c.size = 5.0;
    c.sizeEnd = 2.0;
    c.color = color;
    c.gravity = 200.0;
    c.additive = true;
    Burst(pos, c);
}

void ParticleSystem::Smoke(const Vec2& pos, int n) {
    BurstConfig c;
    c.count = n;
    c.speedMin = 10;
    c.speedMax = 50;
    c.lifeMin = 0.5;
    c.lifeMax = 1.0;
    c.size = 6.0;
    c.sizeEnd = 14.0;
    c.color = 0xFF5A5246;
    c.gravity = -60.0;
    c.directionDeg = -90.0;
    c.spreadDeg = 70.0;
    Burst(pos, c);
}

void ParticleSystem::PaperBits(const Vec2& pos, int n) {
    BurstConfig c;
    c.count = n;
    c.speedMin = 60;
    c.speedMax = 240;
    c.lifeMin = 0.5;
    c.lifeMax = 1.1;
    c.size = 4.0;
    c.sizeEnd = 2.0;
    c.color = 0xFFF5EEDC;
    c.gravity = 140.0;
    Burst(pos, c);
}

void ParticleSystem::Trail(const Vec2& pos, uint32_t color) {
    Particle p;
    p.pos = pos;
    p.vel = {0, 0};
    p.maxLife = 0.22;
    p.life = p.maxLife;
    p.size = 5.0;
    p.sizeEnd = 2.0;
    p.color = color;
    p.additive = true;
    Spawn(p);
}

void ParticleSystem::Shockwave(const Vec2& pos, uint32_t color) {
    BurstConfig c;
    c.count = 14;
    c.speedMin = 220;
    c.speedMax = 340;
    c.lifeMin = 0.18;
    c.lifeMax = 0.3;
    c.size = 3.0;
    c.sizeEnd = 1.0;
    c.color = color;
    c.gravity = 0.0;
    c.directionDeg = 0.0;
    c.spreadDeg = 360.0;
    c.additive = true;
    Burst(pos, c);
}

void ParticleSystem::Update(double dt) {
    renders_.clear();
    for (auto& p : pool_) {
        if (!p.alive)
            continue;
        p.life -= dt;
        if (p.life <= 0.0) {
            p.alive = false;
            continue;
        }
        p.vel.y += p.gravity * dt;
        p.pos += p.vel * dt;
        double t = 1.0 - (p.life / p.maxLife);
        double s = Lerp(p.size, p.sizeEnd, t);
        uint8_t alpha = static_cast<uint8_t>(255.0 * (p.life / p.maxLife));
        uint32_t col = (p.color & 0x00FFFFFF) | (alpha << 24);
        renders_.push_back({p.pos.x - s * 0.5, p.pos.y - s * 0.5, s, s, col, 1.0, p.additive});
    }
}

void ParticleSystem::Clear() {
    for (auto& p : pool_)
        p.alive = false;
    freeIdx_ = 0;
    renders_.clear();
}

int ParticleSystem::AliveCount() const {
    int n = 0;
    for (const auto& p : pool_)
        if (p.alive)
            ++n;
    return n;
}

} // namespace ink
