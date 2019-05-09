#include "legacy_patches.h"

#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>
#include <mcpelauncher/minecraft_version.h>

void LegacyPatches::install(void *handle) {
    if (!MinecraftVersion::isAtLeast(1, 2)) {
        void *ptr = hybris_dlsym(handle, "_ZN15PatchNotesModel17preloadPatchNotesEv");
        if (ptr != nullptr)
            PatchUtils::patchCallInstruction(ptr, (void *) +[]() {}, true);

        ptr = hybris_dlsym(handle, "_ZN14SkinRepository25_generatePremiumSkinPacksEv");
        if (ptr != nullptr)
            PatchUtils::patchCallInstruction(ptr, (void *) +[]() {}, true);
    }
    if (!MinecraftVersion::isAtLeast(0, 15, 2)) {
        void *ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop8init_cllERKSs");
        if (ptr != nullptr)
            PatchUtils::patchCallInstruction(ptr, (void *) &initCllStub, true);
    }
}

xbox::services::xbox_live_result<void> LegacyPatches::initCllStub() {
    xbox::services::xbox_live_result<void> ret;
    ret.code = 0;
    ret.error_code_category = xbox::services::xbox_services_error_code_category();
    return ret;
}