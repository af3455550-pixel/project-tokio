#include "Engine/Application.h"
#include "Core/Log.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <cmath>
#include <cstdio>

namespace ink {

bool Application::Init(const AppConfig& cfg, IAppCallbacks& cb, std::string* err) {
    cfg_ = cfg;
    cb_ = &cb;

    if (cfg_.headless) {
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    }
    Uint32 flags = SDL_INIT_VIDEO;
    if (!cfg_.muteAudio)
        flags |= SDL_INIT_AUDIO;
    if (SDL_Init(flags) != 0) {
        // Retry without audio (headless environments may lack a device).
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            if (err)
                *err = std::string("SDL_Init failed: ") + SDL_GetError();
            return false;
        }
        INK_LOG_WARN("Audio unavailable, running silent: " + std::string(SDL_GetError()));
    }

    Uint32 winFlags = 0;
    if (cfg_.vsync && !cfg_.headless)
        winFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
    window_ = SDL_CreateWindow(cfg_.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               cfg_.width, cfg_.height, winFlags);
    if (!window_) {
        if (err)
            *err = std::string("window: ") + SDL_GetError();
        return false;
    }
    const uint32_t rflags = (cfg_.vsync && !cfg_.headless) ? SDL_RENDERER_PRESENTVSYNC : 0u;
    // Headless (dummy driver) needs the software renderer for real pixel readback.
    if (cfg_.headless)
        renderer_ = SDL_CreateRenderer(window_, -1, rflags | SDL_RENDERER_SOFTWARE);
    else
        renderer_ = SDL_CreateRenderer(window_, -1, rflags);
    if (!renderer_)
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer_) {
        if (err)
            *err = std::string("renderer: ") + SDL_GetError();
        return false;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest-neighbor, pixel art
    INK_LOG_INFO("Application ready: " + std::to_string(cfg_.width) + "x" +
                 std::to_string(cfg_.height) + (cfg_.headless ? " (headless)" : ""));
    return true;
}

void Application::PollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            quit_ = true;
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)
            quit_ = true;
        if (eventSink_)
            eventSink_(e);
    }
}

void Application::Run() {
    using clock = std::chrono::steady_clock;
    auto last = clock::now();
    double acc = 0.0;
    worstMs_ = 0.0;

    while (!quit_) {
        const auto frameStart = clock::now();
        PollEvents();

        double now = std::chrono::duration<double>(clock::now() - frameStart).count();
        double frameDt = now; // measured so far; final value after sim+render
        double delta = std::chrono::duration<double>(clock::now() - last).count();
        last = clock::now();
        if (delta > 0.25)
            delta = 0.25; // spiral-of-death guard (tab switch etc.)
        (void)frameDt;

        acc += delta;
        int steps = 0;
        while (acc >= kSimStep && steps < 4) {
            cb_->OnUpdate(kSimStep);
            simTime_ += kSimStep;
            simFrame_++;
            acc -= kSimStep;
            steps++;
        }
        if (steps == 4)
            acc = 0.0;

        cb_->OnRender(std::chrono::duration<double>(clock::now() - frameStart).count());
        if (capturePending_) {
            CaptureRaw(cfg_.screenshotRaw);
            capturePending_ = false;
        }
        SDL_RenderPresent(renderer_);

        const double totalMs =
            std::chrono::duration<double, std::milli>(clock::now() - frameStart).count();
        statsWindow_ += totalMs;
        statsFrames_++;
        if (totalMs > worstMs_)
            worstMs_ = totalMs;
        if (statsWindow_ >= 500.0) {
            stats_.frameTimeMs = statsWindow_ / statsFrames_;
            stats_.renderFps = 1000.0 / stats_.frameTimeMs;
            stats_.frameTimeMaxMs = worstMs_;
            worstMs_ = 0.0;
            stats_.simFps = 60.0;
            stats_.frameCount = simFrame_;
            statsWindow_ = 0.0;
            statsFrames_ = 0;
        }

        if (cfg_.autoQuitFrames > 0 && simFrame_ >= cfg_.autoQuitFrames) {
            if (!cfg_.screenshotRaw.empty())
                CaptureRaw(cfg_.screenshotRaw);
            break;
        }
    }

    if (cb_)
        cb_->OnClose();
    if (renderer_)
        SDL_DestroyRenderer(renderer_);
    if (window_)
        SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool Application::CaptureRaw(const std::string& path) const {
    if (!renderer_)
        return false;
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(renderer_, &w, &h);
    SDL_Surface* tmp = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!tmp)
        return false;
    bool ok = false;
    {
        // Read the window framebuffer (the current render target is the window).
        ok = SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_RGBA8888, tmp->pixels,
                                  tmp->pitch) == 0;
    }
    if (ok) {
        FILE* f = fopen(path.c_str(), "wb");
        if (f) {
            fwrite(tmp->pixels, 1, static_cast<size_t>(tmp->pitch) * tmp->h, f);
            fclose(f);
        }
    }
    SDL_FreeSurface(tmp);
    return ok;
}

} // namespace ink
