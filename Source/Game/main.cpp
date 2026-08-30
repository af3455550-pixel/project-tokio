// INKBOUND: THE LAST REEL — entry point.
//   inkbound [assetsDir]
// Environment (headless QA / CI):
//   SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
//   INK_HEADLESS=1            force dummy drivers
//   INK_AUTOQUIT=300          exit after N sim frames
//   INK_SCREENSHOT=path.png   capture the final frame
//   INK_SEED=1337             deterministic sim seed
#include "Core/Log.h"
#include "Engine/Application.h"
#include "Game/Game.h"

#include <SDL.h>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace ink;

static const char* EnvOr(const char* name, const char* dflt) {
    const char* v = std::getenv(name);
    return v && *v ? v : dflt;
}

int main(int argc, char** argv) {
    std::string assetsDir = argc > 1 ? argv[1] : "Assets";

    AppConfig cfg;
    cfg.assetsDir = assetsDir;
    cfg.headless = std::strcmp(EnvOr("SDL_VIDEODRIVER", ""), "dummy") == 0 ||
                  std::strcmp(EnvOr("INK_HEADLESS", "0"), "1") == 0;
    const char* aq = std::getenv("INK_AUTOQUIT");
    if (aq && *aq)
        cfg.autoQuitFrames = std::atoi(aq);
    const char* ss = std::getenv("INK_SCREENSHOT");
    if (ss && *ss)
        cfg.screenshotRaw = ss;
    const char* seed = std::getenv("INK_SEED");
    if (seed && *seed)
        cfg.seed = std::atoi(seed);
    if (cfg.headless)
        cfg.vsync = false;

    Application app;
    Game game;
    std::string err;
    if (!app.Init(cfg, game, &err)) {
        INK_LOG_ERROR("app init failed: {}", err);
        return 1;
    }
    if (!game.Init(app, &err)) {
        INK_LOG_ERROR("game init failed: {}", err);
        return 1;
    }
    app.Run();
    return 0;
}
