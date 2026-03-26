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
        return height - Settings::menubarsize.load();
    };
    syms["ANativeWindow_setBuffersGeometry"] = (void*)+[](void* window, int32_t width, int32_t height, int32_t format) -> int32_t {
        return 0; // success
    };
    syms["ANativeWindow_getFormat"] = (void*)+[](void* window) -> int32_t {
        return 1; // WINDOW_FORMAT_RGBA_8888
    };
    syms["ANativeWindow_acquire"] = (void*)+[](void* window) {};
    syms["ANativeWindow_release"] = (void*)+[](void* window) {};
}
