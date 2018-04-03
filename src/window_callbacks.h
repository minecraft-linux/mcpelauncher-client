#pragma once

class MinecraftGame;
class LauncherAppPlatform;
class GameWindow;

class WindowCallbacks {

private:
    MinecraftGame& game;
    LauncherAppPlatform& appPlatform;
    GameWindow& window;

public:
    WindowCallbacks(MinecraftGame& game, LauncherAppPlatform& appPlatform, GameWindow& window) :
            game(game), appPlatform(appPlatform), window(window) { }

    void registerCallbacks();

    void onDraw();

};