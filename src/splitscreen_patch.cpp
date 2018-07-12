#include <game_window_manager.h>
#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>
#include <log.h>
#include "splitscreen_patch.h"

void (*SplitscreenPatch::glScissor)(int x, int y, unsigned int w, unsigned int h);

void SplitscreenPatch::setScissorRect(void*, int x, int y, unsigned int w, unsigned int h) {
    glScissor(x, y, w, h);
}

void SplitscreenPatch::install(void* handle) {
    void* ptr = (void*) ((size_t) hybris_dlsym(handle, "_ZN3mce13RenderContext26setViewportWithFullScissorERKNS_12ViewportInfoE") + (0x85E - 0x740));
    if (*((unsigned char*) ptr) != 0xE8) {
        Log::error("SplitscreenPatch", "Not patching splitscreen - incompatible code");
        return;
    }
    PatchUtils::patchCallInstruction(ptr, (void*) &setScissorRect, false);
}

void SplitscreenPatch::onGLContextCreated() {
    auto getProcAddr = GameWindowManager::getManager()->getProcAddrFunc();
    glScissor = (void (*)(int, int, unsigned int, unsigned int)) getProcAddr("glScissor");
}