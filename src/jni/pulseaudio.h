#pragma once

#include <fake-jni/fake-jni.h>
#include <pulse/simple.h>

class AudioDevice : public FakeJni::JObject {
    pa_simple *s;

   public:
    DEFINE_CLASS_NAME("org/fmod/AudioDevice")

    AudioDevice();

    FakeJni::JBoolean init(FakeJni::JInt channels, FakeJni::JInt samplerate, FakeJni::JInt c, FakeJni::JInt d);

    void write(std::shared_ptr<FakeJni::JByteArray> data, FakeJni::JInt length);

    void close();
};