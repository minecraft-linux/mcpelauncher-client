#include "sdl3audio.h"
#include <game_window_manager.h>
#include <SDL3/SDL.h>
#include <thread>
AudioDevice::AudioDevice() {
    s = nullptr;
    SDL_Init(SDL_INIT_AUDIO);
}

AudioDevice::~AudioDevice() {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

FakeJni::JBoolean AudioDevice::init(FakeJni::JInt channels, FakeJni::JInt samplerate, FakeJni::JInt c, FakeJni::JInt d) {
    if(s != NULL) {
        GameWindowManager::getManager()->getErrorHandler()->onError("sdl3audio failed", "sdl3audio already initialized");
    }
    SDL_AudioSpec spec;
    spec.channels = channels;
    spec.format = SDL_AUDIO_S16LE;
    spec.freq = samplerate;
    maxBufferLen = c * d * channels * 2;
    s = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_OUTPUT, &spec, nullptr, nullptr);
    if(s == NULL) {
        auto errormsg = SDL_GetError();
        GameWindowManager::getManager()->getErrorHandler()->onError("sdl3audio failed", std::string("sdl3audio SDL_OpenAudioDeviceStream failed, audio will be unavailable: ") + (errormsg ? errormsg : "No message from sdl3audio"));
        return false;
    }
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(s));
    return true;
}

void AudioDevice::write(std::shared_ptr<FakeJni::JByteArray> data, FakeJni::JInt length) {
    SDL_PutAudioStreamData(s, data->getArray(), length);
    // SDL3 cannot set any max buf size and fmod doesn't feeding data with the correct rate without it
    // appeared as silence and a queue overflow
    while(SDL_GetAudioStreamQueued(s) > maxBufferLen) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void AudioDevice::close() {
    SDL_DestroyAudioStream(s);
    s = nullptr;
}
