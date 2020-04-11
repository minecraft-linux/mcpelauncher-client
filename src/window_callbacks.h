#pragma once

#include <game_window.h>
#include <unordered_map>
#include "jni/jni_support.h"
#include "fake_inputqueue.h"

class WindowCallbacks {

private:
    struct GamepadData {
        float stickLeft[2];
        float stickRight[2];

        GamepadData();
    };

    GameWindow &window;
    JniSupport &jniSupport;
    FakeInputQueue &inputQueue;
    std::unordered_map<int, GamepadData> gamepads;
    void (*Mouse_feed)(char, char, short, short, short, short);
    bool modCTRL = false;

public:
    WindowCallbacks(GameWindow &window, JniSupport &jniSupport, FakeInputQueue &inputQueue);

    static void loadGamepadMappings();

    void registerCallbacks();

    void onWindowSizeCallback(int w, int h);

    void onClose();

    void onMouseButton(double x, double y, int btn, MouseButtonAction action);
    void onMousePosition(double x, double y);
    void onMouseRelativePosition(double x, double y);
    void onMouseScroll(double x, double y, double dx, double dy);
    void onTouchStart(int id, double x, double y);
    void onTouchUpdate(int id, double x, double y);
    void onTouchEnd(int id, double x, double y);
    void onKeyboard(KeyCode key, KeyAction action);
    void onKeyboardText(std::string const& c);
    void onPaste(std::string const& str);
    void onGamepadState(int gamepad, bool connected);
    void onGamepadButton(int gamepad, GamepadButtonId btn, bool pressed);
    void onGamepadAxis(int gamepad, GamepadAxisId ax, float value);

    static int mapMinecraftToAndroidKey(KeyCode code);

};