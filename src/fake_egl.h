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

EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay display, EGLint *major, EGLint *minor);
EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay display);
EGLint EGLAPIENTRY eglGetError();
char const *EGLAPIENTRY eglQueryString(EGLDisplay display, EGLint name);
EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType dp);
EGLDisplay EGLAPIENTRY eglGetCurrentDisplay();
EGLContext EGLAPIENTRY eglGetCurrentContext();
EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay display, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay display, EGLint const *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value);
EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window, EGLint const *attrib_list);
EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay display, EGLSurface surface);
EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const *attrib_list);
EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay display, EGLContext context);
EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context);
EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay display, EGLSurface surface);
EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay display, EGLint interval);
EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint *value);
EGLBoolean EGLAPIENTRY eglWaitClient();

// Registered as the guest eglGetProcAddress export with the exact EGL ABI.
__eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddressExport(const char *name);

// Internal adapter for consumers such as GLAD that use an object-pointer
// loader signature instead of EGL's function-pointer return type.
void *eglGetProcAddress(const char *name);

}  // namespace fake_egl

struct FakeEGL {
    using HostFunction = void (*)();
    using HostProcAddress = HostFunction (*)(const char*);

    struct ConfigurationInfo {
        EGLint configId;
        EGLint configCaveat;
        EGLint colorBufferType;
        EGLint bufferSize;
        EGLint redSize;
        EGLint greenSize;
        EGLint blueSize;
        EGLint alphaSize;
        EGLint luminanceSize;
        EGLint alphaMaskSize;
        EGLint depthSize;
        EGLint stencilSize;
        EGLint sampleBuffers;
        EGLint samples;
        EGLint surfaceType;
        EGLint renderableType;
        EGLint conformant;
        EGLBoolean nativeRenderable;
        EGLint nativeVisualId;
        EGLint nativeVisualType;
    };

    struct CapabilityInfo {
        EGLint initializeMajor;
        EGLint initializeMinor;
        char const *vendor;
        char const *version;
        char const *clientExtensions;
        char const *displayExtensions;
        char const *clientApis;
        ConfigurationInfo const *configurations;
        std::size_t configurationCount;
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

    // Exercises the successful synthetic query exports against the immutable
    // descriptor without changing fake EGL lifecycle state.
    static bool validateCapabilityContract();

    static bool enableTexturePatch;
};
