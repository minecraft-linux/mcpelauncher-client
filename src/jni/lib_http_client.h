#pragma once

#include <fake-jni/fake-jni.h>
#include "main_activity.h"
#include <log.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <utility>
#include <base64.h>
#include <stdexcept>

class EcdsaPublicKey : public FakeJni::JObject {
    std::string xbase64;
    std::string ybase64;
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/EccPubKey")

    EcdsaPublicKey(EC_KEY * eckey, EC_GROUP * ecgroup) {
        const EC_POINT * point = EC_KEY_get0_public_key(eckey);
        if (!point) {
            throw std::runtime_error("OpenSSL failed to get ecdsa public key");
        }
        BIGNUM *x = BN_new(), *y = BN_new();
        if (!x || !y) {
            throw std::runtime_error("OpenSSL failed to allocate bignum");
        }
        if (EC_POINT_get_affine_coordinates(ecgroup, point, x, y, NULL) != 1) {
            throw std::runtime_error("OpenSSL failed to get affine coordinates");
        }
        auto len = BN_num_bytes(x);
        std::string buf(len, '_');
        if (BN_bn2bin(x, (unsigned char*)buf.data()) != len) {
            throw std::runtime_error("OpenSSL failed to export bignum");
        }
        xbase64 = Base64::encode(buf);
        len = BN_num_bytes(y);
        buf = std::string(len, '_');
        if (BN_bn2bin(y, (unsigned char*)buf.data()) != len) {
            throw std::runtime_error("OpenSSL failed to export bignum");
        }
        ybase64 = Base64::encode(buf);
    }

    std::shared_ptr<FakeJni::JString> getBase64UrlX() {
        return std::make_shared<FakeJni::JString>(xbase64);
    }

    std::shared_ptr<FakeJni::JString> getBase64UrlY() {
        return std::make_shared<FakeJni::JString>(ybase64);
    }
};

class Ecdsa : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/microsoft/xal/crypto/Ecdsa")
    Ecdsa();

    ~Ecdsa() {
        EC_KEY_free(eckey);
        EC_GROUP_free(ecgroup);
    }

    void generateKey(std::shared_ptr<FakeJni::JString> unique_id) {
        this->unique_id = unique_id;
        ecgroup = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1); //openssl alias of secp256r1
        if (!ecgroup) {
            throw std::runtime_error("OpenSSL failed to allocate group prime256v1");
        }
        eckey = EC_KEY_new();
        if (!eckey) {
            throw std::runtime_error("OpenSSL failed to allocate eckey");
        }
        if (EC_KEY_set_group(eckey, ecgroup) != 1) {
            throw std::runtime_error("OpenSSL failed to get affine coordinates");
        }
        if (EC_KEY_generate_key(eckey) != 1) {
            throw std::runtime_error("OpenSSL failed to get affine coordinates");
        }
    }

    static EC_KEY * eckey;
    static EC_GROUP * ecgroup;
    static std::shared_ptr<FakeJni::JString> unique_id;

    std::shared_ptr<FakeJni::JByteArray> sign(std::shared_ptr<FakeJni::JByteArray> a) {
        ECDSA_SIG * sig = ECDSA_do_sign((const unsigned char*)a->getArray(), a->getSize(), eckey);
        if (!sig) {
            throw std::runtime_error("OpenSSL failed to sign bytearray");
        }
        auto n = (EC_GROUP_order_bits(ecgroup) + 7) / 8;
        const BIGNUM* r = ECDSA_SIG_get0_r(sig);
        if (!r) {
            throw std::runtime_error("OpenSSL failed to get r");
        }
        const BIGNUM* s = ECDSA_SIG_get0_s(sig);
        if (!s) {
            throw std::runtime_error("OpenSSL failed to get s");
        }
        auto buf = std::make_shared<FakeJni::JByteArray>(2 * n);
        if (BN_bn2binpad(r, (unsigned char*)buf->getArray(), n) != n) {
            throw std::runtime_error("OpenSSL failed to export bignum");
        }
        if (BN_bn2binpad(s, (unsigned char*)buf->getArray() + n, n) != n) {
            throw std::runtime_error("OpenSSL failed to export bignum");
        }
        ECDSA_SIG_free(sig);
        return buf;
    }

    std::shared_ptr<EcdsaPublicKey> getPublicKey() {
        return std::make_shared<EcdsaPublicKey>(eckey, ecgroup);
    }

    std::shared_ptr<FakeJni::JString> getUniqueId() {
        return unique_id;
    }

    static std::shared_ptr<Ecdsa> restoreKeyAndId(std::shared_ptr<Context> ctx) {
        if(eckey == NULL || ecgroup == NULL) {
            return nullptr;
        }
        return std::make_shared<Ecdsa>();
    }
    
};

class ResponseHeader {
public:
    ResponseHeader(std::string name, std::string value) : name(std::move(name)), value(std::move(value)) {}
    std::string name;
    std::string value;
};

class HttpClientRequest : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientRequest")
    HttpClientRequest();
    ~HttpClientRequest();

    static FakeJni::JBoolean isNetworkAvailable(std::shared_ptr<Context> context);
    static std::shared_ptr<HttpClientRequest> createClientRequest();

    void setHttpUrl(std::shared_ptr<FakeJni::JString> url);
    void setHttpMethodAndBody(std::shared_ptr<FakeJni::JString> method, std::shared_ptr<FakeJni::JString> contentType, std::shared_ptr<FakeJni::JByteArray> body);
    void setHttpHeader(std::shared_ptr<FakeJni::JString> name, std::shared_ptr<FakeJni::JString> value);
    void doRequestAsync(FakeJni::JLong sourceCall);
    static size_t write_callback_wrapper(char *ptr, size_t size, size_t nmemb, void *userdata) {
        auto *self = static_cast<HttpClientRequest*>(userdata);
        return self->write_callback(ptr, size, nmemb);
    }
    static size_t header_callback_wrapper(char *ptr, size_t size, size_t nmemb, void *userdata) {
        auto *self = static_cast<HttpClientRequest*>(userdata);
        return self->header_callback(ptr, size, nmemb);
    }

private:
    void *curl;
    struct curl_slist * header = nullptr;
    std::vector<signed char> response;
    std::vector<ResponseHeader> headers;
    std::vector<char> body;
    std::string method;

    size_t write_callback(char *ptr, size_t size, size_t nmemb);
    size_t header_callback(char *buffer,   size_t size,   size_t nitems);
};

class HttpClientResponse : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/xbox/httpclient/HttpClientResponse")

    HttpClientResponse(int response_code, std::vector<signed char> body, std::vector<ResponseHeader> headers);
    FakeJni::JInt getNumHeaders();
    std::shared_ptr<FakeJni::JString> getHeaderNameAtIndex(FakeJni::JInt index);
    std::shared_ptr<FakeJni::JString> getHeaderValueAtIndex(FakeJni::JInt index);
    std::shared_ptr<FakeJni::JByteArray> getResponseBodyBytes();
    FakeJni::JInt getResponseCode();

private:
    int response_code;
    std::vector<signed char> body;
    std::vector<ResponseHeader> headers;
};
