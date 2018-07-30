#include <log.h>
#include <hybris/hook.h>
#include <game_window_manager.h>
#include <argparser.h>
#include <mcpelauncher/minecraft_utils.h>
#include <mcpelauncher/crash_handler.h>
#include <mcpelauncher/path_helper.h>
#include <minecraft/Common.h>
#include <minecraft/MinecraftGame.h>
#include <mcpelauncher/mod_loader.h>
#include "client_app_platform.h"
#include "xbox_live_patches.h"
#include "store.h"
#include "window_callbacks.h"
#include "http_request_stub.h"
#include "splitscreen_patch.h"
#include "gl_core_patch.h"
#include "xbox_live_helper.h"
#include "xbox_sleep_shutdown_patch.h"

int main(int argc, char *argv[]) {
    auto windowManager = GameWindowManager::getManager();
    CrashHandler::registerCrashHandler();
    MinecraftUtils::workaroundLocaleBug();

    argparser::arg_parser p;
    argparser::arg<std::string> gameDir (p, "--game-dir", "-dg", "Directory with the game and assets");
    argparser::arg<std::string> dataDir (p, "--data-dir", "-dd", "Directory to use for the data");
    argparser::arg<std::string> cacheDir (p, "--cache-dir", "-dc", "Directory to use for cache");
    argparser::arg<int> windowWidth (p, "--width", "-ww", "Window width", 720);
    argparser::arg<int> windowHeight (p, "--height", "-wh", "Window height", 480);
    argparser::arg<float> pixelScale (p, "--scale", "-s", "Pixel Scale", 2.f);
    argparser::arg<bool> mallocZero (p, "--malloc-zero", "-mz", "Patch malloc to always zero initialize memory, this may help workaround MCPE bugs");
    if (!p.parse(argc, (const char**) argv))
        return 1;
    if (!gameDir.get().empty())
        PathHelper::setGameDir(gameDir);
    if (!dataDir.get().empty())
        PathHelper::setDataDir(dataDir);
    if (!cacheDir.get().empty())
        PathHelper::setCacheDir(cacheDir);
    if (mallocZero)
        MinecraftUtils::setMallocZero();

    GraphicsApi graphicsApi = GLCorePatch::mustUseDesktopGL() ? GraphicsApi::OPENGL : GraphicsApi::OPENGL_ES2;

    Log::trace("Launcher", "Loading hybris libraries");
    MinecraftUtils::loadFMod();
    MinecraftUtils::setupHybris();
    hybris_hook("eglGetProcAddress", (void*) windowManager->getProcAddrFunc());

    Log::trace("Launcher", "Loading Minecraft library");
    void* handle = MinecraftUtils::loadMinecraftLib();
    Log::info("Launcher", "Loaded Minecraft library");
    Log::debug("Launcher", "Minecraft is at offset 0x%x", MinecraftUtils::getLibraryBase(handle));

    ModLoader modLoader;
    modLoader.loadModsFromDirectory(PathHelper::getPrimaryDataDirectory() + "mods/");

    MinecraftUtils::initSymbolBindings(handle);
    Log::info("Launcher", "Game version: %s", Common::getGameVersionStringNet().c_str());

    Log::info("Launcher", "Applying patches");
    LauncherStore::install(handle);
    XboxLivePatches::install(handle);
    XboxSleepShutdownPatch::install(handle);
    LinuxHttpRequestHelper::install(handle);
    SplitscreenPatch::install(handle);
    if (graphicsApi == GraphicsApi::OPENGL)
        GLCorePatch::install(handle);

    Log::info("Launcher", "Creating window");
    auto window = windowManager->createWindow("Minecraft", windowWidth, windowHeight, graphicsApi);
    window->setIcon(PathHelper::getIconPath());
    window->show();

    SplitscreenPatch::onGLContextCreated();
    GLCorePatch::onGLContextCreated();

    Log::trace("Launcher", "Initializing AppPlatform (vtable)");
    ClientAppPlatform::initVtable(handle);
    Log::trace("Launcher", "Initializing AppPlatform (create instance)");
    std::unique_ptr<ClientAppPlatform> appPlatform (new ClientAppPlatform());
    appPlatform->setWindow(window);
    Log::trace("Launcher", "Initializing AppPlatform (initialize call)");
    appPlatform->initialize();
    mce::Platform::OGL::InitBindings();

    Log::info("Launcher", "OpenGL: version: %s, renderer: %s, vendor: %s",
              gl::getOpenGLVersion().c_str(), gl::getOpenGLRenderer().c_str(), gl::getOpenGLVendor().c_str());

    Log::trace("Launcher", "Initializing MinecraftGame (create instance)");
    std::unique_ptr<MinecraftGame> game (new MinecraftGame(argc, argv));
    Log::trace("Launcher", "Initializing MinecraftGame (init call)");
    AppContext ctx;
    ctx.platform = appPlatform.get();
    ctx.doRender = true;
    game->init(ctx);
    Log::info("Launcher", "Game initialized");

    modLoader.onGameInitialized(game.get());

    WindowCallbacks windowCallbacks (*game, *appPlatform, *window);
    windowCallbacks.setPixelScale(pixelScale);
    windowCallbacks.registerCallbacks();
    windowCallbacks.handleInitialWindowSize();
    window->runLoop();

    game.reset();
    MinecraftUtils::workaroundShutdownCrash(handle);
    XboxLivePatches::workaroundShutdownFreeze(handle);
    XboxSleepShutdownPatch::notifyShutdown();

    XboxLiveHelper::getInstance().shutdownCll();
    appPlatform->teardown();
    return 0;
}
