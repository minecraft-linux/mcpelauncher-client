#pragma once
#include <string>

struct Settings {
    static int menubarsize;
    static std::string clipboard;
    static bool enable_keyboard_tab_patches_1_20_60;
    static bool enable_keyboard_autofocus_patches_1_20_60;
    static std::string videoMode;

    static void load();
    static void save();
};
