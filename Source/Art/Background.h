#pragma once
// Procedural parallax backgrounds (§30): no bitmap assets — everything is
// drawn with vector primitives each frame, so worlds stay lightweight and
// every layer pans at its own parallax factor.
#include "Rendering/Renderer2D.h"
#include <string>

namespace ink {

class Camera2D;

void DrawMeadowsBackground(Renderer2D& r, const Camera2D& cam, double timeSec,
                           int viewW, int viewH);

} // namespace ink
