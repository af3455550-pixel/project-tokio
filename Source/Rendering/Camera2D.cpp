#include "Rendering/Camera2D.h"
#include <cmath>

namespace ink {

static double ExpSmooth(double dt, double rate) { return 1.0 - std::exp(-rate * dt); }

void Camera2D::Update(double dt, Vec2 targetCenter, double facing, double velX) {
    // Look-ahead eases toward facing bias + velocity bias.
    double want = facing * lookAheadPx_ + velX * 0.06;
    want = Clamp(want, -140.0, 140.0);
    lookAhead_ = Lerp(lookAhead_, want, ExpSmooth(dt, 4.0));

    double vw = ViewWidthWorld();
    double vh = ViewHeightWorld();
    double desiredX = targetCenter.x + lookAhead_ - vw * 0.5;
    double desiredY = targetCenter.y - vh * 0.55; // bias slightly above center
    pos_.x = Lerp(pos_.x, desiredX, ExpSmooth(dt, 9.0));
    pos_.y = Lerp(pos_.y, desiredY, ExpSmooth(dt, 6.0));
    zoom_ = static_cast<float>(Lerp(zoom_, targetZoom_, ExpSmooth(dt, 3.0)));
    ClampToBounds();

    // Shake
    if (shakeAmp_ > 0.01) {
        shakeAmp_ *= std::exp(-7.0 * dt);
        shakePhase_ += dt * 60.0;
        double a = shakeAmp_ * shakeScale_;
        shakeOffset_ = {std::sin(shakePhase_ * 1.7) * a, std::cos(shakePhase_ * 2.3) * a};
    } else {
        shakeAmp_ = 0.0;
        shakeOffset_ = {0, 0};
    }
}

void Camera2D::UpdateBoss(double dt, const Rect& focus) {
    double vw = ViewWidthWorld();
    double vh = ViewHeightWorld();
    double desiredX = focus.Center().x - vw * 0.5;
    double desiredY = focus.Center().y - vh * 0.5;
    pos_.x = Lerp(pos_.x, desiredX, ExpSmooth(dt, 4.0));
    pos_.y = Lerp(pos_.y, desiredY, ExpSmooth(dt, 4.0));
    ClampToBounds();
}

void Camera2D::ClampToBounds() {
    if (!hasBounds_)
        return;
    double vw = ViewWidthWorld();
    double vh = ViewHeightWorld();
    if (bounds_.w < vw)
        pos_.x = bounds_.x + (bounds_.w - vw) * 0.5;
    else
        pos_.x = Clamp(pos_.x, bounds_.x, bounds_.Right() - vw);
    if (bounds_.h < vh)
        pos_.y = bounds_.y + (bounds_.h - vh) * 0.5;
    else
        pos_.y = Clamp(pos_.y, bounds_.y, bounds_.Bottom() - vh);
}

} // namespace ink
