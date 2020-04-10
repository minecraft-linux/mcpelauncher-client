#pragma once

#include <game_window.h>
#include <unordered_map>

class WindowCallbacks {

private:
    struct GamepadData {
        float stickLeft[2];
        float stickRight[2];

        GamepadData();
    };

    GameWindow& window;
    std::unordered_map<int, GamepadData> gamepads;
    bool modCTRL = false;

public:
    WindowCallbacks(GameWindow& window) : window(window) { }

    static void loadGamepadMappings();

    void registerCallbacks();

    void onWindowSizeCallback(int w, int h);

    void onDraw();

    void onClose();

    void onMouseButton(double x, double y, int btn, MouseButtonAction action);
    void onMousePosition(double x, double y);
    void onMouseRelativePosition(double x, double y);
    void onMouseScroll(double x, double y, double dx, double dy);
    void onTouchStart(int id, double x, double y);
    void onTouchUpdate(int id, double x, double y);
    void onTouchEnd(int id, double x, double y);
    void onKeyboard(int key, KeyAction action);
    void onKeyboardText(std::string const& c);
    void onPaste(std::string const& str);
    void onGamepadState(int gamepad, bool connected);
    void onGamepadButton(int gamepad, GamepadButtonId btn, bool pressed);
    void onGamepadAxis(int gamepad, GamepadAxisId ax, float value);

};