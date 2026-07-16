#include "Gameplay/Level.h"
#include "Core/Log.h"
#include <cmath>

namespace ink {

void Level::Load(const LevelData& data) { SetData(data); }

void Level::SetData(const LevelData& d) {
    data_ = d;
    collectibles_ = data_.collectibles;
    plats_.clear();
    for (const auto& pd : data_.platforms) {
        Plat p;
        p.def = pd;
        p.pos = pd.a;
        p.vel = {0, 0};
        p.t = 0.0;
        p.dir = 1.0;
        plats_.push_back(p);
    }
    activeCheckpoint_ = 0;
}

TileType Level::At(int cx, int cy) const {
    if (cx < 0 || cx >= data_.w || cy < 0 || cy >= data_.h)
        return TileType::Empty;
    return data_.tiles[cy * data_.w + cx];
}

TileType Level::AtWorld(double x, double y) const {
    return At(static_cast<int>(std::floor(x / 16.0)), static_cast<int>(std::floor(y / 16.0)));
}

bool Level::IsSolidCell(int cx, int cy) const {
    TileType t = At(cx, cy);
    return t == TileType::Solid || t == TileType::Breakable || t == TileType::Door;
}

bool Level::BreakBrick(int cx, int cy) {
    if (At(cx, cy) != TileType::Breakable)
        return false;
    data_.tiles[cy * data_.w + cx] = TileType::Empty;
    return true;
}

bool Level::RaycastLOS(const Vec2& a, const Vec2& b) const { return ::ink::RaycastLOS(a, b, *this); }

int Level::CollectibleTotal() const { return static_cast<int>(collectibles_.size()); }
int Level::CollectibleTaken() const {
    int n = 0;
    for (const auto& c : collectibles_)
        if (c.taken)
            ++n;
    return n;
}

Vec2 Level::RespawnPoint() const {
    if (activeCheckpoint_ >= 0 && activeCheckpoint_ < static_cast<int>(data_.checkpoints.size()))
        return data_.checkpoints[activeCheckpoint_];
    return data_.playerSpawn;
}

void Level::Update(double dt) {
    for (auto& p : plats_) {
        p.vel = {0, 0};
        const Vec2 dir = (p.def.b - p.def.a).Normalized();
        if (dir.LengthSq() < 1e-9)
            continue;
        double dist = (p.def.b - p.def.a).Length();
        if (dist < 1e-6)
            continue;
        p.t += p.def.speed * dt * p.dir;
        if (p.t >= dist) {
            p.t = dist;
            p.dir = -1.0;
        } else if (p.t <= 0.0) {
            p.t = 0.0;
            p.dir = 1.0;
        }
        Vec2 want = p.def.a + dir * p.t;
        p.vel = want - p.pos;
        p.pos = want;
    }
}

Rect Level::BossZone() const {
    if (!data_.hasBoss)
        return {};
    return {data_.bounds.Right() - 220.0, data_.bounds.Top(), 220.0, data_.bounds.h};
}

Rect Level::ExitZone() const { return {data_.bounds.Right() - 40.0, data_.bounds.Top(), 40.0, data_.bounds.h}; }

bool Level::InBossZone(const Entity& e) const {
    Rect z = BossZone();
    return z.w > 0 && z.Overlaps(e.Box());
}

bool Level::InExitZone(const Entity& e) const {
    Rect z = ExitZone();
    return z.Overlaps(e.Box());
}

bool Level::HazardOverlaps(const Rect& box) const { return ::ink::OverlapsHazard(box, *this); }

} // namespace ink
