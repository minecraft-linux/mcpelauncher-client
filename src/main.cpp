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
#include "jni/store.h"
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
#include "fake_window.h"
#include "fake_assetmanager.h"
#include "fake_egl.h"
#include "symbols.h"
#include "core_patches.h"
#include "thread_mover.h"
#include <FileUtil.h>
#include <properties/property.h>
#include <fstream>
#include "glad/glad.h"
// For getpid
#include <unistd.h>
#include <simpleipc/client/service_client.h>
#include <daemon_utils/auto_shutdown_service.h>
#include "settings.h"
#include "imgui_ui.h"

struct RpcCallbackServer : daemon_utils::auto_shutdown_service {

    RpcCallbackServer(const std::string &path, JniSupport& support) : daemon_utils::auto_shutdown_service(path, daemon_utils::shutdown_policy::never) {
        add_handler_async("mcpelauncher-client/sendfile", [this, &support](simpleipc::connection& conn, std::string const& method, nlohmann::json const& data, result_handler const& cb) {
            std::vector<std::string> files = data;
            for(auto&& file : files) {
                if (file.rfind("minecraft://", 0) == 0) {
                    support.sendUri(file);
                } else {
                    support.importFile(file);
                }
            }
            cb(simpleipc::rpc_json_result::response({}));
        });
    }
};

static size_t base;
LauncherOptions options;

void printVersionInfo();

void loadGameOptions();

int main(int argc, char* argv[]) {
    if(argc == 2 && argv[1][0] != '-') {
        Log::info("Sendfile", "sending file");
        simpleipc::client::service_client sc(PathHelper::getPrimaryDataDirectory() + "file_handler");
        std::vector<std::string> files = { argv[1] };
        static std::mutex mlock;
        mlock.lock();
        auto call = simpleipc::client::rpc_call<int>(sc.rpc("mcpelauncher-client/sendfile", files), [](const nlohmann::json &res) {
            Log::info("Sendfile", "success");
            mlock.unlock();
            return 0;
        });
        call.call();
        mlock.lock();
        return 0;
    }

    CrashHandler::registerCrashHandler();
    MinecraftUtils::workaroundLocaleBug();

    argparser::arg_parser p;
    argparser::arg<bool> printVersion(p, "--version", "-v", "Prints version info");
    argparser::arg<std::string> gameDir(p, "--game-dir", "-dg", "Directory with the game and assets");
    argparser::arg<std::string> dataDir(p, "--data-dir", "-dd", "Directory to use for the data");
    argparser::arg<std::string> cacheDir(p, "--cache-dir", "-dc", "Directory to use for cache");
    argparser::arg<std::string> importFilePath(p, "--import-file-path", "-ifp", "File to import to the game");
    argparser::arg<std::string> sendUri(p, "--uri", "-u", "URI to send to the game");
    argparser::arg<std::string> v8Flags(p, "--v8-flags", "-v8f", "Flags to pass to the v8 engine of the game",
#if defined(__APPLE__) && defined(__aarch64__)
        // Due to problems with running JIT compiled code without using apple specfic workarounds, we just run javascript via jitless
        "--jitless"
#else
        ""
#endif
    );
    argparser::arg<bool> webrtcdebug(p, "--webrtc-debug", "-drtc", "Enable webrtc debug logging");
    argparser::arg<int> windowWidth(p, "--width", "-ww", "Window width", 720);
    argparser::arg<int> windowHeight(p, "--height", "-wh", "Window height", 480);
    argparser::arg<bool> disableFmod(p, "--disable-fmod", "-df", "Disables usage of the FMod audio library");
    argparser::arg<bool> forceEgl(p, "--force-opengles", "-fes", "Force creating an OpenGL ES surface instead of using the glcorepatch hack", !GLCorePatch::mustUseDesktopGL());
    argparser::arg<bool> texturePatch(p, "--texture-patch", "-tp", "Rewrite textures of the game for Minecraft 1.16.210 - 1.17.4X", false);
    argparser::arg<bool> stdinImpt(p, "--stdin-import", "-si", "Use stdin for file import", false);
    argparser::arg<bool> resetSettings(p, "--reset-settings", "-gs", "Save the default Settings", false);
    argparser::arg<bool> freeOnly(p, "--free-only", "-f", "Only allow starting free versions", false);
    argparser::arg<std::string> mods(p, "--mods", "-m", "Additional directories to load mods from split by ','", "");

    if(!p.parse(argc, (const char**)argv))
        return 1;
    if(printVersion) {
        printVersionInfo();
        return 0;
    }
    options.importFilePath = importFilePath;
    options.sendUri = sendUri;
    options.windowWidth = windowWidth;
    options.windowHeight = windowHeight;
    options.graphicsApi = forceEgl.get() ? GraphicsApi::OPENGL_ES2 : GraphicsApi::OPENGL;
    options.useStdinImport = stdinImpt;
    std::vector<std::string> modDirs;
    for(size_t i = 0; i < mods.get().length();) {
        auto r = mods.get().find(',', i);
        if(r == std::string::npos) {
            modDirs.push_back(mods.get().substr(i));
            break;
        } else {
            modDirs.push_back(mods.get().substr(i, r - i));
            i = r + 1;
        }
    }

    FakeEGL::enableTexturePatch = texturePatch.get();

    auto defaultDataDir = PathHelper::getPrimaryDataDirectory();
    if(!gameDir.get().empty())
        PathHelper::setGameDir(gameDir);
    if(!dataDir.get().empty())
        PathHelper::setDataDir(dataDir);
    if(!cacheDir.get().empty())
        PathHelper::setCacheDir(cacheDir);

    Log::info("Launcher", "Version: client %s / manifest %s", CLIENT_GIT_COMMIT_HASH, MANIFEST_GIT_COMMIT_HASH);
#if defined(__linux__)
#define TARGET "Linux"
#elif defined(__APPLE__)
#define TARGET "macOS"
#elif defined(__FreeBSD__)
#define TARGET "FreeBSD"
#else
#define TARGET "Unknown"
#endif
#if defined(__x86_64__)
#define ARCH "x86_64"
#elif defined(__i386__)
#define ARCH "x86"
#elif defined(__aarch64__)
#define ARCH "arm64"
#elif defined(__arm__)
#define ARCH "arm"
#else
#define ARCH "Unknown"
#endif
    Log::info("Launcher", "OS: %s, Arch: %s", TARGET, ARCH);
    loadGameOptions();
#if defined(__i386__) || defined(__x86_64__)
    {
        CpuId cpuid;
        Log::info("Launcher", "CPU: %s %s", cpuid.getManufacturer(), cpuid.getBrandString());
        Log::info("Launcher", "CPU supports SSSE3: %s",
                  cpuid.queryFeatureFlag(CpuId::FeatureFlag::SSSE3) ? "YES" : "NO");
    }
#endif

    if(resetSettings.get()) {
        Log::info("Launcher", "Resetting Launcher Settings File: %s", Settings::getPath().data());
        Settings::save();
        Log::info("Launcher", "Launcher Settings reset");
    } else {
        Log::info("Launcher", "Reading Launcher Settings File: %s", Settings::getPath().data());
        Settings::load();
        Log::info("Launcher", "Applied Launcher Settings");
    }

    Log::trace("Launcher", "Loading android libraries");
    linker::init();
    Log::trace("Launcher", "linker loaded");
    auto windowManager = GameWindowManager::getManager();

#if !defined(__linux__)
    // Fake /proc/cpuinfo
    // https://github.com/pytorch/cpuinfo depends on this file for linux builds
    auto fakeproc = PathHelper::getPrimaryDataDirectory() + "proc/";
    FileUtil::mkdirRecursive(fakeproc);
    std::ofstream fake_cpuinfo(fakeproc + "/cpuinfo", std::ios::binary | std::ios::trunc);
    if(fake_cpuinfo.is_open()) {
#if defined(__i386__) || defined(__x86_64__) 
        fake_cpuinfo << R"(processor	: 0
vendor_id	: GenuineIntel
cpu family	: 6
model		: 142
model name	: Intel(R) Core(TM) i7-8550U CPU @ 1.80GHz
stepping	: 10
microcode	: 0xffffffff
cpu MHz		: 1991.999
cache size	: 8192 KB
physical id	: 0
siblings	: 8
core id		: 0
cpu cores	: 4
apicid		: 0
initial apicid	: 0
fpu		: yes
fpu_exception	: yes
cpuid level	: 22
wp		: yes
flags		: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good nopl xtopology cpuid pni pclmulqdq vmx ssse3 fma cx16 pcid sse4_1 sse4_2 movbe popcnt aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch invpcid_single pti ssbd ibrs ibpb stibp tpr_shadow vnmi ept vpid ept_ad fsgsbase bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt xsaveopt xsavec xgetbv1 xsaves flush_l1d arch_capabilities
vmx flags	: vnmi invvpid ept_x_only ept_ad ept_1gb tsc_offset vtpr ept vpid unrestricted_guest ept_mode_based_exec
bugs		: cpu_meltdown spectre_v1 spectre_v2 spec_store_bypass l1tf mds swapgs itlb_multihit srbds
bogomips	: 3983.99
clflush size	: 64
cache_alignment	: 64
address sizes	: 39 bits physical, 48 bits virtual
power management:


)";
#elif defined(__arm__) || defined(__aarch64__)
        fake_cpuinfo << R"(Processor	: AArch64 Processor rev 4 (aarch64)
processor	: 0
BogoMIPS	: 38.40
Features	: fp asimd evtstrm aes pmull sha1 sha2 crc32
CPU implementer	: 0x51
CPU architecture: 8
CPU variant	: 0xa
CPU part	: 0x801
CPU revision	: 4

Hardware	: Qualcomm Technologies, Inc MSM8998

)";
#endif
        fake_cpuinfo.close();
    }
    // cpuinfo for arm64 fails if these are missing...
    auto fakesys = PathHelper::getPrimaryDataDirectory() + "sys/";
    auto fake_cpu = fakesys + "devices/system/cpu/";
    FileUtil::mkdirRecursive(fake_cpu);
    std::ofstream fake_cpu_present(fake_cpu + "/present", std::ios::binary | std::ios::trunc);
    if(fake_cpu_present.is_open()) {
        fake_cpu_present << R"(0-3)";
        fake_cpu_present.close();
    }
    std::ofstream fake_cpu_possible(fake_cpu + "/possible", std::ios::binary | std::ios::trunc);
    if(fake_cpu_possible.is_open()) {
        fake_cpu_possible << R"(0-3)";
        fake_cpu_possible.close();
    }
#endif

    // Fix saving to internal storage without write access to /data/*
    // TODO research how this path is constructed
    auto pid = getpid();
    shim::rewrite_filesystem_access = {
        // Minecraft 1.16.210 or older
        { "/data/data/com.mojang.minecraftpe", PathHelper::getPrimaryDataDirectory() },
        // Minecraft 1.16.210 or later, absolute path on linux (source build ubuntu 20.04)
        { std::string("/data/data") + PathHelper::getParentDir(PathHelper::getAppDir()) + "/proc/" + std::to_string(pid) + "/cmdline", PathHelper::getPrimaryDataDirectory() }};
    if(argc >= 1 && argv != nullptr && argv[0] != nullptr && argv[0][0] != '\0') {
        // Minecraft 1.16.210 or later, relative path on linux (source build ubuntu 20.04) or every path AppImage / flatpak
        shim::rewrite_filesystem_access.emplace_back(argv[0][0] == '/' ? std::string("/data/data") + argv[0] : std::string("/data/data/") + argv[0], PathHelper::getPrimaryDataDirectory());
    }
    // Minecraft 1.16.210 or later, macOS
    shim::rewrite_filesystem_access.emplace_back("/data/data", PathHelper::getPrimaryDataDirectory());
    // vanilla_music isn't loaded via AAssetManager, it uses libc-shim via relative filepath
    shim::rewrite_filesystem_access.emplace_back(".", PathHelper::getGameDir() + "assets/");
#if !defined(__linux__)
    // fake proc fs needed for macOS and windows
    shim::rewrite_filesystem_access.emplace_back("/proc", fakeproc);
    shim::rewrite_filesystem_access.emplace_back("/sys", fakesys);
#endif
    for(auto&& redir : shim::rewrite_filesystem_access) {
        Log::trace("REDIRECT", "%s to %s", redir.first.data(), redir.second.data());
    }
    auto libC = MinecraftUtils::getLibCSymbols();
    ThreadMover::hookLibC(libC);

#ifdef USE_ARMHF_SUPPORT
    linker::load_library("ld-android.so", {});
    android_dlextinfo extinfo;
    std::vector<mcpelauncher_hook_t> hooks;
    for(auto&& entry : libC) {
        hooks.emplace_back(mcpelauncher_hook_t{entry.first.data(), entry.second});
    }
    hooks.emplace_back(mcpelauncher_hook_t{nullptr, nullptr});
    extinfo.flags = ANDROID_DLEXT_MCPELAUNCHER_HOOKS;
    extinfo.mcpelauncher_hooks = hooks.data();
    if(linker::dlopen_ext(PathHelper::findDataFile("lib/" + std::string(PathHelper::getAbiDir()) + "/libc.so").c_str(), 0, &extinfo) == nullptr) {
        Log::error("LAUNCHER", "Failed to load armhf compat libc.so Original Error: %s", linker::dlerror());
        return 1;
    }
    if(linker::dlopen(PathHelper::findDataFile("lib/" + std::string(PathHelper::getAbiDir()) + "/libm.so").c_str(), 0) == nullptr) {
        Log::error("LAUNCHER", "Failed to load armhf compat libm.so Original Error: %s", linker::dlerror());
        return 1;
    }
#elif defined(__APPLE__) && defined(__aarch64__)
    MinecraftUtils::loadLibM();
    android_dlextinfo extinfo;
    std::vector<mcpelauncher_hook_t> hooks;
    for(auto&& entry : libC) {
        hooks.emplace_back(mcpelauncher_hook_t{entry.first.data(), entry.second});
    }
    hooks.emplace_back(mcpelauncher_hook_t{nullptr, nullptr});
    extinfo.flags = ANDROID_DLEXT_MCPELAUNCHER_HOOKS;
    extinfo.mcpelauncher_hooks = hooks.data();
    if(linker::dlopen_ext(PathHelper::findDataFile("lib/" + std::string(PathHelper::getAbiDir()) + "/libc.so").c_str(), 0, &extinfo) == nullptr) {
        Log::error("LAUNCHER", "Failed to load arm64 variadic compat libc.so Original Error: %s", linker::dlerror());
        return 1;
    }
    if(linker::dlopen(PathHelper::findDataFile("lib/" + std::string(PathHelper::getAbiDir()) + "/liblog.so").c_str(), 0) == nullptr) {
        Log::error("LAUNCHER", "Failed to load arm64 variadic compat liblog.so Original Error: %s", linker::dlerror());
        return 1;
    }
#else
    linker::load_library("libc.so", libC);
    MinecraftUtils::loadLibM();
#endif
    MinecraftUtils::setupHybris();
    try {
        PathHelper::findGameFile(std::string("lib/") + MinecraftUtils::getLibraryAbi() + "/libminecraftpe.so");
    } catch(std::exception& e) {
        Log::error("LAUNCHER", "Could not find the game, use the -dg flag to fix this error. Original Error: %s", e.what());
        return 1;
    }
    linker::update_LD_LIBRARY_PATH(PathHelper::findGameFile(std::string("lib/") + MinecraftUtils::getLibraryAbi()).data());
    if(!disableFmod) {
        try {
            MinecraftUtils::loadFMod();
        } catch(std::exception& e) {
            Log::info("FMOD", "Failed to load host libfmod: '%s', use pulseaudio/sdl3 backend with android fmod if available", e.what());
        }
    }
    FakeEGL::setProcAddrFunction((void* (*)(const char*))windowManager->getProcAddrFunc());
    FakeEGL::installLibrary();
    if(options.graphicsApi == GraphicsApi::OPENGL_ES2) {
        // GLFW needs a window to let eglGetProcAddress return symbols
        FakeLooper::initWindow();
#ifdef USE_IMGUI
        gladLoadGLES2Loader(fake_egl::eglGetProcAddress);
#endif
        MinecraftUtils::setupGLES2Symbols(fake_egl::eglGetProcAddress);
    } else {
        // The glcore patch requires an empty library
        // Otherwise linker has to hide the symbols from dlsym in libminecraftpe.so
        linker::load_library("libGLESv2.so", {});
    }

    std::unordered_map<std::string, void*> android_syms;
    FakeAssetManager::initHybrisHooks(android_syms);
    FakeInputQueue::initHybrisHooks(android_syms);
    FakeLooper::initHybrisHooks(android_syms);
    FakeWindow::initHybrisHooks(android_syms);
    for(auto s = android_symbols; *s; s++)  // stub missing symbols
        android_syms.insert({*s, (void*)+[]() { Log::warn("Main", "Android stub called"); }});
    linker::load_library("libandroid.so", android_syms);
    CorePatches::loadGameWindowLibrary();


    linker::load_library("libmcpelauncher_menu.so", {
        { "mcpelauncher_addmenu", (void*)mcpelauncher_addmenu },
        { "mcpelauncher_show_window", (void*)mcpelauncher_show_window },
        { "mcpelauncher_close_window", (void*)mcpelauncher_close_window },
        { "mcpelauncher_add_draw_callback", (void*)mcpelauncher_add_draw_callback },
        { "mcpelauncher_get_imgui_context", (void*)mcpelauncher_get_imgui_context },
    });

    ModLoader modLoader;
    if(!freeOnly.get()) {
        modLoader.loadModsFromDirectory(PathHelper::getPrimaryDataDirectory() + "mods/", true);
        for(auto&& d : modDirs) {
            modLoader.loadModsFromDirectory(d, true);
        }
    }

    Log::trace("Launcher", "Loading Minecraft library");
    static void* handle = MinecraftUtils::loadMinecraftLib(reinterpret_cast<void*>(&CorePatches::showMousePointer), reinterpret_cast<void*>(&CorePatches::hideMousePointer), reinterpret_cast<void*>(&CorePatches::setFullscreen));
    if(!handle && options.graphicsApi == GraphicsApi::OPENGL) {
        // Old game version or renderdragon
        options.graphicsApi = GraphicsApi::OPENGL_ES2;
        // Unload empty stub library
        auto libGLESv2 = linker::dlopen("libGLESv2.so", 0);
        linker::dlclose(libGLESv2);
        linker::unload_library(libGLESv2);
        // load fake libGLESv2 library
        // GLFW needs a window to let eglGetProcAddress return symbols
        FakeLooper::initWindow();
        MinecraftUtils::setupGLES2Symbols(fake_egl::eglGetProcAddress);
#ifdef USE_IMGUI
        gladLoadGLES2Loader(fake_egl::eglGetProcAddress);
#endif
        // preinit Mods using libGLESv2 can only load now
        if(!freeOnly.get()) {
            modLoader.loadModsFromDirectory(PathHelper::getPrimaryDataDirectory() + "mods/", true);
        }
        // Try load the game again
        handle = MinecraftUtils::loadMinecraftLib(reinterpret_cast<void*>(&CorePatches::showMousePointer), reinterpret_cast<void*>(&CorePatches::hideMousePointer), reinterpret_cast<void*>(&CorePatches::setFullscreen));
    }
    if(!handle && !disableFmod) {
        // 1.21.30.22 technically require newer fmod
        auto libfmod = linker::dlopen("libfmod.so", 0);
        linker::dlclose(libfmod);
        linker::unload_library(libfmod);

        // Try load the game again
        handle = MinecraftUtils::loadMinecraftLib(reinterpret_cast<void*>(&CorePatches::showMousePointer), reinterpret_cast<void*>(&CorePatches::hideMousePointer), reinterpret_cast<void*>(&CorePatches::setFullscreen));
    }
    if(!handle) {
        Log::error("Launcher", "Failed to load Minecraft library, please reinstall or wait for an update to support the new release");
        return 51;
    }
    Log::info("Launcher", "Loaded Minecraft library");
    Log::debug("Launcher", "Minecraft is at offset 0x%" PRIXPTR, (uintptr_t)MinecraftUtils::getLibraryBase(handle));
    base = MinecraftUtils::getLibraryBase(handle);

    if(!freeOnly.get()) {
        modLoader.loadModsFromDirectory(PathHelper::getPrimaryDataDirectory() + "mods/");
        for(auto&& d : modDirs) {
            modLoader.loadModsFromDirectory(d);
        }
    }

    Log::info("Launcher", "Game version: %s", MinecraftVersion::getString().c_str());

    Log::info("Launcher", "Applying patches");
    if(v8Flags.get().size()) {
        void (*V8SetFlagsFromString)(const char * str, int length);
        V8SetFlagsFromString = (decltype(V8SetFlagsFromString))linker::dlsym(handle, "_ZN2v82V818SetFlagsFromStringEPKc");
        if(V8SetFlagsFromString) {
            Log::info("V8", "Applying v8-flags %s", v8Flags.get().data());
            V8SetFlagsFromString(v8Flags.get().data(), v8Flags.get().size());
        } else {
            Log::warn("V8", "Couldn't apply v8-flags %s to the game", v8Flags.get().data());
        }
    }
    if(webrtcdebug.get())
    {
        void (*LogToDebug)(int servity) = (decltype(LogToDebug))linker::dlsym(handle, "_ZN3rtc10LogMessage10LogToDebugENS_15LoggingSeverityE");
        if(LogToDebug) {
            LogToDebug(0);
        }
        void (*SetLogToStderr)(bool on) = (decltype(SetLogToStderr))linker::dlsym(handle, "_ZN3rtc10LogMessage14SetLogToStderrEb");
        if(SetLogToStderr) {
            SetLogToStderr(true);
        }
    }
    bool (*isAndroidTrial)() = (decltype(isAndroidTrial))linker::dlsym(handle, "Java_com_mojang_minecraftpe_MainActivity_isAndroidTrial");
    bool (*isAndroidChromebook)() = (decltype(isAndroidChromebook))linker::dlsym(handle, "Java_com_mojang_minecraftpe_MainActivity_isAndroidChromebook");
    bool (*isAndroidAmazon)() = (decltype(isAndroidAmazon))linker::dlsym(handle, "Java_com_mojang_minecraftpe_MainActivity_isAndroidAmazon");
    bool (*isEduMode)() = (decltype(isEduMode))linker::dlsym(handle, "Java_com_mojang_minecraftpe_MainActivity_isEduMode");
    
    if(isAndroidTrial && isAndroidTrial()) {
        Log::info("Launcher", "Detected Trial build");
    }
    if(isAndroidChromebook && isAndroidChromebook()) {
        Log::info("Launcher", "Detected Chromebook build");
    }
    if(isAndroidAmazon && isAndroidAmazon()) {
        Log::info("Launcher", "Detected Amazon build");
    }
    if(isEduMode && isEduMode()) {
        Log::info("Launcher", "Detected Education build");
    }
    if(freeOnly.get() && !(isAndroidTrial && isAndroidTrial()) && !(isAndroidAmazon && isAndroidAmazon()) && !(isEduMode && isEduMode())) {
        Log::error("Launcher", "Something went wrong");
        return 1;
    }
                
    SymbolsHelper::initSymbols(handle);
    CorePatches::install(handle);
#ifdef __i386__
    TexelAAPatch::install(handle);
    HbuiPatch::install(handle);
    SplitscreenPatch::install(handle);
    ShaderErrorPatch::install(handle);
#endif
    if(options.graphicsApi == GraphicsApi::OPENGL) {
        try {
            GLCorePatch::install(handle);
        } catch(const std::exception& ex) {
            Log::error("GLCOREPATCH", "Failed to apply glcorepatch: %s", ex.what());
            options.graphicsApi = GraphicsApi::OPENGL_ES2;
        }
    }

    Log::info("Launcher", "Initializing JNI");
    JniSupport support;
    FakeLooper::setJniSupport(&support);
    support.registerMinecraftNatives(+[](const char* sym) {
        return linker::dlsym(handle, sym);
    });
    std::thread startThread([&support]() {
        support.startGame((ANativeActivity_createFunc*)linker::dlsym(handle, "ANativeActivity_onCreate"),
                          linker::dlsym(handle, "stbi_load_from_memory"),
                          linker::dlsym(handle, "stbi_image_free"));
        linker::dlclose(handle);
    });
    startThread.detach();

    std::unique_ptr<RpcCallbackServer> file_handler;
    try {
        FileUtil::mkdirRecursive(defaultDataDir);
        file_handler = std::make_unique<RpcCallbackServer>(defaultDataDir + "file_handler", support);
    } catch(const std::exception& ex) {
        Log::error("Launcher", "Failed to bind file_handler, you may be unable to import files: %s", ex.what());
    }

    Log::info("Launcher", "Executing main thread");
    ThreadMover::executeMainThread();
    support.setLooperRunning(false);

    //    XboxLivePatches::workaroundShutdownFreeze(handle);
    XboxLiveHelper::getInstance().shutdown();
    // Workaround for XboxLive ShutdownFreeze
    _Exit(0);
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
    auto glGetString = (const char* (*)(int))windowManager->getProcAddrFunc()("glGetString");
    printf("GL Vendor: %s\n", glGetString(0x1F00 /* GL_VENDOR */));
    printf("GL Renderer: %s\n", glGetString(0x1F01 /* GL_RENDERER */));
    printf("GL Version: %s\n", glGetString(0x1F02 /* GL_VERSION */));
    printf("MSA daemon path: %s\n", XboxLiveHelper::findMsa().c_str());
}

void loadGameOptions() {
    properties::property_list properties(':');
    properties::property<int> leftKey(properties, "keyboard_type_0_key.left", 'A');
    properties::property<int> downKey(properties, "keyboard_type_0_key.back", 'S');
    properties::property<int> rightKey(properties, "keyboard_type_0_key.right", 'D');
    properties::property<int> upKey(properties, "keyboard_type_0_key.forward", 'W');

    properties::property<int> leftKeyFullKeyboard(properties, "keyboard_type_1_key.left", 'A');
    properties::property<int> downKeyFullKeyboard(properties, "keyboard_type_1_key.back", 'S');
    properties::property<int> rightKeyFullKeyboard(properties, "keyboard_type_1_key.right", 'D');
    properties::property<int> upKeyFullKeyboard(properties, "keyboard_type_1_key.forward", 'W');

    properties::property<bool> fullKeyboard(properties, "ctrl_fullkeyboardgameplay", false);

    std::ifstream propertiesFile(PathHelper::getPrimaryDataDirectory() + "/games/com.mojang/minecraftpe/options.txt");
    if (propertiesFile) {
        properties.load(propertiesFile);
    }

    GameOptions::leftKey = leftKey;
    GameOptions::downKey = downKey;
    GameOptions::rightKey = rightKey;
    GameOptions::upKey = upKey;

    GameOptions::leftKeyFullKeyboard = leftKeyFullKeyboard;
    GameOptions::downKeyFullKeyboard = downKeyFullKeyboard;
    GameOptions::rightKeyFullKeyboard = rightKeyFullKeyboard;
    GameOptions::upKeyFullKeyboard = upKeyFullKeyboard;

    GameOptions::fullKeyboard = fullKeyboard;
}
