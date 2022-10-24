#pragma once

#include <cstddef>

class HbuiPatch {
private:
    static bool returnTrue() {
        return true;
    }

    static void writeLog(void* th, int level, const char* what, unsigned int length);

public:
    static void install(void* handle);
};
