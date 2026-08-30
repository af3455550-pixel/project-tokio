#pragma once
// The six worlds of The Last Reel (§27). Catalog is data-driven (worlds.json);
// each world owns its palette, music, levels and boss. The slice implements
// world 1 fully; the other five are registered so progression, map and
// localisation systems already see the complete structure.
#include "Core/Json.h"
#include <cstdint>
#include <string>
#include <vector>

namespace ink {

struct WorldLevelDef {
    std::string id;
    std::string name;
    std::string file; // path relative to the assets dir
    bool secret = false;
};

struct WorldDef {
    std::string id;
    std::string name;
    std::string subtitle;
    std::string description;
    uint32_t skyColor = 0xFFBDE3F5;
    uint32_t groundColor = 0xFF4E7A3A;
    uint32_t accentColor = 0xFF8CC63F;
    std::string musicId;
    std::string bossId;
    std::string bossName;
    std::vector<WorldLevelDef> levels;
};

class WorldCatalog {
public:
    bool LoadJson(const std::string& json, std::string* err = nullptr);

    const std::vector<WorldDef>& Worlds() const { return worlds_; }
    const WorldDef* Get(const std::string& id) const;
    const WorldDef* ByIndex(int i) const {
        return i >= 0 && i < static_cast<int>(worlds_.size()) ? &worlds_[i] : nullptr;
    }
    int Count() const { return static_cast<int>(worlds_.size()); }

private:
    std::vector<WorldDef> worlds_;
};

} // namespace ink
