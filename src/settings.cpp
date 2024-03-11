#include "settings.h"
#include <properties/property_list.h>
#include <properties/property.h>
#include <mcpelauncher/path_helper.h>
#include <fstream>

int Settings::menubarsize = 0;
std::string Settings::clipboard;
bool Settings::enable_keyboard_autofocus_patches_1_20_60 = false;
bool Settings::enable_keyboard_tab_patches_1_20_60 = false;
bool Settings::enable_menubar = true;
int Settings::fps_hud_location;
std::string Settings::videoMode;

static properties::property_list settings('=');
static properties::property<bool> enable_keyboard_autofocus_patches_1_20_60(settings, "enable_keyboard_autofocus_patches_1_20_60", /* default if not defined*/ false);
static properties::property<bool> enable_keyboard_tab_patches_1_20_60(settings, "enable_keyboard_tab_patches_1_20_60", /* default if not defined*/ false);
static properties::property<bool> enable_menubar(settings, "enable_menubar", /* default if not defined*/ true);
static properties::property<int> fps_hud_location(settings, "fps_hud_location", /* default if not defined*/ -1);
static properties::property<std::string> videoMode(settings, "videoMode", "");

void Settings::load() {
    std::ifstream propertiesFile(PathHelper::getPrimaryDataDirectory() + "mcpelauncher-client-settings.txt");
    if (propertiesFile) {
        settings.load(propertiesFile);
    }
    Settings::enable_keyboard_autofocus_patches_1_20_60 = ::enable_keyboard_autofocus_patches_1_20_60.get();
    Settings::enable_keyboard_tab_patches_1_20_60 = ::enable_keyboard_tab_patches_1_20_60.get();
    Settings::enable_menubar = ::enable_menubar.get();
    Settings::fps_hud_location = ::fps_hud_location.get();
    Settings::videoMode = ::videoMode.get();
}

void Settings::save() {
    ::enable_keyboard_autofocus_patches_1_20_60.set(Settings::enable_keyboard_autofocus_patches_1_20_60);
    ::enable_keyboard_tab_patches_1_20_60.set(Settings::enable_keyboard_tab_patches_1_20_60);
    ::enable_menubar.set(Settings::enable_menubar);
    ::fps_hud_location.set(Settings::fps_hud_location);
    ::videoMode.set(Settings::videoMode);
    std::ofstream propertiesFile(PathHelper::getPrimaryDataDirectory() + "mcpelauncher-client-settings.txt");
    if (propertiesFile) {
        settings.save(propertiesFile);
    }
}
