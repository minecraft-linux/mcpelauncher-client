#pragma once

#include <game_window.h>
#include <chrono>
#include <unordered_map>
#include "fake_inputqueue.h"
#include "jni/jni_support.h"

class WindowCallbacks {
    private:
    struct GamepadData {
        float axis[6];
        bool button[15];

        GamepadData();
    };

    GameWindow &window;
    JniSupport &jniSupport;
    FakeInputQueue &inputQueue;
    std::unordered_map<int, GamepadData> gamepads;
    bool useDirectMouseInput, useDirectKeyboardInput;
    bool modCTRL = false;
    bool needsQueueGamepadInput = true;
    bool fullscreen = false;
    enum class InputMode {
        Touch,
        Mouse,
        Gamepad,
        Unknown,
    };
    InputMode inputMode = InputMode::Unknown;
    std::chrono::high_resolution_clock::time_point lastUpdated;
    bool hasInputMode(InputMode want = InputMode::Unknown,
                      bool changeMode = true);

    void queueGamepadAxisInputIfNeeded(int gamepad);

    public:
    WindowCallbacks(GameWindow &window, JniSupport &jniSupport,
                    FakeInputQueue &inputQueue);

    static void loadGamepadMappings();

    void registerCallbacks();

    void markRequeueGamepadInput() { needsQueueGamepadInput = true; }

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
    void onKeyboardText(std::string const &c);
    void onPaste(std::string const &str);
    void onGamepadState(int gamepad, bool connected);
    void onGamepadButton(int gamepad, GamepadButtonId btn, bool pressed);
    void onGamepadAxis(int gamepad, GamepadAxisId ax, float value);

    static int mapMinecraftToAndroidKey(KeyCode code);
    static int mapGamepadToAndroidKey(GamepadButtonId btn);
};
