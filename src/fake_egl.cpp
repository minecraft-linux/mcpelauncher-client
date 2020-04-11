#include "fake_egl.h"

#define __ANDROID__
#include <EGL/egl.h>
#undef __ANDROID__
#include <log.h>
#include <cstring>
#include <game_window.h>
#include <hybris/hook.h>

namespace fake_egl {

static thread_local EGLSurface currentDrawSurface;

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

EGLContext eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const * attrib_list) {
    return (EGLContext *) 1;
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

}

void FakeEGL::initHybrisHooks() {
    hybris_hook("eglGetError", (void *) fake_egl::eglGetError);
    hybris_hook("eglQueryString", (void *) fake_egl::eglQueryString);
    hybris_hook("eglGetDisplay", (void *) fake_egl::eglGetDisplay);
    hybris_hook("eglGetCurrentDisplay", (void *) fake_egl::eglGetCurrentDisplay);
    hybris_hook("eglChooseConfig", (void *) fake_egl::eglChooseConfig);
    hybris_hook("eglGetConfigAttrib", (void *) fake_egl::eglGetConfigAttrib);
    hybris_hook("eglCreateWindowSurface", (void *) fake_egl::eglCreateWindowSurface);
    hybris_hook("eglCreateContext", (void *) fake_egl::eglCreateContext);
    hybris_hook("eglMakeCurrent", (void *) fake_egl::eglMakeCurrent);
    hybris_hook("eglSwapBuffers", (void *) fake_egl::eglSwapBuffers);
    hybris_hook("eglSwapInterval", (void *) fake_egl::eglSwapInterval);
    hybris_hook("eglQuerySurface", (void *) fake_egl::eglQuerySurface);
}