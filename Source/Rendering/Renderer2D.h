#pragma once
// 2D renderer on top of SDL_Renderer. World-space draws are transformed by
// the camera; UI draws are in screen space. Sprites are blitted from a
// sprite atlas with nearest filtering; tinting and alpha are per-draw (§8).
#include "Core/Math.h"
#include "Rendering/Camera2D.h"
#include <map>
#include <string>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;

namespace ink {

class Renderer2D {
public:
    bool Init(SDL_Renderer* r, std::string* err = nullptr);
    void Shutdown();

    // Frame.
    void Clear(uint32_t color);
    void SetCamera(const Camera2D& cam, double viewW, double viewH);

    // Textures.
    int CreateTexture(int w, int h, const uint32_t* rgba);
    bool HasTexture(int id) const;

    // World-space drawing (camera applied).
    void Sprite(int tex, const Rect& src, const Rect& dst, bool flipX, uint32_t tint,
                double alpha = 1.0);
    void RectFillWorld(const Rect& r, uint32_t color, double alpha = 1.0);
    void RectOutlineWorld(const Rect& r, uint32_t color, double alpha = 1.0, int thickness = 1);
    void LineWorld(Vec2 a, Vec2 b, uint32_t color, double alpha = 1.0);

    // Screen-space drawing (UI).
    void RectFillScreen(const Rect& r, uint32_t color, double alpha = 1.0);
    void RectOutlineScreen(const Rect& r, uint32_t color, double alpha = 1.0, int thickness = 1);
    void SpriteScreen(int tex, const Rect& src, const Rect& dst, bool flipX, uint32_t tint,
                      double alpha = 1.0);
    void LineScreen(Vec2 a, Vec2 b, uint32_t color, double alpha = 1.0);

    // View culling helper.
    Rect ViewWorldRect() const {
        return {cam_.Pos().x, cam_.Pos().y, cam_.ViewWidthWorld(), cam_.ViewHeightWorld()};
    }
    bool Culls(const Rect& worldRect) const {
        return !ViewWorldRect().Overlaps(worldRect);
    }

    SDL_Renderer* Raw() const { return sdl_; }
    void SetAlpha(uint8_t a);

private:
    Rect WorldToScreen(const Rect& r) const;

    SDL_Renderer* sdl_ = nullptr;
    Camera2D cam_{};
    std::vector<SDL_Texture*> textures_;
    uint8_t globalAlpha_ = 255;
};

} // namespace ink
