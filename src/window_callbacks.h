#pragma once

class MinecraftGame;
class LauncherAppPlatform;
class GameWindow;

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

};