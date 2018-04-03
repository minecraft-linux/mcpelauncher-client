#pragma once

#include <game_window.h>

class MinecraftGame;
class LauncherAppPlatform;

class WindowCallbacks {

private:
    MinecraftGame& game;
    LauncherAppPlatform& appPlatform;
    GameWindow& window;
    float pixelScale = 2.f;

public:
    WindowCallbacks(MinecraftGame& game, LauncherAppPlatform& appPlatform, GameWindow& window) :
            game(game), appPlatform(appPlatform), window(window) { }

    void setPixelScale(float pixelScale) {
        this->pixelScale = pixelScale;
    }

    void registerCallbacks();

    void handleInitialWindowSize();

    void onWindowSizeCallback(int w, int h);

    void onDraw();

    void onClose();

    void onMouseButton(double x, double y, int btn, MouseButtonAction action);
    void onMousePosition(double x, double y);
    void onMouseRelativePosition(double x, double y);
    void onMouseScroll(double x, double y, double dx, double dy);
    void onKeyboard(int key, KeyAction action);
    void onKeyboardText(std::string const& c);
    void onPaste(std::string const& str);

};