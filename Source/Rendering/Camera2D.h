#pragma once
// Professional 2D camera (§43): smooth exponential follow, look-ahead in the
// facing/velocity direction, dynamic zoom, boss framing, clamped bounds,
// decaying screen shake (§13), optional cinematic letterbox framing.
#include "Core/Math.h"
#include <algorithm>

namespace ink {

class Camera2D {
public:
    void SetBounds(const Rect& world) { bounds_ = world; hasBounds_ = true; }
    void SetZoom(float zoom) { targetZoom_ = zoom; }
    void ForcePos(Vec2 topLeft) { pos_ = topLeft; }
    void SetLookAhead(double px) { lookAheadPx_ = px; }

    void Update(double dt, Vec2 targetCenter, double facing, double velX);

    // Boss framing: keep the whole arena-ish rect in view.
    void UpdateBoss(double dt, const Rect& focus);

    void AddShake(double amount) {
        shakeAmp_ = std::max(shakeAmp_, amount);
    }
    void SetShakeScale(double s) { shakeScale_ = s; }

    Vec2 Pos() const { return pos_; }
    float Zoom() const { return zoom_; }
    Vec2 ShakeOffset() const { return shakeOffset_; }
    Rect ViewRect() const {
        return {pos_.x, pos_.y, viewW_ / zoom_, viewH_ / zoom_};
    }
    double ViewWidthWorld() const { return viewW_ / zoom_; }
    double ViewHeightWorld() const { return viewH_ / zoom_; }

    void SetViewport(double w, double h) {
        viewW_ = w;
        viewH_ = h;
    }

private:
    void ClampToBounds();

    Vec2 pos_{0, 0}; // top-left of the view in world coords
    float zoom_ = 1.0f;
    float targetZoom_ = 1.0f;
    double lookAheadPx_ = 60.0;
    double lookAhead_ = 0.0;
    Rect bounds_{0, 0, 0, 0};
    bool hasBounds_ = false;
    double viewW_ = 1280.0;
    double viewH_ = 720.0;
    double shakeAmp_ = 0.0;
    double shakeScale_ = 1.0;
    Vec2 shakeOffset_{0, 0};
    double shakePhase_ = 0.0;
};

} // namespace ink
