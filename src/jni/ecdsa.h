#pragma once
#include <fake-jni/fake-jni.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <utility>
#include <base64.h>
#include <stdexcept>
#include <string>
#include "main_activity.h"

class EcdsaPublicKey : public FakeJni::JObject {
    std::string xbase64;
    std::string ybase64;

public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/EccPubKey")

    EcdsaPublicKey(EC_KEY* eckey, EC_GROUP* ecgroup);

    std::shared_ptr<FakeJni::JString> getBase64UrlX();

    std::shared_ptr<FakeJni::JString> getBase64UrlY();
};

class Ecdsa : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/Ecdsa")
    Ecdsa();

    ~Ecdsa();

    void generateKey(std::shared_ptr<FakeJni::JString> unique_id);

    EC_KEY* eckey = NULL;
    EC_GROUP* ecgroup = NULL;
    std::shared_ptr<FakeJni::JString> unique_id = std::make_shared<FakeJni::JString>();

    std::shared_ptr<FakeJni::JByteArray> sign(std::shared_ptr<FakeJni::JByteArray> a);

    std::shared_ptr<EcdsaPublicKey> getPublicKey();

    std::shared_ptr<FakeJni::JString> getUniqueId();

    static std::shared_ptr<Ecdsa> restoreKeyAndId(std::shared_ptr<Context> ctx);
};
