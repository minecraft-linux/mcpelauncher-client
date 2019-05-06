#include "legacy_patches.h"

#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>
#include <mcpelauncher/minecraft_version.h>

void LegacyPatches::install(void *handle) {
    if (!MinecraftVersion::isAtLeast(1, 2)) {
        void *ptr = hybris_dlsym(handle, "_ZN15PatchNotesModel17preloadPatchNotesEv");
        if (ptr != nullptr)
            PatchUtils::patchCallInstruction(ptr, (void *) +[]() {}, true);
    }
}