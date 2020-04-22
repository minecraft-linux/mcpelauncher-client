#pragma once

namespace fake_egl {

    void *eglGetProcAddress(const char *name);

}

struct FakeEGL {

    static void setProcAddrFunction(void *(*fn)(const char *));

    static void installLibrary();

    static void setupGLOverrides();

};