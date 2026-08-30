#pragma once
// Frame-by-frame animation handle (§44). Each character maps its state to a
// (base name, frame count, fps, loop) tuple; the sprite bank resolves
// "base_frame" to an atlas rect. Pure logic -> unit-testable.
#include <string>

namespace ink {

struct AnimRef {
    std::string base; // e.g. "milo_run"
    int count = 1;
    int fps = 8;
    bool loop = true;
    bool play = true;
    double t = 0.0;

    int frame = 0;
    bool finished = false; // true once a non-looping clip ran to its last frame
};

void PlayAnim(AnimRef& a, const std::string& base, int count, int fps, bool loop);
void UpdateAnim(AnimRef& a, double dt);
// Returns "base_frame" (zero padded to 2 digits), or base itself when count<=1.
std::string AnimName(const AnimRef& a);

} // namespace ink
