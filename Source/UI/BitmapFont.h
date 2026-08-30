#pragma once
// Original 5x7 pixel bitmap font (hand-designed for Inkbound, §51).
// No external font assets. Uppercase A-Z, 0-9, and common punctuation.
#include "Rendering/Renderer2D.h"
#include <cstdint>
#include <string>

namespace ink {

class BitmapFont {
public:
    static constexpr int kGlyphW = 5;
    static constexpr int kGlyphH = 7;
    static constexpr int kSpace = 1;

    int Measure(const std::string& text, int scale = 1) const;
    // shadow=true draws a 1px ink drop shadow (hand-drawn look).
    void Draw(Renderer2D& r, double x, double y, const std::string& text, int scale,
              uint32_t color, uint32_t shadow = 0xFF201A2E, bool shadowOn = true) const;
    // Draw centered in a rect.
    void DrawCenter(Renderer2D& r, const Rect& box, const std::string& text, int scale,
                    uint32_t color) const;

private:
    uint8_t GlyphRow(char c, int row) const;
};

} // namespace ink
