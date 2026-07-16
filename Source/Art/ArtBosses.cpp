// Original boss designs (§17) drawn procedurally with primitives.
//  - Barnaby Patchface: a towering scarecrow of patchwork cloth and straw,
//    wielding a scythe of living ink. Three phases.
//  - The Patch (master boss): a stitched doll of torn film and white cloth,
//    with a needle of pure ink.
#include "Art/Art.h"
#include <cstdio>

namespace ink {

namespace {
struct BarnabyPose {
    int scythe = 0; // 0 down, 1 raised
    int arms = 0;   // 0 down, 1 up
    int bodyH = 7;  // body rows (shorter = crouched)
    int eye = 0;    // 0 stitch X, 1 glowing ink
    int mouth = 0;  // 0 stitched, 1 open
    int lean = 0;   // head lean units
    int leg = 0;    // 0/1 walk cycle
    int glow = 0;   // 1 = storm glow
};

void DrawBarnaby(PixelCanvas& c, const BarnabyPose& p) {
    const int u = 3; // scale
    int cx = 8 + p.lean;
    // Legs (units rows 15..20)
    int lo = p.leg == 0 ? 0 : 1;
    c.Line(cx * u - 2 * u, 15 * u, (cx - 2 + lo) * u, 20 * u, 'K');
    c.Line((cx + 2) * u, 15 * u, (cx + 2 - lo) * u, 20 * u, 'K');
    c.Rect((cx - 3 + lo) * u, 20 * u, 3 * u, u, 'K');
    c.Rect((cx + 1 - lo) * u, 20 * u, 3 * u, u, 'K');
    // Body patchwork
    int bTop = 8;
    int bBot = 8 + p.bodyH;
    c.Rect((cx - 4) * u, bTop * u, 8 * u, p.bodyH * u, 'B');
    c.Rect((cx - 4) * u, bTop * u, 8 * u, u, 'K');
    c.Rect((cx - 4) * u, (bBot - 1) * u, 8 * u, u, 'K');
    c.Rect((cx - 4) * u, bTop * u, u, p.bodyH * u, 'K');
    c.Rect((cx + 3) * u, bTop * u, u, p.bodyH * u, 'K');
    c.Rect((cx - 3) * u, (bTop + 1) * u, 3 * u, 2 * u, 'R');
    c.Rect((cx) * u, (bTop + 3) * u, 3 * u, 2 * u, 'D');
    c.Rect((cx - 3) * u, (bTop + 4) * u, 3 * u, 2 * u, 'D');
    c.Rect((cx + 1) * u, (bTop + 5) * u, 2 * u, 2 * u, 'R');
    // Arms
    if (p.arms == 1) {
        c.Line((cx - 4) * u, (bTop + 1) * u, (cx - 6) * u, (bTop - 3) * u, 'K');
        c.Line((cx + 4) * u, (bTop + 1) * u, (cx + 6) * u, (bTop - 3) * u, 'K');
    } else {
        c.Line((cx - 4) * u, (bTop + 1) * u, (cx - 5) * u, (bTop + 5) * u, 'K');
        c.Line((cx + 4) * u, (bTop + 1) * u, (cx + 5) * u, (bTop + 5) * u, 'K');
    }
    // Head (straw)
    int hBot = bTop;
    c.Rect((cx - 3) * u, (hBot - 7) * u, 6 * u, 7 * u, 'G');
    c.Rect((cx - 3) * u, (hBot - 7) * u, 6 * u, u, 'K');
    c.Rect((cx - 3) * u, (hBot - 1) * u, 6 * u, u, 'K');
    c.Rect((cx - 3) * u, (hBot - 7) * u, u, 7 * u, 'K');
    c.Rect((cx + 2) * u, (hBot - 7) * u, u, 7 * u, 'K');
    // Straw strands
    c.Line((cx - 2) * u, (hBot - 7) * u, (cx - 3) * u, (hBot - 8) * u, 'Y');
    c.Line((cx) * u, (hBot - 7) * u, (cx) * u, (hBot - 9) * u, 'Y');
    c.Line((cx + 2) * u, (hBot - 7) * u, (cx + 3) * u, (hBot - 8) * u, 'Y');
    // Eyes
    if (p.eye == 1) {
        c.Px((cx - 2) * u, (hBot - 5) * u, 'I');
        c.Px((cx - 1) * u, (hBot - 4) * u, 'I');
        c.Px((cx - 2) * u, (hBot - 4) * u, 'I');
        c.Px((cx - 1) * u, (hBot - 5) * u, 'I');
        c.Px((cx + 1) * u, (hBot - 5) * u, 'I');
        c.Px((cx + 2) * u, (hBot - 4) * u, 'I');
        c.Px((cx + 1) * u, (hBot - 4) * u, 'I');
        c.Px((cx + 2) * u, (hBot - 5) * u, 'I');
    } else {
        c.Line((cx - 3) * u, (hBot - 5) * u, (cx - 1) * u, (hBot - 3) * u, 'K');
        c.Line((cx - 1) * u, (hBot - 5) * u, (cx - 3) * u, (hBot - 3) * u, 'K');
        c.Line((cx + 1) * u, (hBot - 5) * u, (cx + 3) * u, (hBot - 3) * u, 'K');
        c.Line((cx + 3) * u, (hBot - 5) * u, (cx + 1) * u, (hBot - 3) * u, 'K');
    }
    // Mouth
    if (p.mouth == 1) {
        c.Rect((cx - 2) * u, (hBot - 2) * u, 4 * u, 2 * u, 'X');
        c.Rect((cx - 2) * u, (hBot - 2) * u, 4 * u, u, 'K');
    } else {
        c.Line((cx - 2) * u, (hBot - 2) * u, (cx + 2) * u, (hBot - 2) * u, 'K');
        c.Line((cx - 1) * u, (hBot - 3) * u, (cx - 1) * u, (hBot - 1) * u, 'K');
        c.Line((cx + 1) * u, (hBot - 3) * u, (cx + 1) * u, (hBot - 1) * u, 'K');
    }
    // Scythe
    int sx = cx + 5;
    if (p.scythe == 1) {
        c.Line(sx * u, (bBot - 1) * u, (sx + 1) * u, (hBot - 9) * u, 'K');
        c.Rect((sx - 1) * u, (hBot - 10) * u, 7 * u, u, 'C');
        c.Rect((sx - 1) * u, (hBot - 9) * u, 6 * u, u, 'C');
        c.Rect((sx - 1) * u, (hBot - 10) * u, 7 * u, u, 'K');
    } else {
        c.Line(sx * u, (bBot - 1) * u, (sx + 2) * u, 20 * u, 'K');
        c.Rect((sx - 3) * u, 19 * u, 7 * u, u, 'C');
        c.Rect((sx - 3) * u, 18 * u, 6 * u, u, 'C');
        c.Rect((sx - 3) * u, 19 * u, 7 * u, u, 'K');
    }
    // Storm glow aura
    if (p.glow == 1) {
        for (int i = 0; i < 14; ++i) {
            int gx = 1 + (i * 3) % 15;
            int gy = (i * 5) % 20;
            c.Px(gx * u, (gy + 1) * u, 'I');
        }
    }
}

struct PatchPose {
    int needle = 0;  // 0 down, 1 up
    int arms = 0;
    int bodyH = 6;
    int mouth = 0;   // 0 stitched, 1 torn open
    int lean = 0;
    int leg = 0;
    int glow = 0;
};

void DrawPatch(PixelCanvas& c, const PatchPose& p) {
    const int u = 3;
    int cx = 6 + p.lean;
    int lo = p.leg == 0 ? 0 : 1;
    // Legs
    c.Line((cx - 1) * u, 11 * u, (cx - 2 + lo) * u, 16 * u, 'K');
    c.Line((cx + 1) * u, 11 * u, (cx + 2 - lo) * u, 16 * u, 'K');
    c.Rect((cx - 3 + lo) * u, 16 * u, 3 * u, u, 'K');
    c.Rect((cx) * u, 16 * u, 3 * u, u, 'K');
    // Body: white cloth with film fragments
    int bTop = 5, bBot = 5 + p.bodyH;
    c.Rect((cx - 3) * u, bTop * u, 6 * u, p.bodyH * u, 'W');
    c.Rect((cx - 3) * u, bTop * u, 6 * u, u, 'K');
    c.Rect((cx - 3) * u, (bBot - 1) * u, 6 * u, u, 'K');
    c.Rect((cx - 3) * u, bTop * u, u, p.bodyH * u, 'K');
    c.Rect((cx + 2) * u, bTop * u, u, p.bodyH * u, 'K');
    c.Rect((cx - 2) * u, (bTop + 1) * u, 2 * u, 2 * u, 'C');
    c.Rect((cx) * u, (bTop + 3) * u, 2 * u, 2 * u, 'R');
    c.Rect((cx - 2) * u, (bTop + 4) * u, 2 * u, 2 * u, 'R');
    // Arms
    if (p.arms == 1) {
        c.Line((cx - 3) * u, (bTop + 1) * u, (cx - 5) * u, (bTop - 2) * u, 'K');
        c.Line((cx + 3) * u, (bTop + 1) * u, (cx + 5) * u, (bTop - 2) * u, 'K');
    } else {
        c.Line((cx - 3) * u, (bTop + 1) * u, (cx - 4) * u, (bTop + 4) * u, 'K');
        c.Line((cx + 3) * u, (bTop + 1) * u, (cx + 4) * u, (bTop + 4) * u, 'K');
    }
    // Head: big stitched doll face
    int hBot = bTop;
    c.Ellipse(cx * u, (hBot - 4) * u, 3 * u, 4 * u, 'W');
    c.Ellipse(cx * u, (hBot - 4) * u, 3 * u, 4 * u, 'K', false);
    // Seam down the face
    c.Line((cx - 1) * u, (hBot - 8) * u, (cx - 1) * u, (hBot) * u, 'K');
    c.Line((cx - 2) * u, (hBot - 6) * u, (cx) * u, (hBot - 6) * u, 'K');
    c.Line((cx - 2) * u, (hBot - 3) * u, (cx) * u, (hBot - 3) * u, 'K');
    // Button eyes
    c.Ellipse((cx - 2) * u, (hBot - 5) * u, u, u, 'K');
    c.Ellipse((cx + 2) * u, (hBot - 5) * u, u, u, 'K');
    if (p.glow == 1) {
        c.Px((cx - 2) * u, (hBot - 5) * u, 'I');
        c.Px((cx + 2) * u, (hBot - 5) * u, 'I');
    }
    // Mouth
    if (p.mouth == 1) {
        c.Rect((cx - 2) * u, (hBot - 1) * u, 4 * u, u, 'X');
    } else {
        c.Line((cx - 2) * u, (hBot - 1) * u, (cx + 2) * u, (hBot - 1) * u, 'K');
        c.Line((cx - 1) * u, (hBot - 2) * u, (cx - 1) * u, (hBot) * u, 'K');
        c.Line((cx + 1) * u, (hBot - 2) * u, (cx + 1) * u, (hBot) * u, 'K');
    }
    // Needle of pure ink
    int nx = cx + 4;
    if (p.needle == 1) {
        c.Line(nx * u, bBot * u, (nx + 1) * u, 1 * u, 'C');
        c.Px((nx + 1) * u, 1 * u, 'K');
        c.Px((nx + 2) * u, 2 * u, 'K');
    } else {
        c.Line(nx * u, bBot * u, (nx + 2) * u, 16 * u, 'C');
        c.Px((nx + 2) * u, 16 * u, 'K');
    }
    if (p.glow == 1) {
        for (int i = 0; i < 10; ++i) {
            int gx = (i * 4) % 12;
            int gy = (i * 3) % 17;
            c.Px(gx * u + u, gy * u + u, 'I');
        }
    }
}
} // namespace

void BuildBossArt(SpriteBank& bank) {
    auto Key = [](const char* base, int frame) {
        char b[16];
        std::snprintf(b, sizeof(b), "_%02d", frame);
        return std::string(base) + b;
    };
    auto add = [&](const char* name, void (*draw)(PixelCanvas&, const BarnabyPose&),
                   const BarnabyPose* pose) {
        PixelCanvas c(48, 64);
        InitSharedPalette(c);
        draw(c, pose ? *pose : BarnabyPose{});
        c.AddTo(bank, name);
    };
    // Barnaby poses
    static BarnabyPose idle0{}, idle1, walk0, walk1, raise, slam, crouch, hop,
        seeds, roots, storm, spin, crow, phase, defeat;
    idle1.leg = 1; idle1.lean = -1;
    walk0.leg = 1; walk0.lean = 1;
    walk1.leg = 0;
    raise.scythe = 1;
    slam.bodyH = 5; slam.mouth = 1;
    crouch.bodyH = 4;
    hop.bodyH = 9;
    seeds.arms = 1;
    roots.arms = 1; roots.eye = 1;
    storm.eye = 1; storm.mouth = 1; storm.glow = 1; storm.scythe = 1;
    spin.scythe = 1; spin.lean = -2; spin.mouth = 1;
    crow.scythe = 0; crow.mouth = 1; crow.arms = 1;
    phase.eye = 1; phase.scythe = 1; phase.glow = 1;
    defeat.lean = 3; defeat.bodyH = 4; defeat.mouth = 1;

    add(Key("barnaby_idle", 0).c_str(), DrawBarnaby, &idle0);
    add(Key("barnaby_idle", 1).c_str(), DrawBarnaby, &idle1);
    add(Key("barnaby_walk", 0).c_str(), DrawBarnaby, &walk0);
    add(Key("barnaby_walk", 1).c_str(), DrawBarnaby, &walk1);
    add(Key("barnaby_raise", 0).c_str(), DrawBarnaby, &raise);
    add(Key("barnaby_slam", 0).c_str(), DrawBarnaby, &slam);
    add(Key("barnaby_crouch", 0).c_str(), DrawBarnaby, &crouch);
    add(Key("barnaby_hop", 0).c_str(), DrawBarnaby, &hop);
    add(Key("barnaby_seeds", 0).c_str(), DrawBarnaby, &seeds);
    add(Key("barnaby_roots", 0).c_str(), DrawBarnaby, &roots);
    add(Key("barnaby_storm", 0).c_str(), DrawBarnaby, &storm);
    add(Key("barnaby_spin", 0).c_str(), DrawBarnaby, &spin);
    add(Key("barnaby_crow", 0).c_str(), DrawBarnaby, &crow);
    add(Key("barnaby_phase", 0).c_str(), DrawBarnaby, &phase);
    add(Key("barnaby_defeat", 0).c_str(), DrawBarnaby, &defeat);

    // The Patch poses
    auto addP = [&](const char* name, const PatchPose& p) {
        PixelCanvas c(36, 52);
        InitSharedPalette(c);
        DrawPatch(c, p);
        c.AddTo(bank, name);
    };
    static PatchPose p_idle0{}, p_idle1, p_walk0, p_walk1, p_raise, p_slam, p_hop,
        p_needle, p_spin, p_phase, p_defeat;
    p_idle1.leg = 1; p_idle1.lean = -1;
    p_walk0.leg = 1;
    p_walk1.leg = 0;
    p_raise.arms = 1; p_raise.needle = 1;
    p_slam.mouth = 1; p_slam.bodyH = 5;
    p_hop.bodyH = 8;
    p_needle.arms = 1; p_needle.glow = 1;
    p_spin.lean = 2; p_spin.bodyH = 4;
    p_phase.glow = 1; p_phase.needle = 1; p_phase.mouth = 1;
    p_defeat.lean = 3; p_defeat.mouth = 1; p_defeat.bodyH = 4;

    addP(Key("patch_idle", 0).c_str(), p_idle0);
    addP(Key("patch_idle", 1).c_str(), p_idle1);
    addP(Key("patch_walk", 0).c_str(), p_walk0);
    addP(Key("patch_walk", 1).c_str(), p_walk1);
    addP(Key("patch_raise", 0).c_str(), p_raise);
    addP(Key("patch_slam", 0).c_str(), p_slam);
    addP(Key("patch_hop", 0).c_str(), p_hop);
    addP(Key("patch_needle", 0).c_str(), p_needle);
    addP(Key("patch_spin", 0).c_str(), p_spin);
    addP(Key("patch_phase", 0).c_str(), p_phase);
    addP(Key("patch_defeat", 0).c_str(), p_defeat);
}

} // namespace ink
