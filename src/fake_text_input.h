#pragma once

#include <android/game_activity.h>
#include <functional>
#include <game_window.h>
#include <key_mapping.h>
#include <mcpelauncher/linker.h>
#include <string>
#include <vector>
#include "android/game_text_input.h"

class FakeTextInput {
public:
    using EventCallback = std::function<void(GameActivity*, const GameTextInputState*)>;

    static void initHooks(std::vector<mcpelauncher_hook_t>& hooks);
    static void registerActivity(GameActivity* activity, EventCallback callback);
    static void unregisterActivity(GameActivity* activity);

    static bool isEnabled(GameActivity* activity);
    static size_t getEnabledNo(GameActivity* activity);
    static bool isMultiline(GameActivity* activity);
    static bool consumesKey(GameActivity* activity, KeyCode key, int mods);
    static void onTextInput(GameActivity* activity, const std::string& text);
    static void onKeyPressed(GameActivity* activity, KeyCode key, KeyAction action, int mods);
    static void tryFlushRepeatingState(GameActivity* activity);
    static void flushState(GameTextInput* input);
    static std::string getCopyText(GameActivity* activity);
};
