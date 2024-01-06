#include "fake_egl.h"
#include "gl_core_patch.h"
#include <map>

#define __ANDROID__
#include <EGL/egl.h>
#undef __ANDROID__
#include <log.h>
#include <cstring>
#include <game_window.h>
#include <game_window_manager.h>
#include <mcpelauncher/linker.h>
#ifdef USE_ARMHF_SUPPORT
#include "armhf_support.h"
#endif

#include <imgui.h>
#include <imgui_impl_opengl3.h>

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

static std::string g_IniFilename;

EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context) {
    if(draw != nullptr) {
        ((GameWindow *)draw)->makeCurrent(true);
        if(g_IniFilename.empty()) {
            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();

            // Redirect loading/saving of .ini file to our location.
            // Make sure 'g_IniFilename' persists while we use Dear ImGui.
            g_IniFilename = std::string("/tmp") + "/imgui.ini";
            io.IniFilename = g_IniFilename.c_str();;

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            //ImGui::StyleColorsLight();

            // Setup Platform/Renderer backends
            // ImGui_ImplAndroid_Init(g_App->window);
            // ImGuiIO& io = ImGui::GetIO();
            io.BackendPlatformName = "imgui_impl_android";

            ImGui_ImplOpenGL3_Init("#version 100");

            // Load Fonts
            // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
            // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
            // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
            // - Read 'docs/FONTS.md' for more instructions and details.
            // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
            // - Android: The TTF files have to be placed into the assets/ directory (android/app/src/main/assets), we use our GetAssetData() helper to retrieve them.

            // We load the default font with increased size to improve readability on many devices with "high" DPI.
            // FIXME: Put some effort into DPI awareness.
            // Important: when calling AddFontFromMemoryTTF(), ownership of font_data is transfered by Dear ImGui by default (deleted is handled by Dear ImGui), unless we set FontDataOwnedByAtlas=false in ImFontConfig
            ImFontConfig font_cfg;
            font_cfg.SizePixels = 22.0f;
            io.Fonts->AddFontDefault(&font_cfg);
            //void* font_data;
            //int font_data_size;
            //ImFont* font;
            //font_data_size = GetAssetData("segoeui.ttf", &font_data);
            //font = io.Fonts->AddFontFromMemoryTTF(font_data, font_data_size, 16.0f);
            //IM_ASSERT(font != nullptr);
            //font_data_size = GetAssetData("DroidSans.ttf", &font_data);
            //font = io.Fonts->AddFontFromMemoryTTF(font_data, font_data_size, 16.0f);
            //IM_ASSERT(font != nullptr);
            //font_data_size = GetAssetData("Roboto-Medium.ttf", &font_data);
            //font = io.Fonts->AddFontFromMemoryTTF(font_data, font_data_size, 16.0f);
            //IM_ASSERT(font != nullptr);
            //font_data_size = GetAssetData("Cousine-Regular.ttf", &font_data);
            //font = io.Fonts->AddFontFromMemoryTTF(font_data, font_data_size, 15.0f);
            //IM_ASSERT(font != nullptr);
            //font_data_size = GetAssetData("ArialUni.ttf", &font_data);
            //font = io.Fonts->AddFontFromMemoryTTF(font_data, font_data_size, 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
            //IM_ASSERT(font != nullptr);

            // Arbitrary scale-up
            // FIXME: Put some effort into DPI awareness
            ImGui::GetStyle().ScaleAllSizes(3.0f);
        }
    } else {
        ((GameWindow *)currentDrawSurface)->makeCurrent(false);
    }
    currentDrawSurface = draw;
    return EGL_TRUE;
}
#include <time.h>
static double                                   g_Time = 0.0;

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    //    Log::trace("FakeEGL", "eglSwapBuffers");
    ImGuiIO& io = ImGui::GetIO();
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    //ImGui_ImplAndroid_NewFrame();

        // Setup display size (every frame to accommodate for window resizing)
    int32_t window_width;
    int32_t window_height;
    ((GameWindow *)surface)->getWindowSize(window_width, window_height);
    int display_width = window_width;
    int display_height = window_height;

    io.DisplaySize = ImVec2((float)window_width, (float)window_height);
    if (window_width > 0 && window_height > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_width / window_width, (float)display_height / window_height);

    // Setup time step
    struct timespec current_timespec;
    clock_gettime(CLOCK_MONOTONIC, &current_timespec);
    double current_time = (double)(current_timespec.tv_sec) + (current_timespec.tv_nsec / 1000000000.0);
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
    g_Time = current_time;

    ImGui::NewFrame();

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    
    void (*glViewport)(int x, int y, int width, int height) = (decltype(glViewport)) GameWindowManager::getManager()->getProcAddrFunc()("glViewport");

    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    // glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    // glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // eglSwapBuffers(g_EglDisplay, g_EglSurface);

    ((GameWindow *)surface)->swapBuffers();
    return EGL_TRUE;
}

EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval) {
    ((GameWindow *)currentDrawSurface)->setSwapInterval(interval);
    return EGL_TRUE;
}

EGLBoolean eglQuerySurface(EGLDisplay display, EGLSurface surface, EGLint attribute, EGLint *value) {
    if(attribute == EGL_WIDTH || attribute == EGL_HEIGHT) {
        int w, h;
        ((GameWindow *)surface)->getWindowSize(w, h);
        *value = (attribute == EGL_WIDTH ? w : h);
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
