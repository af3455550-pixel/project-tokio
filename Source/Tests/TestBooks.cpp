#include "Bosses/Boss.h"
#include "Charms/CharmDef.h"
#include "Enemies/EnemyBook.h"
#include "Player/PlayerStats.h"
#include "Weapons/WeaponDef.h"
#include "Test.h"

using namespace ink;

INK_TEST(weapon_book) {
    const char* json = R"({
      "weapons": [
        {
          "id": "ink_blaster",
          "name": "Ink Blaster",
          "fireCooldown": 0.16,
          "shot": { "id": "ink", "speed": 460, "damage": 1 },
          "charged": { "id": "ink_charged", "speed": 660, "damage": 2 }
        },
        { "id": "quill_repeater", "name": "Quill Repeater", "fireCooldown": 0.09 }
      ]
    })";
    WeaponBook book;
    std::string err;
    INK_REQUIRE(book.LoadJson(json, &err));
    INK_REQUIRE_EQ(static_cast<int>(book.All().size()), 2);
    const WeaponDef* w = book.Get("ink_blaster");
    INK_REQUIRE(w != nullptr);
    INK_REQUIRE_NEAR(w->fireCooldown, 0.16, 1e-9);
    INK_REQUIRE_NEAR(w->shot.damage, 1.0, 1e-9);
    INK_REQUIRE_NEAR(w->charged.damage, 2.0, 1e-9);
    INK_REQUIRE(book.Get("nope") == nullptr);
    INK_REQUIRE(!book.Names().empty());
    // Default must exist and be usable.
    INK_REQUIRE(!book.Default().id.empty());
}

INK_TEST(enemy_book) {
    const char* json = R"({
      "enemies": [
        { "id": "slome", "name": "Slome", "sprite": "slome", "hp": 3, "speed": 60, "attackDamage": 1 },
        { "id": "inkbat", "name": "Inkbat", "sprite": "inkbat", "hp": 2, "speed": 90 }
      ]
    })";
    EnemyBook book;
    std::string err;
    INK_REQUIRE(book.LoadJson(json, &err));
    INK_REQUIRE_EQ(static_cast<int>(book.All().size()), 2);
    const EnemyDef* e = book.Get("slome");
    INK_REQUIRE(e != nullptr);
    INK_REQUIRE_EQ(e->hp, 3);
    INK_REQUIRE_NEAR(e->speed, 60.0, 1e-9);
    INK_REQUIRE(book.Get("inkbat")->hp == 2);
}

INK_TEST(boss_book) {
    const char* json = R"({
      "bosses": [
        {
          "id": "patchling",
          "name": "The Patchling",
          "maxHp": 26,
          "miniBoss": true,
          "phases": [ { "hpFrac": 0.55, "label": "SEWN TIGHT" } ]
        },
        {
          "id": "barnaby",
          "name": "Barnaby Patchface",
          "maxHp": 90,
          "phases": [
            { "hpFrac": 0.66, "label": "ONE" },
            { "hpFrac": 0.33, "label": "TWO" }
          ]
        }
      ]
    })";
    BossBook book;
    std::string err;
    INK_REQUIRE(book.LoadJson(json, &err));
    INK_REQUIRE_EQ(static_cast<int>(book.All().size()), 2);
    const BossDef* b = book.Get("barnaby");
    INK_REQUIRE(b != nullptr);
    INK_REQUIRE_EQ(b->maxHp, 90);
    INK_REQUIRE_EQ(static_cast<int>(b->phases.size()), 2);
    const BossDef* mini = book.Get("patchling");
    INK_REQUIRE(mini && mini->miniBoss);
}

INK_TEST(charm_book_and_apply) {
    const char* json = R"({
      "charms": [
        { "id": "echo_heart", "name": "Echo Heart", "mods": { "reviveOnce": true } },
        {
          "id": "quick_thread",
          "name": "Quick Thread",
          "mods": { "moveSpeedMul": 1.2, "dashCooldownMul": 0.8 }
        }
      ]
    })";
    CharmBook book;
    std::string err;
    INK_REQUIRE(book.LoadJson(json, &err));
    INK_REQUIRE_EQ(static_cast<int>(book.All().size()), 2);
    const CharmDef* q = book.Get("quick_thread");
    INK_REQUIRE(q && q->mods.moveSpeedMul > 1.0);

    PlayerStats base;
    base.maxHp = 3;
    base.walkSpeed = 100.0;
    base.dashCooldown = 0.32;
    const CharmDef* e = book.Get("echo_heart");
    const std::vector<const CharmDef*> active{q, e};
    PlayerStats out = CharmBook::Apply(base, active);
    INK_REQUIRE_NEAR(out.walkSpeed, 120.0, 1e-9);
    INK_REQUIRE_NEAR(out.dashCooldown, 0.32 * 0.8, 1e-9);
}
