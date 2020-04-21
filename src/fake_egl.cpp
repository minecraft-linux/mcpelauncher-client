#include "fake_egl.h"

#define __ANDROID__
#include <EGL/egl.h>
#undef __ANDROID__
#include <log.h>
#include <cstring>
#include <game_window.h>
#include <mcpelauncher/linker.h>

namespace fake_egl {

static thread_local EGLSurface currentDrawSurface;
static void *(*hostProcAddrFn)(const char *);

EGLBoolean eglInitialize(EGLDisplay display, EGLint *major, EGLint *minor) {
    if (major)
        *major = 1;
    if (minor)
        *minor = 5;
    return EGL_TRUE;
}

EGLBoolean eglTerminate(EGLDisplay display) {
    return EGL_TRUE;
}

EGLint eglGetError() {
    return 0;
}

char const * eglQueryString(EGLDisplay display, EGLint name) {
    if (name == EGL_VENDOR)
        return "mcpelauncher";
    if (name == EGL_VERSION)
        return "1.5 mcpelauncher";
    if (name == EGL_EXTENSIONS)
        return "";
    Log::warn("FakeEGL", "eglQueryString %x", name);
    return nullptr;
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType dp) {
    return (EGLDisplay *) 1;
}

EGLDisplay eglGetCurrentDisplay() {
    return (EGLDisplay *) 1;
}

EGLContext eglGetCurrentContext() {
    return (EGLContext *) (currentDrawSurface ? 1 : 0);
}

EGLBoolean eglChooseConfig(EGLDisplay display, EGLint const *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    *num_config = 1;
    return EGL_TRUE;
}

EGLBoolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint * value) {
    Log::warn("FakeEGL", "eglGetConfigAttrib %x", attribute);
    return EGL_TRUE;
}

EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window, EGLint const * attrib_list) {
    return native_window;
}

EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface) {
    return EGL_TRUE;
}

EGLContext eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const * attrib_list) {
    return (EGLContext *) 1;
}

EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context) {
    return EGL_TRUE;
}

EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context) {
    currentDrawSurface = draw;
    return EGL_TRUE;
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
//    Log::trace("FakeEGL", "eglSwapBuffers");
    ((GameWindow *) surface)->swapBuffers();
    return EGL_TRUE;
}

EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval) {
    ((GameWindow *) currentDrawSurface)->setSwapInterval(interval);
    return EGL_TRUE;
}

EGLBoolean eglQuerySurface(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint *value) {
    if (attribute == EGL_WIDTH || attribute == EGL_HEIGHT) {
        int w, h;
        ((GameWindow *) surface)->getWindowSize(w, h);
        *value = (attribute == EGL_WIDTH ? w : h);
        return EGL_TRUE;
    }
    Log::warn("FakeEGL", "eglQuerySurface %x", attribute);
    return EGL_TRUE;
}

void *eglGetProcAddress(const char *name) {
    if (!strcmp(name, "glInvalidateFramebuffer"))
        return (void *) +[]() {};
    return hostProcAddrFn(name);
}

}

void FakeEGL::setProcAddrFunction(void *(*fn)(const char *)) {
    fake_egl::hostProcAddrFn = fn;
}

void FakeEGL::installLibrary() {
    std::unordered_map<std::string, void*> syms;
    syms["eglInitialize"] = (void *) fake_egl::eglInitialize;
    syms["eglTerminate"] = (void *) fake_egl::eglTerminate;
    syms["eglGetError"] = (void *) fake_egl::eglGetError;
    syms["eglQueryString"] = (void *) fake_egl::eglQueryString;
    syms["eglGetDisplay"] = (void *) fake_egl::eglGetDisplay;
    syms["eglGetCurrentDisplay"] = (void *) fake_egl::eglGetCurrentDisplay;
    syms["eglGetCurrentContext"] = (void *) fake_egl::eglGetCurrentContext;
    syms["eglChooseConfig"] = (void *) fake_egl::eglChooseConfig;
    syms["eglGetConfigAttrib"] = (void *) fake_egl::eglGetConfigAttrib;
    syms["eglCreateWindowSurface"] = (void *) fake_egl::eglCreateWindowSurface;
    syms["eglDestroySurface"] = (void *) fake_egl::eglDestroySurface;
    syms["eglCreateContext"] = (void *) fake_egl::eglCreateContext;
    syms["eglDestroyContext"] = (void *) fake_egl::eglDestroyContext;
    syms["eglMakeCurrent"] = (void *) fake_egl::eglMakeCurrent;
    syms["eglSwapBuffers"] = (void *) fake_egl::eglSwapBuffers;
    syms["eglSwapInterval"] = (void *) fake_egl::eglSwapInterval;
    syms["eglQuerySurface"] = (void *) fake_egl::eglQuerySurface;
    syms["eglGetProcAddress"] = (void *) fake_egl::eglGetProcAddress;
    linker::load_library("libEGL.so", syms);
}
