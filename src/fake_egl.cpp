#include "fake_egl.h"
#include "gl_core_patch.h"
#include "settings.h"
#include "imgui_ui.h"
#include <map>
#include <regex>

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

namespace fake_egl {

static thread_local EGLSurface currentDrawSurface;
static void *(*hostProcAddrFn)(const char *);
static std::unordered_map<std::string, void *> hostProcOverrides;

EGLBoolean eglInitialize(EGLDisplay display, EGLint *major, EGLint *minor) {
    if(major)
        *major = 1;
    if(minor)
        *minor = 5;
    return EGL_TRUE;
}

EGLBoolean eglTerminate(EGLDisplay display) {
    return EGL_TRUE;
}

EGLint eglGetError() {
    return EGL_SUCCESS;
}

char const *eglQueryString(EGLDisplay display, EGLint name) {
    if(name == EGL_VENDOR)
        return "mcpelauncher";
    if(name == EGL_VERSION)
        return "1.5 mcpelauncher";
    if(name == EGL_EXTENSIONS)
        return "";
    Log::warn("FakeEGL", "eglQueryString %x", name);
    return nullptr;
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType dp) {
    return (EGLDisplay *)1;
}

EGLDisplay eglGetCurrentDisplay() {
    return (EGLDisplay *)1;
}

EGLContext eglGetCurrentContext() {
    return currentDrawSurface ? (EGLContext *)1 : (EGLContext *)0;
}

EGLBoolean eglChooseConfig(EGLDisplay display, EGLint const *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    *num_config = 1;
    return EGL_TRUE;
}

EGLBoolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value) {
    if(attribute == EGL_NATIVE_VISUAL_ID) {
        *value = 0;
        return EGL_TRUE;
    }
    if(attribute == EGL_RED_SIZE || attribute == EGL_GREEN_SIZE || attribute == EGL_BLUE_SIZE || attribute == EGL_ALPHA_SIZE || attribute == EGL_DEPTH_SIZE || attribute == EGL_STENCIL_SIZE) {
        *value = 8;
        return EGL_TRUE;
    }
    Log::warn("FakeEGL", "eglGetConfigAttrib %x", attribute);
    return EGL_TRUE;
}

EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window, EGLint const *attrib_list) {
    return native_window;
}

EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface) {
    return EGL_TRUE;
}

EGLContext eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share_context, EGLint const *attrib_list) {
    return (EGLContext *)1;
}

EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context) {
    return EGL_TRUE;
}

EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context) {
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

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
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

EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval) {
    //((GameWindow *)currentDrawSurface)->setSwapInterval(interval);
    return EGL_TRUE;
}

EGLBoolean eglQuerySurface(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint *value) {
    if(attribute == EGL_WIDTH || attribute == EGL_HEIGHT) {
        int w, h;
        ((GameWindow *)surface)->getWindowSize(w, h);
        *value = (attribute == EGL_WIDTH ? w : h - Settings::menubarsize.load());
        return EGL_TRUE;
    }
    Log::warn("FakeEGL", "eglQuerySurface %x", attribute);
    return EGL_TRUE;
}

void *eglGetProcAddress(const char *name) {
    auto it = hostProcOverrides.find(name);
    if(it != hostProcOverrides.end())
        return it->second;
    return hostProcAddrFn(name);
}

}  // namespace fake_egl

bool FakeEGL::enableTexturePatch = false;

void FakeEGL::setProcAddrFunction(void *(*fn)(const char *)) {
    fake_egl::hostProcAddrFn = fn;
}

void FakeEGL::addSwapBuffersCallback(void *user, void (*callback)(void *user, EGLDisplay display, EGLSurface surface)) {
    swapBuffersCallbacksLock.lock();
    swapBuffersCallbacks.emplace_back(SwapBuffersCallback{.user = user, .callback = callback});
    swapBuffersCallbacksLock.unlock();
}

void FakeEGL::installLibrary() {
    std::unordered_map<std::string, void *> syms;
    syms["eglInitialize"] = (void *)fake_egl::eglInitialize;
    syms["eglTerminate"] = (void *)fake_egl::eglTerminate;
    syms["eglGetError"] = (void *)fake_egl::eglGetError;
    syms["eglQueryString"] = (void *)fake_egl::eglQueryString;
    syms["eglGetDisplay"] = (void *)fake_egl::eglGetDisplay;
    syms["eglGetCurrentDisplay"] = (void *)fake_egl::eglGetCurrentDisplay;
    syms["eglGetCurrentContext"] = (void *)fake_egl::eglGetCurrentContext;
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
    syms["eglGetProcAddress"] = (void *)fake_egl::eglGetProcAddress;
    syms["eglWaitClient"] = (void *)+[]() -> EGLBoolean {
        return EGL_TRUE;
    };
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

    // NVIDIA GL_MAX_IMAGE_UNITS fix: remap image binding values >= 8 to val % 8
    // ONLY on lines declaring image uniforms (image2D, image2DArray, etc.)
    // Sampler and UBO bindings must NOT be remapped (higher limits)
    fake_egl::hostProcOverrides["glShaderSource"] = (void *)+[](unsigned int shader, int count, const char *const *string, const int *length) {
        static bool logged = false;
        auto realFn = (void (*)(unsigned int, int, const char *const *, const int *))fake_egl::hostProcAddrFn("glShaderSource");
        std::vector<std::string> modifiedSources(count);
        std::vector<const char *> newPtrs(count);
        std::vector<int> newLens(count);
        std::regex bindingRe(R"((binding\s*=\s*)(\d+))");
        bool anyChanged = false;
        for (int i = 0; i < count; i++) {
            std::string src;
            if (length && length[i] >= 0) {
                src = std::string(string[i], length[i]);
            } else {
                src = std::string(string[i]);
            }
            std::string result;
            size_t pos = 0;
            while (pos < src.size()) {
                size_t eol = src.find('\n', pos);
                if (eol == std::string::npos) eol = src.size();
                std::string line = src.substr(pos, eol - pos);
                // Only remap bindings on lines declaring image uniforms
                if (line.find("image") != std::string::npos) {
                    std::string newLine;
                    std::sregex_iterator it(line.begin(), line.end(), bindingRe), end;
                    size_t lastPos = 0;
                    for (; it != end; ++it) {
                        auto &m = *it;
                        int val = std::stoi(m[2].str());
                        newLine.append(line, lastPos, m.position() - lastPos);
                        if (val >= 8) {
                            newLine.append(m[1].str() + std::to_string(val % 8));
                            anyChanged = true;
                        } else {
                            newLine.append(m[0].str());
                        }
                        lastPos = m.position() + m.length();
                    }
                    newLine.append(line, lastPos, line.size() - lastPos);
                    result += newLine;
                } else {
                    result += line;
                }
                if (eol < src.size()) result += '\n';
                pos = eol + 1;
            }
            modifiedSources[i] = std::move(result);
            newPtrs[i] = modifiedSources[i].c_str();
            newLens[i] = (int)modifiedSources[i].size();
        }
        if (anyChanged && !logged) {
            Log::info("FakeEGL", "Remapped shader image binding >= 8 to val %% 8 (NVIDIA fix, image-only)");
            logged = true;
        }
        realFn(shader, count, newPtrs.data(), newLens.data());
    };

    fake_egl::hostProcOverrides["glBindImageTexture"] = (void *)+[](unsigned int unit, unsigned int texture, int level, unsigned char layered, int layer, unsigned int access, unsigned int format) {
        static bool logged = false;
        auto realFn = (void (*)(unsigned int, unsigned int, int, unsigned char, int, unsigned int, unsigned int))fake_egl::hostProcAddrFn("glBindImageTexture");
        if (unit >= 8) {
            if (!logged) {
                Log::info("FakeEGL", "Remapped glBindImageTexture unit %u -> %u (NVIDIA fix)", unit, unit % 8);
                logged = true;
            }
            unit = unit % 8;
        }
        realFn(unit, texture, level, layered, layer, access, format);
    };

    fake_egl::hostProcOverrides["glCompileShader"] = (void *)+[](unsigned int shader) {
        auto realCompile = (void (*)(unsigned int))fake_egl::hostProcAddrFn("glCompileShader");
        auto realGetiv = (void (*)(unsigned int, unsigned int, int *))fake_egl::hostProcAddrFn("glGetShaderiv");
        auto realGetLog = (void (*)(unsigned int, int, int *, char *))fake_egl::hostProcAddrFn("glGetShaderInfoLog");
        realCompile(shader);
        int status = 0;
        realGetiv(shader, 0x8B81 /*GL_COMPILE_STATUS*/, &status);
        if (!status) {
            char buf[2048];
            int len = 0;
            realGetLog(shader, sizeof(buf), &len, buf);
            Log::error("FakeEGL", "Shader compile error: %.*s", len, buf);
        }
    };

    fake_egl::hostProcOverrides["glLinkProgram"] = (void *)+[](unsigned int program) {
        auto realLink = (void (*)(unsigned int))fake_egl::hostProcAddrFn("glLinkProgram");
        auto realGetiv = (void (*)(unsigned int, unsigned int, int *))fake_egl::hostProcAddrFn("glGetProgramiv");
        auto realGetLog = (void (*)(unsigned int, int, int *, char *))fake_egl::hostProcAddrFn("glGetProgramInfoLog");
        realLink(program);
        int status = 0;
        realGetiv(program, 0x8B82 /*GL_LINK_STATUS*/, &status);
        if (!status) {
            char buf[2048];
            int len = 0;
            realGetLog(program, sizeof(buf), &len, buf);
            Log::error("FakeEGL", "Program link error: %.*s", len, buf);
        }
    };

    GLCorePatch::installGL(fake_egl::hostProcOverrides, fake_egl::eglGetProcAddress);
}
