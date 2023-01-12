#include <mcpelauncher/linker.h>
#include "symbols.h"

void (*Mouse::feed)(char, char, short, short, short, short);

int *Keyboard::_states;
std::vector<Keyboard::InputEvent> *Keyboard::_inputs;
int *Keyboard::_gameControllerId;

void SymbolsHelper::initSymbols(void *handle) {
    void* MouseFeedSym;
    if (!(MouseFeedSym = linker::dlsym(handle, "_ZN5Mouse4feedEccssss"))) {
        MouseFeedSym = linker::dlsym(handle, "_ZN5Mouse4feedEcassss"); // 1.19.60.26 Beta Mouse::feed ABI changed
    }
    Mouse::feed = (void (*)(char, char, short, short, short, short))MouseFeedSym;

    Keyboard::_states = (int *)linker::dlsym(handle, "_ZN8Keyboard7_statesE");
    Keyboard::_inputs = (std::vector<Keyboard::InputEvent> *)linker::dlsym(handle, "_ZN8Keyboard7_inputsE");
    Keyboard::_gameControllerId = (int *)linker::dlsym(handle, "_ZN8Keyboard17_gameControllerIdE");
}
