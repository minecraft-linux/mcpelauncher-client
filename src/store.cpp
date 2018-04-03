#include "store.h"

#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>

void LauncherStore::install(void* handle) {
    void* ptr = hybris_dlsym(handle, "_ZN12AndroidStore21createGooglePlayStoreERKSsR13StoreListener");
    PatchUtils::patchCallInstruction(ptr, (void*) &LauncherStore::createStore, true);
}