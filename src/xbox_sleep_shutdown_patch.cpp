#include "xbox_sleep_shutdown_patch.h"

#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>

std::condition_variable XboxSleepShutdownPatch::cv;
std::mutex XboxSleepShutdownPatch::mutex;
bool XboxSleepShutdownPatch::shuttingDown = false;

void XboxSleepShutdownPatch::sleepHook(unsigned int ms) {
    std::unique_lock<std::mutex> l (mutex);
    cv.wait_for(l, std::chrono::milliseconds(ms), []() { return shuttingDown; });
}

void XboxSleepShutdownPatch::install(void* handle) {
    void* ptr = hybris_dlsym(handle, "_ZN4xbox8services5utils5sleepEj");
    PatchUtils::patchCallInstruction(ptr, (void*) &sleepHook, true);
}

void XboxSleepShutdownPatch::notifyShutdown() {
    std::unique_lock<std::mutex> l (mutex);
    shuttingDown = true;
    cv.notify_all();
}