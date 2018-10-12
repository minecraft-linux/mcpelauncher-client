#pragma once

#include <memory>
#include <minecraft/NullTextToSpeechClient.h>

class TTSPatch {

private:
    static std::shared_ptr<NullTextToSpeechClient> createTTSClient();

public:
    static void install(void* handle);

};