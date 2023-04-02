#pragma once

#include <game_window.h>

struct LauncherOptions {
    int windowWidth, windowHeight;
    GraphicsApi graphicsApi;
    std::string importFilePath;
};
extern LauncherOptions options;
