#pragma once
#include <fake-jni/fake-jni.h>

class SecureRandom : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/SecureRandom")
    static std::shared_ptr<FakeJni::JByteArray> GenerateRandomBytes(int bytes);
};
