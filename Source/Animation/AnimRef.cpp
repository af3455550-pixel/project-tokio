#include "Animation/AnimRef.h"
#include <algorithm>
#include <cstdio>

namespace ink {

void PlayAnim(AnimRef& a, const std::string& base, int count, int fps, bool loop) {
    a.base = base;
    a.count = std::max(1, count);
    a.fps = std::max(1, fps);
    a.loop = loop;
    a.play = true;
    a.t = 0.0;
    a.frame = 0;
    a.finished = false;
}

void UpdateAnim(AnimRef& a, double dt) {
    if (!a.play || a.count <= 1)
        return;
    a.t += dt;
    int f = static_cast<int>(a.t * a.fps);
    if (a.loop) {
        a.frame = f % a.count;
    } else {
        if (f >= a.count) {
            f = a.count - 1;
            a.finished = true;
        }
        a.frame = f;
    }
}

std::string AnimName(const AnimRef& a) {
    if (a.count <= 1)
        return a.base;
    char buf[8];
    std::snprintf(buf, sizeof(buf), "_%02d", a.frame);
    return a.base + buf;
}

} // namespace ink
