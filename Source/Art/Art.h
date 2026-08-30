#pragma once
// Procedural original art for Inkbound. Every sprite is hand-authored pixel
// data in C++ (string grids for characters, primitives for tiles/bosses) —
// 100% original IP, zero external assets (§3, §80).
#include "Rendering/SpriteBank.h"
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace ink {

// Minimal pixel canvas: palette-mapped string grids + primitive fills.
class PixelCanvas {
public:
    PixelCanvas(int w, int h) : w_(w), h_(h), px_(static_cast<std::size_t>(w) * h, 0) {}

    void SetPalette(char c, uint32_t rgba) { pal_[static_cast<unsigned char>(c)] = rgba; }
    void Clear(uint32_t rgba = 0) { px_.assign(px_.size(), rgba); }

    void Px(int x, int y, char c) {
        if (x < 0 || y < 0 || x >= w_ || y >= h_)
            return;
        px_[y * w_ + x] = pal_[static_cast<unsigned char>(c)];
    }
    void PxRaw(int x, int y, uint32_t rgba) {
        if (x < 0 || y < 0 || x >= w_ || y >= h_)
            return;
        px_[y * w_ + x] = rgba;
    }
    void Rect(int x, int y, int w, int h, char c) {
        for (int j = 0; j < h; ++j)
            for (int i = 0; i < w; ++i)
                Px(x + i, y + j, c);
    }
    void Ellipse(int cx, int cy, int rx, int ry, char c, bool fill = true) {
        for (int j = -ry; j <= ry; ++j)
            for (int i = -rx; i <= rx; ++i) {
                double d = static_cast<double>(i * i) / (rx * rx + 0.001) +
                           static_cast<double>(j * j) / (ry * ry + 0.001);
                if (d <= 1.0)
                    Px(cx + i, cy + j, c);
                else if (!fill && d <= 1.35)
                    Px(cx + i, cy + j, c);
            }
    }
    void Line(int x0, int y0, int x1, int y1, char c) {
        int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            Px(x0, y0, c);
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
    // Draw a grid of strings ('.' or palette char = transparent).
    void Grid(int ox, int oy, const std::vector<std::string>& rows) {
        for (int j = 0; j < static_cast<int>(rows.size()); ++j)
            for (int i = 0; i < static_cast<int>(rows[j].size()); ++i)
                if (rows[j][i] != '.')
                    Px(ox + i, oy + j, rows[j][i]);
    }

    int W() const { return w_; }
    int H() const { return h_; }
    const std::vector<uint32_t>& Pixels() const { return px_; }
    void AddTo(SpriteBank& bank, const std::string& name) const {
        bank.Add(name, w_, h_, px_);
    }

private:
    int w_, h_;
    std::vector<uint32_t> px_;
    uint32_t pal_[256] = {};
};

// Shared palette chars.
void InitSharedPalette(PixelCanvas& c);

void BuildMiloArt(SpriteBank& bank);
void BuildEnemyArt(SpriteBank& bank);
void BuildBossArt(SpriteBank& bank);
void BuildNpcArt(SpriteBank& bank);
void BuildTileArt(SpriteBank& bank);
void BuildPickupArt(SpriteBank& bank);

} // namespace ink
