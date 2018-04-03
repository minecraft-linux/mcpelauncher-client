#include "window_callbacks.h"

#include <mcpelauncher/app_platform.h>
#include <minecraft/MinecraftGame.h>
#include <minecraft/Mouse.h>
#include <minecraft/Keyboard.h>
#include <minecraft/Options.h>

void WindowCallbacks::registerCallbacks() {
    using namespace std::placeholders;
    window.setWindowSizeCallback(std::bind(&WindowCallbacks::onWindowSizeCallback, this, _1, _2));
    window.setDrawCallback(std::bind(&WindowCallbacks::onDraw, this));
    window.setCloseCallback(std::bind(&WindowCallbacks::onClose, this));

    window.setMouseButtonCallback(std::bind(&WindowCallbacks::onMouseButton, this, _1, _2, _3, _4));
    window.setMousePositionCallback(std::bind(&WindowCallbacks::onMousePosition, this, _1, _2));
    window.setMouseRelativePositionCallback(std::bind(&WindowCallbacks::onMouseRelativePosition, this, _1, _2));
    window.setMouseScrollCallback(std::bind(&WindowCallbacks::onMouseScroll, this, _1, _2, _3, _4));
    window.setKeyboardCallback(std::bind(&WindowCallbacks::onKeyboard, this, _1, _2));
    window.setKeyboardTextCallback(std::bind(&WindowCallbacks::onKeyboardText, this, _1));
    window.setPasteCallback(std::bind(&WindowCallbacks::onPaste, this, _1));
}

void WindowCallbacks::handleInitialWindowSize() {
    int windowWidth, windowHeight;
    window.getWindowSize(windowWidth, windowHeight);
    onWindowSizeCallback(windowWidth, windowHeight);
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
    game.update();
}

void WindowCallbacks::onClose() {
    game.quit();
}

void WindowCallbacks::onMouseButton(double x, double y, int btn, MouseButtonAction action) {
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
void WindowCallbacks::onKeyboard(int key, KeyAction action) {
    if (key == 112 + 10 && action == KeyAction::PRESS)
        game.getPrimaryUserOptions()->setFullscreen(!game.getPrimaryUserOptions()->getFullscreen());
    if (action == KeyAction::PRESS)
        Keyboard::feed((unsigned char) key, 1);
    else if (action == KeyAction::RELEASE)
        Keyboard::feed((unsigned char) key, 0);

}
void WindowCallbacks::onKeyboardText(std::string const& c) {
    Keyboard::feedText(c, false, 0);
}
void WindowCallbacks::onPaste(std::string const& str) {
    for (int i = 0; i < str.length(); ) {
        char c = str[i];
        int l = 1;
        if ((c & 0b11110000) == 0b11100000)
            l = 3;
        else if ((c & 0b11100000) == 0b11000000)
            l = 2;
        Keyboard::feedText(mcpe::string(&str[i], (size_t) l), false, 0);
        i += l;
    }
}