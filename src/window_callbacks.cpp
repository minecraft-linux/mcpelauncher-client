#include "window_callbacks.h"
#include "minecraft_gamepad_mapping.h"
#include "client_app_platform.h"
#include "minecraft_game_wrapper.h"

#include <mcpelauncher/minecraft_version.h>
#include <minecraft/Mouse.h>
#include <minecraft/Multitouch.h>
#include <minecraft/Keyboard.h>
#include <minecraft/Options.h>
#include <minecraft/GameControllerManager.h>
#include <minecraft/legacy/App.h>
#include <minecraft/legacy/MinecraftGame.h>
#include <minecraft/legacy/Keyboard.h>
#include <game_window_manager.h>

void WindowCallbacks::registerCallbacks() {
    using namespace std::placeholders;
    window.setWindowSizeCallback(std::bind(&WindowCallbacks::onWindowSizeCallback, this, _1, _2));
    window.setDrawCallback(std::bind(&WindowCallbacks::onDraw, this));
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

void WindowCallbacks::handleInitialWindowSize() {
    int windowWidth, windowHeight;
    window.getWindowSize(windowWidth, windowHeight);
    onWindowSizeCallback(windowWidth, windowHeight);

    if (MinecraftVersion::isAtLeast(0, 13) && game.getPrimaryUserOptions()->getFullscreen())
        window.setFullscreen(true);
}

void WindowCallbacks::onWindowSizeCallback(int w, int h) {
    game.setRenderingSize(w, h);
    game.setUISizeAndScale(w, h, pixelScale);
}

void WindowCallbacks::onDraw() {
    if (game.wantToQuit()) {
        window.close();
        return;
    }

    appPlatform.runMainThreadTasks();
    appPlatform.update();
    game.update();
}

void WindowCallbacks::onClose() {
    game.quit("linux launcher", "window close");
}

void WindowCallbacks::onMouseButton(double x, double y, int btn, MouseButtonAction action) {
    if (btn < 1 || btn > 3)
        return;
    Mouse::feed((char) btn, (char) (action == MouseButtonAction::PRESS ? 1 : 0), (short) x, (short) y, 0, 0);
}
void WindowCallbacks::onMousePosition(double x, double y) {
    Mouse::feed(0, 0, (short) x, (short) y, 0, 0);
}
void WindowCallbacks::onMouseRelativePosition(double x, double y) {
    Mouse::feed(0, 0, 0, 0, (short) x, (short) y);
}
void WindowCallbacks::onMouseScroll(double x, double y, double dx, double dy) {
    char cdy = (char) std::max(std::min(dy * 127.0, 127.0), -127.0);
    Mouse::feed(4, cdy, 0, 0, (short) x, (short) y);
}
void WindowCallbacks::onTouchStart(int id, double x, double y) {
    Multitouch::feed(1, 1, (short) x, (short) y, id);
}
void WindowCallbacks::onTouchUpdate(int id, double x, double y) {
    Multitouch::feed(0, 0, (short) x, (short) y, id);
}
void WindowCallbacks::onTouchEnd(int id, double x, double y) {
    Multitouch::feed(1, 0, (short) x, (short) y, id);
}
void WindowCallbacks::onKeyboard(int key, KeyAction action) {
#ifdef __APPLE__
    if (key == 1) /* cmd */
#else
    if (key == 17)
#endif
        modCTRL = (action != KeyAction::RELEASE);
    if (key == 16)
        appPlatform.onKeyboardShiftKey(action != KeyAction::RELEASE);
    if (modCTRL && key == 'C') {
        appPlatform.copyCurrentText();
        return;
    }
    if (action == KeyAction::PRESS || action == KeyAction::REPEAT) {
        if (key == 37)
            appPlatform.onKeyboardDirectionKey(ClientAppPlatform::DirectionKey::LeftKey);
        else if (key == 39)
            appPlatform.onKeyboardDirectionKey(ClientAppPlatform::DirectionKey::RightKey);
        else if (key == 36)
            appPlatform.onKeyboardDirectionKey(ClientAppPlatform::DirectionKey::HomeKey);
        else if (key == 35)
            appPlatform.onKeyboardDirectionKey(ClientAppPlatform::DirectionKey::EndKey);
    }
    if (key == 112 + 10 && action == KeyAction::PRESS && MinecraftVersion::isAtLeast(0, 13))
        game.getPrimaryUserOptions()->setFullscreen(!game.getPrimaryUserOptions()->getFullscreen());
    if ((action == KeyAction::PRESS || action == KeyAction::RELEASE) && key < 256) {
        Keyboard::InputEvent evData;
        evData.key = (unsigned int) key;
        evData.event = (action == KeyAction::PRESS ? 1 : 0);
        if (MinecraftVersion::isAtLeast(1, 2)) {
            evData.controllerId = *Keyboard::_gameControllerId;
            Keyboard::_inputs->push_back(evData);
        } else {
            Legacy::Pre_1_2::Keyboard::_inputs->push_back(evData);
        }
        Keyboard::_states[key] = evData.event;
    }

}
void WindowCallbacks::onKeyboardText(std::string const& c) {
    if ((!appPlatform.isKeyboardMultiline() && (c.size() == 1 && c[0] == '\n')) || !appPlatform.isKeyboardVisible()) {
        if (MinecraftVersion::isAtLeast(0, 17))
            Keyboard::feedText(c, false, 0);
        else
            Legacy::Pre_0_17::Keyboard::feedText(c, false);
    } else {
        appPlatform.onKeyboardText(game, c);
    }
}
void WindowCallbacks::onPaste(std::string const& str) {
    appPlatform.onKeyboardText(game, str);
}
void WindowCallbacks::onGamepadState(int gamepad, bool connected) {
    Log::trace("WindowCallbacks", "Gamepad %s #%i", connected ? "connected" : "disconnected", gamepad);
    if (connected)
        gamepads.insert({gamepad, GamepadData()});
    else
        gamepads.erase(gamepad);
    if (GameControllerManager::sGamePadManager != nullptr)
        GameControllerManager::sGamePadManager->setGameControllerConnected(gamepad, connected);
}

void WindowCallbacks::onGamepadButton(int gamepad, GamepadButtonId btn, bool pressed) {
    int mid = MinecraftGamepadMapping::mapButton(btn);
    auto state = pressed ? GameControllerButtonState::PRESSED : GameControllerButtonState::RELEASED;
    if (GameControllerManager::sGamePadManager != nullptr && mid != -1) {
        GameControllerManager::sGamePadManager->feedButton(gamepad, mid, state, true);
        if (btn == GamepadButtonId::START && pressed)
            GameControllerManager::sGamePadManager->feedJoinGame(gamepad, true);
    }
}

void WindowCallbacks::onGamepadAxis(int gamepad, GamepadAxisId ax, float value) {
    auto gpi = gamepads.find(gamepad);
    if (gpi == gamepads.end())
        return;
    auto& gp = gpi->second;

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