#pragma once
// A level: tile grid (world geometry), collectibles, checkpoints, moving
// platforms, NPC/spawn definitions. The parser (Levels/LevelParser) turns the
// ASCII level format into a LevelData; this class owns the *live* state.
#include "Gameplay/Entity.h"
#include "Physics/Collision.h"
#include <memory>
#include <vector>

namespace ink {

enum class TileType { Empty, Solid, Oneway, Hazard, Breakable, Door };

struct Collectible {
    std::string type; // coin | film | frame | stamp
    std::string itemId;
    Vec2 pos;
    bool taken = false;
    double t = 0.0; // bob animation time
};

struct NpcDef {
    std::string id;
    Vec2 pos;
};

struct SpawnDef {
    std::string type; // enemy id or boss id
    Vec2 pos;
    int data = 0;
};

struct MovingPlatformDef {
    Vec2 a, b; // path endpoints
    double speed = 40.0; // px/s
};

struct LevelData {
    std::string id;
    std::string name;
    std::string worldId;
    std::string musicId;
    std::string exitText; // shown on completion
    int w = 0, h = 0;
    std::vector<TileType> tiles;
    Vec2 playerSpawn{0.0, 0.0};
    Rect bounds{0.0, 0.0, 0.0, 0.0};
    std::vector<SpawnDef> spawns;
    std::vector<NpcDef> npcs;
    std::vector<Collectible> collectibles;
    std::vector<Vec2> checkpoints;
    std::vector<MovingPlatformDef> platforms;
    bool hasBoss = false;
    std::string bossId;
    Vec2 bossSpawn{0.0, 0.0};
};

class Level : public ISolidQuery {
public:
    void Load(const LevelData& data);

    const LevelData& Data() const { return data_; }
    void SetData(const LevelData& d); // re-init live state (level reload / test)

    // ---- ISolidQuery -------------------------------------------------
    int TileSize() const override { return 16; }
    int WidthCells() const override { return data_.w; }
    int HeightCells() const override { return data_.h; }
    bool IsSolidCell(int cx, int cy) const override;
    bool IsOnewayCell(int cx, int cy) const override { return At(cx, cy) == TileType::Oneway; }
    bool IsHazardCell(int cx, int cy) const override { return At(cx, cy) == TileType::Hazard; }

    TileType At(int cx, int cy) const;
    TileType AtWorld(double x, double y) const;
    bool BreakBrick(int cx, int cy);
    bool BreakableAt(int cx, int cy) const { return At(cx, cy) == TileType::Breakable; }

    bool RaycastLOS(const Vec2& a, const Vec2& b) const;

    // ---- Collectibles --------------------------------------------------
    std::vector<Collectible>& Collectibles() { return collectibles_; }
    const std::vector<Collectible>& Collectibles() const { return collectibles_; }
    int CollectibleTotal() const;
    int CollectibleTaken() const;

    // ---- Checkpoints ---------------------------------------------------
    const std::vector<Vec2>& Checkpoints() const { return data_.checkpoints; }
    int ActiveCheckpoint() const { return activeCheckpoint_; }
    void SetActiveCheckpoint(int i) { activeCheckpoint_ = i; }
    Vec2 RespawnPoint() const;

    // ---- Moving platforms ----------------------------------------------
    struct Plat {
        MovingPlatformDef def;
        Vec2 pos; // top-left
        Vec2 vel; // per step delta (for carrying the player)
        double t = 0.0;
        double dir = 1.0;
    };
    std::vector<Plat>& Platforms() { return plats_; }
    const std::vector<Plat>& Platforms() const { return plats_; }
    void Update(double dt);

    // ---- Triggers --------------------------------------------------------
    Rect BossZone() const; // where the boss fight starts (right 60% of arena if hasBoss)
    Rect ExitZone() const; // level exit
    bool InBossZone(const Entity& e) const;
    bool InExitZone(const Entity& e) const;

    bool HasBoss() const { return data_.hasBoss; }
    const std::string& BossId() const { return data_.bossId; }
    const Vec2& BossSpawn() const { return data_.bossSpawn; }

    // Hazard query for actors (spikes etc.)
    bool HazardOverlaps(const Rect& box) const;

private:
    LevelData data_;
    std::vector<Collectible> collectibles_;
    std::vector<Plat> plats_;
    int activeCheckpoint_ = 0;
};

} // namespace ink
