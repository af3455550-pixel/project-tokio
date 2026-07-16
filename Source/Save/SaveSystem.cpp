#include "Save/SaveSystem.h"
#include "Core/Crc.h"
#include "Core/Json.h"
#include "Core/Log.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace ink {

namespace fs = std::filesystem;

// ---------------------------------------------------------------- progress --
bool GameProgress::OwnsWeapon(const std::string& id) const {
    return std::find(weaponsOwned.begin(), weaponsOwned.end(), id) != weaponsOwned.end();
}
void GameProgress::GrantWeapon(const std::string& id) {
    if (!OwnsWeapon(id))
        weaponsOwned.push_back(id);
}
bool GameProgress::OwnsCharm(const std::string& id) const {
    return std::find(charmsOwned.begin(), charmsOwned.end(), id) != charmsOwned.end();
}
void GameProgress::GrantCharm(const std::string& id) {
    if (!OwnsCharm(id))
        charmsOwned.push_back(id);
}
bool GameProgress::HasFrame(const std::string& id) const {
    return std::find(frames.begin(), frames.end(), id) != frames.end();
}
void GameProgress::GrantFrame(const std::string& id) {
    if (!HasFrame(id))
        frames.push_back(id);
}

std::string GameProgress::ToJson() const {
    Json root = Json::Object();
    root.Set("version", Json(static_cast<double>(kVersion)));
    root.Set("saveName", Json(saveName));
    root.Set("playTime", Json(playTime));
    root.Set("currentLevel", Json(currentLevel));
    root.Set("currentWorld", Json(currentWorld));
    root.Set("coins", Json(coins));
    root.Set("hpUpgrades", Json(hpUpgrades));
    root.Set("currentWeapon", Json(currentWeapon));

    Json w = Json::Array();
    for (const auto& x : weaponsOwned)
        w.Push(Json(x));
    root.Set("weaponsOwned", std::move(w));

    Json c = Json::Array();
    for (int i = 0; i < 3; ++i)
        c.Push(Json(charmSlots[i]));
    root.Set("charmSlots", std::move(c));

    Json co = Json::Array();
    for (const auto& x : charmsOwned)
        co.Push(Json(x));
    root.Set("charmsOwned", std::move(co));

    Json b = Json::Object();
    for (const auto& [k, v] : bosses) {
        Json j = Json::Object();
        j.Set("bestRank", Json(v.bestRank));
        j.Set("bestTime", Json(v.bestTime));
        j.Set("bestScore", Json(v.bestScore));
        j.Set("noHit", Json(static_cast<bool>(v.noHit)));
        b.Set(k, std::move(j));
    }
    root.Set("bosses", std::move(b));

    auto strArr = [&root](const char* key, const std::vector<std::string>& v) {
        Json a = Json::Array();
        for (const auto& x : v)
            a.Push(Json(x));
        root.Set(key, std::move(a));
    };
    strArr("achievements", achievements);
    strArr("films", films);
    strArr("frames", frames);
    strArr("stamps", stamps);

    Json sj = Json::Object();
    sj.Set("masterVol", Json(static_cast<double>(settings.masterVol)));
    sj.Set("musicVol", Json(static_cast<double>(settings.musicVol)));
    sj.Set("sfxVol", Json(static_cast<double>(settings.sfxVol)));
    sj.Set("difficulty", Json(settings.difficulty));
    sj.Set("filmFx", Json(static_cast<double>(settings.filmFx)));
    sj.Set("reducedFlash", Json(settings.reducedFlash));
    sj.Set("reducedShake", Json(settings.reducedShake));
    sj.Set("highContrast", Json(settings.highContrast));
    sj.Set("colorAssist", Json(settings.colorAssist));
    sj.Set("fontScale", Json(static_cast<double>(settings.fontScale)));
    Json kb = Json::Object();
    for (const auto& [k, v] : settings.keybinds)
        kb.Set(k, Json(v));
    sj.Set("keybinds", std::move(kb));
    root.Set("settings", std::move(sj));

    return root.Serialize(true);
}

bool GameProgress::FromJson(const std::string& text, std::string* err) {
    Json root = Json::Parse(text, err);
    if (root.IsNull())
        return false;
    const Json* ver = root.Find("version");
    if (!ver || ver->AsInt(0) != kVersion) {
        if (err)
            *err = "save version mismatch (expected " + std::to_string(kVersion) + ")";
        return false;
    }
    auto s = [&](const char* k, std::string& dst) {
        if (const Json* v = root.Find(k))
            dst = v->AsString(dst);
    };
    s("saveName", saveName);
    s("currentLevel", currentLevel);
    s("currentWorld", currentWorld);
    s("currentWeapon", currentWeapon);
    if (const Json* v = root.Find("playTime"))
        playTime = v->AsNumber(playTime);
    if (const Json* v = root.Find("coins"))
        coins = v->AsInt(coins);
    if (const Json* v = root.Find("hpUpgrades"))
        hpUpgrades = v->AsInt(hpUpgrades);

    auto readArr = [&](const char* k, std::vector<std::string>& out) {
        if (const Json* v = root.Find(k); v && v->IsArray()) {
            out.clear();
            for (const auto& x : v->Items())
                if (x.IsString())
                    out.push_back(x.AsString());
        }
    };
    readArr("weaponsOwned", weaponsOwned);
    readArr("charmsOwned", charmsOwned);
    readArr("achievements", achievements);
    readArr("films", films);
    readArr("frames", frames);
    readArr("stamps", stamps);

    if (const Json* v = root.Find("charmSlots"); v && v->IsArray()) {
        for (int i = 0; i < 3; ++i)
            charmSlots[i] = v->At(i).AsString("");
    }

    if (const Json* v = root.Find("bosses"); v && v->IsObject()) {
        bosses.clear();
        for (const auto& [k, jv] : v->Members()) {
            BossRecord r;
            r.bestRank = jv.Find("bestRank") ? jv.Find("bestRank")->AsString("") : "";
            r.bestTime = jv.Find("bestTime") ? jv.Find("bestTime")->AsNumber(0) : 0.0;
            r.bestScore = jv.Find("bestScore") ? jv.Find("bestScore")->AsNumber(0) : 0.0;
            r.noHit = jv.Find("noHit") ? jv.Find("noHit")->AsBool(false) : false;
            bosses[k] = r;
        }
    }

    if (const Json* v = root.Find("settings"); v && v->IsObject()) {
        if (const Json* x = v->Find("masterVol"))
            settings.masterVol = static_cast<float>(x->AsNumber(settings.masterVol));
        if (const Json* x = v->Find("musicVol"))
            settings.musicVol = static_cast<float>(x->AsNumber(settings.musicVol));
        if (const Json* x = v->Find("sfxVol"))
            settings.sfxVol = static_cast<float>(x->AsNumber(settings.sfxVol));
        if (const Json* x = v->Find("difficulty"))
            settings.difficulty = x->AsString(settings.difficulty);
        if (const Json* x = v->Find("filmFx"))
            settings.filmFx = static_cast<float>(x->AsNumber(settings.filmFx));
        settings.reducedFlash = v->Find("reducedFlash") ? v->Find("reducedFlash")->AsBool(false) : false;
        settings.reducedShake = v->Find("reducedShake") ? v->Find("reducedShake")->AsBool(false) : false;
        settings.highContrast = v->Find("highContrast") ? v->Find("highContrast")->AsBool(false) : false;
        settings.colorAssist = v->Find("colorAssist") ? v->Find("colorAssist")->AsBool(false) : false;
        if (const Json* x = v->Find("fontScale"))
            settings.fontScale = static_cast<float>(x->AsNumber(settings.fontScale));
        if (const Json* x = v->Find("keybinds"); x && x->IsObject()) {
            settings.keybinds.clear();
            for (const auto& [k, vv] : x->Members())
                settings.keybinds[k] = vv.AsString();
        }
    }
    return true;
}

// ------------------------------------------------------------------ system --
std::string SaveSystem::Dir() {
    const char* env = std::getenv("INK_SAVEDIR");
    std::string dir = env && *env ? env : "./saves";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

std::string SaveSystem::SlotPath(int slot) const { return Dir() + "/inkbound_slot" + std::to_string(slot) + ".json"; }

bool SaveSystem::Save(int slot, const GameProgress& p, std::string* err) const {
    if (slot < 0 || slot >= kSlots)
        return false;
    const std::string data = p.ToJson();
    Json root = Json::Object();
    root.Set("version", Json(static_cast<double>(GameProgress::kVersion)));
    root.Set("crc", Json(static_cast<double>(Crc32(data))));
    // Store the payload as a JSON object (not an embedded string) so Load's
    // shape check + deterministic re-serialization for the CRC both work.
    root.Set("data", Json::Parse(data));
    const std::string file = root.Serialize(true);
    const std::string path = SlotPath(slot);
    const std::string tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) {
            if (err)
                *err = "cannot write " + tmp;
            return false;
        }
        f << file;
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    if (ec) {
        if (err)
            *err = "cannot replace " + path;
        return false;
    }
    INK_LOG_INFO("Saved slot " + std::to_string(slot) + " (" + std::to_string(file.size()) + " bytes)");
    return true;
}

bool SaveSystem::Load(int slot, GameProgress& p, std::string* err) const {
    if (slot < 0 || slot >= kSlots)
        return false;
    const std::string path = SlotPath(slot);
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        if (err)
            *err = "no save at slot " + std::to_string(slot);
        return false;
    }
    std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    Json root = Json::Parse(text, err);
    if (root.IsNull())
        return false;
    const Json* crcJ = root.Find("crc");
    const Json* dataJ = root.Find("data");
    if (!crcJ || !dataJ || !dataJ->IsObject()) {
        if (err)
            *err = "save file malformed";
        return false;
    }
    // The data payload round-trips deterministically through our serializer
    // (sorted keys, stable number formatting), so re-serializing the parsed
    // object reproduces the exact stored bytes for the CRC check.
    const std::string dataText = dataJ->Serialize(true);
    // CRCs are 32-bit unsigned and can exceed INT_MAX: compare through
    // AsNumber (exact for values < 2^53), never AsInt.
    if (Crc32(dataText) != static_cast<uint32_t>(crcJ->AsNumber(0.0))) {
        if (err)
            *err = "save file corrupted (CRC mismatch)";
        INK_LOG_ERROR("Save slot " + std::to_string(slot) + " failed CRC check");
        return false;
    }
    return p.FromJson(dataText, err);
}

bool SaveSystem::Exists(int slot) const {
    if (slot < 0 || slot >= kSlots)
        return false;
    std::error_code ec;
    return fs::exists(SlotPath(slot), ec);
}

std::string SaveSystem::SlotMeta(int slot) const {
    if (!Exists(slot))
        return "";
    GameProgress p;
    std::string err;
    if (!Load(slot, p, &err))
        return "(corrupted)";
    return p.saveName + " - " + p.currentLevel + " - " + std::to_string(p.coins) + " coins";
}

int SaveSystem::LatestSlot() const {
    int latest = -1;
    for (int i = 0; i < kSlots; ++i)
        if (Exists(i))
            latest = i;
    return latest;
}

bool SaveSystem::SaveSettingsOnly(const GameProgress& p, std::string* err) {
    // Settings are stored inside each slot; keep a shared copy in slot 0's
    // "options" sidecar so options apply before a game exists.
    Json root = Json::Object();
    root.Set("version", Json(static_cast<double>(GameProgress::kVersion)));
    root.Set("crc", Json(static_cast<double>(0)));
    Json data = Json::Object();
    Json sj = Json::Object();
    sj.Set("masterVol", Json(static_cast<double>(p.settings.masterVol)));
    sj.Set("musicVol", Json(static_cast<double>(p.settings.musicVol)));
    sj.Set("sfxVol", Json(static_cast<double>(p.settings.sfxVol)));
    sj.Set("difficulty", Json(p.settings.difficulty));
    sj.Set("filmFx", Json(static_cast<double>(p.settings.filmFx)));
    sj.Set("reducedFlash", Json(p.settings.reducedFlash));
    sj.Set("reducedShake", Json(p.settings.reducedShake));
    sj.Set("highContrast", Json(p.settings.highContrast));
    sj.Set("colorAssist", Json(p.settings.colorAssist));
    sj.Set("fontScale", Json(static_cast<double>(p.settings.fontScale)));
    Json kb = Json::Object();
    for (const auto& [k, v] : p.settings.keybinds)
        kb.Set(k, Json(v));
    sj.Set("keybinds", std::move(kb));
    data.Set("settings", std::move(sj));
    root.Set("data", std::move(data));
    const std::string path = Dir() + "/inkbound_options.json";
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f)
        return false;
    f << root.Serialize(true);
    return true;
}

bool SaveSystem::LoadSettingsOnly(GameProgress& p, std::string* err) {
    const std::string path = Dir() + "/inkbound_options.json";
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    Json root = Json::Parse(text, err);
    if (const Json* s = root.Find("data")) {
        if (const Json* x = s->Find("settings"); x && x->IsObject()) {
            if (const Json* y = x->Find("masterVol"))
                p.settings.masterVol = static_cast<float>(y->AsNumber(p.settings.masterVol));
            if (const Json* y = x->Find("musicVol"))
                p.settings.musicVol = static_cast<float>(y->AsNumber(p.settings.musicVol));
            if (const Json* y = x->Find("sfxVol"))
                p.settings.sfxVol = static_cast<float>(y->AsNumber(p.settings.sfxVol));
            if (const Json* y = x->Find("difficulty"))
                p.settings.difficulty = y->AsString(p.settings.difficulty);
            if (const Json* y = x->Find("filmFx"))
                p.settings.filmFx = static_cast<float>(y->AsNumber(p.settings.filmFx));
            p.settings.reducedFlash = x->Find("reducedFlash") ? x->Find("reducedFlash")->AsBool(false) : false;
            p.settings.reducedShake = x->Find("reducedShake") ? x->Find("reducedShake")->AsBool(false) : false;
            p.settings.highContrast = x->Find("highContrast") ? x->Find("highContrast")->AsBool(false) : false;
            p.settings.colorAssist = x->Find("colorAssist") ? x->Find("colorAssist")->AsBool(false) : false;
            if (const Json* y = x->Find("fontScale"))
                p.settings.fontScale = static_cast<float>(y->AsNumber(p.settings.fontScale));
        }
    }
    return true;
}

} // namespace ink
