#include "xbox_shutdown_patch.h"

#include <mcpelauncher/linker.h>
#include <mcpelauncher/patch_utils.h>
#include <cstring>
#include <log.h>
#include <mcpelauncher/minecraft_version.h>

std::condition_variable XboxShutdownPatch::cv;
std::mutex XboxShutdownPatch::mutex;
bool XboxShutdownPatch::shuttingDown = false;

std::atomic_int XboxShutdownPatch::runningTasks;
std::condition_variable XboxShutdownPatch::runningTasksCv;
std::mutex XboxShutdownPatch::runningTasksMutex;

void XboxShutdownPatch::sleepHook(unsigned int ms) {
    std::unique_lock<std::mutex> l(mutex);
    cv.wait_for(l, std::chrono::milliseconds(ms), []() { return shuttingDown; });
}

void XboxShutdownPatch::install(void* handle) {
    void* ptr = linker::dlsym(handle, "_ZN4xbox8services5utils5sleepEj");
    if(ptr == nullptr) {
        Log::warn("XboxShutdownPatch", "sleep() symbol not found");
        return;
    }
    PatchUtils::patchCallInstruction(ptr, (void*)&sleepHook, true);
}

void XboxShutdownPatch::notifyShutdown() {
    {
        std::unique_lock<std::mutex> l(mutex);
        shuttingDown = true;
        cv.notify_all();
    }
    {
        std::unique_lock<std::mutex> l(runningTasksMutex);
        while(XboxShutdownPatch::runningTasks > 0) {
            Log::trace("XboxLive", "Waiting for %i tasks", (int)XboxShutdownPatch::runningTasks);
            cv.wait_for(l, std::chrono::seconds(1));
        }
    }
    Log::trace("XboxLive", "Finished waiting for tasks");
}

extern "C" void xbox_shutdown_patch_run_one_enter() asm("xbox_shutdown_patch_run_one_enter");
extern "C" void xbox_shutdown_patch_run_one_enter() {
    ++XboxShutdownPatch::runningTasks;
}
extern "C" void xbox_shutdown_patch_run_one_exit() asm("xbox_shutdown_patch_run_one_exit");
extern "C" void xbox_shutdown_patch_run_one_exit() {
    if(XboxShutdownPatch::runningTasks.fetch_sub(1) <= 1) {
        XboxShutdownPatch::runningTasksCv.notify_all();
    }
}
