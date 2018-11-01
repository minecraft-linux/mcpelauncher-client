#pragma once

#include <cstddef>

class GLCorePatch {

private:
    static size_t shaderVertexArrOffset;

    static void (*glGenVertexArrays)(int n, unsigned int* arrays);
    static void (*glBindVertexArray)(unsigned int array);

    static void (*reflectShaderUniformsOriginal)(void*);
    static void reflectShaderUniformsHook(void* th);

    static void (*bindVertexArrayOriginal)(void*, void*, void*);
    static void bindVertexArrayHook(void* th, void* a, void* b);

    static bool supportsImmediateModeHook();

public:
    static void install(void* handle);

    static void onGLContextCreated();


    static bool mustUseDesktopGL();

};