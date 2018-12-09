#include "xbox_sleep_shutdown_patch.h"

#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>
#include <cstring>
#include <log.h>

extern "C" void xbox_shutdown_patch_run_one_hook_1() asm("xbox_shutdown_patch_run_one_hook_1");
extern "C" void xbox_shutdown_patch_run_one_hook_2() asm("xbox_shutdown_patch_run_one_hook_2");

std::condition_variable XboxSleepShutdownPatch::cv;
std::mutex XboxSleepShutdownPatch::mutex;
bool XboxSleepShutdownPatch::shuttingDown = false;

std::atomic_int XboxSleepShutdownPatch::runningTasks;
std::condition_variable XboxSleepShutdownPatch::runningTasksCv;
std::mutex XboxSleepShutdownPatch::runningTasksMutex;

void XboxSleepShutdownPatch::sleepHook(unsigned int ms) {
    std::unique_lock<std::mutex> l (mutex);
    cv.wait_for(l, std::chrono::milliseconds(ms), []() { return shuttingDown; });
}

void XboxSleepShutdownPatch::install(void* handle) {
    void* ptr = hybris_dlsym(handle, "_ZN4xbox8services5utils5sleepEj");
    PatchUtils::patchCallInstruction(ptr, (void*) &sleepHook, true);

    ptr = hybris_dlsym(handle, "_ZN5boost4asio6detail15task_io_service10do_run_oneERNS1_11scoped_lockINS1_11posix_mutexEEERNS1_27task_io_service_thread_infoERKNS_6system10error_codeE");
    ptr = (void*) ((size_t) ptr + (0x30C2942 - 0x30C2620));
    PatchUtils::patchCallInstruction((void*) ((size_t) ptr - 7), (void*) &xbox_shutdown_patch_run_one_hook_1, false);
    memset((void*) ((size_t) ptr - 2), 0x90, 2);
    PatchUtils::patchCallInstruction((void*) ((size_t) ptr + 5), (void*) &xbox_shutdown_patch_run_one_hook_2, false);
}

void XboxSleepShutdownPatch::notifyShutdown() {
    {
        std::unique_lock<std::mutex> l(mutex);
        shuttingDown = true;
        cv.notify_all();
    }
    Log::trace("XboxLive", "Waiting for tasks");
    {
        std::unique_lock<std::mutex> l(runningTasksMutex);
        while (XboxSleepShutdownPatch::runningTasks > 0) {
            cv.wait_for(l, std::chrono::seconds(1));
            Log::trace("XboxLive", "Waiting for %i tasks", (int) XboxSleepShutdownPatch::runningTasks);
        }
    }
    Log::trace("XboxLive", "Finished waiting for tasks");
}

extern "C" void xbox_shutdown_patch_run_one_enter() asm("xbox_shutdown_patch_run_one_enter");
extern "C" void xbox_shutdown_patch_run_one_enter() {
    ++XboxSleepShutdownPatch::runningTasks;
}
extern "C" void xbox_shutdown_patch_run_one_exit() asm("xbox_shutdown_patch_run_one_exit");
extern "C" void xbox_shutdown_patch_run_one_exit() {
    if (XboxSleepShutdownPatch::runningTasks.fetch_sub(1) <= 1) {
        XboxSleepShutdownPatch::runningTasksCv.notify_all();
    }
}