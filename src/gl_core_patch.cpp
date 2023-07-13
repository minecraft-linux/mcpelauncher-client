#include "gl_core_patch.h"

#include <cstring>
#include <mcpelauncher/linker.h>
#include <log.h>
#include <mcpelauncher/minecraft_version.h>
#include <mcpelauncher/patch_utils.h>
#include <stdexcept>

bool GLCorePatch::enabled = false;
std::unordered_map<unsigned int, unsigned int> GLCorePatch::vaoMap;
std::pair<int, unsigned int> GLCorePatch::buffers[2] = {{0x8892, 0}, {0x8893, 0}};
void (*GLCorePatch::glGenVertexArrays)(int n, unsigned int *arrays);
void (*GLCorePatch::glBindVertexArray)(unsigned int array);
void (*GLCorePatch::glShaderSource_orig)(unsigned int shader, unsigned int count, const char **string, int *length);
void (*GLCorePatch::glLinkProgram_orig)(unsigned int program);
void (*GLCorePatch::glUseProgram_orig)(unsigned int program);
void (*GLCorePatch::glBindBuffer_orig)(int target, unsigned int buffer);

void GLCorePatch::install(void *handle) {
    if(linker::dlsym(handle, "bgfx_init")) {
        throw std::runtime_error("Glcore patch not supported on render dragon versions");
    }
#if __i386__
    void *ptr = PatchUtils::patternSearch(handle, "53 83 EC 18 E8 00 00 00 00 5B 81 C3 ?? ?? ?? ?? 8B 83 ?? ?? ?? ?? 85 C0 79 5F 8D 44 24 08 89 04 24 E8 ?? ?? ?? ?? 83 EC 04 8B 44 24 08 83 F8 02");
#elif __x86_64__
    void *ptr = PatchUtils::patternSearch(handle, "8B 15 ?? ?? ?? ?? 85 D2 78 07 83 FA 01 0F 94 C0 C3 50 E8 ?? ?? ?? ?? C1 EA 10 F7 D2 83 E2 01 89");
    if(!ptr) {
        ptr = PatchUtils::patternSearch(handle, "50 ?? ?? ?? ?? ?? ?? 85 d2 ?? ?? ?? ?? ?? ?? ?? c1 ea 10 f7 d2 83 e2 01"); // Pattern for 1.17-1.18.12
    }
#else
    void *ptr = nullptr;
#endif
    if(!ptr) {
        ptr = linker::dlsym(handle, "_ZN2gl21supportsImmediateModeEv");
    }

    if(!ptr)
        throw std::runtime_error("Failed to find gl::supportsImmediateMode");
    unsigned char replace[6] = {0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3};
    memcpy(ptr, replace, 6);

    enabled = true;
}

void GLCorePatch::installGL(std::unordered_map<std::string, void *> &overrides, void *(*resolver)(const char *)) {
    if(!enabled)
        return;

    glGenVertexArrays = (void (*)(int, unsigned int *))resolver("glGenVertexArrays");
    glBindVertexArray = (void (*)(unsigned int))resolver("glBindVertexArray");

    glShaderSource_orig = (void (*)(unsigned int, unsigned int, const char **, int *))resolver("glShaderSource");
    glLinkProgram_orig = (void (*)(unsigned int))resolver("glLinkProgram");
    glUseProgram_orig = (void (*)(unsigned int))resolver("glUseProgram");
    glBindBuffer_orig = (void (*)(int, unsigned int))resolver("glBindBuffer");

    overrides["glShaderSource"] = (void *)glShaderSource;
    overrides["glLinkProgram"] = (void *)glLinkProgram;
    overrides["glUseProgram"] = (void *)glUseProgram;
    overrides["glBindBuffer"] = (void *)glBindBuffer;
}

void GLCorePatch::glShaderSource(unsigned int shader, unsigned int count, const char **string, int *length) {
    if(*length > 0 && !strcmp(string[0], "#version 300 es\n")) {
        string[0] = "#version 410\n";
        length[0] = strlen("#version 410\n");
    }
    glShaderSource_orig(shader, count, string, length);
}

void GLCorePatch::glLinkProgram(unsigned int program) {
    glLinkProgram_orig(program);

    unsigned int vertexArr;
    glGenVertexArrays(1, &vertexArr);
    glBindVertexArray(vertexArr);
    vaoMap[program] = vertexArr;
}

void GLCorePatch::glUseProgram(unsigned int program) {
    glUseProgram_orig(program);

    if(program != 0) {
        glBindVertexArray(vaoMap.at(program));
        for(auto &e : GLCorePatch::buffers)
            glBindBuffer_orig(e.first, e.second);
    }
}

void GLCorePatch::glBindBuffer(int target, unsigned int buffer) {
    glBindBuffer_orig(target, buffer);

    for(auto &e : GLCorePatch::buffers) {
        if(e.first == target) {
            e.second = buffer;
            return;
        }
    }
}

bool GLCorePatch::mustUseDesktopGL() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}
