#pragma once

#include <game_window.h>
#include <properties/property_list.h>
#include <properties/property.h>

struct LauncherOptions {
    int windowWidth, windowHeight;
    bool useStdinImport;
    bool emulateTouch;
    GraphicsApi graphicsApi;
    std::string importFilePath;
    std::string sendUri;
};
extern LauncherOptions options;
