#include "window_callbacks.h"
#include "minecraft_gamepad_mapping.h"

#include <mcpelauncher/minecraft_version.h>
#include <game_window_manager.h>
#include <log.h>
#include <mcpelauncher/path_helper.h>
#include <hybris/dlfcn.h>

WindowCallbacks::WindowCallbacks(GameWindow &window, JniSupport &jniSupport, FakeInputQueue &inputQueue) :
    window(window), jniSupport(jniSupport), inputQueue(inputQueue) {
    void *lib = hybris_dlopen("libminecraftpe.so", 0);
    (void *&) Mouse_feed = hybris_dlsym(lib, "_ZN5Mouse4feedEccssss");
    hybris_dlclose(lib);
}

void WindowCallbacks::registerCallbacks() {
    using namespace std::placeholders;
    window.setWindowSizeCallback(std::bind(&WindowCallbacks::onWindowSizeCallback, this, _1, _2));
    window.setCloseCallback(std::bind(&WindowCallbacks::onClose, this));

    window.setMouseButtonCallback(std::bind(&WindowCallbacks::onMouseButton, this, _1, _2, _3, _4));
    window.setMousePositionCallback(std::bind(&WindowCallbacks::onMousePosition, this, _1, _2));
    window.setMouseRelativePositionCallback(std::bind(&WindowCallbacks::onMouseRelativePosition, this, _1, _2));
    window.setMouseScrollCallback(std::bind(&WindowCallbacks::onMouseScroll, this, _1, _2, _3, _4));
    window.setTouchStartCallback(std::bind(&WindowCallbacks::onTouchStart, this, _1, _2, _3));
    window.setTouchUpdateCallback(std::bind(&WindowCallbacks::onTouchUpdate, this, _1, _2, _3));
    window.setTouchEndCallback(std::bind(&WindowCallbacks::onTouchEnd, this, _1, _2, _3));
    window.setKeyboardCallback(std::bind(&WindowCallbacks::onKeyboard, this, _1, _2));
    window.setKeyboardTextCallback(std::bind(&WindowCallbacks::onKeyboardText, this, _1));
    window.setPasteCallback(std::bind(&WindowCallbacks::onPaste, this, _1));
    window.setGamepadStateCallback(std::bind(&WindowCallbacks::onGamepadState, this, _1, _2));
    window.setGamepadButtonCallback(std::bind(&WindowCallbacks::onGamepadButton, this, _1, _2, _3));
    window.setGamepadAxisCallback(std::bind(&WindowCallbacks::onGamepadAxis, this, _1, _2, _3));
}

void WindowCallbacks::onWindowSizeCallback(int w, int h) {
    jniSupport.onWindowResized(w, h);
}

void WindowCallbacks::onClose() {
    // TODO: 
}

void WindowCallbacks::onMouseButton(double x, double y, int btn, MouseButtonAction action) {
    if (btn < 1 || btn > 3)
        return;
    Mouse_feed((char) btn, (char) (action == MouseButtonAction::PRESS ? 1 : 0), (short) x, (short) y, 0, 0);
}
void WindowCallbacks::onMousePosition(double x, double y) {
    Mouse_feed(0, 0, (short) x, (short) y, 0, 0);
}
void WindowCallbacks::onMouseRelativePosition(double x, double y) {
    Mouse_feed(0, 0, 0, 0, (short) x, (short) y);
}
void WindowCallbacks::onMouseScroll(double x, double y, double dx, double dy) {
    char cdy = (char) std::max(std::min(dy * 127.0, 127.0), -127.0);
    Mouse_feed(4, cdy, 0, 0, (short) x, (short) y);
}
void WindowCallbacks::onTouchStart(int id, double x, double y) {
//    Multitouch::feed(1, 1, (short) x, (short) y, id);
}
void WindowCallbacks::onTouchUpdate(int id, double x, double y) {
//    Multitouch::feed(0, 0, (short) x, (short) y, id);
}
void WindowCallbacks::onTouchEnd(int id, double x, double y) {
//    Multitouch::feed(1, 0, (short) x, (short) y, id);
}
void WindowCallbacks::onKeyboard(KeyCode key, KeyAction action) {
#ifdef __APPLE__
    if (key == KeyCode::LEFT_SUPER || key == KeyCode::RIGHT_SUPER)
#else
    if (key == KeyCode::LEFT_CTRL || key == KeyCode::RIGHT_CTRL)
#endif
        modCTRL = (action != KeyAction::RELEASE);
    if (action == KeyAction::PRESS)
        inputQueue.addEvent(FakeKeyEvent(AKEY_EVENT_ACTION_DOWN, mapMinecraftToAndroidKey(key)));
    else if (action == KeyAction::RELEASE)
        inputQueue.addEvent(FakeKeyEvent(AKEY_EVENT_ACTION_UP, mapMinecraftToAndroidKey(key)));

}
void WindowCallbacks::onKeyboardText(std::string const& c) {
    // TODO:
}
void WindowCallbacks::onPaste(std::string const& str) {
    // TODO:
}
void WindowCallbacks::onGamepadState(int gamepad, bool connected) {
    Log::trace("WindowCallbacks", "Gamepad %s #%i", connected ? "connected" : "disconnected", gamepad);
    if (connected)
        gamepads.insert({gamepad, GamepadData()});
    else
        gamepads.erase(gamepad);
//    if (GameControllerManager::sGamePadManager != nullptr)
//        GameControllerManager::sGamePadManager->setGameControllerConnected(gamepad, connected);
}

void WindowCallbacks::onGamepadButton(int gamepad, GamepadButtonId btn, bool pressed) {
    int mid = MinecraftGamepadMapping::mapButton(btn);
    /*
    auto state = pressed ? GameControllerButtonState::PRESSED : GameControllerButtonState::RELEASED;
    if (GameControllerManager::sGamePadManager != nullptr && mid != -1) {
        GameControllerManager::sGamePadManager->feedButton(gamepad, mid, state, true);
        if (btn == GamepadButtonId::START && pressed)
            GameControllerManager::sGamePadManager->feedJoinGame(gamepad, true);
    }
     */
}

void WindowCallbacks::onGamepadAxis(int gamepad, GamepadAxisId ax, float value) {
    auto gpi = gamepads.find(gamepad);
    if (gpi == gamepads.end())
        return;
    auto& gp = gpi->second;

    /*
    if (ax == GamepadAxisId::LEFT_X || ax == GamepadAxisId::LEFT_Y) {
        gp.stickLeft[ax == GamepadAxisId::LEFT_Y ? 1 : 0] = value;
        GameControllerManager::sGamePadManager->feedStick(gamepad, 0, (GameControllerStickState) 3, gp.stickLeft[0], -gp.stickLeft[1]);
    } else if (ax == GamepadAxisId::RIGHT_X || ax == GamepadAxisId::RIGHT_Y) {
        gp.stickRight[ax == GamepadAxisId::RIGHT_Y ? 1 : 0] = value;
        GameControllerManager::sGamePadManager->feedStick(gamepad, 1, (GameControllerStickState) 3, gp.stickRight[0], -gp.stickRight[1]);
    } else if (ax == GamepadAxisId::LEFT_TRIGGER) {
        GameControllerManager::sGamePadManager->feedTrigger(gamepad, 0, value);
    } else if (ax == GamepadAxisId::RIGHT_TRIGGER) {
        GameControllerManager::sGamePadManager->feedTrigger(gamepad, 1, value);
    }
     */
}

void WindowCallbacks::loadGamepadMappings() {
    auto windowManager = GameWindowManager::getManager();
    std::vector<std::string> controllerDbPaths;
    PathHelper::findAllDataFiles("gamecontrollerdb.txt", [&controllerDbPaths](std::string const &path) {
        controllerDbPaths.push_back(path);
    });
    for (std::string const& path : controllerDbPaths) {
        Log::trace("Launcher", "Loading gamepad mappings: %s", path.c_str());
        windowManager->addGamepadMappingFile(path);
    }
}

WindowCallbacks::GamepadData::GamepadData() {
    stickLeft[0] = stickLeft[1] = 0.f;
    stickRight[0] = stickRight[1] = 0.f;
}

int WindowCallbacks::mapMinecraftToAndroidKey(KeyCode code) {
    if (code >= KeyCode::NUM_0 && code <= KeyCode::NUM_9)
        return (int) code - (int) KeyCode::NUM_0 + AKEYCODE_0;
    if (code >= KeyCode::A && code <= KeyCode::Z)
        return (int) code - (int) KeyCode::A + AKEYCODE_A;
    if (code >= KeyCode::FN1 && code <= KeyCode::FN12)
        return (int) code - (int) KeyCode::FN1 + AKEYCODE_F1;
    switch (code) {
        case KeyCode::BACK: return AKEYCODE_BACK;
        case KeyCode::BACKSPACE: return AKEYCODE_DEL;
        case KeyCode::TAB: return AKEYCODE_TAB;
        case KeyCode::ENTER: return AKEYCODE_ENTER;
        case KeyCode::LEFT_SHIFT: return AKEYCODE_SHIFT_LEFT;
        case KeyCode::RIGHT_SHIFT: return AKEYCODE_SHIFT_RIGHT;
        case KeyCode::LEFT_CTRL: return AKEYCODE_CTRL_LEFT;
        case KeyCode::RIGHT_CTRL: return AKEYCODE_CTRL_RIGHT;
        case KeyCode::PAUSE: return AKEYCODE_BREAK;
        case KeyCode::CAPS_LOCK: return AKEYCODE_CAPS_LOCK;
        case KeyCode::ESCAPE: return AKEYCODE_ESCAPE;
        case KeyCode::SPACE: return AKEYCODE_SPACE;
        case KeyCode::PAGE_UP: return AKEYCODE_PAGE_UP;
        case KeyCode::PAGE_DOWN: return AKEYCODE_PAGE_DOWN;
        case KeyCode::END: return AKEYCODE_MOVE_END;
        case KeyCode::HOME: return AKEYCODE_MOVE_HOME;
        case KeyCode::LEFT: return AKEYCODE_DPAD_LEFT;
        case KeyCode::UP: return AKEYCODE_DPAD_UP;
        case KeyCode::RIGHT: return AKEYCODE_DPAD_RIGHT;
        case KeyCode::DOWN: return AKEYCODE_DPAD_DOWN;
        case KeyCode::INSERT: return AKEYCODE_INSERT;
        case KeyCode::DELETE: return AKEYCODE_FORWARD_DEL;
        case KeyCode::NUM_LOCK: return AKEYCODE_NUM_LOCK;
        case KeyCode::SCROLL_LOCK: return AKEYCODE_SCROLL_LOCK;
        case KeyCode::SEMICOLON: return AKEYCODE_SEMICOLON;
        case KeyCode::EQUAL: return AKEYCODE_EQUALS;
        case KeyCode::COMMA: return AKEYCODE_COMMA;
        case KeyCode::MINUS: return AKEYCODE_MINUS;
        case KeyCode::PERIOD: return AKEYCODE_PERIOD;
        case KeyCode::SLASH: return AKEYCODE_SLASH;
        case KeyCode::GRAVE: return AKEYCODE_GRAVE;
        case KeyCode::LEFT_BRACKET: return AKEYCODE_LEFT_BRACKET;
        case KeyCode::BACKSLASH: return AKEYCODE_BACKSLASH;
        case KeyCode::RIGHT_BRACKET: return AKEYCODE_RIGHT_BRACKET;
        case KeyCode::APOSTROPHE: return AKEYCODE_APOSTROPHE;
        case KeyCode::MENU: return AKEYCODE_MENU;
        case KeyCode::LEFT_SUPER: return AKEYCODE_META_LEFT;
        case KeyCode::RIGHT_SUPER: return AKEYCODE_META_RIGHT;
        case KeyCode::LEFT_ALT: return AKEYCODE_ALT_LEFT;
        case KeyCode::RIGHT_ALT: return AKEYCODE_ALT_RIGHT;
        default: return AKEYCODE_UNKNOWN;
    }
}