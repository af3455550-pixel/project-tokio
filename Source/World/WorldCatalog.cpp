#include "World/WorldCatalog.h"

namespace ink {

static uint32_t ParseColor(const Json* j, uint32_t def) {
    if (!j || !j->IsString())
        return def;
    std::string s = j->AsString("#RRGGBB");
    if (s.size() == 7 && s[0] == '#') {
        unsigned v = 0;
        if (std::sscanf(s.c_str() + 1, "%06x", &v) == 1)
            return 0xFF000000u | (v & 0xFFFFFFu);
    }
    return def;
}

bool WorldCatalog::LoadJson(const std::string& json, std::string* err) {
    worlds_.clear();
    Json root = Json::Parse(json, err);
    if (root.IsNull()) {
        if (err && err->empty())
            *err = "worlds.json: not valid JSON";
        return false;
    }
    const Json* arr = root.Find("worlds");
    if (!arr || !arr->IsArray())
        return err ? (*err = "worlds.json: missing 'worlds' array"), false : false;

    for (const auto& jw : arr->Items()) {
        WorldDef w;
        w.id = jw.Find("id") ? jw.Find("id")->AsString() : "world";
        w.name = jw.Find("name") ? jw.Find("name")->AsString() : w.id;
        w.subtitle = jw.Find("subtitle") ? jw.Find("subtitle")->AsString() : "";
        w.description = jw.Find("description") ? jw.Find("description")->AsString() : "";
        w.skyColor = ParseColor(jw.Find("sky"), w.skyColor);
        w.groundColor = ParseColor(jw.Find("ground"), w.groundColor);
        w.accentColor = ParseColor(jw.Find("accent"), w.accentColor);
        w.musicId = jw.Find("music") ? jw.Find("music")->AsString() : "meadows";
        w.bossId = jw.Find("bossId") ? jw.Find("bossId")->AsString() : "";
        w.bossName = jw.Find("bossName") ? jw.Find("bossName")->AsString() : "";
        const Json* levels = jw.Find("levels");
        if (levels && levels->IsArray()) {
            for (const auto& jl : levels->Items()) {
                WorldLevelDef l;
                l.id = jl.Find("id") ? jl.Find("id")->AsString() : "level";
                l.name = jl.Find("name") ? jl.Find("name")->AsString() : l.id;
                l.file = jl.Find("file") ? jl.Find("file")->AsString() : "";
                l.secret = jl.Find("secret") ? jl.Find("secret")->AsBool(false) : false;
                w.levels.push_back(l);
            }
        }
        worlds_.push_back(std::move(w));
    }
    return !worlds_.empty();
}

const WorldDef* WorldCatalog::Get(const std::string& id) const {
    for (const auto& w : worlds_)
        if (w.id == id)
            return &w;
    return nullptr;
}

} // namespace ink
