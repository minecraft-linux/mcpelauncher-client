#include "texel_aa_patch.h"

#include <mcpelauncher/linker.h>
#include <memory.h>
#include <log.h>

void TexelAAPatch::install(void *handle) {
    auto ptr = (unsigned char *)linker::dlsym(handle, "_ZN31GeneralSettingsScreenController28_registerControllerCallbacksEv");
    if(ptr == nullptr)
        return;
    int hash = 0x96F031FF;
    for(int i = 0; i < 0x4000; i++) {
        if((int &)ptr[i + 4] == hash && ptr[i] == 0xC7 && ptr[i + 1] == 0x44 && ptr[i + 2] == 0x24) {
            Log::trace("TexelAAPatch", "Found patch at @%x", i);
            if(ptr[i + 0x24] != 0x8D && ptr[i + 0x24 + 1] != 0x83) {
                Log::trace("TexelAAPatch", "LDR instruction invalid; patch incompatible");
                return;
            }
            ptr[i + 0x24] = 0xB8;  // mov eax, lamda_addr
            (void *&)ptr[i + 0x24 + 1] = (void *)+[] { return true; };
            ptr[i + 0x24 + 5] = 0x90;  // nop
            break;
        }
    }
}
