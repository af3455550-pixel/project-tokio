#include "Core/Json.h"
#include "Save/SaveSystem.h"
#include "Test.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unistd.h>

using namespace ink;

namespace fs = std::filesystem;

namespace {
std::string TempSaveDir() {
    std::string dir = "inktest_saves_" + std::to_string(::getpid());
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    return dir;
}

GameProgress MakeProgress() {
    GameProgress p;
    p.saveName = "Milo";
    p.currentLevel = "meadows_02";
    p.currentWorld = "meadows";
    p.coins = 1234;
    p.hpUpgrades = 2;
    p.currentWeapon = "ink_blaster";
    p.GrantWeapon("ink_blaster");
    p.GrantCharm("echo_heart");
    p.charmSlots[0] = "echo_heart";
    p.films.push_back("film_0");
    p.frames.push_back("frame_1");
    BossRecord rec;
    rec.bestTime = 42.5;
    rec.bestScore = 1500;
    rec.bestRank = "A";
    rec.noHit = false;
    p.bosses["patchling"] = rec;
    return p;
}
} // namespace

INK_TEST(save_roundtrip) {
    setenv("INK_SAVEDIR", TempSaveDir().c_str(), 1);
    SaveSystem s;
    std::string err;
    GameProgress in = MakeProgress();
    INK_REQUIRE(s.Save(1, in, &err));
    INK_REQUIRE(s.Exists(1));
    INK_REQUIRE(!s.Exists(0));

    GameProgress out;
    INK_REQUIRE(s.Load(1, out, &err));
    INK_REQUIRE(out.saveName == "Milo");
    INK_REQUIRE(out.currentLevel == "meadows_02");
    INK_REQUIRE_EQ(out.coins, 1234);
    INK_REQUIRE_EQ(out.hpUpgrades, 2);
    INK_REQUIRE(out.OwnsWeapon("ink_blaster"));
    INK_REQUIRE(out.OwnsCharm("echo_heart"));
    INK_REQUIRE(out.charmSlots[0] == "echo_heart");
    INK_REQUIRE_EQ(static_cast<int>(out.films.size()), 1);
    INK_REQUIRE_EQ(static_cast<int>(out.frames.size()), 1);
    INK_REQUIRE(out.bosses.count("patchling") == 1);
    INK_REQUIRE_NEAR(out.bosses["patchling"].bestTime, 42.5, 1e-9);
    INK_REQUIRE(out.bosses["patchling"].bestRank == "A");
}

INK_TEST(save_corruption_rejected) {
    setenv("INK_SAVEDIR", TempSaveDir().c_str(), 1);
    SaveSystem s;
    std::string err;
    GameProgress in = MakeProgress();
    INK_REQUIRE(s.Save(2, in, &err));
    const std::string path = s.SlotPath(2);
    {
        std::ifstream f(path, std::ios::binary);
        std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        // Corrupt the payload: flip a byte inside the "data" region.
        std::size_t at = text.find("\"saveName\"");
        INK_REQUIRE(at != std::string::npos && at + 20 < text.size());
        text[at + 12] ^= 0x7F;
        std::ofstream o(path, std::ios::binary | std::ios::trunc);
        o << text;
    }
    GameProgress out;
    INK_REQUIRE(!s.Load(2, out, &err));
    INK_REQUIRE(!err.empty());

    // Garbage file must also be rejected cleanly.
    {
        std::ofstream o(s.SlotPath(3), std::ios::binary);
        o << "this is not json at all";
    }
    INK_REQUIRE(!s.Load(3, out, &err));
}

INK_TEST(save_latest_slot) {
    setenv("INK_SAVEDIR", TempSaveDir().c_str(), 1);
    SaveSystem s;
    std::string err;
    GameProgress in = MakeProgress();
    INK_REQUIRE(s.Save(0, in, &err));
    INK_REQUIRE(s.Save(2, in, &err));
    INK_REQUIRE_EQ(s.LatestSlot(), 2);
    INK_REQUIRE(!s.SlotMeta(2).empty());
}
