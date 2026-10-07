#ifdef HAVE_SDL3AUDIO
#include "fake_audio.h"
#include "settings.h"
#include <game_window_manager.h>
#include <mcpelauncher/fmod_utils.h>
#include <thread>
#include "util.h"

int32_t FakeAudio::defaultSampleRate = 48000;
int32_t FakeAudio::defaultNumChannels = 2;
int32_t FakeAudio::defaultBufSize = 512;
void FakeAudio::initHybrisHooks(std::unordered_map<std::string, void *> &syms) {
    syms["AAudioStreamBuilder_openStream"] = (void *)+[](FakeAudioStreamBuilder *_Nonnull builder, FakeAudioStream *_Nullable *_Nonnull stream) -> aaudio_result_t {
        *stream = new FakeAudioStream{.dataCallback = builder->dataCallback, .dataCallbackUser = builder->dataCallbackUser, .errorCallback = builder->errorCallback, .errorCallbackUser = builder->errorCallbackUser, .bufferCap = builder->bufferCap};
        if (builder->sampleRate > 0) (*stream)->sampleRate = builder->sampleRate;
        if (builder->channelCount > 0) (*stream)->channelCount = builder->channelCount;
        if (builder->format != AAUDIO_FORMAT_UNSPECIFIED) (*stream)->format = builder->format;
        (*stream)->audioBufferSize = builder->bufferCap * (*stream)->getBytesPerSample() * (*stream)->channelCount;
        (*stream)->audioBuffer = malloc((*stream)->audioBufferSize);
        return AAUDIO_OK;
    };
    syms["AAudio_createStreamBuilder"] = (void *)+[](FakeAudioStreamBuilder *_Nullable *_Nonnull builder) -> aaudio_result_t {
        SDL_Init(SDL_INIT_AUDIO);
        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_ICON_NAME, "mcpelauncher-ui-qt");
        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, "Minecraft");
        FakeAudio::updateDefaults();
        *builder = new FakeAudioStreamBuilder{};
        return AAUDIO_OK;
    };
    syms["AAudioStreamBuilder_setBufferCapacityInFrames"] = (void *)+[](FakeAudioStreamBuilder *_Nonnull builder, int32_t newCap) -> void {
        builder->bufferCap = newCap;
    };
    syms["AAudioStreamBuilder_setDataCallback"] = (void *)+[](FakeAudioStreamBuilder *_Nonnull builder, AAudioStream_dataCallback _Nullable callback, void *_Nullable userData) {
        builder->dataCallback = callback;
        builder->dataCallbackUser = userData;
    };
    syms["AAudioStream_getXRunCount"] = (void *)+[]() -> int32_t {
        return 0;
    };
    syms["AAudioStreamBuilder_setErrorCallback"] = (void *)+[](FakeAudioStreamBuilder *_Nonnull builder, AAudioStream_errorCallback _Nullable callback, void *_Nullable userData) {
        builder->errorCallback = callback;
        builder->errorCallbackUser = userData;
    };
    syms["AAudioStream_getBufferSizeInFrames"] = (void *)+[](FakeAudioStream *_Nonnull stream) -> int32_t {
        return stream->bufferSize;
    };
    syms["AAudioStream_close"] = (void *)+[](FakeAudioStream *_Nonnull stream) {
        free(stream->audioBuffer);
        stream->audioBuffer = NULL;
        stream->audioBufferSize = 0;
    };
    syms["AAudioStreamBuilder_setDirection"] = (void *)+[](AAudioStreamBuilder *_Nonnull builder, aaudio_direction_t direction) {
    };
    syms["AAudioStream_setBufferSizeInFrames"] = (void *)+[](FakeAudioStream *_Nonnull stream, int32_t newSize) -> void {
        stream->bufferSize = newSize;
        stream->audioBufferSize = stream->bufferSize * stream->channelCount * stream->getBytesPerSample();
        stream->audioBuffer = realloc(stream->audioBuffer, stream->audioBufferSize);
    };
    syms["AAudioStream_getChannelCount"] = (void *)+[](FakeAudioStream *_Nonnull stream) -> int32_t {
        return stream->channelCount;
    };
    syms["AAudioStream_getFramesPerBurst"] = (void *)+[](FakeAudioStream *_Nonnull stream) -> int32_t {
        return stream->bufferSize;
    };
    syms["AAudioStreamBuilder_delete"] = (void *)+[]() {
    };
    syms["AAudioStream_requestStop"] = (void *)+[](FakeAudioStream *_Nonnull stream) {
        SDL_AudioStream *s = stream->s;
        stream->s = NULL;
        SDL_DestroyAudioStream(s);
    };
    syms["AAudioStream_getBufferCapacityInFrames"] = (void *)+[](FakeAudioStream *_Nonnull stream) -> int32_t {
        return stream->bufferCap;
    };
    syms["AAudioStreamBuilder_setInputPreset"] = (void *)+[]() {
    };
    syms["AAudioStreamBuilder_setSampleRate"] = (void *)+[](FakeAudioStreamBuilder *_Nonnull builder, int32_t sampleRate) {
        builder->sampleRate = sampleRate;
    };
    syms["AAudioStreamBuilder_setChannelCount"] = (void *)+[](FakeAudioStreamBuilder *_Nonnull builder, int32_t channelCount) {
        builder->channelCount = channelCount;
    };
    syms["AAudioStreamBuilder_setFormat"] = (void *)+[](FakeAudioStreamBuilder *_Nonnull builder, aaudio_format_t format) {
        builder->format = format;
    };
    syms["AAudioStream_getSampleRate"] = (void *)+[](FakeAudioStream *_Nonnull stream) -> int32_t {
        return stream->sampleRate;
    };
    syms["AAudioStream_read"] = (void *)+[]() {
    };
    syms["AAudioStreamBuilder_setPerformanceMode"] = (void *)+[](AAudioStreamBuilder *_Nonnull builder, aaudio_performance_mode_t mode) -> void {
    };
    syms["AAudioStream_getState"] = (void *)+[](FakeAudioStream *_Nonnull stream) -> aaudio_stream_state_t {
        if(!stream->s) {
            return AAUDIO_STREAM_STATE_OPEN;
        }
        // Backport to SDL 3.1 Audio API for legacy macOS support
        SDL_AudioDeviceID devid = SDL_GetAudioStreamDevice(stream->s);
        if (!devid) {
            return AAUDIO_STREAM_STATE_CLOSED;
        }
    
        return SDL_AudioDevicePaused(devid) ? AAUDIO_STREAM_STATE_PAUSED : AAUDIO_STREAM_STATE_STARTED;
    };
    syms["AAudioStream_getFormat"] = (void *)+[](FakeAudioStream *_Nonnull stream) -> aaudio_format_t {
        return stream->format;
    };
    syms["AAudioStreamBuilder_setUsage"] = (void *)+[](AAudioStreamBuilder *_Nonnull builder, aaudio_usage_t usage) {
    };
    syms["AAudioStream_requestStart"] = (void *)+[](FakeAudioStream *_Nonnull stream) -> aaudio_result_t {
        SDL_AudioSpec spec;
        spec.channels = stream->channelCount;
        switch(stream->format) {
        case AAUDIO_FORMAT_PCM_I16:
            spec.format = SDL_AUDIO_S16LE;
            break;
        case AAUDIO_FORMAT_PCM_I32:
            spec.format = SDL_AUDIO_S32LE;
            break;
        default:
            spec.format = SDL_AUDIO_S16LE;
            break;
        }
        spec.freq = stream->sampleRate;
        stream->s = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, [](void *userdata, SDL_AudioStream *sdlStream, int additional_amount, int total_amount) {
            FakeAudioStream *stream = (FakeAudioStream *)userdata; 
            if(stream->dataCallback == NULL || stream->s == NULL || stream->audioBuffer == NULL) {
                return;
            }
            if (additional_amount > stream->audioBufferSize) {
                stream->audioBufferSize = additional_amount;
                stream->audioBuffer = realloc(stream->audioBuffer, stream->audioBufferSize);        
            }
            stream->dataCallback((AAudioStream *)stream, stream->dataCallbackUser, stream->audioBuffer, additional_amount / (stream->channelCount * stream->getBytesPerSample()));
            if(!SDL_PutAudioStreamData(stream->s, stream->audioBuffer, additional_amount)) {
                if(stream->errorCallback != NULL) {
                    stream->errorCallback((AAudioStream *)stream, stream->errorCallbackUser, AAUDIO_ERROR_DISCONNECTED);
                }
            } }, stream);
        if(stream->s == NULL) {
            auto errormsg = SDL_GetError();
            GameWindowManager::getManager()->getErrorHandler()->onError("sdl3audio failed", std::string("sdl3audio SDL_OpenAudioDeviceStream failed, audio will be unavailable: ") + (errormsg ? errormsg : "No message from sdl3audio"));
            return AAUDIO_OK;  // fmod tries to open it over and over again if it fails
        }
        SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream->s));

        return AAUDIO_OK;
    };
}

void FakeAudio::updateDefaults() {
    SDL_AudioSpec spec = {};
    int sampleFrames = 0;
    if (SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &sampleFrames)) {
        if (spec.freq > 0) defaultSampleRate = ReadEnvInt("AUDIO_SAMPLE_RATE", spec.freq);
        if (spec.channels > 0 && spec.channels <= 8) defaultNumChannels = spec.channels;
        if (sampleFrames > 0) defaultBufSize = sampleFrames;
    }

    FmodUtils::setSampleRate(defaultSampleRate);
}
#endif
