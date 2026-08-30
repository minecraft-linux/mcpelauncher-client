#pragma once

#include <game_window.h>
#include <unordered_map>
#include "jni/jni_support.h"
#include "fake_inputqueue.h"
#include <chrono>
#include <vector>
#include <mutex>
#include "main.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif
class WindowCallbacks {
private:
    struct GamepadData {
        float axis[6];
        bool button[15];

        GamepadData();
    };
    struct KeyboardInputCallback {
        void* user;
        bool (*callback)(void* user, int keyCode, int action);
    };
    struct MouseButtonCallback {
        void* user;
        bool (*callback)(void* user, double x, double y, int button, int action);
    };
    struct MousePositionCallback {
        void* user;
        bool (*callback)(void* user, double x, double y, bool relative);
    };
    struct MouseScrollCallback {
        void* user;
        bool (*callback)(void* user, double x, double y, double dx, double dy);
    };

    std::vector<KeyboardInputCallback> keyboardCallbacks;
    std::vector<MouseButtonCallback> mouseButtonCallbacks;
    std::vector<MousePositionCallback> mousePositionCallbacks;
    std::vector<MouseScrollCallback> mouseScrollCallbacks;
    std::mutex keyboardCallbacksLock;
    std::mutex mouseButtonCallbacksLock;
    std::mutex mousePositionCallbacksLock;
    std::mutex mouseScrollCallbacksLock;

    GameWindow& window;
    JniSupport& jniSupport;
    FakeInputQueue& inputQueue;
    std::unordered_map<int, GamepadData> gamepads;
    int32_t buttonState = 0;
    KeyCode lastKey = (KeyCode)0;
    size_t lastEnabledNo = 0;
    uint8_t delayedPaste = 0;
    std::string lastPasteStr = "";
    bool useDirectMouseInput, useDirectKeyboardInput;
    bool needsQueueGamepadInput = true;
    bool sendEvents = false;
    bool cursorLocked = false;
    bool imguiTextInput = false;
    int menubarsize = 0;
    enum class InputMode {
        Touch,
        Mouse,
        Gamepad,
        Unknown,
    };
    int imGuiTouchId = -1;
    bool useRawInput = false;
    InputMode inputMode = InputMode::Unknown;
    InputMode forcedMode = InputMode::Unknown;
    int inputModeSwitchDelay = 100;
    std::chrono::high_resolution_clock::time_point lastUpdated;
    bool hasInputMode(InputMode want = InputMode::Unknown, bool changeMode = true);

    void queueGamepadAxisInputIfNeeded(int gamepad);

    void sendMouseEvent(int32_t source, int32_t deviceId, int32_t action, int32_t buttonState, float x, float y, float scrollY);

    void sendTouchEvent(int32_t pointerId, int32_t action, float x, float y);

public:
    WindowCallbacks(GameWindow& window, JniSupport& jniSupport, FakeInputQueue& inputQueue);

    static void loadGamepadMappings();

    void registerCallbacks();

    void startSendEvents();

    void markRequeueGamepadInput() { needsQueueGamepadInput = true; }

    void onWindowSizeCallback(int w, int h);

    void setCursorLocked(bool locked);

    void onClose();

    void setFullscreen(bool isFs);

    InputMode getInputMode();

    void onMouseButton(double x, double y, int btn, MouseButtonAction action);
    void onMousePosition(double x, double y);
    void onMouseRelativePosition(double x, double y);
    void onMouseScroll(double x, double y, double dx, double dy);
    void onTouchStart(int id, double x, double y);
    void onTouchUpdate(int id, double x, double y);
    void onTouchEnd(int id, double x, double y);
    void onKeyboard(KeyCode key, KeyAction action, int mods);
    void onKeyboardText(std::string const& c);
    void onDrop(std::string const& path);
    void onPaste(std::string const& str);
    void onGamepadState(int gamepad, bool connected);
    void onGamepadButton(int gamepad, GamepadButtonId btn, bool pressed);
    void onGamepadAxis(int gamepad, GamepadAxisId ax, float value);

    void addKeyboardCallback(void* user, bool (*callback)(void* user, int keyCode, int action));
    void addMouseButtonCallback(void* user, bool (*callback)(void* user, double x, double y, int button, int action));
    void addMousePositionCallback(void* user, bool (*callback)(void* user, double x, double y, bool relative));
    void addMouseScrollCallback(void* user, bool (*callback)(void* user, double x, double y, double dx, double dy));

    void setDelayedPaste();

    static int mapMouseButtonToAndroid(int btn);
    static int mapMinecraftToAndroidKey(KeyCode code);
    static int mapGamepadToAndroidKey(GamepadButtonId btn);
};
