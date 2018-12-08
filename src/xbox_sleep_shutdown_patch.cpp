#include "xbox_sleep_shutdown_patch.h"

#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>
#include <log.h>

std::condition_variable XboxSleepShutdownPatch::cv;
std::mutex XboxSleepShutdownPatch::mutex;
bool XboxSleepShutdownPatch::shuttingDown = false;
std::mutex XboxSleepShutdownPatch::ensureConnectedMutex;
void (*XboxSleepShutdownPatch::ensureConnectedOriginal)(void*);

void XboxSleepShutdownPatch::sleepHook(unsigned int ms) {
    std::unique_lock<std::mutex> l (mutex);
    cv.wait_for(l, std::chrono::milliseconds(ms), []() { return shuttingDown; });
}

void XboxSleepShutdownPatch::install(void* handle) {
    void* ptr = hybris_dlsym(handle, "_ZN4xbox8services5utils5sleepEj");
    PatchUtils::patchCallInstruction(ptr, (void*) &sleepHook, true);
    if (!tryInstallWSPatch(handle))
        Log::warn("Launcher", "Failed to install Xbox Web Socket patch");
}

bool XboxSleepShutdownPatch::tryInstallWSPatch(void *handle) {
    size_t instr, ebx;
    size_t ptr = (size_t) hybris_dlsym(handle, "_ZN4xbox8services21web_socket_connection16ensure_connectedEv");
    if (!ptr)
        return false;
    instr = ptr + (0x2DE89A7 - 0x2DE8790);
    if (*((const unsigned char*) instr) != 0xE8)
        return false;
    ptr = instr + *(int*) (instr + 1) + 5;
    // sub_2DE9660
    instr = ptr + (0x2DE96E2 - 0x2DE9660);
    if (*((const unsigned char*) instr) != 0xE8)
        return false;
    ptr = instr + *(int*) (instr + 1) + 5;
    // sub_2DEA840
    ebx = ptr + 0x13 - 1;
    ebx += *((unsigned int*) (ptr + 0x13 + 2));
    instr = ptr + (0x2DEAAAD - 0x2DEA840);
    if (*((const unsigned char*) instr) != 0x8D)
        return false;
    ptr = ebx + *(int*) (instr + 2);
    // `vtable for'pplx::task<unsigned char>::_InitialTaskHandle<void, xbox::services::web_socket_connection::ensure_connected(void)::$_0, pplx::details::_TypeSelectorNoAsync>
    ptr = *(size_t*) (ptr + 8);
    // sub_2DEAD50
    ebx = ptr + 0xd - 1;
    ebx += *((unsigned int*) (ptr + 0xd + 2));
    instr = ptr + (0x2DEAF09 - 0x2DEAE20);
    if (*((const unsigned char*) instr) != 0x8D)
        return false;
    ptr = ebx + *(int*) (instr + 2);
    *(int*) (instr + 2) = (size_t) ensureConnectedHook - ebx;
    (void*&) ensureConnectedOriginal = (void*) ptr;
    return true;
}

void XboxSleepShutdownPatch::notifyShutdown() {
    {
        std::unique_lock<std::mutex> l(mutex);
        shuttingDown = true;
        cv.notify_all();
    }
    {
        std::unique_lock<std::mutex> l(ensureConnectedMutex);
    }
}

void XboxSleepShutdownPatch::ensureConnectedHook(void* th) {
    std::unique_lock<std::mutex> l (ensureConnectedMutex);
    {
        std::unique_lock<std::mutex> l2(mutex);
        if (shuttingDown)
            return;
    }
    ensureConnectedOriginal(th);
}