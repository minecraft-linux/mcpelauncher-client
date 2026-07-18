#include "fake_egl.h"
#include "gl_core_patch.h"
#include "settings.h"
#include "imgui_ui.h"
#include <algorithm>
#include <limits>
#include <map>

#define __ANDROID__
#include <EGL/egl.h>
#undef __ANDROID__
#include <log.h>
#include <cstring>
#include <game_window.h>
#include <mcpelauncher/linker.h>
#ifdef USE_ARMHF_SUPPORT
#include "armhf_support.h"
#endif

std::vector<FakeEGL::SwapBuffersCallback> FakeEGL::swapBuffersCallbacks = {};
std::mutex FakeEGL::swapBuffersCallbacksLock;

FakeEGL::CapabilityInfo FakeEGL::getCapabilityInfo() {
    static constexpr ConfigurationInfo configurations[] = {{
        1,
        EGL_NONE,
        EGL_RGB_BUFFER,
        32,
        8,
        8,
        8,
        8,
        0,
        0,
        8,
        8,
        0,
        0,
        EGL_WINDOW_BIT,
        EGL_OPENGL_ES2_BIT,
        EGL_OPENGL_ES2_BIT,
        EGL_FALSE,
        0,
        0
    }};
    return {1, 5, "mcpelauncher", "1.5 mcpelauncher", "", "", "OpenGL_ES",
            configurations, sizeof(configurations) / sizeof(configurations[0])};
}

namespace fake_egl {

static thread_local EGLSurface currentDrawSurface;
static FakeEGL::HostProcAddress hostProcAddrFn;
static std::unordered_map<std::string, void *> hostProcOverrides;

EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay display, EGLint *major, EGLint *minor) {
    auto capabilityInfo = FakeEGL::getCapabilityInfo();
    if(major)
        *major = capabilityInfo.initializeMajor;
    if(minor)
        *minor = capabilityInfo.initializeMinor;
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay display) {
    return EGL_TRUE;
}

EGLint EGLAPIENTRY eglGetError() {
    return EGL_SUCCESS;
}

char const *EGLAPIENTRY eglQueryString(EGLDisplay display, EGLint name) {
    auto capabilityInfo = FakeEGL::getCapabilityInfo();
    if(name == EGL_VENDOR)
        return capabilityInfo.vendor;
    if(name == EGL_VERSION)
        return capabilityInfo.version;
    if(name == EGL_EXTENSIONS)
        return display == EGL_NO_DISPLAY ? capabilityInfo.clientExtensions : capabilityInfo.displayExtensions;
    if(name == EGL_CLIENT_APIS)
        return capabilityInfo.clientApis;
    Log::warn("FakeEGL", "eglQueryString %x", name);
    return nullptr;
}

EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType dp) {
    return (EGLDisplay *)1;
}

EGLDisplay EGLAPIENTRY eglGetCurrentDisplay() {
    return (EGLDisplay *)1;
}

EGLContext EGLAPIENTRY eglGetCurrentContext() {
    return currentDrawSurface ? (EGLContext *)1 : (EGLContext *)0;
}

static EGLConfig configurationHandle(std::size_t index) {
    return reinterpret_cast<EGLConfig>(index + 1);
}

EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay display, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    if(num_config == nullptr || config_size < 0) {
        return EGL_FALSE;
    }
    auto capabilityInfo = FakeEGL::getCapabilityInfo();
    auto available = static_cast<EGLint>(capabilityInfo.configurationCount);
    if(configs == nullptr) {
        *num_config = available;
        return EGL_TRUE;
    }
    auto returned = std::min(config_size, available);
    for(EGLint index = 0; index < returned; ++index) {
        configs[index] = configurationHandle(static_cast<std::size_t>(index));
    }
    *num_config = returned;
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay display, EGLint const *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    // Preserve the synthetic implementation's historical single-config
    // behavior for every Minecraft attribute list.
    return eglGetConfigs(display, configs, config_size, num_config);
}

EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value) {
    if(value == nullptr) {
        return EGL_FALSE;
    }
    auto capabilityInfo = FakeEGL::getCapabilityInfo();
    if(capabilityInfo.configurationCount == 0 || config != configurationHandle(0)) {
        return EGL_FALSE;
    }
    const auto& configuration = capabilityInfo.configurations[0];
    switch(attribute) {
        case EGL_CONFIG_ID: *value = configuration.configId; return EGL_TRUE;
        case EGL_CONFIG_CAVEAT: *value = configuration.configCaveat; return EGL_TRUE;
        case EGL_COLOR_BUFFER_TYPE: *value = configuration.colorBufferType; return EGL_TRUE;
        case EGL_BUFFER_SIZE: *value = configuration.bufferSize; return EGL_TRUE;
        case EGL_RED_SIZE: *value = configuration.redSize; return EGL_TRUE;
        case EGL_GREEN_SIZE: *value = configuration.greenSize; return EGL_TRUE;
        case EGL_BLUE_SIZE: *value = configuration.blueSize; return EGL_TRUE;
        case EGL_ALPHA_SIZE: *value = configuration.alphaSize; return EGL_TRUE;
        case EGL_LUMINANCE_SIZE: *value = configuration.luminanceSize; return EGL_TRUE;
        case EGL_ALPHA_MASK_SIZE: *value = configuration.alphaMaskSize; return EGL_TRUE;
        case EGL_DEPTH_SIZE: *value = configuration.depthSize; return EGL_TRUE;
        case EGL_STENCIL_SIZE: *value = configuration.stencilSize; return EGL_TRUE;
        case EGL_SAMPLE_BUFFERS: *value = configuration.sampleBuffers; return EGL_TRUE;
        case EGL_SAMPLES: *value = configuration.samples; return EGL_TRUE;
        case EGL_SURFACE_TYPE: *value = configuration.surfaceType; return EGL_TRUE;
        case EGL_RENDERABLE_TYPE: *value = configuration.renderableType; return EGL_TRUE;
        case EGL_CONFORMANT: *value = configuration.conformant; return EGL_TRUE;
        case EGL_NATIVE_RENDERABLE: *value = configuration.nativeRenderable; return EGL_TRUE;
        case EGL_NATIVE_VISUAL_ID: *value = configuration.nativeVisualId; return EGL_TRUE;
        case EGL_NATIVE_VISUAL_TYPE: *value = configuration.nativeVisualType; return EGL_TRUE;
    }
    Log::warn("FakeEGL", "eglGetConfigAttrib %x", attribute);
    return EGL_FALSE;
}

EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window, EGLint const *attrib_list) {
    return native_window;
}

EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay display, EGLSurface surface) {
    return EGL_TRUE;
}

EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const *attrib_list) {
    return (EGLContext *)1;
}

EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay display, EGLContext context) {
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context) {
    if(draw != nullptr) {
        ((GameWindow *)draw)->makeCurrent(true);
#ifdef USE_IMGUI
        ImGuiUIInit((GameWindow *)draw);
#endif
    } else {
        ((GameWindow *)currentDrawSurface)->makeCurrent(false);
    }
    currentDrawSurface = draw;
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    if(FakeEGL::swapBuffersCallbacksLock.try_lock()) {
        for(size_t i = 0; i < FakeEGL::swapBuffersCallbacks.size(); i++) {
            FakeEGL::swapBuffersCallbacks[i].callback(FakeEGL::swapBuffersCallbacks[i].user, display, surface);
        }
        FakeEGL::swapBuffersCallbacksLock.unlock();
    }
    //    Log::trace("FakeEGL", "eglSwapBuffers");
#ifdef USE_IMGUI
    ImGuiUIDrawFrame((GameWindow *)surface);
#endif
    ((GameWindow *)surface)->swapBuffers();
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay display, EGLint interval) {
    //((GameWindow *)currentDrawSurface)->setSwapInterval(interval);
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint *value) {
    if(attribute == EGL_WIDTH || attribute == EGL_HEIGHT) {
        int w, h;
        ((GameWindow *)surface)->getWindowSize(w, h);
        *value = (attribute == EGL_WIDTH ? w : h - Settings::menubarsize.load());
        return EGL_TRUE;
    }
    Log::warn("FakeEGL", "eglQuerySurface %x", attribute);
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY eglWaitClient() {
    return EGL_TRUE;
}

__eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddressExport(const char *name) {
    auto it = hostProcOverrides.find(name);
    if(it != hostProcOverrides.end())
        return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(it->second);
    return hostProcAddrFn(name);
}

void *eglGetProcAddress(const char *name) {
    return reinterpret_cast<void*>(eglGetProcAddressExport(name));
}

}  // namespace fake_egl

bool FakeEGL::enableTexturePatch = false;

void FakeEGL::setProcAddrFunction(HostProcAddress fn) {
    fake_egl::hostProcAddrFn = fn;
}

bool FakeEGL::validateCapabilityContract() {
    const auto capabilities = getCapabilityInfo();
    EGLint major = -1;
    EGLint minor = -1;
    EGLDisplay display = reinterpret_cast<EGLDisplay>(1);
    if(fake_egl::eglInitialize(display, &major, &minor) != EGL_TRUE ||
       major != capabilities.initializeMajor || minor != capabilities.initializeMinor) {
        return false;
    }
    auto stringMatches = [](const char* actual, const char* expected) {
        return actual != nullptr && expected != nullptr && strcmp(actual, expected) == 0;
    };
    if(!stringMatches(fake_egl::eglQueryString(display, EGL_VENDOR), capabilities.vendor) ||
       !stringMatches(fake_egl::eglQueryString(display, EGL_VERSION), capabilities.version) ||
       !stringMatches(fake_egl::eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS), capabilities.clientExtensions) ||
       !stringMatches(fake_egl::eglQueryString(display, EGL_EXTENSIONS), capabilities.displayExtensions) ||
       !stringMatches(fake_egl::eglQueryString(display, EGL_CLIENT_APIS), capabilities.clientApis)) {
        return false;
    }
    if(capabilities.configurationCount > static_cast<std::size_t>(std::numeric_limits<EGLint>::max())) {
        return false;
    }
    if(capabilities.configurationCount != 0 && capabilities.configurations == nullptr) {
        return false;
    }

    EGLint reportedCount = -1;
    if(fake_egl::eglGetConfigs(display, nullptr, 0, &reportedCount) != EGL_TRUE ||
       reportedCount != static_cast<EGLint>(capabilities.configurationCount)) {
        return false;
    }
    std::vector<EGLConfig> configurations(capabilities.configurationCount, nullptr);
    EGLint returnedCount = -1;
    if(fake_egl::eglGetConfigs(display, configurations.data(), reportedCount, &returnedCount) != EGL_TRUE ||
       returnedCount != reportedCount) {
        return false;
    }
    std::vector<EGLConfig> chosenConfigurations(capabilities.configurationCount, nullptr);
    EGLint chosenCount = -1;
    if(fake_egl::eglChooseConfig(display, nullptr, chosenConfigurations.data(), reportedCount,
                                 &chosenCount) != EGL_TRUE ||
       chosenCount != reportedCount || chosenConfigurations != configurations) {
        return false;
    }

    for(std::size_t index = 0; index < capabilities.configurationCount; ++index) {
        const auto& expected = capabilities.configurations[index];
        struct AttributeExpectation {
            EGLint attribute;
            EGLint value;
        };
        const AttributeExpectation attributes[] = {
            {EGL_CONFIG_ID, expected.configId},
            {EGL_CONFIG_CAVEAT, expected.configCaveat},
            {EGL_COLOR_BUFFER_TYPE, expected.colorBufferType},
            {EGL_BUFFER_SIZE, expected.bufferSize},
            {EGL_RED_SIZE, expected.redSize},
            {EGL_GREEN_SIZE, expected.greenSize},
            {EGL_BLUE_SIZE, expected.blueSize},
            {EGL_ALPHA_SIZE, expected.alphaSize},
            {EGL_LUMINANCE_SIZE, expected.luminanceSize},
            {EGL_ALPHA_MASK_SIZE, expected.alphaMaskSize},
            {EGL_DEPTH_SIZE, expected.depthSize},
            {EGL_STENCIL_SIZE, expected.stencilSize},
            {EGL_SAMPLE_BUFFERS, expected.sampleBuffers},
            {EGL_SAMPLES, expected.samples},
            {EGL_SURFACE_TYPE, expected.surfaceType},
            {EGL_RENDERABLE_TYPE, expected.renderableType},
            {EGL_CONFORMANT, expected.conformant},
            {EGL_NATIVE_RENDERABLE, static_cast<EGLint>(expected.nativeRenderable)},
            {EGL_NATIVE_VISUAL_ID, expected.nativeVisualId},
            {EGL_NATIVE_VISUAL_TYPE, expected.nativeVisualType}
        };
        for(const auto& attribute : attributes) {
            EGLint actual = std::numeric_limits<EGLint>::min();
            if(fake_egl::eglGetConfigAttrib(display, configurations[index], attribute.attribute,
                                            &actual) != EGL_TRUE || actual != attribute.value) {
                return false;
            }
        }
    }
    return true;
}

void FakeEGL::addSwapBuffersCallback(void *user, void (*callback)(void *user, EGLDisplay display, EGLSurface surface)) {
    swapBuffersCallbacksLock.lock();
    swapBuffersCallbacks.emplace_back(SwapBuffersCallback{.user = user, .callback = callback});
    swapBuffersCallbacksLock.unlock();
}

void FakeEGL::installLibrary() {
    auto capabilityInfo = getCapabilityInfo();
    Log::info("FakeEGL", "Installing synthetic EGL %s with %zu reported configuration(s)",
              capabilityInfo.version, capabilityInfo.configurationCount);
    std::unordered_map<std::string, void *> syms;
    syms["eglInitialize"] = (void *)fake_egl::eglInitialize;
    syms["eglTerminate"] = (void *)fake_egl::eglTerminate;
    syms["eglGetError"] = (void *)fake_egl::eglGetError;
    syms["eglQueryString"] = (void *)fake_egl::eglQueryString;
    syms["eglGetDisplay"] = (void *)fake_egl::eglGetDisplay;
    syms["eglGetCurrentDisplay"] = (void *)fake_egl::eglGetCurrentDisplay;
    syms["eglGetCurrentContext"] = (void *)fake_egl::eglGetCurrentContext;
    syms["eglGetConfigs"] = (void *)fake_egl::eglGetConfigs;
    syms["eglChooseConfig"] = (void *)fake_egl::eglChooseConfig;
    syms["eglGetConfigAttrib"] = (void *)fake_egl::eglGetConfigAttrib;
    syms["eglCreateWindowSurface"] = (void *)fake_egl::eglCreateWindowSurface;
    syms["eglDestroySurface"] = (void *)fake_egl::eglDestroySurface;
    syms["eglCreateContext"] = (void *)fake_egl::eglCreateContext;
    syms["eglDestroyContext"] = (void *)fake_egl::eglDestroyContext;
    syms["eglMakeCurrent"] = (void *)fake_egl::eglMakeCurrent;
    syms["eglSwapBuffers"] = (void *)fake_egl::eglSwapBuffers;
    syms["eglSwapInterval"] = (void *)fake_egl::eglSwapInterval;
    syms["eglQuerySurface"] = (void *)fake_egl::eglQuerySurface;
    syms["eglGetProcAddress"] = (void *)fake_egl::eglGetProcAddressExport;
    syms["eglWaitClient"] = (void *)fake_egl::eglWaitClient;
    linker::load_library("libEGL.so", syms);
}

void FakeEGL::setupGLOverrides() {
#ifdef USE_ARMHF_SUPPORT
    ArmhfSupport::install(fake_egl::hostProcOverrides);
#endif
    // fake_egl::hostProcOverrides["glViewport"] = (void *)+[](int x,
    // int y,
    // int width,
    // int height) {
    //     ((void (*)(int x,
    // int y,
    // int width,
    // int height))(fake_egl::hostProcAddrFn("glViewport")))(x, y, width, height);

    // };
    // MESA 23.1 blackscreen Workaround Start for 1.18.30+, bgfy will disable the extension and the game works
    fake_egl::hostProcOverrides["glDrawElementsInstancedOES"] = nullptr;
    fake_egl::hostProcOverrides["glDrawArraysInstancedOES"] = nullptr;
    fake_egl::hostProcOverrides["glVertexAttribDivisorOES"] = nullptr;
    // MESA 23.1 blackscreen Workaround End
    fake_egl::hostProcOverrides["glInvalidateFramebuffer"] = (void *)+[]() {};  // Stub for a NVIDIA bug
    if(FakeEGL::enableTexturePatch) {
        // Minecraft Intel/Amd Texture Bug 1.16.210-1.17.2 and beyond
        // This patch reduces the visual glitch of blocks, does not work with high resolution textures
        // TODO improve Bugdetection
        fake_egl::hostProcOverrides["glTexSubImage2D"] = (void *)+[](unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, unsigned int type, const void *data) {
            if(width == 1024 && height == 1024) {
                size_t z = 0;
                for(long long y = 0; y < height; ++y) {
                    if(*((int32_t *)data + 987 + y * width) == *((int32_t *)data + 988 + y * width) && *((int32_t *)data + 988 + y * width) == *((int32_t *)data + 989 + y * width) && *((int32_t *)data + 989 + y * width) == *((int32_t *)data + 990 + y * width) && *((int32_t *)data + 990 + y * width) != *((int32_t *)data + 991 + y * width)) {
                        z++;
                    }
                }
                if(z >= 64) {
                    for(long long y = 0; y < 32; ++y) {
                        memmove((char *)data + y * width * 4 + 32 * 4, (char *)data + y * width * 4 + 31 * 4, width * 4 - 32 * 4);
                    }
                    for(long long y = height - 2; y >= 31; --y) {
                        memcpy((char *)data + (y + 1) * width * 4 + 32 * 4, (char *)data + y * width * 4 + 31 * 4, width * 4 - 32 * 4);
                        memcpy((char *)data + (y + 1) * width * 4, (char *)data + y * width * 4, 32 * 4);
                    }
                }
            }
            if(width == 2048 && height == 1024) {
                if(*((int32_t *)data + 989 + 1024) == *((int32_t *)data + 990 + 1024) && *((int32_t *)data + 990 + 1024) != *((int32_t *)data + 991 + 1024)) {
                    for(long long y = 0; y < 32; ++y) {
                        memmove((char *)data + y * width * 4 + 32 * 4, (char *)data + y * width * 4 + 31 * 4, width * 4 - 32 * 4);
                    }
                    for(long long y = height - 2; y >= 31; --y) {
                        memcpy((char *)data + (y + 1) * width * 4 + 32 * 4, (char *)data + y * width * 4 + 31 * 4, width * 4 - 32 * 4);
                        memcpy((char *)data + (y + 1) * width * 4, (char *)data + y * width * 4, 32 * 4);
                    }
                }
            }

            if(width == 512 && height == 512) {
                size_t uscore = 0;
                size_t itemscorea = 0, itemscoreb = 0, itemscorec = 0, itemscored = 0;
                for(int y = 0; y < height; ++y) {
                    if(*((uint32_t *)data + y * width + 511 - 14) != 0) {
                        ++itemscorea;
                    }
                    if(*((uint32_t *)data + y * width + 511 - 13) != 0) {
                        ++itemscoreb;
                    }
                    if(*((uint32_t *)data + y * width + 511 - 12) != 0) {
                        ++itemscorec;
                    }
                    if(*((uint32_t *)data + y * width + 511 - 11) == 0) {
                        ++itemscored;
                    }
                }
                for(int x = 0; x < width; ++x) {
                    if(*((uint32_t *)data + 1 * width + x) != 0) {
                        ++uscore;
                    }
                }
                size_t z = 0;
                for(long long y = 0; y < height; ++y) {
                    if(*((int32_t *)data + 511 - 20 + y * width) == *((int32_t *)data + 511 - 19 + y * width) && *((int32_t *)data + 511 - 19 + y * width) == *((int32_t *)data + 511 - 18 + y * width) && *((int32_t *)data + 511 - 18 + y * width) == *((int32_t *)data + 511 - 17 + y * width) && *((int32_t *)data + 511 - 17 + y * width) != *((int32_t *)data + 511 - 16 + y * width)) {
                        z++;
                    }
                }
                if(z >= 64 || (itemscorea > 64 && itemscoreb > 64 && itemscorec > 64 && itemscored > 64)) {
                    if(z >= 64 || uscore < 16) {
                        for(long long y = 0; y < 16; ++y) {
                            memmove((char *)data + y * width * 4 + 16 * 4, (char *)data + y * width * 4 + 15 * 4, width * 4 - 16 * 4);
                        }
                    } else {
                        for(long long y = 15; y >= 0; --y) {
                            memcpy((char *)data + (y + 1) * width * 4 + 16 * 4, (char *)data + y * width * 4 + 15 * 4, width * 4 - 16 * 4);
                        }
                    }
                    if(z >= 64) {
                        for(long long y = height - 2; y >= 16; --y) {
                            memcpy((char *)data + (y + 1) * width * 4 + 16 * 4, (char *)data + y * width * 4 + 15 * 4, width * 4 - 16 * 4);
                            memcpy((char *)data + (y + 1) * width * 4, (char *)data + y * width * 4, 16 * 4);
                        }
                    } else {
                        for(long long y = height - 2; y >= 16; --y) {
                            memcpy((char *)data + (y + 1) * width * 4 + 4, (char *)data + y * width * 4 + 0, width * 4 - 4);
                        }
                    }
                }
            }
            ((void (*)(unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, unsigned int type, const void *data))(fake_egl::hostProcAddrFn("glTexSubImage2D")))(target, level, xoffset, yoffset, width, height, format, type, data);
        };
    }
    GLCorePatch::installGL(fake_egl::hostProcOverrides, fake_egl::eglGetProcAddress);
}
