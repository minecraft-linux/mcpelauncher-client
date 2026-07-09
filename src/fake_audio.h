#ifdef HAVE_SDL3AUDIO
#include <unordered_map>
#include <string>
#include <SDL3/SDL.h>
#include <android/audio.h>

class FakeAudio {
private:
    static int32_t defaultSampleRate;
    static int32_t defaultNumChannels;
    static int32_t defaultBufSize;

    struct FakeAudioStreamBuilder {
        AAudioStream_dataCallback _Nullable dataCallback = NULL;
        void *_Nullable dataCallbackUser = NULL;

        AAudioStream_errorCallback _Nullable errorCallback = NULL;
        void *_Nullable errorCallbackUser = NULL;

        int32_t bufferCap = defaultBufSize;
        int32_t sampleRate = 0;  // 0 = use default
        int32_t channelCount = 0;  // 0 = use default
        aaudio_format_t format = AAUDIO_FORMAT_UNSPECIFIED;
    };

    struct FakeAudioStream {
        AAudioStream_dataCallback _Nullable dataCallback;
        void *_Nullable dataCallbackUser;

        AAudioStream_errorCallback _Nullable errorCallback;
        void *_Nullable errorCallbackUser;

        int32_t bufferCap;
        int32_t bufferSize = defaultBufSize;
        int32_t sampleRate = defaultSampleRate;
        int32_t channelCount = defaultNumChannels;

        aaudio_format_t format = AAUDIO_FORMAT_PCM_I16;

        void *_Nullable audioBuffer;
        int audioBufferSize = 0;

        SDL_AudioStream *_Nullable s = NULL;

        int32_t getBytesPerSample() {
            switch(format) {
            case AAUDIO_FORMAT_INVALID:
                return 0;
            case AAUDIO_FORMAT_PCM_I16:
                return 2;
            case AAUDIO_FORMAT_PCM_FLOAT:
            case AAUDIO_FORMAT_PCM_I32:
                return 4;
            case AAUDIO_FORMAT_PCM_I24_PACKED:
                return 3;
            default:
                return 1;
            }
        }
    };

public:
    static void
    initHybrisHooks(std::unordered_map<std::string, void *> &syms);

    static void updateDefaults();
};
#endif