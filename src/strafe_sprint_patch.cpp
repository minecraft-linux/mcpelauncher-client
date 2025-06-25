#include "strafe_sprint_patch.h"
#include <mcpelauncher/patch_utils.h>
#include <log.h>
#include <string.h>

void StrafeSprintPatch::install(void* handle) {
    // 1.20.30+
    void* ptr = PatchUtils::patternSearch(handle, "F3 0F 10 05 ?? ?? ?? ?? 41 0F 2E ?? ?? 76 ?? EB");
    if (!ptr) {
        // <= 1.21.80
        ptr = PatchUtils::patternSearch(handle, "F3 0F 10 05 ?? ?? ?? ?? F3 0F 59 E0 0F 28 EA");
    }
    if(!ptr) {
        Log::error("StrafeSprintPatch", "Not patching - Pattern not found");
        return;
    }
    int32_t value;
    memcpy(&value, (void*)((unsigned long)ptr + 4), sizeof(int32_t));

    float* val = (float*)((unsigned long)ptr + value + 4 + sizeof(int32_t));
    float orig = *val;

    if(orig != 0.70710677f) {
        Log::error("StrafeSprintPatch", "Not patching - Wrong value! Expected 0.70710677, got %.8f (address: %p)", orig, val);
        return;
    }
    *val = 0.7071068f;
    Log::info("StrafeSprintPatch", "Patching - Original: %.8f, New: %.8f, Address: %p", orig, *val, val);
}
