#include "window_callbacks.h"
#include "symbols.h"

#include <mcpelauncher/minecraft_version.h>
#include <game_window_manager.h>
#include <log.h>
#include <mcpelauncher/path_helper.h>

WindowCallbacks::WindowCallbacks(GameWindow& window, JniSupport& jniSupport, FakeInputQueue& inputQueue) : window(window), jniSupport(jniSupport), inputQueue(inputQueue) {
    useDirectMouseInput = true;
    useDirectKeyboardInput = (Keyboard::_states && (Keyboard::_inputs || Keyboard::_inputsLegacy) && Keyboard::_gameControllerId);
    if (fullscreen) {
        window.setFullscreen(true);
    }
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

void WindowCallbacks::startSendEvents() {
    if(!sendEvents) {
        sendEvents = true;
        for(auto && gp : gamepads) {
            jniSupport.setGameControllerConnected(gp.first, true);
        }
    }
}

void WindowCallbacks::onWindowSizeCallback(int w, int h) {
    jniSupport.onWindowResized(w, h);
}

void WindowCallbacks::onClose() {
    jniSupport.onWindowClosed();
}

void WindowCallbacks::setFullscreen(bool isFs) {
    if(fullscreen != isFs) {
        window.setFullscreen(isFs);
        fullscreen = isFs;
    }
}

bool WindowCallbacks::hasInputMode(WindowCallbacks::InputMode want, bool changeMode) {
    if(!sendEvents) {
        return false;
    }
    auto now = std::chrono::high_resolution_clock::now();
    if(inputMode == want || (changeMode && ((int)want < (int)inputMode || (now - lastUpdated) > std::chrono::seconds(2)))) {
        if(inputMode != want) {
#ifndef NDEBUG
            printf("Input Mode changed to %d\n", (int)want);
#endif
            if(want == InputMode::Mouse) {
                window.setCursorDisabled(false);
            } else {
                window.setCursorDisabled(true);
            }
        }
        inputMode = want;
        lastUpdated = now;
        return true;
    }
    return false;
}

void WindowCallbacks::onMouseButton(double x, double y, int btn, MouseButtonAction action) {
    if(hasInputMode(InputMode::Mouse)) {
        if(btn < 1)
            return;
        if(btn > 3) {
            // Seems to get recognized same as regular Mousebuttons as Button4 or higher, but ignored from mouse
            return onKeyboard((KeyCode)btn, action == MouseButtonAction::PRESS ? KeyAction::PRESS : KeyAction::RELEASE);
        }
        if(useDirectMouseInput)
            Mouse::feed((char)btn, (char)(action == MouseButtonAction::PRESS ? 1 : 0), (short)x, (short)y, 0, 0);
        else if(action == MouseButtonAction::PRESS)
            inputQueue.addEvent(FakeMotionEvent(AMOTION_EVENT_ACTION_DOWN, 0, x, y));
        else if(action == MouseButtonAction::RELEASE)
            inputQueue.addEvent(FakeMotionEvent(AMOTION_EVENT_ACTION_UP, 0, x, y));
    }
}
void WindowCallbacks::onMousePosition(double x, double y) {
    if(hasInputMode(InputMode::Mouse)) {
        if(useDirectMouseInput)
            Mouse::feed(0, 0, (short)x, (short)y, 0, 0);
        else
            inputQueue.addEvent(FakeMotionEvent(AMOTION_EVENT_ACTION_MOVE, 0, x, y));
    }
}
void WindowCallbacks::onMouseRelativePosition(double x, double y) {
    if(hasInputMode(InputMode::Mouse, std::abs(x) > 10 || std::abs(y) > 10)) {
        if(useDirectMouseInput)
            Mouse::feed(0, 0, 0, 0, (short)x, (short)y);
        else
            inputQueue.addEvent(FakeMotionEvent(AMOTION_EVENT_ACTION_MOVE, 0, x, y));
    }
}
void WindowCallbacks::onMouseScroll(double x, double y, double dx, double dy) {
    if(hasInputMode(InputMode::Mouse)) {
#ifdef __APPLE__
        signed char cdy = (signed char)std::max(std::min((dx + dy) * 127.0, 127.0), -127.0);
#else
        signed char cdy = (signed char)std::max(std::min(dy * 127.0, 127.0), -127.0);
#endif
        if(useDirectMouseInput)
            Mouse::feed(4, (char&)cdy, 0, 0, (short)x, (short)y);
    }
}
void WindowCallbacks::onTouchStart(int id, double x, double y) {
    if(hasInputMode(InputMode::Touch)) {
        inputQueue.addEvent(FakeMotionEvent(AMOTION_EVENT_ACTION_DOWN, id, x, y));
    }
}
void WindowCallbacks::onTouchUpdate(int id, double x, double y) {
    if(hasInputMode(InputMode::Touch)) {
        inputQueue.addEvent(FakeMotionEvent(AMOTION_EVENT_ACTION_MOVE, id, x, y));
    }
}
void WindowCallbacks::onTouchEnd(int id, double x, double y) {
    if(hasInputMode(InputMode::Touch)) {
        inputQueue.addEvent(FakeMotionEvent(AMOTION_EVENT_ACTION_UP, id, x, y));
    }
}
void WindowCallbacks::onKeyboard(KeyCode key, KeyAction action) {
    if(hasInputMode(InputMode::Mouse)) {
        // return onKeyboard((KeyCode) 4, KeyAction::PRESS);
        // key = (KeyCode) 0x21;
#ifdef __APPLE__
        if(key == KeyCode::LEFT_SUPER || key == KeyCode::RIGHT_SUPER)
#else
        if(key == KeyCode::LEFT_CTRL || key == KeyCode::RIGHT_CTRL)
#endif
            modCTRL = (action != KeyAction::RELEASE);

        if(modCTRL && key == KeyCode::C && jniSupport.getTextInputHandler().getCopyText() != "") {
            window.setClipboardText(jniSupport.getTextInputHandler().getCopyText());
        } else {
            jniSupport.getTextInputHandler().onKeyPressed(key, action);
        }

        if(key == KeyCode::FN11 && action == KeyAction::PRESS)
            setFullscreen(!fullscreen);

        if(useDirectKeyboardInput && (action == KeyAction::PRESS || action == KeyAction::RELEASE)) {
            if (Keyboard::useLegacyKeyboard) {
                Keyboard::LegacyInputEvent evData{};
                evData.key = (unsigned int)key & 0xff;
                evData.event = (action == KeyAction::PRESS ? 1 : 0);
                evData.controllerId = *Keyboard::_gameControllerId;
                Keyboard::_inputsLegacy->push_back(evData);
                Keyboard::_states[(int)key & 0xff] = evData.event;
            } else {
                Keyboard::InputEvent evData{};
                evData.modShift = Keyboard::_states[16];
                evData.modCtrl = Keyboard::_states[17];
                evData.modAlt = Keyboard::_states[18];
                evData.key = (unsigned int)key & 0xff;
                evData.event = (action == KeyAction::PRESS ? 1 : 0);
                evData.controllerId = *Keyboard::_gameControllerId;
                Keyboard::_inputs->push_back(evData);
                Keyboard::_states[(int)key & 0xff] = evData.event;
            }
            return;
        }

        if(action == KeyAction::PRESS)
            inputQueue.addEvent(FakeKeyEvent(AKEY_EVENT_ACTION_DOWN, mapMinecraftToAndroidKey(key)));
        else if(action == KeyAction::RELEASE)
            inputQueue.addEvent(FakeKeyEvent(AKEY_EVENT_ACTION_UP, mapMinecraftToAndroidKey(key)));
    }
}
void WindowCallbacks::onKeyboardText(std::string const& c) {
    if(c == "\n" && !jniSupport.getTextInputHandler().isMultiline())
        jniSupport.onReturnKeyPressed();
    else
        jniSupport.getTextInputHandler().onTextInput(c);
}
void WindowCallbacks::onPaste(std::string const& str) {
    jniSupport.getTextInputHandler().onTextInput(str);
}
void WindowCallbacks::onGamepadState(int gamepad, bool connected) {
    Log::trace("WindowCallbacks", "Gamepad %s #%i", connected ? "connected" : "disconnected", gamepad);
    if(connected)
        gamepads.insert({gamepad, GamepadData()});
    else
        gamepads.erase(gamepad);

    if(sendEvents) {
        // This crashs the game 1.16.210+ during init, but works after loading
        // We block sendEvents before the game starts polling the looper, to avoid the crash
        // 1.19.60+ requires calling this method, otherwise the game ignores the gamepad input
        jniSupport.setGameControllerConnected(gamepad, connected);
    }
}

void WindowCallbacks::queueGamepadAxisInputIfNeeded(int gamepad) {
    if(!needsQueueGamepadInput)
        return;
    inputQueue.addEvent(FakeMotionEvent(AINPUT_SOURCE_GAMEPAD, gamepad, AMOTION_EVENT_ACTION_MOVE, 0, 0.f, 0.f,
                                        [this, gamepad](int axis) {
                                            auto gpi = gamepads.find(gamepad);
                                            if(gpi == gamepads.end())
                                                return 0.f;
                                            auto& gp = gpi->second;
                                            if(axis == AMOTION_EVENT_AXIS_X)
                                                return gp.axis[(int)GamepadAxisId::LEFT_X];
                                            if(axis == AMOTION_EVENT_AXIS_Y)
                                                return gp.axis[(int)GamepadAxisId::LEFT_Y];
                                            if(axis == AMOTION_EVENT_AXIS_RX)
                                                return gp.axis[(int)GamepadAxisId::RIGHT_X];
                                            if(axis == AMOTION_EVENT_AXIS_RY)
                                                return gp.axis[(int)GamepadAxisId::RIGHT_Y];
                                            if(axis == AMOTION_EVENT_AXIS_BRAKE)
                                                return gp.axis[(int)GamepadAxisId::LEFT_TRIGGER];
                                            if(axis == AMOTION_EVENT_AXIS_GAS)
                                                return gp.axis[(int)GamepadAxisId::RIGHT_TRIGGER];
                                            if(axis == AMOTION_EVENT_AXIS_HAT_X) {
                                                if(gp.button[(int)GamepadButtonId::DPAD_LEFT])
                                                    return -1.f;
                                                if(gp.button[(int)GamepadButtonId::DPAD_RIGHT])
                                                    return 1.f;
                                                return 0.f;
                                            }
                                            if(axis == AMOTION_EVENT_AXIS_HAT_Y) {
                                                if(gp.button[(int)GamepadButtonId::DPAD_UP])
                                                    return -1.f;
                                                if(gp.button[(int)GamepadButtonId::DPAD_DOWN])
                                                    return 1.f;
                                                return 0.f;
                                            }
                                            return 0.f;
                                        }));
    needsQueueGamepadInput = false;
}

void WindowCallbacks::onGamepadButton(int gamepad, GamepadButtonId btn, bool pressed) {
    if(hasInputMode(InputMode::Gamepad)) {
        auto gpi = gamepads.find(gamepad);
        if(gpi == gamepads.end())
            return;
        auto& gp = gpi->second;
        if((int)btn < 0 || (int)btn >= 15)
            throw std::runtime_error("bad button id");
        if(gp.button[(int)btn] == pressed)
            return;
        gp.button[(int)btn] = pressed;

        if(btn == GamepadButtonId::DPAD_UP || btn == GamepadButtonId::DPAD_DOWN || btn == GamepadButtonId::DPAD_LEFT || btn == GamepadButtonId::DPAD_RIGHT) {
            queueGamepadAxisInputIfNeeded(gamepad);
            return;
        }

        if(pressed)
            inputQueue.addEvent(FakeKeyEvent(AINPUT_SOURCE_GAMEPAD, gamepad, AKEY_EVENT_ACTION_DOWN, mapGamepadToAndroidKey(btn)));
        else
            inputQueue.addEvent(FakeKeyEvent(AINPUT_SOURCE_GAMEPAD, gamepad, AKEY_EVENT_ACTION_UP, mapGamepadToAndroidKey(btn)));
    }
}

void WindowCallbacks::onGamepadAxis(int gamepad, GamepadAxisId ax, float value) {
    if(hasInputMode(InputMode::Gamepad, std::abs(value) > 0.4f)) {
        auto gpi = gamepads.find(gamepad);
        if(gpi == gamepads.end())
            return;
        auto& gp = gpi->second;
        if((int)ax < 0 || (int)ax >= 6)
            throw std::runtime_error("bad axis id");
        gp.axis[(int)ax] = value;
        queueGamepadAxisInputIfNeeded(gamepad);
    }
}

void WindowCallbacks::loadGamepadMappings() {
    auto windowManager = GameWindowManager::getManager();
    std::vector<std::string> controllerDbPaths;
    PathHelper::findAllDataFiles("gamecontrollerdb.txt", [&controllerDbPaths](std::string const& path) {
        controllerDbPaths.push_back(path);
    });
    // Bugfix: allow users to change internal gamepad layouts
    std::reverse(controllerDbPaths.begin(), controllerDbPaths.end());
    for(std::string const& path : controllerDbPaths) {
        Log::trace("Launcher", "Loading gamepad mappings: %s", path.c_str());
        windowManager->addGamepadMappingFile(path);
    }
}

WindowCallbacks::GamepadData::GamepadData() {
    for(int i = 0; i < 6; i++)
        axis[i] = 0.f;
    memset(button, 0, sizeof(button));
}

int WindowCallbacks::mapMinecraftToAndroidKey(KeyCode code) {
    if(code >= KeyCode::NUM_0 && code <= KeyCode::NUM_9)
        return (int)code - (int)KeyCode::NUM_0 + AKEYCODE_0;
    if(code >= KeyCode::A && code <= KeyCode::Z)
        return (int)code - (int)KeyCode::A + AKEYCODE_A;
    if(code >= KeyCode::FN1 && code <= KeyCode::FN12)
        return (int)code - (int)KeyCode::FN1 + AKEYCODE_F1;
    switch(code) {
    case KeyCode::BACK:
        return AKEYCODE_BACK;
    case KeyCode::BACKSPACE:
        return AKEYCODE_DEL;
    case KeyCode::TAB:
        return AKEYCODE_TAB;
    case KeyCode::ENTER:
        return AKEYCODE_ENTER;
    case KeyCode::LEFT_SHIFT:
        return AKEYCODE_SHIFT_LEFT;
    case KeyCode::RIGHT_SHIFT:
        return AKEYCODE_SHIFT_RIGHT;
    case KeyCode::LEFT_CTRL:
        return AKEYCODE_CTRL_LEFT;
    case KeyCode::RIGHT_CTRL:
        return AKEYCODE_CTRL_RIGHT;
    case KeyCode::PAUSE:
        return AKEYCODE_BREAK;
    case KeyCode::CAPS_LOCK:
        return AKEYCODE_CAPS_LOCK;
    case KeyCode::ESCAPE:
        return AKEYCODE_ESCAPE;
    case KeyCode::SPACE:
        return AKEYCODE_SPACE;
    case KeyCode::PAGE_UP:
        return AKEYCODE_PAGE_UP;
    case KeyCode::PAGE_DOWN:
        return AKEYCODE_PAGE_DOWN;
    case KeyCode::END:
        return AKEYCODE_MOVE_END;
    case KeyCode::HOME:
        return AKEYCODE_MOVE_HOME;
    case KeyCode::LEFT:
        return AKEYCODE_DPAD_LEFT;
    case KeyCode::UP:
        return AKEYCODE_DPAD_UP;
    case KeyCode::RIGHT:
        return AKEYCODE_DPAD_RIGHT;
    case KeyCode::DOWN:
        return AKEYCODE_DPAD_DOWN;
    case KeyCode::INSERT:
        return AKEYCODE_INSERT;
    case KeyCode::DELETE:
        return AKEYCODE_FORWARD_DEL;
    case KeyCode::NUM_LOCK:
        return AKEYCODE_NUM_LOCK;
    case KeyCode::SCROLL_LOCK:
        return AKEYCODE_SCROLL_LOCK;
    case KeyCode::SEMICOLON:
        return AKEYCODE_SEMICOLON;
    case KeyCode::EQUAL:
        return AKEYCODE_EQUALS;
    case KeyCode::COMMA:
        return AKEYCODE_COMMA;
    case KeyCode::MINUS:
        return AKEYCODE_MINUS;
    case KeyCode::PERIOD:
        return AKEYCODE_PERIOD;
    case KeyCode::SLASH:
        return AKEYCODE_SLASH;
    case KeyCode::GRAVE:
        return AKEYCODE_GRAVE;
    case KeyCode::LEFT_BRACKET:
        return AKEYCODE_LEFT_BRACKET;
    case KeyCode::BACKSLASH:
        return AKEYCODE_BACKSLASH;
    case KeyCode::RIGHT_BRACKET:
        return AKEYCODE_RIGHT_BRACKET;
    case KeyCode::APOSTROPHE:
        return AKEYCODE_APOSTROPHE;
    case KeyCode::MENU:
        return AKEYCODE_MENU;
    case KeyCode::LEFT_SUPER:
        return AKEYCODE_META_LEFT;
    case KeyCode::RIGHT_SUPER:
        return AKEYCODE_META_RIGHT;
    case KeyCode::LEFT_ALT:
        return AKEYCODE_ALT_LEFT;
    case KeyCode::RIGHT_ALT:
        return AKEYCODE_ALT_RIGHT;
    default:
        return AKEYCODE_UNKNOWN;
    }
}

int WindowCallbacks::mapGamepadToAndroidKey(GamepadButtonId btn) {
    switch(btn) {
    case GamepadButtonId::A:
        return AKEYCODE_BUTTON_A;
    case GamepadButtonId::B:
        return AKEYCODE_BUTTON_B;
    case GamepadButtonId::X:
        return AKEYCODE_BUTTON_X;
    case GamepadButtonId::Y:
        return AKEYCODE_BUTTON_Y;
    case GamepadButtonId::LB:
        return AKEYCODE_BUTTON_L1;
    case GamepadButtonId::RB:
        return AKEYCODE_BUTTON_R1;
    case GamepadButtonId::BACK:
        return AKEYCODE_BUTTON_SELECT;
    case GamepadButtonId::START:
        return AKEYCODE_BUTTON_START;
    case GamepadButtonId::GUIDE:
        return AKEYCODE_BUTTON_MODE;
    case GamepadButtonId::LEFT_STICK:
        return AKEYCODE_BUTTON_THUMBL;
    case GamepadButtonId::RIGHT_STICK:
        return AKEYCODE_BUTTON_THUMBR;
    case GamepadButtonId::DPAD_UP:
        return AKEYCODE_DPAD_UP;
    case GamepadButtonId::DPAD_RIGHT:
        return AKEYCODE_DPAD_RIGHT;
    case GamepadButtonId::DPAD_DOWN:
        return AKEYCODE_DPAD_DOWN;
    case GamepadButtonId::DPAD_LEFT:
        return AKEYCODE_DPAD_LEFT;
    default:
        return -1;
    }
}
