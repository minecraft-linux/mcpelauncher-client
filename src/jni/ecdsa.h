#pragma once
#include <base64.h>
#include <fake-jni/fake-jni.h>
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <stdexcept>
#include <string>
#include <utility>
#include "main_activity.h"

class EcdsaPublicKey : public FakeJni::JObject {
    std::string xbase64;
    std::string ybase64;

   public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/EccPubKey")

    EcdsaPublicKey(EC_KEY *eckey, EC_GROUP *ecgroup);

    std::shared_ptr<FakeJni::JString> getBase64UrlX();

    std::shared_ptr<FakeJni::JString> getBase64UrlY();
};

class Ecdsa : public FakeJni::JObject {
   public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/Ecdsa")
    Ecdsa();

    ~Ecdsa();

    void generateKey(std::shared_ptr<FakeJni::JString> unique_id);

    static EC_KEY *eckey;
    static EC_GROUP *ecgroup;
    static std::shared_ptr<FakeJni::JString> unique_id;

    std::shared_ptr<FakeJni::JByteArray> sign(std::shared_ptr<FakeJni::JByteArray> a);

    std::shared_ptr<EcdsaPublicKey> getPublicKey();

    std::shared_ptr<FakeJni::JString> getUniqueId();

    static std::shared_ptr<Ecdsa> restoreKeyAndId(std::shared_ptr<Context> ctx);
};