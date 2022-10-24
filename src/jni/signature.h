#pragma once
#include <fake-jni/fake-jni.h>

class PublicKey : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/security/PublicKey")
};

class Signature : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/security/Signature")

    void initVerify(std::shared_ptr<PublicKey>);
    FakeJni::JBoolean verify(std::shared_ptr<FakeJni::JByteArray>);

    static std::shared_ptr<Signature> getInstance(std::shared_ptr<FakeJni::JString>);
};
