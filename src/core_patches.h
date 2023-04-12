#pragma once

#include <memory>
#include <game_window.h>
#include "window_callbacks.h"

class CorePatches {
private:
    static std::shared_ptr<GameWindow> currentGameWindow;

    static std::shared_ptr<WindowCallbacks> currentGameWindowCallbacks;

public:
    static void install(void *handle);

    static void showMousePointer();

    static void hideMousePointer();

    static void setFullscreen(void* t, bool fullscreen);

    static void setGameWindow(std::shared_ptr<GameWindow> gameWindow);

    static void setGameWindowCallbacks(std::shared_ptr<WindowCallbacks> gameWindowCallbacks);
};
