#include "Levels/LevelParser.h"
#include "Core/Log.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace ink {

std::string ReadFile(const std::string& path, std::string* err) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        if (err)
            *err = "cannot open file: " + path;
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

namespace {

TileType TileFromChar(char c) {
    switch (c) {
    case '#': return TileType::Solid;
    case '=': return TileType::Oneway;
    case '^': return TileType::Hazard;
    case '?': return TileType::Breakable;
    case 'D': return TileType::Door;
    default: return TileType::Empty;
    }
}

bool IsEntityChar(char c) {
    return c == 'P' || c == 'C' || c == 'B' || c == 'b' || c == 'i' || c == 'f' || c == 'm' || c == 's';
}

double ParseNum(const std::string& s, double def = 0.0) {
    try {
        return std::stod(s);
    } catch (...) {
        return def;
    }
}

} // namespace

bool ParseLevelText(const std::string& text, LevelData& out, std::string* err) {
    out = LevelData{};
    std::istringstream ss(text);
    std::string line;
    enum Section { None, Level, Map, Entities } section = None;

    int mapWidth = 0;
    std::vector<std::string> rows;
    int tile = 16;

    auto fail = [&](const std::string& msg) {
        if (err)
            *err = msg;
        INK_LOG_ERROR("Level parse: " + msg);
        return false;
    };

    while (std::getline(ss, line)) {
        // trim
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (line.empty())
            continue;

        if (line.rfind("[", 0) == 0) {
            if (line == "[level]")
                section = Section::Level;
            else if (line == "[map]")
                section = Section::Map;
            else if (line == "[entities]")
                section = Section::Entities;
            else
                return fail("unknown section: " + line);
            continue;
        }

        // NOTE: '#' comment lines are only honoured OUTSIDE [map]; inside the
        // map a leading '#' is a solid tile, not a comment.
        if (section != Section::Map && line[0] == '#')
            continue;

        if (section == Section::Level) {
            auto eq = line.find('=');
            if (eq == std::string::npos)
                continue;
            std::string k = line.substr(0, eq), v = line.substr(eq + 1);
            if (k == "id") out.id = v;
            else if (k == "name") out.name = v;
            else if (k == "world") out.worldId = v;
            else if (k == "music") out.musicId = v;
            else if (k == "exit") out.exitText = v;
            continue;
        }

        if (section == Section::Map) {
            for (char c : line) {
                if (c == '.' || TileFromChar(c) != TileType::Empty || IsEntityChar(c))
                    continue;
                if (c != ' ')
                    return fail(std::string("illegal map character '") + c + "'");
            }
            rows.push_back(line);
            mapWidth = std::max(mapWidth, static_cast<int>(line.size()));
            continue;
        }

        if (section == Section::Entities) {
            std::istringstream ls(line);
            std::string kind;
            ls >> kind;
            if (kind == "spawn") {
                SpawnDef sdef;
                ls >> sdef.type >> sdef.pos.x >> sdef.pos.y;
                if (ls >> sdef.data) {}
                out.spawns.push_back(sdef);
            } else if (kind == "npc") {
                NpcDef n;
                ls >> n.id >> n.pos.x >> n.pos.y;
                out.npcs.push_back(n);
            } else if (kind == "plat") {
                MovingPlatformDef p;
                ls >> p.a.x >> p.a.y >> p.b.x >> p.b.y >> p.speed;
                out.platforms.push_back(p);
            } else if (kind == "boss") {
                out.hasBoss = true;
                ls >> out.bossId >> out.bossSpawn.x >> out.bossSpawn.y;
            } else {
                return fail("unknown entity line: " + line);
            }
        }
    }

    if (rows.empty())
        return fail("no map rows found");
    if (out.id.empty())
        out.id = "level_" + std::to_string(rows.size());

    out.w = mapWidth;
    out.h = static_cast<int>(rows.size());
    out.tiles.assign(static_cast<std::size_t>(mapWidth) * rows.size(), TileType::Empty);

    int filmN = 0, frameN = 0, stampN = 0, coinN = 0;
    for (int y = 0; y < out.h; ++y) {
        const std::string& row = rows[y];
        for (int x = 0; x < mapWidth; ++x) {
            char c = (x < static_cast<int>(row.size())) ? row[x] : '.';
            if (c == ' ')
                c = '.';
            if (TileFromChar(c) != TileType::Empty) {
                out.tiles[y * mapWidth + x] = TileFromChar(c);
            } else if (c == 'P') {
                out.playerSpawn = {x * tile + 2.0, y * tile + 2.0};
            } else if (c == 'C') {
                out.checkpoints.push_back({x * tile + 2.0, y * tile + 2.0 - 8.0});
            } else if (c == 'B') {
                if (!out.hasBoss) {
                    out.hasBoss = true;
                    out.bossId = "unknown_boss";
                }
                out.bossSpawn = {x * tile, y * tile - 8.0};
            } else if (c == 'i') {
                out.collectibles.push_back({"coin", "coin_" + std::to_string(coinN++), {x * tile + 4.0, y * tile + 4.0}, false, 0.0});
            } else if (c == 'f') {
                out.collectibles.push_back({"film", "film_" + std::to_string(filmN++), {x * tile + 4.0, y * tile + 4.0}, false, 0.0});
            } else if (c == 'm') {
                out.collectibles.push_back({"frame", "frame_" + std::to_string(frameN++), {x * tile + 4.0, y * tile + 4.0}, false, 0.0});
            } else if (c == 's') {
                out.collectibles.push_back({"stamp", "stamp_" + std::to_string(stampN++), {x * tile + 4.0, y * tile + 4.0}, false, 0.0});
            }
        }
    }

    out.bounds = {0.0, 0.0, mapWidth * tile, static_cast<double>(rows.size()) * tile};
    if (out.playerSpawn == Vec2{})
        out.playerSpawn = {24.0, 24.0};
    return true;
}

} // namespace ink
