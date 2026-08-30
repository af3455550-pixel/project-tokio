#pragma once
// Tunable player numbers. Base values are here; charms multiply them at
// runtime (Progression), so balance never lives in the controller code.
#include <string>

namespace ink {

struct PlayerStats {
    int maxHp = 3;
    double walkSpeed = 150.0;
    double runSpeed = 262.0;
    double accel = 2400.0;
    double airControl = 0.85;
    double friction = 2200.0;
    double jumpVel = 565.0;
    double gravity = 1650.0;
    double maxFall = 800.0;
    double variableJumpCut = 0.42; // velocity multiplier on early release
    double dashSpeed = 580.0;
    double dashTime = 0.13;
    double dashCooldown = 0.32;
    double dashIFrames = 0.06;
    int airDashCount = 1;
    double coyoteTime = 0.10;       // §12
    double jumpBufferTime = 0.10;   // §12
    double inputBufferTime = 0.10;  // §12 attack/dash input buffer
    double attackTime = 0.22;
    double attackFireT = 0.06;
    double chargedTime = 0.45;
    double parryTime = 0.16;
    double parryActiveFrom = 0.04; // parry window inside the parry pose
    double parryActiveTo = 0.14;
    double invulnTime = 1.0;
    double superTime = 1.2;
    double damageMul = 1.0;
    double dmgTakenMul = 1.0;
    double energyGainMul = 1.0;
    double coinMul = 1.0;
};

} // namespace ink
