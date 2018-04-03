#include "window_callbacks.h"

#include <minecraft/MinecraftGame.h>
#include <mcpelauncher/app_platform.h>
#include <game_window.h>

void WindowCallbacks::registerCallbacks() {
    using namespace std::placeholders;
    window.setDrawCallback(std::bind(&WindowCallbacks::onDraw, this));
}

void WindowCallbacks::onDraw() {
    if (game.wantToQuit()) {
        window.close();
        return;
    }

    appPlatform.runMainThreadTasks();
    game.update();
}