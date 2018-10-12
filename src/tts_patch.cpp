#include "tts_patch.h"

#include <mcpelauncher/patch_utils.h>
#include <hybris/dlfcn.h>

void TTSPatch::install(void* handle) {
    void* sym = hybris_dlsym(handle, "_ZN18TextToSpeechSystem15createTTSClientEb");
    PatchUtils::patchCallInstruction(sym, (void*) &TTSPatch::createTTSClient, true);
}

std::shared_ptr<NullTextToSpeechClient> TTSPatch::createTTSClient() {
    auto ret = new NullTextToSpeechClient;
    ret->vtable = &NullTextToSpeechClient::myVtable[2];
    return std::shared_ptr<NullTextToSpeechClient>(ret);
}