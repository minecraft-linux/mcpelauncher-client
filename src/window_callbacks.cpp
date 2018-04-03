#include "window_callbacks.h"

#include <minecraft/MinecraftGame.h>
#include <mcpelauncher/app_platform.h>
#include <game_window.h>

void WindowCallbacks::registerCallbacks() {
    using namespace std::placeholders;
    window.setWindowSizeCallback(std::bind(&WindowCallbacks::onWindowSizeCallback, this, _1, _2));
    window.setDrawCallback(std::bind(&WindowCallbacks::onDraw, this));
    window.setCloseCallback(std::bind(&WindowCallbacks::onClose, this));
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