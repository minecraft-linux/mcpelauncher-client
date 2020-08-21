#include <fake-jni/fake-jni.h>

class JBase64 : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("android/util/Base64")

    static std::shared_ptr<FakeJni::JByteArray> decode(std::shared_ptr<FakeJni::JString>, int flags);
};


class Arrays : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("java/util/Arrays")

    static std::shared_ptr<FakeJni::JByteArray> copyOfRange(std::shared_ptr<FakeJni::JByteArray>, int i, int n);
};

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