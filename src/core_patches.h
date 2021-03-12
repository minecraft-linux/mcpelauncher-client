#pragma once

#include <memory>
#include <game_window.h>

class CorePatches {

private:
    static std::shared_ptr<GameWindow> currentGameWindow;

public:
    static void showMousePointer();

    static void hideMousePointer();

    static void setGameWindow(std::shared_ptr<GameWindow> gameWindow);

};
