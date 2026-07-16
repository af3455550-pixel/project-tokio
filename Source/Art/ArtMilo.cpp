// MILO INKWHISTLE — original character design (§11): a small elastic
// cartoon with a big head, huge expressive eyes, white gloves, oversized
// shoes, and a Living-Ink symbol on the chest. 16x20 frames, hand-drawn
// as string grids. Squash & stretch is baked into the poses (§44).
#include "Art/Art.h"
#include <cstdio>

namespace ink {

void InitSharedPalette(PixelCanvas& c) {
    c.SetPalette('K', 0xFF241E33); // ink outline
    c.SetPalette('S', 0xFFF6E7C8); // cream face/skin
    c.SetPalette('B', 0xFF3A3153); // ink suit (dark)
    c.SetPalette('W', 0xFFF5F2EA); // gloves
    c.SetPalette('I', 0xFF7FD4FF); // living ink cyan
    c.SetPalette('P', 0xFFE8A0A8); // blush
    c.SetPalette('G', 0xFF8C8678); // grey straw
    c.SetPalette('Y', 0xFFFFE08A); // warm highlight
    c.SetPalette('R', 0xFFC8452E); // red accent
    c.SetPalette('D', 0xFF5C7A2E); // leaf green
    c.SetPalette('T', 0xFF8A6B45); // wood brown
    c.SetPalette('C', 0xFFBFE3FF); // pale blue
    c.SetPalette('X', 0xFF17131F); // deep shadow
}

namespace {
// Register a frame as "base_00"/"base_01"... matching the renderer's key
// convention (AnimName: base + zero-padded 2-digit frame).
std::string Key(const char* base, int frame) {
    char b[16];
    std::snprintf(b, sizeof(b), "_%02d", frame);
    return std::string(base) + b;
}

const std::vector<std::string> kMiloIdle00 = {
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "...KKBIIBBBK....",
    "..KSBKIIIBKBK...",
    "..KSKBBBBBBKS...",
    "..KSKBBBBBBKS...",
    "...KKBBBBBBK....",
    "....KBBBBBK.....",
    "....KWWKKWWK....",
    "....KWK..KWK....",
    ".....K....K.....",
    "..KKKK...KKKK...",
    ".KSSSK...KSSSK..",
};
const std::vector<std::string> kMiloIdle01 = { // blink
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSKKKKSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "...KKBIIBBBK....",
    "..KSBKIIIBKBK...",
    "..KSKBBBBBBKS...",
    "..KSKBBBBBBKS...",
    "...KKBBBBBBK....",
    "....KBBBBBK.....",
    "....KWWKKWWK....",
    "....KWK..KWK....",
    ".....K....K.....",
    "..KKKK...KKKK...",
    ".KSSSK...KSSSK..",
};
const std::vector<std::string> kMiloRun00 = {
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "...KKBIIBBBK....",
    "..KSBKIIIBKBK...",
    "..KSKBBBBBBKS...",
    "..KSKBBBBBBKS...",
    "...KKBBBBBBK....",
    "....KBBBBBK.....",
    ".....KWWKKK.....",
    "....KWK.WK......",
    ".....K..K.......",
    "...KKKK.KKKK....",
    "..KSSSK.KSSSK...",
};
const std::vector<std::string> kMiloRun01 = {
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "...KKBIIBBBK....",
    "..KSBKIIIBKBK...",
    "..KSKBBBBBBKS...",
    "..KSKBBBBBBKS...",
    "...KKBBBBBBK....",
    "....KBBBBBK.....",
    "....KWWKKWWK....",
    ".....KK..KK.....",
    "......K..K......",
    "....KKKK.KKKK...",
    "...KSSSK..KSSSK.",
};
const std::vector<std::string> kMiloRun02 = {
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "...KKBIIBBBK....",
    "..KSBKIIIBKBK...",
    "..KSKBBBBBBKS...",
    "..KSKBBBBBBKS...",
    "...KKBBBBBBK....",
    "....KBBBBBK.....",
    ".....KWWKKK.....",
    ".....KK.WK......",
    ".....K..K.......",
    "...KKKK..KKKK...",
    "..KSSSK...KSSSK.",
};
const std::vector<std::string> kMiloJump = {
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    "...KKBBBBBBKK...",
    "..KBKBIIBBBKBK..",
    ".KWWKBKIIIBKBK..",
    ".KWKKBKBBBBKBK..",
    "..KSKBBBBBBKS...",
    "...KKBBBBBBK....",
    "....KBBBBBK.....",
    "....KWWKWWK.....",
    "...KKWK..KWK....",
    "....KK....KK....",
    "..KKKK...KKKK...",
    "..KSSSK..KSSSK..",
};
const std::vector<std::string> kMiloFall = {
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "...KKBIIBBBK....",
    ".KWWKIIIBKBKW...",
    ".KWKKBKBBBBKBK..",
    "..KSKBBBBBBKS...",
    "...KKBBBBBBK....",
    "....KBBBBBK.....",
    "....KWWKKWWK....",
    "....KWK..KWK....",
    "...KWK....KWK...",
    "..KKKK....KKKK..",
    ".KSSSK....KSSSK.",
};
const std::vector<std::string> kMiloCrouch = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "...KSSSSSSSSK...",
    "..KKKBBBBBBKK...",
    ".KSKBIIBBIBKS...",
    ".KWWKBBBBKBWK...",
    "...KKKKKKKKK....",
    "..KSSKKKKKSSK...",
    ".KSSSK....KSSSK.",
    "................",
    "................",
};
const std::vector<std::string> kMiloDash = {
    "................",
    "................",
    "...X...KKKK.....",
    ".....KKSSSSKK...",
    "....KSSSSSSSSK..",
    "....KSSBSSBSSK..",
    "....KSSSSSSSSK..",
    ".....KSSSSSSK...",
    "....KKSSSSKK....",
    "..KKKBBBBBBKK...",
    "KWKBBIIIBBBK....",
    "KKKBBBBBBBK.....",
    "....BBBBBBK.....",
    "....BBBBBK......",
    "....KWWKKK......",
    "....KWKWK.......",
    ".....K.K........",
    "....KKK.KKKK....",
    "...KSSSKKSSSK...",
    "................",
};
const std::vector<std::string> kMiloAttack00 = {
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "...KKBIIBBBK....",
    "..KSBKIIIBBKWB..",
    "..KSKBBBBBBKWWB.",
    "..KSKBBBBBBKKKK.",
    "...KKBBBBBBK....",
    "....KBBBBBK.....",
    "....KWWKKWWK....",
    "....KWK..KWK....",
    ".....K....K.....",
    "..KKKK...KKKK...",
    ".KSSSK...KSSSK..",
};
const std::vector<std::string> kMiloAttack01 = {
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "...KKBIIBBBKY...",
    "..KSBKIIIBBKWYY.",
    "..KSKBBBBBBKWY..",
    "..KSKBBBBBBKK...",
    "...KKBBBBBBK....",
    "....KBBBBBK.....",
    "....KWWKKWWK....",
    "....KWK..KWK....",
    ".....K....K.....",
    "..KKKK...KKKK...",
    ".KSSSK...KSSSK..",
};
const std::vector<std::string> kMiloParry = {
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSSBSSBSSSSK..",
    "..KSSSSSSSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "...KKBIIBBBK....",
    "..KSBKIIIBBKK...",
    "..KSKBBBBBBKKKK.",
    "..KSKBBBBBBBBIC.",
    "...KKBBBBBBCIC..",
    "....KBBBBBBCIC..",
    "....KWWKKWWK....",
    "....KWK..KWK....",
    ".....K....K.....",
    "..KKKK...KKKK...",
    ".KSSSK...KSSSK..",
};
const std::vector<std::string> kMiloHurt = {
    "................",
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSKXSSXSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    ".....KBBBBBK....",
    "....KBIIIBBK....",
    "...KBKIIIBKBK...",
    "...KSKBBBBBKS...",
    "...KSKBBBBBKS...",
    "....KKBBBBBK....",
    ".....KBBBBBK....",
    ".....KWWKKW.....",
    "....KWK.WK......",
    ".....K..K.......",
    "....KKK.KKKK....",
    "...KSSSK.KSSSK..",
};
const std::vector<std::string> kMiloDead = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "......KKKK......",
    "....KKSSSSKK....",
    "...KSSSSSSSSK...",
    "..KSSSSSSSSSSK..",
    "..KSKXSSXSSSSK..",
    "...KSSSSSSSSK...",
    "....KKSSSSKK....",
    "..KKKBBBBBBKK...",
    ".KSKBIIBBIBKS...",
    ".KSKKBBBBBKSK...",
    ".KSSKBBBBBKSS...",
    "..KKKKKKKKKKK...",
    ".KSSSKKKKKKSSSK.",
    "KSSSK.......KSSK",
    "................",
};
} // namespace

void BuildMiloArt(SpriteBank& bank) {
    struct Frame {
        const char* base;
        int frame;
        const std::vector<std::string>* rows;
    };
    const Frame frames[] = {
        {"milo_idle", 0, &kMiloIdle00},  {"milo_idle", 1, &kMiloIdle01},
        {"milo_run", 0, &kMiloRun00},    {"milo_run", 1, &kMiloRun01},
        {"milo_run", 2, &kMiloRun02},    {"milo_jump", 0, &kMiloJump},
        {"milo_fall", 0, &kMiloFall},    {"milo_crouch", 0, &kMiloCrouch},
        {"milo_dash", 0, &kMiloDash},    {"milo_attack", 0, &kMiloAttack00},
        {"milo_attack", 1, &kMiloAttack01}, {"milo_parry", 0, &kMiloParry},
        {"milo_hurt", 0, &kMiloHurt},    {"milo_dead", 0, &kMiloDead},
        {"milo_super", 0, &kMiloJump},
    };
    for (const auto& f : frames) {
        PixelCanvas c(16, 20);
        InitSharedPalette(c);
        c.Grid(0, 0, *f.rows);
        c.AddTo(bank, Key(f.base, f.frame));
    }
}

} // namespace ink
