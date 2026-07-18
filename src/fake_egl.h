#pragma once
#define __ANDROID__
#include <EGL/egl.h>
#undef __ANDROID__
#ifdef USE_ARMHF_SUPPORT
#include "armhf_support.h"
#endif
#include <mutex>
#include <vector>

namespace fake_egl {

EGLBoolean eglInitialize(EGLDisplay display, EGLint *major, EGLint *minor);
EGLBoolean eglTerminate(EGLDisplay display);
EGLint eglGetError();
char const *eglQueryString(EGLDisplay display, EGLint name);
EGLDisplay eglGetDisplay(EGLNativeDisplayType dp);
EGLDisplay eglGetCurrentDisplay();
EGLContext eglGetCurrentContext();
EGLBoolean eglChooseConfig(EGLDisplay display, EGLint const *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLBoolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value);
EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window, EGLint const *attrib_list);
EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface);
EGLContext eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const *attrib_list);
EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context);
EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context);
EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface);
EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval);
EGLBoolean eglQuerySurface(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint *value);

void *eglGetProcAddress(const char *name);

}  // namespace fake_egl

struct FakeEGL {
    using HostFunction = void (*)();
    using HostProcAddress = HostFunction (*)(const char*);

    struct CapabilityInfo {
        EGLint initializeMajor;
        EGLint initializeMinor;
        char const *vendor;
        char const *version;
        char const *extensions;
    };

    struct SwapBuffersCallback {
        void *user;
        void (*callback)(void *user, EGLDisplay display, EGLSurface surface);
    };
    static std::vector<SwapBuffersCallback> swapBuffersCallbacks;
    static std::mutex swapBuffersCallbacksLock;

    static void setProcAddrFunction(HostProcAddress fn);

    static void addSwapBuffersCallback(void *user, void (*callback)(void *user, EGLDisplay display, EGLSurface surface));

    static void installLibrary();

    static void setupGLOverrides();

    // Returns the immutable values implemented by the synthetic libEGL
    // exports without invoking EGL lifecycle functions for diagnostics.
    static CapabilityInfo getCapabilityInfo();

    static bool enableTexturePatch;
};
