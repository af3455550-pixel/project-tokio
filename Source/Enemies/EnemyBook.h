#pragma once
// Loads enemy definitions (enemies.json) into an EnemyBook.
#include "Enemies/Enemy.h"
#include <string>
#include <vector>

namespace ink {

class EnemyBook {
public:
    bool LoadJson(const std::string& json, std::string* err = nullptr);
    const EnemyDef* Get(const std::string& id) const;
    const std::vector<EnemyDef>& All() const { return defs_; }

private:
    std::vector<EnemyDef> defs_;
};

} // namespace ink
