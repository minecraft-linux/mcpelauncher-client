#include "core_patches.h"

std::shared_ptr<GameWindow> CorePatches::currentGameWindow;

void CorePatches::showMousePointer() {
    currentGameWindow->setCursorDisabled(false);
}

void CorePatches::hideMousePointer() {
    currentGameWindow->setCursorDisabled(true);
}

void CorePatches::setGameWindow(std::shared_ptr<GameWindow> gameWindow) {
    currentGameWindow = gameWindow;
}
