#include <log.h>
#include <hybris/hook.h>
#include <game_window_manager.h>
#include <mcpelauncher/minecraft_utils.h>
#include <mcpelauncher/crash_handler.h>
#include <mcpelauncher/path_helper.h>
#include <minecraft/Common.h>
#include <minecraft/MinecraftGame.h>
#include "client_app_platform.h"
#include "xbox_live_patches.h"
#include "store.h"
#include "window_callbacks.h"

int main(int argc, char *argv[]) {
    auto windowManager = GameWindowManager::getManager();
    CrashHandler::registerCrashHandler();
    MinecraftUtils::workaroundLocaleBug();

    Log::trace("Launcher", "Loading hybris libraries");
    MinecraftUtils::loadFMod();
    MinecraftUtils::setupHybris();
    hybris_hook("eglGetProcAddress", (void*) windowManager->getProcAddrFunc());

    Log::trace("Launcher", "Loading Minecraft library");
    void* handle = MinecraftUtils::loadMinecraftLib();
    Log::info("Launcher", "Loaded Minecraft library");

    Log::debug("Launcher", "Minecraft is at offset 0x%x", MinecraftUtils::getLibraryBase(handle));
    MinecraftUtils::initSymbolBindings(handle);
    Log::info("Launcher", "Game version: %s", Common::getGameVersionStringNet().c_str());

    Log::info("Launcher", "Applying patches");
    LauncherStore::install(handle);
    XboxLivePatches::install(handle);

    Log::info("Launcher", "Creating window");
    int windowWidth = 720;
    int windowHeight = 480;
    GraphicsApi graphicsApi = GraphicsApi::OPENGL_ES2;
    auto window = windowManager->createWindow("Minecraft", windowWidth, windowHeight, graphicsApi);
    window->setIcon(PathHelper::getIconPath());
    window->show();

    Log::trace("Launcher", "Initializing AppPlatform (vtable)");
    ClientAppPlatform::initVtable(handle);
    Log::trace("Launcher", "Initializing AppPlatform (create instance)");
    std::unique_ptr<ClientAppPlatform> appPlatform (new ClientAppPlatform());
    appPlatform->setWindow(window);
    Log::trace("Launcher", "Initializing AppPlatform (initialize call)");
    appPlatform->initialize();
    mce::Platform::OGL::InitBindings();

    Log::trace("Launcher", "Initializing MinecraftGame (create instance)");
    std::unique_ptr<MinecraftGame> game (new MinecraftGame(argc, argv));
    Log::trace("Launcher", "Initializing MinecraftGame (init call)");
    AppContext ctx;
    ctx.platform = appPlatform.get();
    ctx.doRender = true;
    game->init(ctx);
    Log::info("Launcher", "Game initialized");

    WindowCallbacks windowCallbacks (*game, *appPlatform, *window);
    windowCallbacks.registerCallbacks();
    window->runLoop();

    return 0;
}
