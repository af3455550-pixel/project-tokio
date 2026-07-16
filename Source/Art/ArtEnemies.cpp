// Original enemy designs (§20): Slome (melee ink-blob), InkBat (swooper),
// QuillGunner (paper doll with a quiller), PaperWisp (swarm).
#include "Art/Art.h"
#include <cstdio>

namespace ink {

namespace {
// ---- Slome (16x12) ----
const std::vector<std::string> kSlomeIdle = {
    "................",
    "................",
    "................",
    "....KKKKKK......",
    "..KKSSSSSSSK....",
    ".KSSSSSSSSSSK...",
    ".KSBSSSSSSBSK...",
    ".KSSSSSSSSSSK...",
    ".KSSSKSSKKSSK...",
    "..KSSSSSSSSK....",
    "...KKKKKKKK.....",
    "................",
};
const std::vector<std::string> kSlomeWalk = {
    "................",
    "................",
    "................",
    "...KKKKKKKK.....",
    ".KKSSSSSSSSK....",
    "KSSSSSSSSSSSK...",
    "KSBSSSSSSSSSK...",
    "KSSSSSSSSSSSK...",
    "KSSKKSSSSKKSK...",
    ".KSSSSSSSSSSK...",
    "..KKKKKKKKKK....",
    "................",
};
const std::vector<std::string> kSlomeNotice = {
    "................",
    "................",
    "....KKKKKK......",
    "..KKSSSSSSSK....",
    ".KSSSSSSSSSSK...",
    ".KSWSSSSSSWSK...",
    ".KSBSSSSSSBSK...",
    ".KSSSSSSSSSSK...",
    ".KSSSKSSSSKSK...",
    "..KSSSSSSSSK....",
    "...KKKKKKKK.....",
    "................",
};
const std::vector<std::string> kSlomeTele = {
    "................",
    "................",
    "................",
    "................",
    "..KKKKKKKKKK....",
    ".KSSSSSSSSSSSK..",
    ".KSXSSSSSSXSK...",
    ".KSSSSSSSSSSSK..",
    ".KSSSSKKKKSSSK..",
    ".KSSSSSSSSSSK...",
    "..KKKKKKKKKKK...",
    "................",
};
const std::vector<std::string> kSlomeLunge = {
    "................",
    "................",
    "................",
    "....KKKK........",
    "..KKSSSSKK......",
    ".KSSSSSSSSSK....",
    "KSBSSSSSSSSSK...",
    "KSSSSSSSSSSSK...",
    "KSSSSKKKKSSSK...",
    ".KSSSSSSSSSSK...",
    "..KKKKKKKKKKK...",
    "................",
};
const std::vector<std::string> kSlomeStun = {
    "................",
    "................",
    "....Y..Y........",
    "....KKKKKK......",
    "..KKSSSSSSSK....",
    ".KSSSSSSSSSSK...",
    ".KSXSSSSSSXSK...",
    ".KSSSSSSSSSSK...",
    ".KSSSSSSSSSSK...",
    "..KSSSSSSSSK....",
    "...KKKKKKKK.....",
    "................",
};

// ---- InkBat (18x12) ----
const std::vector<std::string> kBatFlap0 = {
    "..................",
    "KK..............KK",
    "KKKK..........KKKK",
    ".KKKKK.KKKK.KKKKK.",
    "..KKKKKBBBBKKKKKK.",
    "....KKBBBBBBBBK...",
    ".....KBWBBBBWBK...",
    ".....KBBBBBBBBK...",
    "......KBBBBBBK....",
    ".......KKKKKK.....",
    "..................",
    "..................",
};
const std::vector<std::string> kBatFlap1 = {
    "..................",
    "..................",
    "..................",
    "KK....KKKKKK....KK",
    "KKKK.KKBBBBKK.KKKK",
    ".KKKKKKBBBBKKKKKK.",
    "..KKKBBBBBBBBKKK..",
    "....KBWBBBBWBK....",
    "....KBBBBBBBBK....",
    ".....KBBBBBBK.....",
    "......KKKKKK......",
    "..................",
};
const std::vector<std::string> kBatNotice = {
    "..................",
    "..................",
    "..................",
    "....KKKKKKKKKK....",
    "..KKKKBBBBKKKK....",
    ".KKKKBBBBBBBBKKK..",
    "....KBYBBBBYBK....",
    "....KBBBBBBBBK....",
    ".....KBBBBBBK.....",
    "......KKKKKK......",
    "..................",
    "..................",
};
const std::vector<std::string> kBatTele = {
    "..................",
    "..................",
    "..................",
    "......KKKKKK......",
    "....KKBBBBKK......",
    "...KBBBBBBBBK.....",
    "...KBYYYYYYBYK....",
    "...KBBBBBBBBK.....",
    "....KBBBBBBK......",
    ".....KKKKKK.......",
    "..................",
    "..................",
};
const std::vector<std::string> kBatDive = {
    "..................",
    "..................",
    "..KK..............",
    ".KKKKKK...........",
    ".KKKBBBBKK........",
    "..KBWBBBBK........",
    "..KBBBBBBK........",
    "..KKBBBBKK........",
    "....KKKKKK........",
    "..................",
    "..................",
    "..................",
};
const std::vector<std::string> kBatStun = {
    "..................",
    "......Y..Y........",
    "....KKKKKKKK......",
    "..KKKBBBBKKK......",
    ".KKBBBBBBBBKK.....",
    ".KBXBBBBBBXBK.....",
    ".KBBBBBBBBBK......",
    "..KBBBBBBBK.......",
    "...KKKKKKK........",
    "..................",
    "..................",
    "..................",
};

// ---- QuillGunner (16x20): paper doll with a feather quiller ----
const std::vector<std::string> kGunnerIdle = {
    ".....KKKKKK.....",
    "....KWWWWWWK....",
    "....KWCWWCWK....",
    "....KWWWWWWK....",
    ".....KKKKKK.....",
    "....KKTTTTKK....",
    "...KTWTTTTWTK...",
    "...KTWTTTTWTK...",
    "...KTWTTTTWTK...",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KKKKKKKK....",
    "....KT....TK....",
    "....KT....TK....",
    "....KT....TK....",
    "...KTTK..KTTK...",
    "..KKTTK..KKTTK..",
    "................",
    "................",
};
const std::vector<std::string> kGunnerWalk = {
    ".....KKKKKK.....",
    "....KWWWWWWK....",
    "....KWCWWCWK....",
    "....KWWWWWWK....",
    ".....KKKKKK.....",
    "....KKTTTTKK....",
    "...KTWTTTTWTK...",
    "...KTWTTTTWTK...",
    "...KTWTTTTWTK...",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KKKKKKKK....",
    "....KT....TK....",
    "....KT....TK....",
    "...KTK....TKK...",
    "..KTTK....KTTK..",
    "................",
    "................",
    "................",
};
const std::vector<std::string> kGunnerNotice = {
    ".....KKKKKK.....",
    "....KWWWWWWK....",
    "....KWKWWKWK....",
    "....KWWWWWWK....",
    ".....KKKKKK.....",
    "....KKTTTTKK....",
    "...KTWTTTTWTK...",
    "...KTWTTTTWTK...",
    "...KTWTTTTWTK...",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KKKKKKKK....",
    "....KT....TK....",
    "....KT....TK....",
    "....KT....TK....",
    "...KTTK..KTTK...",
    "..KKTTK..KKTTK..",
    "................",
    "................",
};
const std::vector<std::string> kGunnerTele = {
    "..........GK....",
    ".........GGK....",
    "....KKKKKGGK....",
    "...KWWWWWWGK....",
    "...KWCWWCWK.....",
    "....KKKKKKK.....",
    "...KTWTTTTK.....",
    "...KTWTTTTK.....",
    "...KTWTTTTK.....",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KKKKKKKK....",
    "....KT....TK....",
    "....KT....TK....",
    "....KT....TK....",
    "...KTTK..KTTK...",
    "..KKTTK..KKTTK..",
    "................",
    "................",
};
const std::vector<std::string> kGunnerShoot = {
    "................",
    "................",
    "....KKKKK.......",
    "...KWWWWWW......",
    "...KWCWWCW......",
    "....KKKKKK......",
    "...KTWTTTTWWWW..",
    "...KTWTTTTWWG...",
    "...KTWTTTTK.....",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KKKKKKKK....",
    "....KT....TK....",
    "....KT....TK....",
    "....KT....TK....",
    "...KTTK..KTTK...",
    "..KKTTK..KKTTK..",
    "................",
    "................",
};
const std::vector<std::string> kGunnerStun = {
    ".....Y..Y.......",
    ".....KKKKKK.....",
    "....KWWWWWWK....",
    "....KWKWWKWK....",
    "....KWWWWWWK....",
    ".....KKKKKK.....",
    "....KKTTTTKK....",
    "...KTWTTTTWTK...",
    "...KTWTTTTWTK...",
    "...KTWTTTTWTK...",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KTTTTTTK....",
    "....KKKKKKKK....",
    "....KT....TK....",
    "....KT....TK....",
    "...KTTK..KTTK...",
    "..KKTTK..KKTTK..",
    "................",
    "................",
};

// ---- PaperWisp (12x12) ----
const std::vector<std::string> kWisp0 = {
    "............",
    "............",
    "....KK......",
    "..KKWWK.....",
    ".KWWWWK.....",
    ".KWKKWWK....",
    ".KWWWWWWK...",
    ".KWWWWWWK...",
    "..KWWWWK....",
    "...KWWK.....",
    "....KK......",
    "............",
};
const std::vector<std::string> kWisp1 = {
    "............",
    "............",
    "......KK....",
    ".....KWWK...",
    ".....KWWWWK.",
    "....KWWKKW..",
    "....KWWWWWW.",
    "...KWWWWWWK.",
    "....KWWWWK..",
    ".....KWWK...",
    "......KK....",
    "............",
};
const std::vector<std::string> kWispDive = {
    "............",
    "............",
    "............",
    "....KKK.....",
    "..KKWWWWK...",
    ".KWWKKWWWWK.",
    ".KWWWWWWWWK.",
    ".KWWWWWWWWK.",
    "..KWWWWWWK..",
    "...KKWWKK...",
    "....KKKK....",
    "............",
};
const std::vector<std::string> kWispStun = {
    "............",
    "....Y.......",
    "..Y.KKK.....",
    ".....KWWK...",
    "....KWWWWK..",
    "...KWWKKW...",
    "...KWWWWWWK.",
    "...KWWWWWWK.",
    "....KWWWWK..",
    ".....KKWWK..",
    "......KKK...",
    "............",
};
} // namespace

void BuildEnemyArt(SpriteBank& bank) {
    auto Key = [](const char* base, int frame) {
        char b[16];
        std::snprintf(b, sizeof(b), "_%02d", frame);
        return std::string(base) + b;
    };
    struct Frame {
        const char* base;
        int frame;
        int w, h;
        const std::vector<std::string>* rows;
    };
    const Frame frames[] = {
        {"slome_idle", 0, 16, 12, &kSlomeIdle},
        {"slome_walk", 1, 16, 12, &kSlomeWalk},
        {"slome_notice", 0, 16, 12, &kSlomeNotice},
        {"slome_tele", 0, 16, 12, &kSlomeTele},
        {"slome_lunge", 0, 16, 12, &kSlomeLunge},
        {"slome_stun", 0, 16, 12, &kSlomeStun},
        {"inkbat_flap", 0, 18, 12, &kBatFlap0},
        {"inkbat_flap", 1, 18, 12, &kBatFlap1},
        {"inkbat_notice", 0, 18, 12, &kBatNotice},
        {"inkbat_tele", 0, 18, 12, &kBatTele},
        {"inkbat_dive", 0, 18, 12, &kBatDive},
        {"inkbat_stun", 0, 18, 12, &kBatStun},
        {"quillgunner_idle", 0, 16, 20, &kGunnerIdle},
        {"quillgunner_walk", 1, 16, 20, &kGunnerWalk},
        {"quillgunner_notice", 0, 16, 20, &kGunnerNotice},
        {"quillgunner_tele", 0, 16, 20, &kGunnerTele},
        {"quillgunner_shoot", 0, 16, 20, &kGunnerShoot},
        {"quillgunner_stun", 0, 16, 20, &kGunnerStun},
        {"wisp_wobble", 0, 12, 12, &kWisp0},
        {"wisp_wobble", 1, 12, 12, &kWisp1},
        {"wisp_dive", 0, 12, 12, &kWispDive},
        {"wisp_stun", 0, 12, 12, &kWispStun},
    };
    for (const auto& f : frames) {
        PixelCanvas c(f.w, f.h);
        InitSharedPalette(c);
        c.Grid(0, 0, *f.rows);
        c.AddTo(bank, Key(f.base, f.frame));
    }
}

} // namespace ink
