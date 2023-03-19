#pragma once

#include <vector>

struct Mouse {
    static void (*feed)(char, char, short, short, short, short);
};

struct Keyboard {
    static bool useLegacyKeyboard;
    struct InputEvent {
        int event;
        unsigned int key;  // it's actually an unsigned char, but the asm code does suspicious stuff with the padding so use an int so it gets zeroed out
        int controllerId;
        int modShift, modCtrl, modAlt;
    };
    struct LegacyInputEvent {
        int event;
        unsigned int key;  // it's actually an unsigned char, but the asm code does suspicious stuff with the padding so use an int so it gets zeroed out
        int controllerId;
    };

    static int* _states;
    static std::vector<Keyboard::InputEvent>* _inputs;
    static std::vector<Keyboard::LegacyInputEvent>* _inputsLegacy;
    static int* _gameControllerId;
};

struct SymbolsHelper {
    static void initSymbols(void* handle);
};
