#pragma once

#include <fake-jni/fake-jni.h>
#include <jnivm/bytebuffer.h>

#include <memory>
#include <string>

class CharBuffer : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/nio/CharBuffer")

    explicit CharBuffer(std::string text);
    std::shared_ptr<FakeJni::JString> toString();

private:
    std::string text;
};

class Charset : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/nio/charset/Charset")

    explicit Charset(std::string name);

    static std::shared_ptr<Charset> forName(std::shared_ptr<FakeJni::JString> name);
    std::shared_ptr<CharBuffer> decode(std::shared_ptr<jnivm::ByteBuffer> input);

private:
    std::string name;
};
