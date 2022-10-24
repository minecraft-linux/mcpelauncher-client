#pragma once
#include <unordered_map>
#include <string>

class ArmhfSupport {
public:
    static void install(std::unordered_map<std::string, void*>& overrides);
};
