#include <log.h>
#include <dlfcn.h>
#include <game_window_manager.h>
#include <argparser.h>
#include <mcpelauncher/minecraft_utils.h>
#include <mcpelauncher/minecraft_version.h>
#include <mcpelauncher/crash_handler.h>
#include <mcpelauncher/path_helper.h>
#include <mcpelauncher/mod_loader.h>
#include "window_callbacks.h"
#include "splitscreen_patch.h"
#include "gl_core_patch.h"
#include "xbox_live_helper.h"
#include "shader_error_patch.h"
#include "hbui_patch.h"
#include "jni/jni_support.h"
#ifdef USE_ARMHF_SUPPORT
#include "armhf_support.h"
#endif
#if defined(__i386__) || defined(__x86_64__)
#include "cpuid.h"
#include "texel_aa_patch.h"
#include "xbox_shutdown_patch.h"
#endif
#include <build_info.h>
#include <mcpelauncher/patch_utils.h>
#include <libc_shim.h>
#include <mcpelauncher/linker.h>
#include <minecraft/imported/android_symbols.h>
#include "main.h"
#include "fake_looper.h"
#include "fake_assetmanager.h"
#include "fake_egl.h"
#include "symbols.h"
#include "core_patches.h"
#include "thread_mover.h"
#include "log.h"
 #include <unistd.h>
#include <minecraft/imported/egl_symbols.h>
#include <minecraft/imported/libm_symbols.h>
#include <minecraft/imported/fmod_symbols.h>
#include <jnivm.h>

static size_t base;
LauncherOptions options;

void printVersionInfo();

enum class AndroidLogPriority {
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT
};

static LogLevel convertAndroidLogLevel(int level) {
    if (level <= (int) AndroidLogPriority::ANDROID_LOG_VERBOSE)
        return LogLevel::LOG_TRACE;
    if (level == (int) AndroidLogPriority::ANDROID_LOG_DEBUG)
        return LogLevel::LOG_DEBUG;
    if (level == (int) AndroidLogPriority::ANDROID_LOG_INFO)
        return LogLevel::LOG_INFO;
    if (level == (int) AndroidLogPriority::ANDROID_LOG_WARN)
        return LogLevel::LOG_WARN;
    if (level >= (int) AndroidLogPriority::ANDROID_LOG_ERROR)
        return LogLevel::LOG_ERROR;
    return LogLevel::LOG_ERROR;
}

static void __android_log_vprint(int prio, const char *tag, const char *fmt, va_list args) {
    Log::vlog(convertAndroidLogLevel(prio), tag, fmt, args);
}
static void __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Log::vlog(convertAndroidLogLevel(prio), tag, fmt, args);
    va_end(args);
}
static void __android_log_write(int prio, const char *tag, const char *text) {
    Log::log(convertAndroidLogLevel(prio), tag, "%s", text);
}

#define hybris_hook(a, b) symbols[a] = b;

std::shared_ptr<GameWindow> window;

void InstallALooper(std::unordered_map<std::string, void *>& symbols) {
    struct Looper {
      int fd;
      int indent;
      void * data;
      int indent2;
      void * data2;
    };
    static Looper looper;
    symbols["ALooper_pollAll"] = (void *)+[](  int timeoutMillis,
    int *outFd,
    int *outEvents,
    void **outData) {
      fd_set rfds;
      struct timeval tv;
      int retval;

      /* Watch stdin (fd 0) to see when it has input. */

      FD_ZERO(&rfds);
      FD_SET(looper.fd, &rfds);

      tv.tv_sec = 0;
      tv.tv_usec = 0;

      retval = select(looper.fd + 1, &rfds, NULL, NULL, &tv);
      /* Don't rely on the value of tv now! */

      if (retval == -1)
          perror("select()");
      else if (retval) {
          // printf("Data is available now.\n");
          *outData = looper.data;
          return looper.indent;
          /* FD_ISSET(0, &rfds) will be true. */
      }
      if(window) {
        window->pollEvents();
      }

      return -3;
    };
    hybris_hook("ALooper_addFd", (void *)+[](  void *loopere ,
      int fd,
      int ident,
      int events,
      int(* callback)(int fd, int events, void *data),
      void *data) {
      looper.fd = fd;
      looper.indent = ident;
      looper.data = data;
      return 1;
    });
    hybris_hook("AInputQueue_attachLooper", (void *)+[](  void *queue,
    void *looper2,
    int ident,
    void* callback,
    void *data) {
      looper.indent2 = ident;
      looper.data2 = data;
    });
}

#define EGL_NONE 0x3038
#define EGL_TRUE 1
#define EGL_FALSE 0
#define EGL_WIDTH 0x3057
#define EGL_HEIGHT 0x3056
using EGLint = int;
using EGLDisplay = void*;
using EGLSurface = void*;
using EGLContext = void*;
using EGLConfig = void*;
using NativeWindowType = void*;
using NativeDisplayType = void*;

void CreateIfNeededWindow() {
    if(!window) {
        window = GameWindowManager::getManager()->createWindow("mcpelauncher "
#ifdef _LP64
"64"
#else
"32"
#endif
        "bit alpha", 1280, 720, GraphicsApi::OPENGL_ES2);
        window->show();
    }
}


#ifdef __i386__
#define ANDROID_ARCH "x86"
#elif defined(__x86_64__)
#define ANDROID_ARCH "x86_64"
#elif defined(__arm__)
#define ANDROID_ARCH "armeabi-v7a"
#elif defined(__aarch64__)
#define ANDROID_ARCH "arm64-v8a"
#endif
// extern "C" void sigsetjmp();

void InstallEGL(std::unordered_map<std::string, void *>& symbols) {
    hybris_hook("eglChooseConfig", (void *)+[](EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
      *num_config = 1;
      return EGL_TRUE;
    });
    hybris_hook("eglGetError", (void *)(void (*)())[]() {
    });
    hybris_hook("eglGetCurrentDisplay", (void *)+[]() -> EGLDisplay {
      return (EGLDisplay)1;
    });
    hybris_hook("eglCreateWindowSurface", (void *)+[](EGLDisplay display,
      EGLConfig config,
      NativeWindowType native_window,
      EGLint const * attrib_list) {
      return native_window;
    });
    hybris_hook("eglGetConfigAttrib", (void *)+[](EGLDisplay display,
      EGLConfig config,
      EGLint attribute,
      EGLint * value) {
      return EGL_TRUE;
    });
    hybris_hook("eglCreateContext", (void *)+[](EGLDisplay display,
      EGLConfig config,
      EGLContext share_context,
      EGLint const * attrib_list) {
    //   CreateIfNeededWindow();
      return 1;
    });
    hybris_hook("eglDestroySurface", (void *)(void (*)())[]() {
    });
    symbols["eglSwapBuffers"] = (void *)+[](EGLDisplay *display, EGLSurface surface) { 
      window->swapBuffers();
    };
    // hybris_hook("eglSwapBuffers", (void *)+[](EGLDisplay *display,
    //   EGLSurface surface) {
    //     window->swapBuffers();
    // });
    hybris_hook("eglMakeCurrent", (void *)+[](EGLDisplay display,
      EGLSurface draw,
      EGLSurface read,
      EGLContext context) {
      Log::warn("Launcher", "EGL stub %s called", "eglMakeCurrent");
    //   CreateIfNeededWindow();
      return EGL_TRUE;
    });
    hybris_hook("eglDestroyContext", (void *)(void (*)())[]() {
    });
    hybris_hook("eglTerminate", (void *)(void (*)())[]() {
    });
    hybris_hook("eglGetDisplay", (void *)+[](NativeDisplayType native_display) {
      return 1; 
    });
    hybris_hook("eglInitialize", (void *)+[](void* display, uint32_t * major, uint32_t * minor) {

      return EGL_TRUE;
    });
    hybris_hook("eglQuerySurface", (void *) + [](void* dpy, EGLSurface surface, EGLint attribute, EGLint *value) {
      int dummy;
    //   CreateIfNeededWindow();
      switch (attribute)
      {
      case EGL_WIDTH:
          window->getWindowSize(*value, dummy);
          break;
      case EGL_HEIGHT:
          window->getWindowSize(dummy, *value);
          break;
      default:
          return EGL_FALSE;
      }
      return EGL_TRUE;
    });
    hybris_hook("eglSwapInterval", (void *)+[](EGLDisplay display, EGLint interval) {
        window->setSwapInterval(interval);
      return EGL_TRUE;
    });
    hybris_hook("eglQueryString", (void *)+[](void* display, int32_t name) {
        return 0;
    });
    hybris_hook("eglGetProcAddress", ((void*)+[](char* ch)->void*{
      static std::unordered_map<std::string, void*> eglfuncs = {{ "glInvalidateFramebuffer", (void*)+[]() {}}};
      auto hook = eglfuncs[ch];
      if(!hook) {
          hook = ((void* (*)(const char*))GameWindowManager::getManager()->getProcAddrFunc())(ch);
      }
      return hook;
    }));
    hybris_hook("eglGetCurrentContext", (void*) + []() -> int {
      return 0;
    });
}
#include "window_callbacks.h"

int main(int argc, char *argv[]) {
    auto windowManager = GameWindowManager::getManager();
    CrashHandler::registerCrashHandler();
    MinecraftUtils::workaroundLocaleBug();

    argparser::arg_parser p;
    argparser::arg<bool> printVersion (p, "--version", "-v", "Prints version info");
    argparser::arg<std::string> gameDir (p, "--game-dir", "-dg", "Directory with the game and assets");
    argparser::arg<std::string> dataDir (p, "--data-dir", "-dd", "Directory to use for the data");
    argparser::arg<std::string> cacheDir (p, "--cache-dir", "-dc", "Directory to use for cache");
    argparser::arg<int> windowWidth (p, "--width", "-ww", "Window width", 720);
    argparser::arg<int> windowHeight (p, "--height", "-wh", "Window height", 480);
    argparser::arg<bool> disableFmod (p, "--disable-fmod", "-df", "Disables usage of the FMod audio library");
    if (!p.parse(argc, (const char**) argv))
        return 1;
    if (printVersion) {
        printVersionInfo();
        return 0;
    }
    options.windowWidth = windowWidth;
    options.windowHeight = windowHeight;
    options.graphicsApi = GLCorePatch::mustUseDesktopGL() ? GraphicsApi::OPENGL : GraphicsApi::OPENGL_ES2;

    if (!gameDir.get().empty())
        PathHelper::setGameDir(gameDir);
    if (!dataDir.get().empty())
        PathHelper::setDataDir(dataDir);
    if (!cacheDir.get().empty())
        PathHelper::setCacheDir(cacheDir);
    CreateIfNeededWindow();
    linker::init();
    std::unordered_map<std::string, void *> symbols;
    auto h = dlopen("libm."
#ifdef __APPLE__
    "dylib"
#else
    "so.1"
#endif
    , RTLD_LAZY);

    auto libcshim = shim::get_shimmed_symbols();
    for (auto && item : libcshim) {
        symbols[item.name] = item.value;
    }

    for (size_t i = 0; libm_symbols[i]; i++) {
        symbols[libm_symbols[i]] = dlsym(h, libm_symbols[i]);
    }
symbols["__cxa_pure_virtual"] = (void*) +[]() {

};
symbols["__cxa_guard_acquire"] = (void*) +[]() {

};
symbols["__cxa_guard_release"] = (void*) +[]() {

};
symbols["setpriority"] = (void*) +[]() {

};
    //  symbols["__libc_init"] = (void*)+ []() {

    //  };
    //  symbols["isascii"] = (void*)isascii;
    //  symbols["sigsetjmp"] = (void*)__sigsetjmp;
    //  symbols["siglongjmp"] = (void*)siglongjmp;
    //  symbols["wprintf"] = (void*)wprintf;
    //  symbols["sigfillset"] = (void*)sigfillset;
    //  symbols["pthread_sigmask"] = (void*)pthread_sigmask;
    //  symbols["lstat"] = (void*)lstat;
    //  symbols["statfs"] = (void*)statfs;
     
    
    // linker::load_library("libhybris.so", symbols);
    // linker::load_library("libdl.so", { { std::string("dl_iterate_phdr"), (void*)&__loader_dl_iterate_phdr },
    //                                    { std::string("dlopen"), (void*)+ [](const char * filename, int flags)-> void* {
    //                                        return __loader_dlopen(filename, flags, nullptr);
    //                                    }},
    //                                    { std::string("dlsym"), (void*)+ [](void* dl, const char * name)-> void* {
    //                                        return __loader_dlsym(dl, name, nullptr);
    //                                    }},
    //                                    { std::string("dlclose"), (void*)&__loader_dlclose },
    //                                    { std::string("dlerror"), (void*)&__loader_dlerror},
                                       
    //                                     });
    // linker::load_library("libdl.so.2", { { std::string("dl_iterate_phdr"), (void*)&__loader_dl_iterate_phdr },
    //                                    { std::string("dlopen"), (void*)+ [](const char * filename, int flags)-> void* {
    //                                        return __loader_dlopen(filename, flags, nullptr);
    //                                    }},
    //                                    { std::string("dlsym"), (void*)+ [](void* dl, const char * name)-> void* {
    //                                        return __loader_dlsym(dl, name, nullptr);
    //                                    }},
    //                                    { std::string("dlclose"), (void*)&__loader_dlclose },
    //                                    { std::string("dlerror"), (void*)&__loader_dlerror},
                                       
    //                                     });
    symbols["_ZN6cohtml17VerifiyLicenseKeyEPKc"] = (void*) + []() {
        return true;
    };
    symbols["_ZN3web4http6client7details35verify_cert_chain_platform_specificERN5boost4asio3ssl14verify_contextERKSs"] = (void*) + []() {
        return true;
    };

    static std::promise<std::pair<void *(*)(void*), void *>> pthread_main;
    auto fut = pthread_main.get_future();
    static std::atomic_bool run_pthread_main(true);
    static pthread_t pthread_main_v = pthread_self();
    static auto oldpthread_create = (int(*)(pthread_t *thread, const pthread_attr_t *__attr, void *(*start_routine)(void*), void *arg))symbols["pthread_create"];
    symbols["pthread_create"] = (void*) + [](pthread_t *thread, const pthread_attr_t *__attr, void *(*start_routine)(void*), void *arg) -> int {
        if(run_pthread_main.load()) {
            run_pthread_main.store(false);
            *thread = pthread_main_v;
            pthread_main.set_value({start_routine, arg});
            return 0;
        }
        return oldpthread_create(thread, __attr, start_routine, arg);
    };
    // Hack pthread to run mainthread on the main function #macoscacoa support
    // static std::atomic_bool uithread_started;
    // uithread_started = false;
    // static void *(*main_routine)(void*) = nullptr;
    // static void *main_arg = nullptr;
    // static pthread_t mainthread = pthread_self();
    // static int (*my_pthread_create)(pthread_t *thread, const pthread_attr_t *__attr,
    //                          void *(*start_routine)(void*), void *arg) = 0;
    // // my_pthread_create = (int (*)(pthread_t *thread, const pthread_attr_t *__attr,
    // //                          void *(*start_routine)(void*), void *arg))get_hooked_symbol("pthread_create");
    // hybris_hook("pthread_create", (void*) + [](pthread_t *thread, const pthread_attr_t *__attr,
    //     void *(*start_routine)(void*), void *arg) {
    //     if(uithread_started.load()) {
    //       return my_pthread_create(thread, __attr, start_routine, arg);
    //     } else {
    //       uithread_started = true;
    //       *thread = mainthread;
    //       main_routine = start_routine;
    //       main_arg = arg;
    //       return 0;
    //     }
    //   }
    // );
    // symbols["pthread_create"] = (void*) my_pthread_create;
    linker::load_library("libc.so", symbols);
    linker::load_library("libc.so.6", symbols);
    // linker::load_empty_library("libpthread.so.0");
    // symbols.clear();
    // auto h = dlopen("libm.so.6", RTLD_LAZY);
    // for (size_t i = 0; libm_symbols[i]; i++) {
    //     symbols[libm_symbols[i]] = dlsym(h, libm_symbols[i]);
    // }
    linker::load_library("libm.so", /* symbols */ {});
    symbols.clear();
    for (size_t i = 0; egl_symbols[i]; i++) {
        symbols[egl_symbols[i]] = (void*)+[]() {
            // std::cout << "egl_symbols Stub called" << "\n";
        };
    }
    symbols["eglGetProcAddress"] = (void*) + [](const char* name) -> void* {
        return nullptr;
    };
    InstallEGL(symbols);
    linker::load_library("libEGL.so", symbols);
    symbols.clear();
    symbols["__android_log_print"] = (void*) __android_log_print;
    symbols["__android_log_vprint"] = (void*) __android_log_vprint;
    symbols["__android_log_write"] = (void*) __android_log_write;

    linker::load_library("liblog.so", symbols);
    symbols.clear();
    for (size_t i = 0; android_symbols[i]; i++) {
        symbols[android_symbols[i]] = (void*)+[]() {
            
        };
    }
    InstallALooper(symbols);
    symbols["ANativeActivity_finish"] = (void *)+[](ANativeActivity *activity) {
      Log::warn("Launcher", "Android %s called", "ANativeActivity_finish");
      _Exit(0);
    };
    linker::load_library("libandroid.so", symbols);
    linker::load_library("libOpenSLES.so", { });
    (void)chdir((PathHelper::getGameDir() + "/assets").data());
    auto libcpp =  __loader_dlopen("../lib/" ANDROID_ARCH "/libc++_shared.so", 0, 0);
    if(!libcpp) {
        libcpp = __loader_dlopen("../lib/" ANDROID_ARCH "/libgnustl_shared.so", 0, 0);
    }
    symbols.clear();
    for (size_t i = 0; fmod_symbols[i]; i++) {
        symbols[fmod_symbols[i]] = (void*)+[]() {
            
        };
    }
    linker::load_library("libstdc++.so", symbols);
    // auto libcrypro = __loader_dlopen("./libcrypto.so", 0, 0);
    // auto libssl = __loader_dlopen("./libssl.so", 0, 0);
    // __loader_dlopen("../lib/" ANDROID_ARCH "/libfmod.so", 0, 0);
    MinecraftUtils::loadFMod();
    static void * libmcpe = __loader_dlopen("../lib/" ANDROID_ARCH "/libminecraftpe.so", 0, 0);
    // if(!libmcpe) {
    //     std::cout << "Please change the current working directory to the assets folder.\nOn linux e.g \"cd ~/.local/share/mcpelauncher/versions/1.16.0.55/assets\"\n";
    //     return -1;
    // }.
    CorePatches::install(libmcpe);
    linker::patch(libmcpe);
    CorePatches::setGameWindow(window);
    JniSupport sup;
    FakeInputQueue q;
    sup.registerMinecraftNatives([](const char* s) {
        return __loader_dlsym(libmcpe, s, nullptr);
    });
    auto vm = &sup.vm;
    auto mainActivity = std::make_shared<MainActivity>();
    auto MainActivity_ = vm->GetEnv()->GetClass("com/mojang/minecraftpe/MainActivity");
    mainActivity->clazz = MainActivity_;
    mainActivity->storageDirectory = dataDir;
    mainActivity->textInput = &sup.textInput;

    auto JNI_OnLoad = (jint (*)(JavaVM* vm, void* reserved))__loader_dlsym(libmcpe, "JNI_OnLoad", nullptr);

    auto ver = JNI_OnLoad(vm->GetJavaVM(), nullptr);
    ANativeActivity activity;
    ANativeActivityCallbacks callbacks;
    memset(&activity, 0, sizeof(ANativeActivity));
    activity.internalDataPath = "../data/";
    activity.externalDataPath = "../data/";
    activity.obbPath = "../data/";
    activity.sdkVersion = 28;
    activity.vm = vm->GetJavaVM();
    activity.clazz = (jobject)mainActivity.get();
    // activity.assetManager = (struct AAssetManager*)23;
    memset(&callbacks, 0, sizeof(ANativeActivityCallbacks));
    activity.callbacks = &callbacks;
    activity.vm->GetEnv(&(void*&)activity.env, 0);
    auto nativeRegisterThis = (void(*)(JNIEnv * env, void*))__loader_dlsym(libmcpe, "Java_com_mojang_minecraftpe_MainActivity_nativeRegisterThis", 0);
    if(nativeRegisterThis)
        nativeRegisterThis(activity.env, activity.clazz);
    std::thread starter([&]() {
        auto ANativeActivity_onCreate = (ANativeActivity_createFunc*)__loader_dlsym(libmcpe, "ANativeActivity_onCreate", 0);
        ANativeActivity_onCreate(&activity, nullptr, 0);
        activity.callbacks->onInputQueueCreated(&activity, (AInputQueue *)1);
        activity.callbacks->onNativeWindowCreated(&activity, (ANativeWindow *)window.get());
        activity.callbacks->onStart(&activity);
        activity.callbacks->onResume(&activity);
    });
    auto run_main = fut.get();
    SymbolsHelper::initSymbols(libmcpe);
    sup.activity = mainActivity;
    WindowCallbacks callbacksc(*window, sup, q);
    callbacksc.registerCallbacks();
    // window->prepareRunLoop();
    run_main.first(run_main.second);
    return 0;
}

void printVersionInfo() {
    printf("mcpelauncher-client %s / manifest %s\n", CLIENT_GIT_COMMIT_HASH, MANIFEST_GIT_COMMIT_HASH);
#if defined(__i386__) || defined(__x86_64__)
    CpuId cpuid;
    printf("CPU: %s %s\n", cpuid.getManufacturer(), cpuid.getBrandString());
    printf("SSSE3 support: %s\n", cpuid.queryFeatureFlag(CpuId::FeatureFlag::SSSE3) ? "YES" : "NO");
#endif
    auto windowManager = GameWindowManager::getManager();
    GraphicsApi graphicsApi = GLCorePatch::mustUseDesktopGL() ? GraphicsApi::OPENGL : GraphicsApi::OPENGL_ES2;
    auto window = windowManager->createWindow("mcpelauncher", 32, 32, graphicsApi);
    auto glGetString = (const char* (*)(int)) windowManager->getProcAddrFunc()("glGetString");
    printf("GL Vendor: %s\n", glGetString(0x1F00 /* GL_VENDOR */));
    printf("GL Renderer: %s\n", glGetString(0x1F01 /* GL_RENDERER */));
    printf("GL Version: %s\n", glGetString(0x1F02 /* GL_VERSION */));
    printf("MSA daemon path: %s\n", XboxLiveHelper::findMsa().c_str());
}