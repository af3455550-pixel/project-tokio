#include "Audio/AudioEngine.h"
#include "Core/Log.h"
#include "Core/Math.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <cstring>
#include <mutex>

namespace ink {

namespace {
constexpr double kTwoPi = 6.283185307179586;

double MidiToFreq(int m) { return 440.0 * std::pow(2.0, (m - 69) / 12.0); }

double Noise(unsigned seed, unsigned tick) {
    unsigned h = seed ^ (tick * 2654435761u);
    h ^= h >> 13;
    h ^= h << 7;
    h ^= h >> 17;
    return (static_cast<double>(h) / 2147483648.0) - 1.0;
}

double Tri(double phase) {
    double p = std::fmod(phase / kTwoPi, 1.0);
    if (p < 0)
        p += 1.0;
    return p < 0.25 ? 4 * p : (p < 0.75 ? 2 - 4 * p : 4 * p - 4);
}
} // namespace

// ------------------------------------------------------------------ init ---
bool AudioEngine::Init(bool mute, std::string* err) {
    muted_ = mute;
    musicPool_.resize(kMusicVoices);
    sfxPool_.resize(kSfxVoices);
    if (muted_) {
        INK_LOG_INFO("Audio muted by flag");
        return true;
    }
    SDL_AudioSpec want{};
    want.freq = static_cast<int>(kSampleRate);
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = &AudioEngine::MixCallback;
    want.userdata = this;
    SDL_AudioSpec got{};
    dev_ = SDL_OpenAudioDevice(nullptr, 0, &want, &got, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (dev_ == 0) {
        INK_LOG_WARN("Audio device unavailable: " + std::string(SDL_GetError()));
        if (err)
            *err = std::string(SDL_GetError());
        return true; // continue silently
    }
    enabled_ = true;
    INK_LOG_INFO("Audio engine ready (22 kHz mono synth)");
    return true;
}

void AudioEngine::Shutdown() {
    if (enabled_) {
        SDL_CloseAudioDevice(dev_);
        dev_ = 0;
        enabled_ = false;
    }
}

// ---------------------------------------------------------------- music ----
void AudioEngine::LoadTrack(const MusicTrack& t) {
    for (auto& tr : tracks_)
        if (tr.id == t.id)
            tr = t;
    if (tracks_.empty() || tracks_.back().id != t.id)
        tracks_.push_back(t);
}

void AudioEngine::PlayMusic(const std::string& id, int layer) {
    const MusicTrack* t = nullptr;
    for (const auto& tr : tracks_)
        if (tr.id == id)
            t = &tr;
    if (!t) {
        INK_LOG_WARN("Unknown music id: " + id);
        return;
    }
    currentTrack_ = t;
    currentTrackId_ = id;
    trackStep_ = 0.0;
    trackLayer_ = layer;
    musicActive_ = true;
    musicPaused_ = false;
    musicFadeTarget_ = 1.0;
}

void AudioEngine::SetMusicLayer(int layer) {
    trackLayer_ = layer;
}

void AudioEngine::StopMusic(double fadeSec) {
    musicFadeTarget_ = 0.0;
    (void)fadeSec;
}

void AudioEngine::SetMusicPlaying(bool playing) { musicPaused_ = !playing; }

void AudioEngine::SetMasterVolume(float v) { masterVol_ = Clamp(v, 0.0f, 1.0f); }
void AudioEngine::SetMusicVolume(float v) { musicVol_ = Clamp(v, 0.0f, 1.0f); }
void AudioEngine::SetSfxVolume(float v) { sfxVol_ = Clamp(v, 0.0f, 1.0f); }

// ------------------------------------------------------------------ sfx ----
const AudioEngine::SfxProg* AudioEngine::SfxTable(int sfx, int& count) {
    static const SfxProg jump[] = {{320, 640, 0.09, 0.5, 2, 0}};
    static const SfxProg land[] = {{140, 60, 0.08, 0.5, 2, 0}, {0, 0, 0.05, 0.3, 1, 0}};
    static const SfxProg dash[] = {{900, 200, 0.12, 0.5, 2, 0}, {0, 0, 0.1, 0.35, 1, 0}};
    static const SfxProg shoot[] = {{680, 180, 0.07, 0.45, 2, 0}};
    static const SfxProg charged[] = {{220, 90, 0.16, 0.6, 2, 0}, {0, 0, 0.12, 0.4, 1, 0.02}};
    static const SfxProg parry[] = {{1320, 1320, 0.09, 0.5, 0, 0}, {1980, 1980, 0.14, 0.3, 0, 0.02}};
    static const SfxProg hurt[] = {{200, 70, 0.16, 0.6, 2, 0}};
    static const SfxProg death[] = {{300, 40, 0.5, 0.6, 2, 0}};
    static const SfxProg boom[] = {{0, 0, 0.35, 0.8, 1, 0}, {90, 30, 0.3, 0.7, 2, 0}};
    static const SfxProg coin[] = {{880, 880, 0.06, 0.4, 0, 0}, {1318, 1318, 0.1, 0.4, 0, 0.06}};
    static const SfxProg hit[] = {{110, 90, 0.05, 0.6, 0, 0}, {0, 0, 0.04, 0.3, 1, 0}};
    static const SfxProg ui[] = {{660, 660, 0.05, 0.35, 0, 0}};
    static const SfxProg sel[] = {{880, 1174, 0.09, 0.4, 2, 0}};
    static const SfxProg boss[] = {{55, 55, 0.8, 0.8, 0, 0}, {0, 0, 0.5, 0.3, 1, 0.1}};
    static const SfxProg vict[] = {{523, 523, 0.12, 0.5, 0, 0}, {659, 659, 0.12, 0.5, 0, 0.09},
                                   {784, 784, 0.12, 0.5, 0, 0.18}, {1046, 1046, 0.2, 0.5, 0, 0.27}};
    static const SfxProg defeat[] = {{330, 320, 0.25, 0.6, 0, 0}, {262, 255, 0.25, 0.6, 0, 0.25},
                                     {196, 185, 0.5, 0.6, 0, 0.5}};
    static const SfxProg sup[] = {{120, 900, 0.5, 0.7, 2, 0}, {0, 0, 0.4, 0.5, 1, 0.05}};
    static const SfxProg phase[] = {{100, 400, 0.35, 0.7, 2, 0}, {0, 0, 0.25, 0.4, 1, 0.1}};
    static const SfxProg stomp[] = {{0, 0, 0.12, 0.7, 1, 0}, {70, 30, 0.15, 0.7, 2, 0}};
    static const SfxProg creak[] = {{180, 120, 0.3, 0.3, 2, 0}};
    static const SfxProg root[] = {{90, 40, 0.3, 0.6, 2, 0}, {0, 0, 0.2, 0.5, 1, 0.02}};
    static const SfxProg quest[] = {{660, 660, 0.09, 0.4, 0, 0}, {880, 880, 0.12, 0.4, 0, 0.09}};
    static const SfxProg ach[] = {{784, 784, 0.08, 0.45, 0, 0}, {988, 988, 0.08, 0.45, 0, 0.08},
                                  {1175, 1175, 0.16, 0.45, 0, 0.16}};
    static const SfxProg cp[] = {{523, 523, 0.08, 0.4, 0, 0}, {659, 659, 0.08, 0.4, 0, 0.08},
                                 {784, 784, 0.14, 0.4, 0, 0.16}};
    static const SfxProg brk[] = {{0, 0, 0.15, 0.6, 1, 0}};
    static const SfxProg swish[] = {{0, 0, 0.08, 0.3, 1, 0}};

    switch (sfx) {
    case SfxJump: count = 1; return jump;
    case SfxLand: count = 2; return land;
    case SfxDash: count = 2; return dash;
    case SfxShoot: count = 1; return shoot;
    case SfxCharged: count = 2; return charged;
    case SfxParry: count = 2; return parry;
    case SfxHurt: count = 1; return hurt;
    case SfxDeath: count = 1; return death;
    case SfxExplosion: count = 2; return boom;
    case SfxCoin: count = 2; return coin;
    case SfxHit: count = 2; return hit;
    case SfxUiMove: count = 1; return ui;
    case SfxUiSelect: count = 1; return sel;
    case SfxBossIntro: count = 2; return boss;
    case SfxVictory: count = 4; return vict;
    case SfxDefeat: count = 3; return defeat;
    case SfxSuper: count = 2; return sup;
    case SfxPhaseUp: count = 2; return phase;
    case SfxStomp: count = 2; return stomp;
    case SfxCreak: count = 1; return creak;
    case SfxRoot: count = 2; return root;
    case SfxQuest: count = 2; return quest;
    case SfxAchievement: count = 3; return ach;
    case SfxCheckpoint: count = 3; return cp;
    case SfxBreak: count = 1; return brk;
    case SfxSwish: count = 1; return swish;
    default: count = 0; return nullptr;
    }
}

void AudioEngine::PlaySfx(int sfx) {
    if (muted_)
        return;
    int count = 0;
    const SfxProg* t = SfxTable(sfx, count);
    if (!t)
        return;
    now_ = SDL_GetTicks() / 1000.0;
    for (int i = 0; i < count; ++i) {
        SfxVoice& v = sfxPool_[sfxCursor_];
        sfxCursor_ = (sfxCursor_ + 1) % kSfxVoices;
        v.active = true;
        v.t0 = now_ + t[i].delay;
        v.dur = t[i].dur;
        v.f0 = t[i].f0;
        v.f1 = t[i].f1;
        v.inst = t[i].inst;
        v.gain = t[i].gain;
        v.phase = 0.0;
        v.noise = 0x9E3779B9u + static_cast<unsigned>(sfx * 31 + i * 7);
    }
}

// --------------------------------------------------------------- mixer ----
void AudioEngine::MixCallback(void* ud, unsigned char* stream, int len) {
    auto* self = static_cast<AudioEngine*>(ud);
    const int samples = len / 2;
    auto* out = reinterpret_cast<int16_t*>(stream);
    const double dt = 1.0 / kSampleRate;
    const double now = SDL_GetTicks() / 1000.0;

    // Music fade
    double fadeStep = 3.0 * dt;
    if (self->musicFade_ < self->musicFadeTarget_)
        self->musicFade_ = std::min(self->musicFadeTarget_, self->musicFade_ + fadeStep);
    else
        self->musicFade_ = std::max(self->musicFadeTarget_, self->musicFade_ - fadeStep);
    if (self->musicFadeTarget_ == 0.0 && self->musicFade_ <= 0.005)
        self->musicActive_ = false;

    // Sequencer
    if (self->musicActive_ && !self->musicPaused_ && self->currentTrack_) {
        double stepDur = 60.0 / self->currentTrack_->bpm / 4.0;
        static double acc = 0.0;
        acc += dt * self->currentTrack_->bpm / 120.0; // normalized below
        (void)acc;
        self->trackStep_ += dt / stepDur;
        int stepNow = static_cast<int>(self->trackStep_);
        const auto& evs = self->currentTrack_->events;
        for (const auto& ev : evs) {
            if (static_cast<int>(ev.step) != stepNow)
                continue;
            if (ev.layer > self->trackLayer_)
                continue;
            bool played = false;
            for (auto& v : self->musicPool_)
                if (!v.active) {
                    v.active = true;
                    v.t0 = now + 0.001;
                    v.dur = ev.dur * stepDur;
                    v.freq = MidiToFreq(ev.midi);
                    v.inst = ev.inst;
                    v.gain = (ev.vel / 127.0) * 0.42;
                    v.phase = 0.0;
                    v.noise = static_cast<unsigned>(ev.midi * 2654435761u + ev.step);
                    played = true;
                    break;
                }
            (void)played;
        }
        if (self->currentTrack_->loop) {
            if (self->trackStep_ >= self->currentTrack_->totalSteps)
                self->trackStep_ -= self->currentTrack_->totalSteps;
        } else if (self->trackStep_ >= self->currentTrack_->totalSteps) {
            self->musicActive_ = false;
            self->musicFadeTarget_ = 0.0;
        }
    }

    const double master = self->masterVol_.load() * 2.0;
    const double mvol = self->musicVol_.load() * self->musicFade_ * master;
    const double svol = self->sfxVol_.load() * master;

    for (int i = 0; i < samples; ++i) {
        double t = now + i * dt;
        double sum = 0.0;
        // music voices
        for (auto& v : self->musicPool_) {
            if (!v.active)
                continue;
            double lt = t - v.t0;
            if (lt < 0.0)
                continue;
            if (lt > v.dur + 0.05) {
                v.active = false;
                continue;
            }
            double s = 0.0;
            double p = kTwoPi * v.freq * lt;
            switch (v.inst) {
            case 0: s = (std::sin(p) + 0.45 * std::sin(2 * p + 0.3) + 0.2 * std::sin(3 * p)) *
                        std::exp(-5.5 * lt) * std::min(1.0, lt * 300.0); break;
            case 1: s = Tri(p) * std::exp(-2.2 * lt) * 0.9; break;
            case 2: s = std::tanh(2.8 * std::sin(p + 0.006 * std::sin(kTwoPi * 5.5 * lt))) *
                       std::min(1.0, lt * 220.0) * std::exp(-2.8 * lt); break;
            case 3: {
                double q = std::fmod(p / kTwoPi, 1.0);
                if (q < 0)
                    q += 1.0;
                s = (q < 0.5 ? 1.0 : -1.0) * std::exp(-3.0 * lt) * std::min(1.0, lt * 200.0);
                break;
            }
            case 4:
                s = (std::sin(p) + 0.35 * std::sin(3 * p) + 0.15 * std::sin(5 * p)) *
                    std::exp(-4.5 * lt) * std::min(1.0, lt * 400.0);
                break;
            case 5: s = (Tri(p) + 0.4 * std::sin(2 * p)) * std::exp(-3.5 * lt); break;
            case 10: {
                double ph = kTwoPi * (42.0 * lt + (90.0 / 30.0) * (1.0 - std::exp(-30.0 * lt)));
                s = std::sin(ph) * std::exp(-8.0 * lt);
                break;
            }
            case 11: {
                double n = Noise(v.noise, static_cast<unsigned>(lt * kSampleRate));
                s = n * std::exp(-16.0 * lt) + 0.25 * std::sin(kTwoPi * 180.0 * lt) *
                    std::exp(-14.0 * lt);
                break;
            }
            case 12: {
                double n = Noise(v.noise, static_cast<unsigned>(lt * kSampleRate));
                s = n * std::exp(-55.0 * lt) * 0.5;
                break;
            }
            default: s = 0.0;
            }
            sum += s * v.gain * mvol;
        }
        // sfx voices
        for (auto& v : self->sfxPool_) {
            if (!v.active)
                continue;
            double lt = t - v.t0;
            if (lt < 0.0)
                continue;
            if (lt > v.dur) {
                v.active = false;
                continue;
            }
            double s = 0.0;
            if (v.inst == 0) {
                s = std::sin(kTwoPi * v.f0 * lt) * std::min(1.0, lt * 500.0) * std::exp(-6.0 * lt);
            } else if (v.inst == 1) {
                s = Noise(v.noise, static_cast<unsigned>(lt * kSampleRate)) *
                    std::exp(-18.0 * lt);
            } else {
                double k = (v.f1 - v.f0) / (2.0 * v.dur);
                s = std::sin(kTwoPi * (v.f0 * lt + k * lt * lt)) *
                    std::min(1.0, lt * 500.0) * (1.0 - lt / v.dur);
            }
            sum += s * v.gain * svol;
        }
        // soft clip
        if (sum > 1.0)
            sum = std::tanh(sum);
        if (sum < -1.0)
            sum = std::tanh(sum);
        out[i] = static_cast<int16_t>(sum * 30000.0);
    }
}

} // namespace ink
