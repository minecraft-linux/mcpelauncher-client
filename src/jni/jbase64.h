#pragma once
#include <fake-jni/fake-jni.h>

class JBase64 : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("android/util/Base64")

    static std::shared_ptr<FakeJni::JByteArray> decode(std::shared_ptr<FakeJni::JString>, int flags);
};
