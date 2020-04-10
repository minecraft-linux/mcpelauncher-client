#include <log.h>
#include <hybris/hook.h>
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
#ifdef __i386__
#include "cpuid.h"
#include "texel_aa_patch.h"
#include "xbox_shutdown_patch.h"
#endif
#include <build_info.h>
#include <hybris/dlfcn.h>
#include "main.h"
#include "fake_looper.h"
#include "fake_assetmanager.h"
#include "fake_egl.h"

static size_t base;
LauncherOptions options;

void printVersionInfo();

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
    argparser::arg<bool> mallocZero (p, "--malloc-zero", "-mz", "Patch malloc to always zero initialize memory, this may help workaround MCPE bugs");
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
    if (mallocZero)
        MinecraftUtils::setMallocZero();

    Log::info("Launcher", "Version: client %s / manifest %s", CLIENT_GIT_COMMIT_HASH, MANIFEST_GIT_COMMIT_HASH);
#ifdef __i386__
    {
        CpuId cpuid;
        Log::info("Launcher", "CPU: %s %s", cpuid.getManufacturer(), cpuid.getBrandString());
        Log::info("Launcher", "CPU supports SSSE3: %s",
                cpuid.queryFeatureFlag(CpuId::FeatureFlag::SSSE3) ? "YES" : "NO");
    }
#endif

    Log::trace("Launcher", "Loading hybris libraries");
    if (!disableFmod)
        MinecraftUtils::loadFMod();
    else
        MinecraftUtils::stubFMod();
    MinecraftUtils::setupHybris();
    FakeAssetManager::initHybrisHooks();
    FakeLooper::initHybrisHooks();
    FakeEGL::initHybrisHooks();
    hybris_hook("eglGetProcAddress", (void*) windowManager->getProcAddrFunc());
    MinecraftUtils::setupGLES2Symbols((void* (*)(const char*)) windowManager->getProcAddrFunc());
    hybris_hook("glInvalidateFramebuffer", (void*) +[]{});
#ifdef USE_ARMHF_SUPPORT
    ArmhfSupport::install();
#endif

    Log::trace("Launcher", "Loading Minecraft library");
    static void* handle = MinecraftUtils::loadMinecraftLib();
    Log::info("Launcher", "Loaded Minecraft library");
    Log::debug("Launcher", "Minecraft is at offset 0x%x", MinecraftUtils::getLibraryBase(handle));
    base = MinecraftUtils::getLibraryBase(handle);

    ModLoader modLoader;
    modLoader.loadModsFromDirectory(PathHelper::getPrimaryDataDirectory() + "mods/");

    Log::info("Launcher", "Game version: %s", MinecraftVersion::getString().c_str());

    Log::info("Launcher", "Applying patches");
#ifdef __i386__
//    XboxShutdownPatch::install(handle);
    TexelAAPatch::install(handle);
#endif
    HbuiPatch::install(handle);
    SplitscreenPatch::install(handle);
    ShaderErrorPatch::install(handle);
    if (options.graphicsApi == GraphicsApi::OPENGL)
        GLCorePatch::install(handle);

    Log::info("Launcher", "Initializing JNI");
    JniSupport support;
    FakeLooper::setJniSupport(&support);
    support.registerMinecraftNatives(+[](const char *sym) {
        return hybris_dlsym(handle, sym);
    });
    support.startGame((ANativeActivity_createFunc *) hybris_dlsym(handle, "ANativeActivity_onCreate"));
    support.waitForGameExit();

    MinecraftUtils::workaroundShutdownCrash(handle);
//    XboxLivePatches::workaroundShutdownFreeze(handle);
#ifdef __i386__
    XboxShutdownPatch::notifyShutdown();
#endif

    XboxLiveHelper::getInstance().shutdown();
    return 0;
}

void printVersionInfo() {
    printf("mcpelauncher-client %s / manifest %s\n", CLIENT_GIT_COMMIT_HASH, MANIFEST_GIT_COMMIT_HASH);
#ifdef __i386__
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