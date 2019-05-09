#pragma once

#include <minecraft/Xbox.h>

class LegacyPatches {

private:
    static xbox::services::xbox_live_result<void> initCllStub();

public:
    static void install(void* handle);

};