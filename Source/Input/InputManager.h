#pragma once
// Input Manager (§77): logical actions decoupled from physical keys,
// remappable, with frame-buffered "pressed within N frames" queries that
// power the jump buffer and input buffer (§12). Keyboard + generic gamepad
// (Xbox layout); vibration is optional.
#include <functional>
#include <map>
#include <string>
#include <vector>

union SDL_Event; // SDL declares SDL_Event as a union — keep the tag in sync
struct _SDL_Joystick;
typedef struct _SDL_Joystick SDL_Joystick;

namespace ink {

enum class Action {
    MoveLeft,
    MoveRight,
    Jump,
    Crouch,
    Dash,
    Attack,
    Parry,
    Special,
    Interact,
    Pause,
    NextWeapon,
    Count
};

const char* ActionName(Action a);
const char* ActionLabel(Action a);

class InputManager {
public:
    // Default bindings (keyboard). Controller always maps Xbox layout.
    static std::map<Action, std::string> DefaultBindings();

    void Init(std::function<void(const SDL_Event&)> eventSink);
    void Shutdown();

    void BeginFrame(); // clear edge state (call once per sim frame)
    void HandleEvent(const SDL_Event& e);

    bool Down(Action a) const;
    bool Pressed(Action a) const;              // this frame
    bool PressedWithin(Action a, int frames) const; // last N frames
    float AxisX() const;                        // -1..1 (move)
    bool CrouchHeld() const { return Down(Action::Crouch); }
    bool DropThrough() const;                   // crouch + down on one-way

    void SetBindings(const std::map<Action, std::string>& b) { binds_ = b; }
    const std::map<Action, std::string>& Bindings() const { return binds_; }
    void SetDeadzone(float dz) { deadzone_ = dz; }
    float Deadzone() const { return deadzone_; }
    bool VibrationEnabled() const { return vibration_; }
    void SetVibrationEnabled(bool v) { vibration_ = v; }
    void Vibrate(double amount, double seconds);

    bool ControllerPresent() const { return joy_ != nullptr; }
    // For the options UI: capture the next key press into an action.
    void BeginRemap(Action a) { remapping_ = a; }
    bool Remapping() const { return remapping_ != Action::Count; }

private:
    int KeyToAction(int scancode) const;
    const char* ActionToKey(Action a) const { return binds_.at(a).c_str(); }

    std::function<void(const SDL_Event&)> sink_;
    std::map<Action, std::string> binds_;
    SDL_Joystick* joy_ = nullptr;

    static constexpr int kHistory = 16;
    std::vector<std::vector<bool>> held_;    // [action][frame]
    std::vector<std::vector<bool>> pressed_; // [action][frame]
    int frame_ = 0;

    float deadzone_ = 0.25f;
    bool vibration_ = true;
    Action remapping_ = Action::Count;
    bool gamepadMoveLeft = false;
    bool gamepadMoveRight = false;
};

} // namespace ink
