#pragma once

#include <cstddef>

class HbuiPatch {

private:
    static bool returnTrue() {
        return true;
    }

public:
    static void install(void* handle);

};