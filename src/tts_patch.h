#pragma once

#include <memory>
#include <minecraft/NullTextToSpeechClient.h>
#include <minecraft/std/shared_ptr.h>

class TTSPatch {

private:
    static mcpe::shared_ptr<NullTextToSpeechClient> createTTSClient();

public:
    static void install(void* handle);

};