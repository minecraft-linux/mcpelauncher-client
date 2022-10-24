#pragma once
#include <fake-jni/fake-jni.h>
#include <openssl/evp.h>

class ShaHasher : public FakeJni::JObject {
    EVP_MD_CTX *mdctx = NULL;

public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/ShaHasher")

    ShaHasher();

    ~ShaHasher();

    void AddBytes(std::shared_ptr<FakeJni::JByteArray> barray);

    std::shared_ptr<FakeJni::JByteArray> SignHash();
};
