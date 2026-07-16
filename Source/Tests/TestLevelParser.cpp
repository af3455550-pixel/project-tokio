#include "Levels/LevelParser.h"
#include "Test.h"

using namespace ink;

namespace {
const char* kSmallLevel = R"([level]
id=tiny
name=Tiny
world=meadows
music=meadows
# a comment outside the map is ignored
[map]
##.P.#
#..=.#
#Cii.#
######
[entities]
npc owl 100 30
spawn slome 40 40
plat 16 48 64 48 24
boss patchling 120 16
)";
} // namespace

INK_TEST(level_parser_basic) {
    LevelData d;
    std::string err;
    INK_REQUIRE(ParseLevelText(kSmallLevel, d, &err));
    INK_REQUIRE(d.id == "tiny");
    INK_REQUIRE(d.worldId == "meadows");
    INK_REQUIRE_EQ(d.w, 6);
    INK_REQUIRE_EQ(d.h, 4);
    // bounds in pixels
    INK_REQUIRE_EQ(d.bounds.w, 96.0);
    INK_REQUIRE_EQ(d.bounds.h, 64.0);
    // tile classification
    INK_REQUIRE(d.tiles[0] == TileType::Solid);       // (0,0)
    INK_REQUIRE(d.tiles[2] == TileType::Empty);       // (2,0)
    INK_REQUIRE(d.tiles[6 + 3] == TileType::Oneway);  // (3,1) '='
    INK_REQUIRE(d.tiles[3 * 6 + 0] == TileType::Solid);
}

INK_TEST(level_parser_entities_in_map) {
    LevelData d;
    std::string err;
    INK_REQUIRE(ParseLevelText(kSmallLevel, d, &err));
    // P at (3,0): x=3*16+2, y=0*16+2
    INK_REQUIRE_NEAR(d.playerSpawn.x, 50.0, 1e-9);
    INK_REQUIRE_NEAR(d.playerSpawn.y, 2.0, 1e-9);
    // C at (1,2): x=1*16+2, y=2*16+2-8
    INK_REQUIRE_EQ(static_cast<int>(d.checkpoints.size()), 1);
    INK_REQUIRE_NEAR(d.checkpoints[0].x, 18.0, 1e-9);
    INK_REQUIRE_NEAR(d.checkpoints[0].y, 26.0, 1e-9);
    // two 'i' coins
    INK_REQUIRE_EQ(static_cast<int>(d.collectibles.size()), 2);
    INK_REQUIRE(d.collectibles[0].type == "coin");
}

INK_TEST(level_parser_entity_lines) {
    LevelData d;
    std::string err;
    INK_REQUIRE(ParseLevelText(kSmallLevel, d, &err));
    INK_REQUIRE_EQ(static_cast<int>(d.npcs.size()), 1);
    INK_REQUIRE(d.npcs[0].id == "owl");
    INK_REQUIRE_EQ(static_cast<int>(d.spawns.size()), 1);
    INK_REQUIRE(d.spawns[0].type == "slome");
    INK_REQUIRE_EQ(static_cast<int>(d.platforms.size()), 1);
    INK_REQUIRE_NEAR(d.platforms[0].speed, 24.0, 1e-9);
    INK_REQUIRE(d.hasBoss);
    INK_REQUIRE(d.bossId == "patchling");
    INK_REQUIRE_NEAR(d.bossSpawn.x, 120.0, 1e-9);
}

INK_TEST(level_parser_solid_first_row_is_not_comment) {
    // Regression: a leading '#' inside [map] must be a solid tile, not a comment.
    const char* txt = "[level]\nid=r1\n[map]\n##\n##\n";
    LevelData d;
    std::string err;
    INK_REQUIRE(ParseLevelText(txt, d, &err));
    INK_REQUIRE_EQ(d.h, 2);
    INK_REQUIRE(d.tiles[0] == TileType::Solid);
    INK_REQUIRE(d.tiles[1] == TileType::Solid);
    INK_REQUIRE(d.tiles[2] == TileType::Solid);
    INK_REQUIRE(d.tiles[3] == TileType::Solid);
}

INK_TEST(level_parser_errors) {
    LevelData d;
    std::string err;
    INK_REQUIRE(!ParseLevelText("[bogus]\nx=1\n", d, &err));
    INK_REQUIRE(!err.empty());
    err.clear();
    INK_REQUIRE(!ParseLevelText("[map]\n##\n#@\n##\n", d, &err)); // illegal char
    INK_REQUIRE(!err.empty());
    err.clear();
    INK_REQUIRE(!ParseLevelText("[level]\nid=no_map\n", d, &err)); // no map rows
    INK_REQUIRE(!err.empty());
}
