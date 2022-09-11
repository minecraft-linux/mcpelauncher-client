#pragma once
#include <string>
#include <unordered_map>

class ArmhfSupport {
   public:
    static void install(std::unordered_map<std::string, void *> &overrides);
};