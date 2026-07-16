#pragma once
// Application: window, renderer, fixed-timestep game loop (§9).
// Simulation runs at 60 Hz regardless of the display refresh; rendering
// happens once per vsync. Headless mode (SDL dummy drivers) supports CI,
// automated play tests and screenshot capture.
#include <functional>
#include <string>

struct SDL_Renderer;
struct SDL_Window;
union SDL_Event; // SDL declares SDL_Event as a union — keep the tag in sync

namespace ink {

struct FrameStats {
    double renderFps = 0.0;
    double frameTimeMs = 0.0;
    double frameTimeMaxMs = 0.0;
    double simFps = 0.0;
    int frameCount = 0;
};

struct AppConfig {
    std::string title = "INKBOUND: The Last Reel";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool headless = false;
    bool muteAudio = false;
    int autoQuitFrames = 0;      // exit after this many SIM frames (0 = disabled)
    std::string screenshotRaw;   // write raw RGBA of the final frame here
    std::string assetsDir = "Assets";
    int seed = 1337;
};

class IAppCallbacks {
public:
    virtual ~IAppCallbacks() = default;
    // Fixed 1/60 s steps.
    virtual void OnUpdate(double dt) = 0;
    // Once per rendered frame.
    virtual void OnRender(double frameDt) = 0;
    virtual void OnClose() {}
    virtual void OnError(const std::string& msg) {}
};

class Application {
public:
    static constexpr double kSimStep = 1.0 / 60.0;

    bool Init(const AppConfig& cfg, IAppCallbacks& cb, std::string* err = nullptr);
    void Run();
    void RequestQuit() { quit_ = true; }

    SDL_Renderer* Renderer() const { return renderer_; }
    SDL_Window* Window() const { return window_; }
    const AppConfig& Config() const { return cfg_; }
    const FrameStats& Stats() const { return stats_; }
    bool Headless() const { return cfg_.headless; }
    double SimTime() const { return simTime_; }
    int SimFrame() const { return simFrame_; }

    // Raw RGBA capture of the last presented frame (headless QA / screenshots).
    bool CaptureRaw(const std::string& path) const;

    // Forward SDL events (input subscription).
    void SetEventSink(std::function<void(const SDL_Event&)> sink) { eventSink_ = std::move(sink); }

private:
    void PollEvents();

    AppConfig cfg_{};
    IAppCallbacks* cb_ = nullptr;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    std::function<void(const SDL_Event&)> eventSink_;

    bool quit_ = false;
    bool capturePending_ = false;
    double simTime_ = 0.0;
    int simFrame_ = 0;
    FrameStats stats_{};
    double statsWindow_ = 0.0;
    int statsFrames_ = 0;
    double worstMs_ = 0.0;
};

} // namespace ink
