#include "pulseaudio.h"
#include <pulse/simple.h>
#include <pulse/error.h>

AudioDevice::AudioDevice() {
    s = nullptr;
}

FakeJni::JBoolean AudioDevice::init(FakeJni::JInt channels, FakeJni::JInt samplerate, FakeJni::JInt c, FakeJni::JInt d) {
    if (s != NULL) {
        throw std::runtime_error("Illegal State, pulseaudio already initialized");
    }
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16NE;
    ss.channels = channels;
    ss.rate = samplerate;
    pa_buffer_attr b;
    b.fragsize = -1;
    b.maxlength = c * d * channels * 2;
    b.minreq = -1;
    b.prebuf = -1;
    b.tlength = -1;
    int error = 0;
    s = pa_simple_new(NULL,             // Use the default server.
                    "mcpelauncher",     // Our application's name.
                    PA_STREAM_PLAYBACK,
                    NULL,               // Use the default device.
                    "Music",            // Description of our stream.
                    &ss,                // Our sample format.
                    NULL,               // Use default channel map
                    &b,                 // buffering attributes.
                    &error              // error code.
                    );
    if (s == NULL) {
        auto errormsg = pa_strerror(error);
        throw std::runtime_error(std::string("Illegal State, pulseaudio pa_simple_new failed: ") + (errormsg ? errormsg : "No message from pulseaudio"));
    }
    return true;
}

void AudioDevice::write(std::shared_ptr<FakeJni::JByteArray> data, FakeJni::JInt length) {
    int error = 0;
    if (pa_simple_write(s, data->getArray(), length, &error)) {
        auto errormsg = pa_strerror(error);
        throw std::runtime_error(std::string("Illegal State, pulseaudio pa_simple_write failed: ") + (errormsg ? errormsg : "No message from pulseaudio"));
    }
}

void AudioDevice::close() {
    int error = 0;
    if (pa_simple_flush(s, &error)) {
        auto errormsg = pa_strerror(error);
        throw std::runtime_error(std::string("Illegal State, pulseaudio pa_simple_flush failed: ") + (errormsg ? errormsg : "No message from pulseaudio"));
    }
    pa_simple_free(s);
    s = nullptr;
}