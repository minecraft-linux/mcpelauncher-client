#include "core_patches.h"

#include <mcpelauncher/linker.h>
#include <mcpelauncher/patch_utils.h>
#include <log.h>

std::shared_ptr<GameWindow> CorePatches::currentGameWindow;

std::shared_ptr<WindowCallbacks> CorePatches::currentGameWindowCallbacks;

void CorePatches::install(void* handle) {
    // void* ptr = linker::dlsym(handle, "_ZN3web4http6client7details35verify_cert_chain_platform_specificERN5boost4asio3ssl14verify_contextERKSs");
    // PatchUtils::patchCallInstruction(ptr, (void*) +[]() { return true; }, true);

    void* appPlatform = linker::dlsym(handle, "_ZTV21AppPlatform_android23");
    if(appPlatform) {
        void** vta = &((void**)appPlatform)[2];
        PatchUtils::VtableReplaceHelper vtr(handle, vta, vta);
        vtr.replace("_ZN11AppPlatform16hideMousePointerEv", &hideMousePointer);
        vtr.replace("_ZN11AppPlatform16showMousePointerEv", &showMousePointer);
    } else {
        Log::debug("CorePatches", "Failed to patch, vtable _ZTV21AppPlatform_android23 not found");
    }
}

void CorePatches::showMousePointer() {
    currentGameWindow->setCursorDisabled(false);
}

void CorePatches::hideMousePointer() {
    currentGameWindow->setCursorDisabled(true);
}

void CorePatches::setFullscreen(void* t, bool fullscreen) {
    currentGameWindowCallbacks->setFullscreen(fullscreen, true);
}

void CorePatches::setGameWindow(std::shared_ptr<GameWindow> gameWindow) {
    currentGameWindow = gameWindow;
}

void CorePatches::setGameWindowCallbacks(std::shared_ptr<WindowCallbacks> gameWindowCallbacks) {
    currentGameWindowCallbacks = gameWindowCallbacks;
}