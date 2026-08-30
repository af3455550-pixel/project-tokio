#include "Input/InputManager.h"
#include "Core/Log.h"
#include <SDL2/SDL.h>
#include <cmath>

namespace ink {

const char* ActionName(Action a) {
    switch (a) {
    case Action::MoveLeft: return "movel";
    case Action::MoveRight: return "mover";
    case Action::Jump: return "jump";
    case Action::Crouch: return "crouch";
    case Action::Dash: return "dash";
    case Action::Attack: return "attack";
    case Action::Parry: return "parry";
    case Action::Special: return "special";
    case Action::Interact: return "interact";
    case Action::Pause: return "pause";
    case Action::NextWeapon: return "nextweapon";
    case Action::Count: return "?";
    }
    return "?";
}

const char* ActionLabel(Action a) {
    switch (a) {
    case Action::MoveLeft: return "MOVE LEFT";
    case Action::MoveRight: return "MOVE RIGHT";
    case Action::Jump: return "JUMP";
    case Action::Crouch: return "CROUCH";
    case Action::Dash: return "DASH";
    case Action::Attack: return "ATTACK";
    case Action::Parry: return "PARRY";
    case Action::Special: return "SPECIAL";
    case Action::Interact: return "INTERACT";
    case Action::Pause: return "PAUSE";
    case Action::NextWeapon: return "NEXT WEAPON";
    case Action::Count: return "?";
    }
    return "?";
}

std::map<Action, std::string> InputManager::DefaultBindings() {
    return {
        {Action::MoveLeft, "a"},
        {Action::MoveRight, "d"},
        {Action::Jump, "space"},
        {Action::Crouch, "s"},
        {Action::Dash, "shift"},
        {Action::Attack, "j"},
        {Action::Parry, "k"},
        {Action::Special, "l"},
        {Action::Interact, "e"},
        {Action::Pause, "escape"},
        {Action::NextWeapon, "tab"},
    };
}

void InputManager::Init(std::function<void(const SDL_Event&)> eventSink) {
    sink_ = std::move(eventSink);
    binds_ = DefaultBindings();
    held_.assign(static_cast<int>(Action::Count), std::vector<bool>(kHistory, false));
    pressed_.assign(static_cast<int>(Action::Count), std::vector<bool>(kHistory, false));
    frame_ = 0;
    if (!joy_) {
        if (SDL_NumJoysticks() > 0) {
            joy_ = SDL_JoystickOpen(0);
            if (joy_)
                INK_LOG_INFO("Gamepad attached: " + std::string(SDL_JoystickName(joy_)));
        }
    }
}

void InputManager::Shutdown() {
    if (joy_) {
        SDL_JoystickClose(joy_);
        joy_ = nullptr;
    }
}

int InputManager::KeyToAction(int scancode) const {
    auto match = [&](const char* name) { return SDL_GetScancodeFromName(name) == scancode; };
    for (const auto& [a, name] : binds_) {
        if (match(name.c_str()))
            return static_cast<int>(a);
    }
    // Fallbacks: arrows / common aliases
    if (scancode == SDL_SCANCODE_LEFT)
        return static_cast<int>(Action::MoveLeft);
    if (scancode == SDL_SCANCODE_RIGHT)
        return static_cast<int>(Action::MoveRight);
    if (scancode == SDL_SCANCODE_DOWN)
        return static_cast<int>(Action::Crouch);
    if (scancode == SDL_SCANCODE_UP || scancode == SDL_SCANCODE_W)
        return static_cast<int>(Action::Jump);
    if (scancode == SDL_SCANCODE_X)
        return static_cast<int>(Action::Attack);
    if (scancode == SDL_SCANCODE_C)
        return static_cast<int>(Action::Dash);
    return -1;
}

void InputManager::HandleEvent(const SDL_Event& e) {
    if (remapping_ != Action::Count && e.type == SDL_KEYDOWN && !e.key.repeat) {
        std::string name = SDL_GetScancodeName(e.key.keysym.scancode);
        binds_[remapping_] = name;
        remapping_ = Action::Count;
        return;
    }
    if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        int a = KeyToAction(e.key.keysym.scancode);
        if (a < 0)
            return;
        if (e.type == SDL_KEYDOWN) {
            held_[a][frame_ & (kHistory - 1)] = true;
            if (!e.key.repeat)
                pressed_[a][frame_ & (kHistory - 1)] = true;
        } else {
            held_[a][frame_ & (kHistory - 1)] = false;
        }
        return;
    }
    if (e.type == SDL_JOYBUTTONDOWN || e.type == SDL_JOYBUTTONUP) {
        // Xbox layout: 0 A (jump), 1 B (parry), 2 X (attack), 3 Y (dash),
        // 4 LB, 5 RB, 6 back, 7 start (pause), 9 D-pad down (crouch/drop),
        // 8 D-pad up (drop-through up), 10 dpad left, 11 dpad right.
        static const int map[] = {0, 5, 2, 3, -1, 4, -1, 9, 10, 11, -1, -1, -1, -1, -1, -1};
        int b = e.jbutton.button;
        if (b >= 0 && b < 16) {
            int a = map[b];
            if (a >= 0) {
                // reuse the same history ring as keyboard (actions are shared)
                if (e.type == SDL_JOYBUTTONDOWN) {
                    if (b == 0) pressed_[static_cast<int>(Action::Jump)][frame_ & (kHistory - 1)] = true;
                    if (b == 1) pressed_[static_cast<int>(Action::Parry)][frame_ & (kHistory - 1)] = true;
                    if (b == 2) pressed_[static_cast<int>(Action::Attack)][frame_ & (kHistory - 1)] = true;
                    if (b == 3) pressed_[static_cast<int>(Action::Dash)][frame_ & (kHistory - 1)] = true;
                    if (b == 9) pressed_[static_cast<int>(Action::Crouch)][frame_ & (kHistory - 1)] = true;
                    if (b == 7) pressed_[static_cast<int>(Action::Pause)][frame_ & (kHistory - 1)] = true;
                    held_[static_cast<int>(a)][frame_ & (kHistory - 1)] = true;
                } else {
                    held_[static_cast<int>(a)][frame_ & (kHistory - 1)] = false;
                }
            }
        }
        return;
    }
    if (e.type == SDL_JOYAXISMOTION) {
        if (e.jaxis.axis == 0) { // left stick X
            float v = static_cast<float>(e.jaxis.value) / 32767.0f;
            if (std::abs(v) < deadzone_)
                v = 0.0f;
            gamepadMoveLeft = v < -deadzone_ * 0.5f;
            gamepadMoveRight = v > deadzone_ * 0.5f;
        }
    }
}

void InputManager::BeginFrame() {
    frame_++;
}

bool InputManager::Down(Action a) const {
    int i = static_cast<int>(a);
    if (i < 0 || i >= static_cast<int>(Action::Count))
        return false;
    return held_[i][frame_ & (kHistory - 1)];
}

bool InputManager::Pressed(Action a) const {
    int i = static_cast<int>(a);
    return pressed_[i][frame_ & (kHistory - 1)];
}

bool InputManager::PressedWithin(Action a, int frames) const {
    int i = static_cast<int>(a);
    for (int n = 0; n < frames && n < kHistory; ++n) {
        int f = (frame_ - n) & (kHistory - 1);
        if (pressed_[i][f])
            return true;
    }
    return false;
}

float InputManager::AxisX() const {
    float x = 0.0f;
    if (Down(Action::MoveLeft))
        x -= 1.0f;
    if (Down(Action::MoveRight))
        x += 1.0f;
    if (joy_) {
        if (gamepadMoveLeft)
            x = -1.0f;
        if (gamepadMoveRight)
            x = 1.0f;
        // D-pad
        if (SDL_JoystickGetButton(joy_, 10))
            x = -1.0f;
        if (SDL_JoystickGetButton(joy_, 11))
            x = 1.0f;
    }
    return x < 0 ? -1.0f : (x > 0 ? 1.0f : 0.0f);
}

bool InputManager::DropThrough() const {
    // Crouch while on ground = drop-through (also gamepad d-pad down)
    return Down(Action::Crouch);
}

void InputManager::Vibrate(double amount, double seconds) {
    if (!vibration_ || !joy_)
        return;
    if (SDL_JoystickRumble(joy_, static_cast<int>(amount * 0xFFFF),
                           static_cast<int>(amount * 0.5 * 0xFFFF),
                           static_cast<int>(seconds * 1000.0)) != 0) {
        // No haptic support on this controller; ignore gracefully.
    }
}

} // namespace ink
