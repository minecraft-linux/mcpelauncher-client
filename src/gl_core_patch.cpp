#include "gl_core_patch.h"

#include <hybris/dlfcn.h>
#include <log.h>
#include <mcpelauncher/patch_utils.h>
#include <game_window_manager.h>

void (*GLCorePatch::glGenVertexArrays)(int n, unsigned int* arrays);
void (*GLCorePatch::glBindVertexArray)(unsigned int array);

void (*GLCorePatch::reflectShaderUniformsOriginal)(void*);
void (*GLCorePatch::bindVertexArrayOriginal)(void*, void*, void*);

void GLCorePatch::install(void* handle) {
    void* ptr = hybris_dlsym(handle, "_ZN3mce15ShaderGroupBase10loadShaderERKSsS2_S2_S2_");
    if (((unsigned char*) ptr)[0x5C9 - 0x4C0 + 1] != 0xAC) {
        Log::error("Launcher", "Graphics patch error: unexpected byte");
        exit(1);
    }
    ((unsigned char*) ptr)[0x5C9 - 0x4C0 + 1] += 4;

    reflectShaderUniformsOriginal = (void (*)(void*)) hybris_dlsym(handle, "_ZN3mce9ShaderOGL21reflectShaderUniformsEv");
    ptr = (void*) ((size_t) hybris_dlsym(handle, "_ZN3mce9ShaderOGLC2ERNS_11ShaderCacheERNS_13ShaderProgramES4_S4_") + (0x720 - 0x6A0));
    PatchUtils::patchCallInstruction(ptr, (void*) &reflectShaderUniformsHook, false);

    bindVertexArrayOriginal = (void (*)(void*, void*, void*)) hybris_dlsym(handle, "_ZN3mce9ShaderOGL18bindVertexPointersERKNS_12VertexFormatEPv");
    ptr = (void*) ((size_t) hybris_dlsym(handle, "_ZN3mce9ShaderOGL10bindShaderERNS_13RenderContextERKNS_12VertexFormatEPvj") + (0x83E - 0x7E0));
    PatchUtils::patchCallInstruction(ptr, (void*) &bindVertexArrayHook, false);

    ptr = hybris_dlsym(handle, "_ZN2gl21supportsImmediateModeEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &supportsImmediateModeHook, true);

    ptr = hybris_dlsym(handle, "_ZNK3mce9BufferOGL10bindBufferERNS_13RenderContextE");
    ((unsigned char*) ptr)[0x29] = 0x90;
    ((unsigned char*) ptr)[0x2A] = 0x90;

    ptr = hybris_dlsym(handle, "_ZN3mce16ShaderProgramOGL20compileShaderProgramERNS_11ShaderCacheE");
    const char* versionStr = "#version 410\n";
    unsigned char* patchData = (unsigned char*) ((size_t) ptr + (0xB3 - 0x40));
    patchData[0] = 0xB9;
    *((size_t*) (patchData + 1)) = (size_t) versionStr;
    patchData[5] = 0x90;
}

void GLCorePatch::onGLContextCreated() {
    auto getProcAddr = GameWindowManager::getManager()->getProcAddrFunc();
    glGenVertexArrays = (void (*)(int, unsigned int*)) getProcAddr("glGenVertexArrays");
    glBindVertexArray = (void (*)(unsigned int)) getProcAddr("glBindVertexArray");
}

void GLCorePatch::reflectShaderUniformsHook(void* th) {
    unsigned int vertexArr;
    glGenVertexArrays(1, &vertexArr);
    glBindVertexArray(vertexArr);
    *((unsigned int*) ((unsigned int) th + 0xAC)) = vertexArr;
    reflectShaderUniformsOriginal(th);
}

void GLCorePatch::bindVertexArrayHook(void* th, void* a, void* b) {
    unsigned int vertexArr = *((unsigned int*) ((unsigned int) th + 0xAC));
    if (bindVertexArrayOriginal == nullptr)
        abort();
    glBindVertexArray(vertexArr);
    bindVertexArrayOriginal(th, a, b);
}

bool GLCorePatch::supportsImmediateModeHook() {
    return false;
}

bool GLCorePatch::mustUseDesktopGL() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}