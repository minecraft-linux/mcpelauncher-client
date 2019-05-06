#include "minecraft_game_wrapper.h"

#include <minecraft/ClientInstance.h>
#include <mcpelauncher/minecraft_version.h>
#include "legacy/minecraft_game_wrapper_legacy.h"

MinecraftGameWrapper* MinecraftGameWrapper::create(int argc, char **argv) {
    if (!MinecraftVersion::isAtLeast(1, 1))
        return new MinecraftGameAppWrapper_Pre_1_1(argc, argv);
    if (!MinecraftVersion::isAtLeast(1, 2))
        return new MinecraftGameAppWrapper_Pre_1_2(argc, argv);
    if (!MinecraftVersion::isAtLeast(1, 2, 10))
        return new MinecraftGameAppWrapper_Pre_1_2_10(argc, argv);
    if (!MinecraftVersion::isAtLeast(1, 8))
        return new MinecraftGameAppWrapper_Pre_1_8(argc, argv);
    return new MinecraftGameDefaultWrapper(argc, argv);
}

MinecraftGame* MinecraftGameDefaultWrapper::createInstance(int argc, char **argv) {
    return new MinecraftGame(argc, argv);
}

void MinecraftGameDefaultWrapper::leaveGame() {
    if (game->isInGame()) {
        game->requestLeaveGame(true, false);
        game->continueLeaveGame();
        game->startLeaveGame();
        game->getPrimaryClientInstance()->_startLeaveGame();
        game->getPrimaryClientInstance()->_syncDestroyGame();
    }
}