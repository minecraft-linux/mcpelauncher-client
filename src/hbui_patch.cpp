#include "hbui_patch.h"
#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>

void HbuiPatch::install(void *handle) {
    void* ptr = hybris_dlsym(handle, "_ZN6cohtml17VerifiyLicenseKeyEPKc");
    if (ptr)
        PatchUtils::patchCallInstruction(ptr, (void*) returnTrue, true);
}