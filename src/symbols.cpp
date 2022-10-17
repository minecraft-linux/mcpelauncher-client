#include "symbols.h"
#include <mcpelauncher/linker.h>

void (*Mouse::feed)(char, char, short, short, short, short);

int *Keyboard::_states;
std::vector<Keyboard::InputEvent> *Keyboard::_inputs;
int *Keyboard::_gameControllerId;

void SymbolsHelper::initSymbols(void *handle) {
    Mouse::feed = (void (*)(char, char, short, short, short, short))linker::dlsym(handle, "_ZN5Mouse4feedEccssss");

    Keyboard::_states = (int *)linker::dlsym(handle, "_ZN8Keyboard7_statesE");
    Keyboard::_inputs = (std::vector<Keyboard::InputEvent> *)linker::dlsym(handle, "_ZN8Keyboard7_inputsE");
    Keyboard::_gameControllerId = (int *)linker::dlsym(handle, "_ZN8Keyboard17_gameControllerIdE");
}