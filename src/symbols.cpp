#include <mcpelauncher/linker.h>
#include "symbols.h"

void (*Mouse::feed)(char, char, short, short, short, short);

bool Keyboard::useLegacyKeyboard;
int *Keyboard::_states;
std::vector<Keyboard::InputEvent> *Keyboard::_inputs;
std::vector<Keyboard::LegacyInputEvent> *Keyboard::_inputsLegacy;
int *Keyboard::_gameControllerId;

void SymbolsHelper::initSymbols(void *handle) {
    void* MouseFeedSym;
    if (!(MouseFeedSym = linker::dlsym(handle, "_ZN5Mouse4feedEccssss"))) {
        MouseFeedSym = linker::dlsym(handle, "_ZN5Mouse4feedEcassss"); // 1.19.60.26 Beta Mouse::feed ABI changed
    }
    Mouse::feed = (void (*)(char, char, short, short, short, short))MouseFeedSym;

    // Checks if the version requires legacy keyboard input
    // This is unreliable on 1.17.40 arm betas, 1.18.10 betas, and 1.18.20 betas
    if(linker::dlsym(handle, "bgfx_init")) {
        Keyboard::useLegacyKeyboard = false;
    } else {
        Keyboard::useLegacyKeyboard = true;
    }

    Keyboard::_states = (int *)linker::dlsym(handle, "_ZN8Keyboard7_statesE");
    if (Keyboard::useLegacyKeyboard) {
        Keyboard::_inputsLegacy = (std::vector<Keyboard::LegacyInputEvent> *)linker::dlsym(handle, "_ZN8Keyboard7_inputsE");
    } else {
        Keyboard::_inputs = (std::vector<Keyboard::InputEvent> *)linker::dlsym(handle, "_ZN8Keyboard7_inputsE");
    }
    Keyboard::_gameControllerId = (int *)linker::dlsym(handle, "_ZN8Keyboard17_gameControllerIdE");
}
