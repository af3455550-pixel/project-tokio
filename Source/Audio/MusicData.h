#pragma once
// Original compositions for The Last Reel (§48). All melodies are
// hand-written note data (100% original — no external music).
// Boss themes are layered: layer 0 = base, +1 per boss phase (§49).
#include "Audio/AudioEngine.h"
#include <vector>

namespace ink {
void LoadAllMusic(std::vector<MusicTrack>& out);
}
