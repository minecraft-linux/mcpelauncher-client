#include <game_window_manager.h>
#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>
#include "splitscreen_patch.h"

void (*SplitscreenPatch::glScissor)(int x, int y, unsigned int w, unsigned int h);

void SplitscreenPatch::setScissorRect(void*, int x, int y, unsigned int w, unsigned int h) {
    glScissor(x, y, w, h);
}

void SplitscreenPatch::install(void* handle) {
    auto getProcAddr = GameWindowManager::getManager()->getProcAddrFunc();
    glScissor = (void (*)(int, int, unsigned int, unsigned int)) getProcAddr("glScissor");

    void* ptr = (void*) ((size_t) hybris_dlsym(handle, "_ZN3mce13RenderContext26setViewportWithFullScissorERKNS_12ViewportInfoE") + (0x85E - 0x740));
    PatchUtils::patchCallInstruction(ptr, (void*) &setScissorRect, false);
}