#include "fake_window.h"
#include "settings.h"
#include <game_window.h>

void FakeWindow::initHybrisHooks(std::unordered_map<std::string, void*>& syms) {
    syms["ANativeWindow_getWidth"] = (void*)+[](void* window) -> int32_t {
        int width, height;
        ((GameWindow*)window)->getWindowSize(width, height);
        return width;
    };
    syms["ANativeWindow_getHeight"] = (void*)+[](void* window) -> int32_t {
        int width, height;
        ((GameWindow*)window)->getWindowSize(width, height);
        return height - Settings::menubarsize;
    };
}
