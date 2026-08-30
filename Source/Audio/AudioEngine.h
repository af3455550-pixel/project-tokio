#pragma once
// Audio engine (§78): all sound is ORIGINAL and synthesized at runtime —
// no external audio assets. A small software mixer (22.05 kHz mono) drives:
//  - a step sequencer playing original jazz/swing/ragtime tracks with
//    per-boss-phase layers (dynamic music, §49);
//  - a pool of one-shot SFX voices (procedural blips, whooshes, dings).
// If no audio device exists (headless CI) the engine degrades to silent.
#include <atomic>
#include <string>
#include <vector>

namespace ink {

struct NoteEventRaw {
    double step;
    int midi;
    double dur; // in 16th-note steps
    int inst;
    int layer;
    int vel; // 0..127
};

struct MusicTrack {
    std::string id;
    double bpm = 120.0;
    int totalSteps = 64; // 16th-note steps; 0 = play once
    bool loop = true;
    std::vector<NoteEventRaw> events;
};

class AudioEngine {
public:
    bool Init(bool mute, std::string* err = nullptr);
    void Shutdown();

    // Music (MusicManager / BossMusicController, §78).
    void LoadTrack(const MusicTrack& t);
    void PlayMusic(const std::string& id, int layer = 0);
    void SetMusicLayer(int layer); // boss phase layering (§49)
    void StopMusic(double fadeSec = 0.3);
    void SetMusicPlaying(bool playing); // pause/resume (menus)
    bool MusicActive() const { return musicActive_; }

    // SFX (SFXManager).
    enum Sfx : int {
        SfxNone = 0,
        SfxJump, SfxLand, SfxDash, SfxShoot, SfxCharged, SfxParry, SfxHurt, SfxDeath,
        SfxExplosion, SfxCoin, SfxHit, SfxUiMove, SfxUiSelect, SfxBossIntro,
        SfxVictory, SfxDefeat, SfxSuper, SfxPhaseUp, SfxStomp, SfxCreak, SfxRoot,
        SfxQuest, SfxAchievement, SfxCheckpoint, SfxBreak, SfxSwish
    };
    void PlaySfx(int sfx);

    // Mixing (AudioManager).
    void SetMasterVolume(float v);
    void SetMusicVolume(float v);
    void SetSfxVolume(float v);

private:
    static void MixCallback(void* ud, unsigned char* stream, int len);

    struct NoteVoice {
        bool active = false;
        double t0 = 0.0;
        double dur = 0.0;
        double freq = 440.0;
        int inst = 0;
        double gain = 0.0;
        double phase = 0.0;
        unsigned int noise = 0x12345678u;
    };

    struct SfxVoice {
        bool active = false;
        double t0 = 0.0;
        double dur = 0.0;
        double f0 = 0.0, f1 = 0.0;
        int inst = 0; // 0 tone, 1 noise, 2 sweep
        double gain = 0.0;
        double phase = 0.0;
        unsigned int noise = 0x87654321u;
    };

    struct SfxProg {
        double f0, f1, dur, gain;
        int inst;
        double delay;
    };
    static const SfxProg* SfxTable(int sfx, int& count);

    static constexpr int kMusicVoices = 28;
    static constexpr int kSfxVoices = 10;
    static constexpr double kSampleRate = 22050.0;

    std::vector<MusicTrack> tracks_;
    std::vector<NoteVoice> musicPool_;
    std::vector<SfxVoice> sfxPool_;
    int sfxCursor_ = 0;

    std::string currentTrackId_;
    double trackStep_ = 0.0;
    int trackLayer_ = 0;
    bool musicActive_ = false;
    bool musicPaused_ = false;
    const MusicTrack* currentTrack_ = nullptr;
    double musicFade_ = 1.0;
    double musicFadeTarget_ = 1.0;

    double now_ = 0.0; // mixer clock (seconds)

    std::atomic<float> masterVol_{0.8f};
    std::atomic<float> musicVol_{0.75f};
    std::atomic<float> sfxVol_{0.9f};
    bool enabled_ = false;
    bool muted_ = false;
    unsigned int dev_ = 0;
};

} // namespace ink
