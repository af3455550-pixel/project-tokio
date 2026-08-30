// inkbound-tools — designer utilities (SDL-free).
//   inkbound-tools validate <assetsDir>
//   inkbound-tools dump <assetsDir> <bossId|enemyId|weaponId>
// The JSON data files *are* the editor (§70/§71): designers tune values there
// and validate them from the command line before shipping.
#include "Bosses/Boss.h"
#include "Enemies/EnemyBook.h"
#include "Levels/LevelParser.h"
#include "Progression/Achievements.h"
#include "Quests/QuestBook.h"
#include "Weapons/WeaponDef.h"
#include "World/WorldCatalog.h"
#include "Core/Log.h"
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

using namespace ink;

static std::string Read(const fs::path& p) {
    std::string s = ReadFile(p.string());
    return s;
}

static int ValidateDir(const fs::path& assets) {
    int errors = 0;
    auto check = [&](const fs::path& p, bool ok, const std::string& err) {
        if (!fs::exists(p)) {
            std::cout << "[MISSING] " << p << "\n";
            ++errors;
            return;
        }
        if (ok)
            std::cout << "[OK]      " << p.filename().string() << "\n";
        else {
            std::cout << "[ERROR]   " << p.filename().string() << ": " << err << "\n";
            ++errors;
        }
    };

    // Data files
    {
        std::string err;
        WeaponBook wb;
        check(assets / "Data" / "weapons.json", wb.LoadJson(Read(assets / "Data" / "weapons.json"), &err), err);
    }
    {
        std::string err;
        EnemyBook eb;
        check(assets / "Data" / "enemies.json", eb.LoadJson(Read(assets / "Data" / "enemies.json"), &err), err);
    }
    {
        std::string err;
        BossBook bb;
        check(assets / "Data" / "bosses.json", bb.LoadJson(Read(assets / "Data" / "bosses.json"), &err), err);
    }
    {
        std::string err;
        WorldCatalog wc;
        check(assets / "Data" / "worlds.json", wc.LoadJson(Read(assets / "Data" / "worlds.json"), &err), err);
    }
    {
        std::string err;
        QuestBook qb;
        check(assets / "Data" / "quests.json", qb.LoadJson(Read(assets / "Data" / "quests.json"), &err), err);
    }
    {
        std::string err;
        AchievementBook ab;
        check(assets / "Data" / "achievements.json", ab.LoadJson(Read(assets / "Data" / "achievements.json"), &err), err);
    }

    // Levels referenced by the world catalog
    {
        std::string err;
        WorldCatalog wc;
        if (wc.LoadJson(Read(assets / "Data" / "worlds.json"), &err)) {
            for (const auto& w : wc.Worlds()) {
                for (const auto& l : w.levels) {
                    if (l.file.empty())
                        continue;
                    fs::path p = assets / l.file;
                    if (!fs::exists(p)) {
                        std::cout << "[MISSING] level file " << p << "\n";
                        ++errors;
                        continue;
                    }
                    LevelData ld;
                    if (!ParseLevelText(Read(p), ld, &err)) {
                        std::cout << "[ERROR]   " << p.filename().string() << ": " << err << "\n";
                        ++errors;
                    } else {
                        std::cout << "[OK]      " << p.filename().string() << " (" << ld.w << "x" << ld.h
                                  << ", " << ld.spawns.size() << " spawns, " << ld.collectibles.size()
                                  << " pickups" << (ld.hasBoss ? ", boss" : "") << ")\n";
                    }
                }
            }
        }
    }

    if (errors == 0)
        std::cout << "Validation passed.\n";
    else
        std::cout << errors << " error(s) found.\n";
    return errors == 0 ? 0 : 1;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "inkbound-tools\n"
                  << "  validate <assetsDir>                 check all data + levels\n"
                  << "  dump <assetsDir> <bossId>            print a boss's effective config\n"
                  << "  dump <assetsDir> <enemyId|weaponId>  print its config\n";
        return 2;
    }
    const std::string cmd = argv[1];
    const fs::path assets = argv[2];
    if (cmd == "validate")
        return ValidateDir(assets);
    if (cmd == "dump" && argc >= 4) {
        const std::string id = argv[3];
        std::string err;
        BossBook bb;
        if (bb.LoadJson(Read(assets / "Data" / "bosses.json"), &err)) {
            if (const BossDef* b = bb.Get(id)) {
                std::cout << "BOSS " << b->id << " " << b->name << " hp=" << b->maxHp << " phases="
                          << b->phases.size() << " (mini=" << (b->miniBoss ? 1 : 0) << ")\n";
                for (std::size_t i = 0; i < b->phases.size(); ++i)
                    std::cout << "  phase " << i << ": hpFrac=" << b->phases[i].hpFrac
                              << " musicLayer=" << b->phases[i].musicLayer << " label=" << b->phases[i].label
                              << "\n";
                return 0;
            }
        }
        EnemyBook eb;
        if (eb.LoadJson(Read(assets / "Data" / "enemies.json"), &err)) {
            if (const EnemyDef* e = eb.Get(id)) {
                std::cout << "ENEMY " << e->id << " hp=" << e->hp << " speed=" << e->speed
                          << " detect=" << e->detectRange << " cd=" << e->attackCooldown << "\n";
                return 0;
            }
        }
        WeaponBook wb;
        if (wb.LoadJson(Read(assets / "Data" / "weapons.json"), &err)) {
            if (const WeaponDef* w = wb.Get(id)) {
                std::cout << "WEAPON " << w->id << " cd=" << w->fireCooldown << " dmg=" << w->shot.damage
                          << " speed=" << w->shot.speed << " pierce=" << w->shot.pierce
                          << " ricochet=" << w->shot.ricochet << " explosive=" << (w->shot.explosive ? 1 : 0)
                          << "\n";
                return 0;
            }
        }
        std::cerr << "unknown id: " << id << "\n";
        return 1;
    }
    return 2;
}
