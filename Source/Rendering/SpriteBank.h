#pragma once
// SpriteBank: collects hand-authored pixel frames and packs them into a
// single texture atlas (§8). All art in the slice is generated procedurally
// in C++ (Source/Art) — 100% original IP, no external assets.
#include "Core/Math.h"
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ink {

struct SpriteFrame {
    int w = 0, h = 0;
    std::vector<uint32_t> rgba; // w*h, 0 = transparent
};

// Atlas-space rect (double precision, converts to Rect for the renderer).
struct RectF {
    double x = 0.0, y = 0.0, w = 0.0, h = 0.0;

    RectF() = default;
    // See Math.h Vec2: avoids narrowing diagnostics for integer arguments.
    template <typename Tx, typename Ty, typename Tw, typename Th>
    RectF(Tx x_, Ty y_, Tw w_, Th h_)
        : x(static_cast<double>(x_)), y(static_cast<double>(y_)),
          w(static_cast<double>(w_)), h(static_cast<double>(h_)) {}

    operator Rect() const { return Rect{x, y, w, h}; }
};

class SpriteBank {
public:
    // Register a frame (call before Build).
    void Add(const std::string& name, int w, int h, const std::vector<uint32_t>& rgba);
    void Add(const std::string& name, int w, int h, const uint32_t* rgba);

    // Build the packed atlas through the given texture creator.
    // Returns the texture id (-1 on failure).
    int Build(const std::function<int(int, int, const uint32_t*)>& createTexture);

    bool Has(const std::string& name) const { return entries_.count(name) > 0; }
    // Atlas-space rect for a frame.
    RectF Rect(const std::string& name) const;
    Vec2 Size(const std::string& name) const;
    int TextureId() const { return textureId_; }
    std::size_t FrameCount() const { return frames_.size(); }

private:
    struct Entry {
        int x = 0, y = 0;
    };
    std::map<std::string, SpriteFrame> frames_;
    std::map<std::string, Entry> entries_;
    int textureId_ = -1;
    int atlasW_ = 0, atlasH_ = 0;
};

} // namespace ink
