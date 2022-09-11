#pragma once

#include <game_window.h>
#include <memory>

class CorePatches
{
   private:
    static std::shared_ptr<GameWindow> currentGameWindow;

   public:
    static void install(void *handle);

    static void showMousePointer();

    static void hideMousePointer();

    static void setGameWindow(std::shared_ptr<GameWindow> gameWindow);
};
