// Original NPCs (§24), tiles and pickups for the vertical slice.
#include "Art/Art.h"

namespace ink {

namespace {
// ---- Birdie (16x14) — a friendly paper sparrow NPC ----
const std::vector<std::string> kBird0 = {
    "................",
    "................",
    "......KKKK......",
    "....KKWWWWKK....",
    "...KWWWWWWWWK...",
    "...KWKWWWWWWK...",
    "...KWWWWWWWWK...",
    "....KWWWWWWK....",
    ".....KWWWWK.....",
    "....KKKKKKK.....",
    "......KYYK......",
    ".....KK..KK.....",
    "................",
    "................",
};
const std::vector<std::string> kBird1 = {
    "................",
    "................",
    "......KKKK......",
    "....KKWWWWKK....",
    "...KWWWWWWWWK...",
    "...KWWWWWWWWK...",
    "...KWWKWWWWWK...",
    "....KWWWWWWK....",
    ".....KWWWWK.....",
    "....KKKKKKK.....",
    "......KYYK......",
    ".....KK..KK.....",
    "................",
    "................",
};
// ---- Inkwell Old Owl (18x22) — the sage NPC with the quill-stick ----
const std::vector<std::string> kOwl0 = {
    "..................",
    "......KKKK........",
    "....KKWWWWKK......",
    "...KWWWWWWWWK.....",
    "...KWKKWWKKWK.....",
    "...KWKKWWKKWK.....",
    "...KWWWWWWWWK.....",
    "....KKWWWWKK......",
    "..KKKKWWWWKKKK....",
    ".KWWWWWWWWWWWWK...",
    ".KWKKWWWWWWKKWK...",
    ".KWWWWWWWWWWWWK...",
    ".KWKKWWWWWWKKWK...",
    ".KWWWWWWWWWWWWK...",
    ".KWKKWWWWWWKKWK...",
    ".KWWWWWWWWWWWWK...",
    "..KWWWWWWWWWWK....",
    "...KKKKKKKKKK.....",
    "......KKKKK.......",
    ".....KK..KK.......",
    "..................",
    "..................",
};
const std::vector<std::string> kOwl1 = {
    "..................",
    "......KKKK........",
    "....KKWWWWKK......",
    "...KWWWWWWWWK.....",
    "...KWKKWWKKWK.....",
    "...KWKKWWKKWK.....",
    "...KWWWWWWWWK.....",
    "....KKWWWWKK......",
    "..KKKKWWWWKKKK....",
    ".KWWWWWWWWWWWWK...",
    ".KWKKWWWWWWKKWK...",
    ".KWWWWWWWWWWWWK...",
    ".KWKKWWWWWWKKWK...",
    ".KWWWWWWWWWWWWK...",
    ".KWKKWWWWWWKKWK...",
    ".KWWWWWWWWWWWWK...",
    "..KWWWWWWWWWWK....",
    "...KKKKKKKKKK.....",
    "......KKKKK.......",
    ".....KK..KK.......",
    "..................",
    "..................",
};
} // namespace

void BuildNpcArt(SpriteBank& bank) {
    struct Frame {
        const char* name;
        int w, h;
        const std::vector<std::string>* rows;
    };
    const Frame frames[] = {
        {"birdie_00", 16, 14, &kBird0},
        {"birdie_01", 16, 14, &kBird1},
        {"owl_00", 18, 22, &kOwl0},
        {"owl_01", 18, 22, &kOwl1},
    };
    for (const auto& f : frames) {
        PixelCanvas c(f.w, f.h);
        InitSharedPalette(c);
        c.Grid(0, 0, *f.rows);
        c.AddTo(bank, f.name);
    }
}

void BuildTileArt(SpriteBank& bank) {
    const int T = 16;
    // Solid tile: ink-dirt with speckles
    {
        PixelCanvas c(T, T);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('T', 0xFF6B5A3E);
        c.SetPalette('t', 0xFF5C4D34);
        c.Rect(0, 0, T, T, 't');
        for (int i = 0; i < 14; ++i) {
            int x = (i * 5 + 3) % 15;
            int y = (i * 7 + 2) % 15;
            c.Px(x, y, 'T');
        }
        c.AddTo(bank, "tile_solid");
    }
    // Solid tile + grass lip (drawn when exposed)
    {
        PixelCanvas c(T, T);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('T', 0xFF6B5A3E);
        c.SetPalette('t', 0xFF5C4D34);
        c.SetPalette('D', 0xFF5C7A2E);
        c.SetPalette('d', 0xFF74923A);
        c.Rect(0, 0, T, T, 't');
        for (int i = 0; i < 14; ++i) {
            int x = (i * 5 + 3) % 15;
            int y = (i * 7 + 5) % 15;
            c.Px(x, y, 'T');
        }
        c.Rect(0, 0, T, 4, 'D');
        c.Rect(0, 0, T, 2, 'd');
        for (int i = 0; i < 4; ++i)
            c.Px(2 + i * 4, 4, 'd');
        c.AddTo(bank, "tile_grass");
    }
    // Oneway planks
    {
        PixelCanvas c(T, T);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('T', 0xFF8A6B45);
        c.Rect(0, 0, T, 6, 'T');
        c.Rect(0, 0, T, 1, 'K');
        c.Rect(0, 5, T, 1, 'K');
        c.Line(5, 0, 5, 6, 'K');
        c.Line(11, 0, 11, 6, 'K');
        c.AddTo(bank, "tile_oneway");
    }
    // Hazard: ink spikes
    {
        PixelCanvas c(T, T);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('I', 0xFF7FD4FF);
        for (int i = 0; i < 4; ++i) {
            int x = i * 4;
            for (int j = 0; j < 10; ++j)
                c.Line(x + j / 2, 15 - j, x + j / 2 + (j / 2), 15 - j, 'K');
            c.Px(x + 2, 4, 'I');
        }
        c.AddTo(bank, "tile_hazard");
    }
    // Breakable film brick
    {
        PixelCanvas c(T, T);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('B', 0xFF3A3153);
        c.SetPalette('C', 0xFFBFE3FF);
        c.Rect(0, 0, T, T, 'B');
        c.Rect(0, 0, T, T, 'B');
        c.Line(0, 7, 15, 7, 'K');
        c.Line(7, 0, 7, 7, 'K');
        c.Line(3, 8, 3, 15, 'K');
        c.Line(11, 8, 11, 15, 'K');
        c.Line(2, 2, 4, 4, 'C');
        c.Line(10, 10, 12, 12, 'C');
        c.AddTo(bank, "tile_break");
    }
    // Door (level exit) — a film frame
    {
        PixelCanvas c(T, T);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('B', 0xFF3A3153);
        c.SetPalette('Y', 0xFFFFE08A);
        c.Rect(0, 0, T, T, 'B');
        c.Rect(3, 3, 10, 12, 'K');
        c.Rect(4, 4, 8, 10, 0xFF17131F);
        c.Rect(1, 1, 2, 2, 'Y');
        c.Rect(13, 1, 2, 2, 'Y');
        c.AddTo(bank, "tile_door");
    }
    // Checkpoint (inactive / active)
    for (int act = 0; act < 2; ++act) {
        PixelCanvas c(T, T);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('T', 0xFF8A6B45);
        c.SetPalette('R', 0xFFC8452E);
        c.SetPalette('W', 0xFFF5F2EA);
        c.Line(4, 2, 4, 15, 'T');
        c.Rect(2, 14, 5, 2, 'T');
        if (act) {
            c.Rect(5, 2, 9, 6, 'R');
            c.Line(13, 5, 5, 5, 'K');
        } else {
            c.Rect(5, 2, 9, 6, 'W');
        }
        c.AddTo(bank, act ? "tile_cp_on" : "tile_cp_off");
    }
}

void BuildPickupArt(SpriteBank& bank) {
    // Ink coin (8x8, two spin frames)
    for (int f = 0; f < 2; ++f) {
        PixelCanvas c(8, 8);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('Y', 0xFFFFE08A);
        c.SetPalette('I', 0xFF7FD4FF);
        int w = f == 0 ? 8 : 4;
        int ox = f == 0 ? 0 : 2;
        c.Rect(ox, 1, w, 6, 'K');
        c.Rect(ox + 1, 2, w - 2, 4, 'Y');
        if (f == 0)
            c.Px(3, 3, 'I');
        c.AddTo(bank, f ? "coin_01" : "coin_00");
    }
    // Film strip (12x10)
    {
        PixelCanvas c(12, 10);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('B', 0xFF3A3153);
        c.SetPalette('Y', 0xFFFFE08A);
        c.Rect(0, 0, 12, 10, 'B');
        c.Rect(0, 0, 12, 2, 'K');
        c.Rect(0, 8, 12, 2, 'K');
        for (int i = 0; i < 3; ++i) {
            c.Rect(1 + i * 4, 0, 2, 2, 'Y');
            c.Rect(1 + i * 4, 8, 2, 2, 'Y');
        }
        c.Px(5, 4, 'Y');
        c.Px(6, 5, 'Y');
        c.AddTo(bank, "film_strip");
    }
    // The Master Frame (16x16, glowing)
    {
        PixelCanvas c(16, 16);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('I', 0xFF7FD4FF);
        c.SetPalette('C', 0xFFBFE3FF);
        c.Rect(1, 1, 14, 14, 'K');
        c.Rect(3, 3, 10, 10, 'C');
        c.Rect(5, 5, 6, 6, 'I');
        c.Px(2, 2, 'I');
        c.Px(13, 2, 'I');
        c.Px(2, 13, 'I');
        c.Px(13, 13, 'I');
        c.AddTo(bank, "master_frame");
    }
    // Director's Stamp (10x10)
    {
        PixelCanvas c(10, 10);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('R', 0xFFC8452E);
        c.Ellipse(5, 5, 4, 4, 'K');
        c.Ellipse(5, 5, 3, 3, 'R');
        c.Px(4, 4, 'K');
        c.Px(5, 5, 'K');
        c.Px(6, 6, 'K');
        c.AddTo(bank, "director_stamp");
    }
    // Ink vial (heal, 8x10)
    {
        PixelCanvas c(8, 10);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('C', 0xFFBFE3FF);
        c.SetPalette('I', 0xFF7FD4FF);
        c.Rect(2, 0, 4, 2, 'K');
        c.Rect(1, 2, 6, 7, 'K');
        c.Rect(2, 3, 4, 5, 'C');
        c.Rect(2, 5, 4, 3, 'I');
        c.AddTo(bank, "ink_vial");
    }
    // Boss HP pip
    {
        PixelCanvas c(8, 8);
        c.SetPalette('K', 0xFF241E33);
        c.SetPalette('I', 0xFF7FD4FF);
        c.Ellipse(4, 4, 3, 3, 'K');
        c.Ellipse(4, 4, 2, 2, 'I');
        c.AddTo(bank, "hp_pip");
    }
}

} // namespace ink
