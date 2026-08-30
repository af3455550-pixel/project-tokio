#include "Audio/MusicData.h"

namespace ink {

namespace {

void Note(MusicTrack& t, double step, int midi, double dur, int inst, int layer, int vel = 90) {
    t.events.push_back({step, midi, dur, inst, layer, vel});
}

// Ragtime "oom-pah-pah" bass for one bar.
void RagBass(MusicTrack& t, int bar, int root, int third, int fifth, int layer) {
    int s = bar * 16;
    Note(t, s + 0, root, 2, 1, layer, 100);
    Note(t, s + 4, fifth, 2, 1, layer, 68);
    Note(t, s + 8, third, 2, 1, layer, 68);
    Note(t, s + 12, fifth, 2, 1, layer, 68);
}

void ChordStab(MusicTrack& t, double step, int root, int layer, int vel = 60) {
    Note(t, step, root, 2, 2, layer, vel);
    Note(t, step, root + 4, 2, 2, layer, vel);
    Note(t, step, root + 7, 2, 2, layer, vel);
}

void Kick(MusicTrack& t, double step, int layer) { Note(t, step, 0, 1, 10, layer, 100); }
void Snare(MusicTrack& t, double step, int layer) { Note(t, step, 0, 2, 11, layer, 85); }
void Hat(MusicTrack& t, double step, int layer) { Note(t, step, 0, 1, 12, layer, 45); }

// Swung drum pattern for four bars.
void SwingDrums(MusicTrack& t, int layer) {
    for (int b = 0; b < 4; ++b) {
        int s = b * 16;
        Kick(t, s + 0, layer);
        Kick(t, s + 8, layer);
        Snare(t, s + 8, layer);
        Hat(t, s + 2, layer);
        Hat(t, s + 6, layer); // swung offbeat
        Hat(t, s + 10, layer);
        Hat(t, s + 14, layer);
    }
}

// Lazy swing title theme (C major).
void BuildMenu(MusicTrack& t) {
    t.id = "menu";
    t.bpm = 118.0;
    t.totalSteps = 64;
    t.loop = true;
    RagBass(t, 0, 36, 40, 43, 0); // C
    RagBass(t, 1, 43, 47, 50, 0); // G
    RagBass(t, 2, 45, 48, 52, 0); // Am
    RagBass(t, 3, 41, 44, 48, 0); // F
    // Melody
    Note(t, 0, 64, 4, 0, 0);
    Note(t, 4, 67, 4, 0, 0);
    Note(t, 8, 69, 4, 0, 0);
    Note(t, 12, 72, 4, 0, 0);
    Note(t, 16, 69, 4, 0, 0);
    Note(t, 20, 72, 4, 0, 0);
    Note(t, 24, 69, 4, 0, 0);
    Note(t, 28, 67, 4, 0, 0);
    Note(t, 32, 64, 4, 0, 0);
    Note(t, 36, 67, 4, 0, 0);
    Note(t, 40, 69, 4, 0, 0);
    Note(t, 44, 74, 4, 0, 0);
    Note(t, 48, 72, 4, 0, 0);
    Note(t, 52, 67, 4, 0, 0);
    Note(t, 56, 64, 4, 0, 0);
    Note(t, 60, 60, 4, 0, 0);
    // Horn stabs (layer 1)
    ChordStab(t, 4, 48, 1);
    ChordStab(t, 12, 48, 1);
    ChordStab(t, 20, 55, 1);
    ChordStab(t, 28, 55, 1);
    ChordStab(t, 36, 57, 1);
    ChordStab(t, 44, 57, 1);
    ChordStab(t, 52, 65, 1);
    ChordStab(t, 60, 65, 1);
    // Drums (layer 1)
    SwingDrums(t, 1);
}

// Whispering Meadows level theme (C major, bouncy).
void BuildMeadows(MusicTrack& t) {
    t.id = "meadows";
    t.bpm = 132.0;
    t.totalSteps = 64;
    t.loop = true;
    RagBass(t, 0, 36, 40, 43, 0); // C
    RagBass(t, 1, 36, 40, 43, 0); // C
    RagBass(t, 2, 41, 44, 48, 0); // F
    RagBass(t, 3, 43, 47, 50, 0); // G
    // Melody (celesta)
    Note(t, 0, 67, 4, 4, 0, 95);
    Note(t, 4, 71, 4, 4, 0, 95);
    Note(t, 8, 72, 4, 4, 0, 95);
    Note(t, 12, 69, 4, 4, 0, 95);
    Note(t, 16, 72, 4, 4, 0, 95);
    Note(t, 20, 69, 4, 4, 0, 95);
    Note(t, 24, 67, 4, 4, 0, 95);
    Note(t, 28, 64, 4, 4, 0, 95);
    Note(t, 32, 65, 4, 4, 0, 95);
    Note(t, 36, 67, 4, 4, 0, 95);
    Note(t, 40, 69, 4, 4, 0, 95);
    Note(t, 44, 71, 4, 4, 0, 95);
    Note(t, 48, 67, 4, 4, 0, 95);
    Note(t, 52, 64, 4, 4, 0, 95);
    Note(t, 56, 62, 4, 4, 0, 95);
    Note(t, 60, 64, 4, 4, 0, 95);
    // Guitar on the beat (layer 1)
    Note(t, 0, 48, 2, 5, 1, 55);
    Note(t, 16, 48, 2, 5, 1, 55);
    Note(t, 32, 53, 2, 5, 1, 55); // F
    Note(t, 48, 55, 2, 5, 1, 55); // G
    SwingDrums(t, 1);
}

// BARNABY THE HARVEST KING — boss theme (A minor, 3 layers = 3 phases).
void BuildBarnaby(MusicTrack& t) {
    t.id = "barnaby";
    t.bpm = 126.0;
    t.totalSteps = 64;
    t.loop = true;
    // Driving 8th bass (layer 0)
    for (int b = 0; b < 4; ++b) {
        int s = b * 16;
        int alt = (b == 1) ? 36 : 36; // A1 / C2
        (void)alt;
        for (int i = 0; i < 8; ++i)
            Note(t, s + i * 2, (i % 2 == 0) ? 33 : 36, 1.5, 1, 0, 95);
        if (b == 3) {
            Note(t, s + 12, 31, 1.5, 1, 0, 95); // G1 lift
            Note(t, s + 14, 33, 1.5, 1, 0, 95);
        }
    }
    // Ominous melody (brass)
    Note(t, 0, 57, 4, 2, 0, 90);
    Note(t, 4, 62, 4, 2, 0, 90);
    Note(t, 8, 64, 4, 2, 0, 90);
    Note(t, 12, 60, 4, 2, 0, 90);
    Note(t, 16, 64, 4, 2, 0, 90);
    Note(t, 20, 62, 4, 2, 0, 90);
    Note(t, 24, 57, 4, 2, 0, 90);
    Note(t, 28, 55, 4, 2, 0, 90);
    Note(t, 32, 60, 4, 2, 0, 90);
    Note(t, 36, 62, 4, 2, 0, 90);
    Note(t, 40, 64, 4, 2, 0, 90);
    Note(t, 44, 65, 4, 2, 0, 90);
    Note(t, 48, 64, 4, 2, 0, 90);
    Note(t, 52, 62, 4, 2, 0, 90);
    Note(t, 56, 60, 4, 2, 0, 90);
    Note(t, 60, 57, 4, 2, 0, 90);
    // Layer 1: half-time horns + snare
    for (int b = 0; b < 4; ++b) {
        int s = b * 16;
        ChordStab(t, s + 0, 57, 1, 62); // Am
        ChordStab(t, s + 8, (b == 3) ? 55 : 57, 1, 62);
        Snare(t, s + 8, 1);
        Kick(t, s + 0, 1);
        Kick(t, s + 8, 1);
    }
    // Layer 2: full band, 16th arps + busy drums
    for (int s = 0; s < 64; ++s) {
        static const int arp[] = {57, 60, 64, 67, 64, 60, 57, 60};
        Note(t, s, arp[s % 8], 1, 0, 2, 42);
    }
    for (int b = 0; b < 4; ++b) {
        int s = b * 16;
        Kick(t, s + 0, 2);
        Kick(t, s + 6, 2);
        Kick(t, s + 8, 2);
        Kick(t, s + 14, 2);
        Snare(t, s + 4, 2);
        Snare(t, s + 12, 2);
        Hat(t, s + 2, 2);
        Hat(t, s + 6, 2);
        Hat(t, s + 10, 2);
        Hat(t, s + 14, 2);
    }
}

// PATCH — mini-boss (C minor, quick).
void BuildPatch(MusicTrack& t) {
    t.id = "patch";
    t.bpm = 140.0;
    t.totalSteps = 32;
    t.loop = true;
    for (int i = 0; i < 16; ++i)
        Note(t, i * 2, (i % 2 == 0) ? 36 : 36, 1.5, 1, 0, 95);
    Note(t, 0, 72, 4, 2, 0, 88);
    Note(t, 4, 71, 4, 2, 0, 88);
    Note(t, 8, 72, 4, 2, 0, 88);
    Note(t, 12, 69, 4, 2, 0, 88);
    Note(t, 16, 67, 4, 2, 0, 88);
    Note(t, 20, 69, 4, 2, 0, 88);
    Note(t, 24, 71, 4, 2, 0, 88);
    Note(t, 28, 72, 4, 2, 0, 88);
    for (int b = 0; b < 2; ++b) {
        int s = b * 16;
        Kick(t, s + 0, 1);
        Kick(t, s + 8, 1);
        Snare(t, s + 8, 1);
        Hat(t, s + 2, 1);
        Hat(t, s + 6, 1);
        Hat(t, s + 10, 1);
        Hat(t, s + 14, 1);
        ChordStab(t, s + 0, 60, 1, 55); // C minor-ish
    }
}

// Defeat sting (A minor descent, plays once).
void BuildDefeat(MusicTrack& t) {
    t.id = "defeat";
    t.bpm = 90.0;
    t.totalSteps = 32;
    t.loop = false;
    Note(t, 0, 57, 3, 0, 0, 90);
    Note(t, 3, 55, 3, 0, 0, 85);
    Note(t, 6, 52, 6, 0, 0, 80);
    Note(t, 0, 33, 8, 1, 0, 90);
    Note(t, 6, 31, 8, 1, 0, 85);
}

// Victory jingle (plays once).
void BuildVictory(MusicTrack& t) {
    t.id = "victory";
    t.bpm = 104.0;
    t.totalSteps = 16;
    t.loop = false;
    Note(t, 0, 60, 2, 0, 0, 90);
    Note(t, 2, 64, 2, 0, 0, 90);
    Note(t, 4, 67, 2, 0, 0, 90);
    Note(t, 6, 72, 3, 0, 0, 95);
    Note(t, 8, 76, 5, 0, 0, 95);
    Note(t, 0, 48, 4, 1, 0, 80);
    Note(t, 8, 52, 6, 1, 0, 80);
    Kick(t, 0, 0);
    Kick(t, 8, 0);
    Snare(t, 8, 0);
}

} // namespace

void LoadAllMusic(std::vector<MusicTrack>& out) {
    out.clear();
    MusicTrack menu; BuildMenu(menu); out.push_back(menu);
    MusicTrack meadows; BuildMeadows(meadows); out.push_back(meadows);
    MusicTrack barnaby; BuildBarnaby(barnaby); out.push_back(barnaby);
    MusicTrack patch; BuildPatch(patch); out.push_back(patch);
    MusicTrack defeat; BuildDefeat(defeat); out.push_back(defeat);
    MusicTrack victory; BuildVictory(victory); out.push_back(victory);
}

} // namespace ink
