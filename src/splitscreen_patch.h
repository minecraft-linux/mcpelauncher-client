#pragma once

class SplitscreenPatch {
private:
    static void (*glScissor)(int x, int y, unsigned int w, unsigned int h);

    static void setScissorRect(void*, int x, int y, unsigned int w, unsigned int h);

public:
    static void install(void* handle);

    static void onGLContextCreated();
};
