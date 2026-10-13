#pragma once

#include <string>
#include <unordered_map>

// Android (AAPCS64) passes every stack argument in an 8 byte slot, Apple arm64 packs them to their natural size.
// GL functions with more than one stack argument therefore read garbage when the game calls them directly.
class AppleArm64GLABI {
public:
    static void install(std::unordered_map<std::string, void *> &overrides, void *(*resolver)(const char *));
};
