#pragma once
#include <string>
#include <unordered_map>

class FakeWindow {
public:
    static void initHybrisHooks(std::unordered_map<std::string, void *> &syms);
};
