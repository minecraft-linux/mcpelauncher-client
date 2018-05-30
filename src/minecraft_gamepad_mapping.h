#pragma once

#include <game_window.h>

class MinecraftGamepadMapping {

public:
    static const int BUTTON_A = 0;
    static const int BUTTON_B = 1;
    static const int BUTTON_X = 2;
    static const int BUTTON_Y = 3;
    static const int BUTTON_DPAD_UP = 4;
    static const int BUTTON_DPAD_DOWN = 5;
    static const int BUTTON_DPAD_LEFT = 6;
    static const int BUTTON_DPAD_RIGHT = 7;
    static const int BUTTON_LEFT_STICK = 8;
    static const int BUTTON_RIGHT_STICK = 9;
    static const int BUTTON_LB = 10;
    static const int BUTTON_RB = 11;
    static const int BUTTON_SELECT = 12;
    static const int BUTTON_START = 13;

    static int mapButton(GamepadButtonId i) {
        switch (i) {
            case GamepadButtonId::A: return BUTTON_A;
            case GamepadButtonId::B: return BUTTON_B;
            case GamepadButtonId::X: return BUTTON_X;
            case GamepadButtonId::Y: return BUTTON_Y;
            case GamepadButtonId::LB: return BUTTON_LB;
            case GamepadButtonId::RB: return BUTTON_RB;
            case GamepadButtonId::BACK: return BUTTON_SELECT;
            case GamepadButtonId::START: return BUTTON_START;
            case GamepadButtonId::LEFT_STICK: return BUTTON_LEFT_STICK;
            case GamepadButtonId::RIGHT_STICK: return BUTTON_RIGHT_STICK;
            case GamepadButtonId::DPAD_UP: return BUTTON_DPAD_UP;
            case GamepadButtonId::DPAD_RIGHT: return BUTTON_DPAD_RIGHT;
            case GamepadButtonId::DPAD_DOWN: return BUTTON_DPAD_DOWN;
            case GamepadButtonId::DPAD_LEFT: return BUTTON_DPAD_LEFT;
            default: return -1;
        }
    }

};