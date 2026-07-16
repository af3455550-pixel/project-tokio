#include "Rendering/Renderer2D.h"
#include <SDL2/SDL.h>

namespace ink {

bool Renderer2D::Init(SDL_Renderer* r, std::string* err) {
    sdl_ = r;
    if (!sdl_) {
        if (err)
            *err = "no renderer";
        return false;
    }
    textures_.clear();
    return true;
}

void Renderer2D::Shutdown() {
    for (auto* t : textures_)
        if (t)
            SDL_DestroyTexture(t);
    textures_.clear();
    sdl_ = nullptr;
}

void Renderer2D::Clear(uint32_t color) {
    SDL_SetRenderDrawColor(sdl_, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0xFF);
    SDL_RenderClear(sdl_);
}

void Renderer2D::SetCamera(const Camera2D& cam, double viewW, double viewH) {
    cam_ = cam;
    cam_.SetViewport(viewW, viewH);
}

int Renderer2D::CreateTexture(int w, int h, const uint32_t* rgba) {
    SDL_Texture* tex =
        SDL_CreateTexture(sdl_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex)
        return -1;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
    const int pitch = w * 4;
    SDL_UpdateTexture(tex, nullptr, rgba, pitch);
    textures_.push_back(tex);
    return static_cast<int>(textures_.size()) - 1;
}

bool Renderer2D::HasTexture(int id) const {
    return id >= 0 && id < static_cast<int>(textures_.size());
}

void Renderer2D::SetAlpha(uint8_t a) { globalAlpha_ = a; }

Rect Renderer2D::WorldToScreen(const Rect& r) const {
    double ox = -cam_.Pos().x + cam_.ShakeOffset().x;
    double oy = -cam_.Pos().y + cam_.ShakeOffset().y;
    double z = cam_.Zoom();
    return {(r.x + ox) * z, (r.y + oy) * z, r.w * z, r.h * z};
}

static void ApplyAlpha(SDL_Renderer* r, uint32_t color, uint8_t globalAlpha) {
    uint8_t a = (color >> 24) & 0xFF;
    a = static_cast<uint8_t>(a * globalAlpha / 255);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, a);
}

void Renderer2D::Sprite(int tex, const Rect& src, const Rect& dst, bool flipX, uint32_t tint,
                        double alpha) {
    if (tex < 0 || tex >= static_cast<int>(textures_.size()))
        return;
    SDL_Rect s{static_cast<int>(src.x), static_cast<int>(src.y),
               static_cast<int>(std::max(1.0, src.w)), static_cast<int>(std::max(1.0, src.h))};
    Rect d = WorldToScreen(dst);
    SDL_Rect dr{static_cast<int>(d.x), static_cast<int>(d.y),
                static_cast<int>(std::max(1.0, d.w)), static_cast<int>(std::max(1.0, d.h))};
    SDL_SetTextureAlphaMod(textures_[tex], static_cast<uint8_t>(alpha * globalAlpha_));
    SDL_SetTextureColorMod(textures_[tex], (tint >> 16) & 0xFF, (tint >> 8) & 0xFF, tint & 0xFF);
    SDL_RenderCopyEx(sdl_, textures_[tex], &s, &dr, 0.0, nullptr,
                    flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    SDL_SetTextureColorMod(textures_[tex], 255, 255, 255);
    SDL_SetTextureAlphaMod(textures_[tex], 255);
}

void Renderer2D::RectFillWorld(const Rect& r, uint32_t color, double alpha) {
    ApplyAlpha(sdl_, color, static_cast<uint8_t>(alpha * globalAlpha_));
    Rect d = WorldToScreen(r);
    SDL_Rect dr{static_cast<int>(d.x), static_cast<int>(d.y),
                static_cast<int>(std::max(1.0, d.w)), static_cast<int>(std::max(1.0, d.h))};
    SDL_RenderFillRect(sdl_, &dr);
}

void Renderer2D::RectOutlineWorld(const Rect& r, uint32_t color, double alpha, int thickness) {
    ApplyAlpha(sdl_, color, static_cast<uint8_t>(alpha * globalAlpha_));
    Rect d = WorldToScreen(r);
    SDL_Rect dr{static_cast<int>(d.x), static_cast<int>(d.y),
                static_cast<int>(std::max(1.0, d.w)), static_cast<int>(std::max(1.0, d.h))};
    SDL_RenderDrawRect(sdl_, &dr);
    if (thickness > 1) {
        SDL_Rect d2{dr.x + thickness, dr.y + thickness, dr.w - thickness * 2, dr.h - thickness * 2};
        SDL_RenderDrawRect(sdl_, &d2);
    }
}

void Renderer2D::LineWorld(Vec2 a, Vec2 b, uint32_t color, double alpha) {
    ApplyAlpha(sdl_, color, static_cast<uint8_t>(alpha * globalAlpha_));
    Rect ra{a.x, a.y, 1, 1}, rb{b.x, b.y, 1, 1};
    SDL_RenderDrawLine(sdl_, static_cast<int>(WorldToScreen(ra).x), static_cast<int>(WorldToScreen(ra).y),
                   static_cast<int>(WorldToScreen(rb).x), static_cast<int>(WorldToScreen(rb).y));
}

void Renderer2D::RectFillScreen(const Rect& r, uint32_t color, double alpha) {
    ApplyAlpha(sdl_, color, static_cast<uint8_t>(alpha * globalAlpha_));
    SDL_Rect dr{static_cast<int>(r.x), static_cast<int>(r.y),
                static_cast<int>(std::max(1.0, r.w)), static_cast<int>(std::max(1.0, r.h))};
    SDL_RenderFillRect(sdl_, &dr);
}

void Renderer2D::RectOutlineScreen(const Rect& r, uint32_t color, double alpha, int thickness) {
    ApplyAlpha(sdl_, color, static_cast<uint8_t>(alpha * globalAlpha_));
    SDL_Rect dr{static_cast<int>(r.x), static_cast<int>(r.y),
                static_cast<int>(std::max(1.0, r.w)), static_cast<int>(std::max(1.0, r.h))};
    SDL_RenderDrawRect(sdl_, &dr);
    if (thickness > 1) {
        SDL_Rect d2{dr.x + thickness, dr.y + thickness, dr.w - thickness * 2, dr.h - thickness * 2};
        SDL_RenderDrawRect(sdl_, &d2);
    }
}

void Renderer2D::SpriteScreen(int tex, const Rect& src, const Rect& dst, bool flipX, uint32_t tint,
                              double alpha) {
    if (tex < 0 || tex >= static_cast<int>(textures_.size()))
        return;
    SDL_Rect s{static_cast<int>(src.x), static_cast<int>(src.y),
               static_cast<int>(std::max(1.0, src.w)), static_cast<int>(std::max(1.0, src.h))};
    SDL_Rect dr{static_cast<int>(dst.x), static_cast<int>(dst.y),
                static_cast<int>(std::max(1.0, dst.w)), static_cast<int>(std::max(1.0, dst.h))};
    SDL_SetTextureAlphaMod(textures_[tex], static_cast<uint8_t>(alpha * globalAlpha_));
    SDL_SetTextureColorMod(textures_[tex], (tint >> 16) & 0xFF, (tint >> 8) & 0xFF, tint & 0xFF);
    SDL_RenderCopyEx(sdl_, textures_[tex], &s, &dr, 0.0, nullptr, flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    SDL_SetTextureColorMod(textures_[tex], 255, 255, 255);
    SDL_SetTextureAlphaMod(textures_[tex], 255);
}

void Renderer2D::LineScreen(Vec2 a, Vec2 b, uint32_t color, double alpha) {
    ApplyAlpha(sdl_, color, static_cast<uint8_t>(alpha * globalAlpha_));
    SDL_RenderDrawLine(sdl_, static_cast<int>(a.x), static_cast<int>(a.y), static_cast<int>(b.x),
                   static_cast<int>(b.y));
}

} // namespace ink
