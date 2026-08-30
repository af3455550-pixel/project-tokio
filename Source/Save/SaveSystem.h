#pragma once
// Robust save system (§53): 3 slots, versioned JSON, CRC32 integrity check,
// atomic writes (tmp + rename). Stores progress, inventory, settings,
// boss records, achievements and best times.
#include <array>
#include <map>
#include <string>
#include <vector>

namespace ink {

struct Settings {
    float masterVol = 0.8f;
    float musicVol = 0.75f;
    float sfxVol = 0.9f;
    std::string difficulty = "Standard"; // Story | Standard | Expert | Nightmare
    float filmFx = 1.0f;                 // 0..1 film effect intensity
    bool reducedFlash = false;
    bool reducedShake = false;
    bool highContrast = false;
    bool colorAssist = false;
    float fontScale = 1.0f;
    // action -> key name (e.g. "jump" -> "space")
    std::map<std::string, std::string> keybinds;
};

struct BossRecord {
    std::string bestRank;
    double bestTime = 0.0;
    double bestScore = 0.0;
    bool noHit = false;
};

struct GameProgress {
    static constexpr int kVersion = 3;

    std::string saveName = "Milo";
    double playTime = 0.0;
    std::string currentLevel = "meadows_01";
    std::string currentWorld = "meadows";

    int coins = 0;
    std::vector<std::string> weaponsOwned = {"ink_blaster"};
    std::string currentWeapon = "ink_blaster";
    std::array<std::string, 3> charmSlots{{"", "", ""}};
    std::vector<std::string> charmsOwned;
    int hpUpgrades = 0;

    std::map<std::string, BossRecord> bosses;
    std::vector<std::string> achievements;
    std::vector<std::string> films;
    std::vector<std::string> frames;
    std::vector<std::string> stamps;

    Settings settings;

    bool OwnsWeapon(const std::string& id) const;
    void GrantWeapon(const std::string& id);
    bool OwnsCharm(const std::string& id) const;
    void GrantCharm(const std::string& id);
    bool HasFrame(const std::string& id) const;
    void GrantFrame(const std::string& id);

    // (De)serialization of the *data* payload (version + fields).
    std::string ToJson() const;
    bool FromJson(const std::string& text, std::string* err);
};

class SaveSystem {
public:
    static constexpr int kSlots = 3;

    static std::string Dir(); // created on demand
    bool Save(int slot, const GameProgress& p, std::string* err = nullptr) const;
    bool Load(int slot, GameProgress& p, std::string* err = nullptr) const;
    bool Exists(int slot) const;
    // Small one-line summary for the slot menu ("Milo - Meadows 1-1 - 120 coins").
    std::string SlotMeta(int slot) const;
    int LatestSlot() const;
    std::string SlotPath(int slot) const;

    bool SaveSettingsOnly(const GameProgress& p, std::string* err = nullptr);
    bool LoadSettingsOnly(GameProgress& p, std::string* err = nullptr);
};

} // namespace ink
