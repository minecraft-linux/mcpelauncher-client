#include <hybris/dlfcn.h>
#include "symbols.h"

void (*Mouse::feed)(char, char, short, short, short, short);

int *Keyboard::_states;
std::vector<Keyboard::InputEvent> *Keyboard::_inputs;
int *Keyboard::_gameControllerId;

void SymbolsHelper::initSymbols(void *handle) {
    Mouse::feed = (void (*)(char, char, short, short, short, short)) hybris_dlsym(handle, "_ZN5Mouse4feedEccssss");

    Keyboard::_states = (int *) hybris_dlsym(handle, "_ZN8Keyboard7_statesE");
    Keyboard::_inputs = (std::vector<Keyboard::InputEvent> *) hybris_dlsym(handle, "_ZN8Keyboard7_inputsE");
    Keyboard::_gameControllerId = (int *) hybris_dlsym(handle, "_ZN8Keyboard17_gameControllerIdE");
}