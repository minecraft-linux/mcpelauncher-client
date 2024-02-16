#include "settings.h"
#include <properties/property_list.h>
#include <properties/property.h>
#include <mcpelauncher/path_helper.h>
#include <fstream>

int Settings::menubarsize = 0;
std::string Settings::clipboard;
bool Settings::enable_keyboard_autofocus_patches_1_20_60 = false;
bool Settings::enable_keyboard_tab_patches_1_20_60 = false;
std::string Settings::videoMode;

static properties::property_list settings('=');
static properties::property<bool> enable_keyboard_autofocus_patches_1_20_60(settings, "enable_keyboard_autofocus_patches_1_20_60", /* default if not defined*/ false);
static properties::property<bool> enable_keyboard_tab_patches_1_20_60(settings, "enable_keyboard_tab_patches_1_20_60", /* default if not defined*/ false);
static properties::property<std::string> videoMode(settings, "videoMode", "");

void Settings::load() {
    std::ifstream propertiesFile(PathHelper::getPrimaryDataDirectory() + "mcpelauncher-client-settings.txt");
    if (propertiesFile) {
        settings.load(propertiesFile);
    }
    Settings::enable_keyboard_autofocus_patches_1_20_60 = ::enable_keyboard_autofocus_patches_1_20_60.get();
    Settings::enable_keyboard_tab_patches_1_20_60 = ::enable_keyboard_tab_patches_1_20_60.get();
    Settings::videoMode = ::videoMode.get();
}

void Settings::save() {
    ::enable_keyboard_autofocus_patches_1_20_60.set(Settings::enable_keyboard_autofocus_patches_1_20_60);
    ::enable_keyboard_tab_patches_1_20_60.set(Settings::enable_keyboard_tab_patches_1_20_60);
    ::videoMode.set(Settings::videoMode);
    std::ofstream propertiesFile(PathHelper::getPrimaryDataDirectory() + "mcpelauncher-client-settings.txt");
    if (propertiesFile) {
        settings.save(propertiesFile);
    }
}
