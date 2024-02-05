#include "ecdsa.h"

// TODO migrate to OPENSSL 3 API

Ecdsa::Ecdsa() = default;

class defer
{
    std::function<void()> exec;
public:
    defer(std::function<void()> exec) {
        this->exec = exec;
    }
    ~defer() {
        exec();
    }
};


Ecdsa::~Ecdsa() {
    EC_KEY_free(eckey);
    eckey = nullptr;
    EC_GROUP_free(ecgroup);
    ecgroup = nullptr;
}

void Ecdsa::generateKey(std::shared_ptr<FakeJni::JString> unique_id) {
    if(eckey) {
        EC_KEY_free(eckey);
        eckey = nullptr;
    }
    if(ecgroup) {
        EC_GROUP_free(ecgroup);
        ecgroup = nullptr;
    }
    this->unique_id = unique_id;
    ecgroup = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);  // openssl alias of secp256r1
    if(!ecgroup) {
        throw std::runtime_error("OpenSSL failed to allocate group prime256v1");
    }
    eckey = EC_KEY_new();
    if(!eckey) {
        throw std::runtime_error("OpenSSL failed to allocate eckey");
    }
    if(EC_KEY_set_group(eckey, ecgroup) != 1) {
        throw std::runtime_error("OpenSSL failed to EC_KEY_set_group");
    }
    if(EC_KEY_generate_key(eckey) != 1) {
        throw std::runtime_error("OpenSSL failed to EC_KEY_generate_key");
    }
}

std::shared_ptr<FakeJni::JByteArray> Ecdsa::sign(std::shared_ptr<FakeJni::JByteArray> a) {
    ECDSA_SIG *sig = ECDSA_do_sign((const unsigned char *)a->getArray(), a->getSize(), eckey);
    if(!sig) {
        throw std::runtime_error("OpenSSL failed to sign bytearray");
    }
    defer _s([&]() {
        ECDSA_SIG_free(sig);
    });
    auto n = (EC_GROUP_order_bits(ecgroup) + 7) / 8;
    const BIGNUM *r = ECDSA_SIG_get0_r(sig);
    if(!r) {
        throw std::runtime_error("OpenSSL failed to get r");
    }
    const BIGNUM *s = ECDSA_SIG_get0_s(sig);
    if(!s) {
        throw std::runtime_error("OpenSSL failed to get s");
    }
    auto buf = std::make_shared<FakeJni::JByteArray>(2 * n);
    if(BN_bn2binpad(r, (unsigned char *)buf->getArray(), n) != n) {
        throw std::runtime_error("OpenSSL failed to export bignum r");
    }
    if(BN_bn2binpad(s, (unsigned char *)buf->getArray() + n, n) != n) {
        throw std::runtime_error("OpenSSL failed to export bignum s");
    }
    return buf;
}

std::shared_ptr<EcdsaPublicKey> Ecdsa::getPublicKey() {
    return std::make_shared<EcdsaPublicKey>(eckey, ecgroup);
}

std::shared_ptr<FakeJni::JString> Ecdsa::getUniqueId() {
    return unique_id;
}

std::shared_ptr<Ecdsa> Ecdsa::restoreKeyAndId(std::shared_ptr<Context> ctx) {
    return nullptr;
}

EcdsaPublicKey::EcdsaPublicKey(EC_KEY *eckey, EC_GROUP *ecgroup) {
    const EC_POINT *point = EC_KEY_get0_public_key(eckey);
    if(!point) {
        throw std::runtime_error("OpenSSL failed to get ecdsa public key");
    }
    BIGNUM *x = BN_new(), *y = BN_new();
    defer _s([&]() {
        BN_free(x);
        BN_free(y);
    });
    if(!x || !y) {
        throw std::runtime_error("OpenSSL failed to allocate bignum");
    }
    if(EC_POINT_get_affine_coordinates(ecgroup, point, x, y, NULL) != 1) {
        throw std::runtime_error("OpenSSL failed to get affine coordinates");
    }
    auto len = BN_num_bytes(x);
    std::string buf(len, '_');
    if(BN_bn2bin(x, (unsigned char *)buf.data()) != len) {
        throw std::runtime_error("OpenSSL failed to export bignum");
    }
    // the game uses three base64 flags for https://developer.android.com/reference/android/util/Base64
    // NO_PADDING | NO_WRAP | URL_SAFE, although the sisu auth service seem to accept normal base64 with padding
    auto base64tobase64url = [](char x) {
        if(x == '+') {
            return '-';
        }
        if(x == '/') {
            return '_';
        }
        return x;
    };
    xbase64 = Base64::encode(buf, false);
    std::transform(xbase64.begin(), xbase64.end(), xbase64.begin(), base64tobase64url);
    len = BN_num_bytes(y);
    buf = std::string(len, '_');
    if(BN_bn2bin(y, (unsigned char *)buf.data()) != len) {
        throw std::runtime_error("OpenSSL failed to export bignum");
    }
    ybase64 = Base64::encode(buf, false);
    std::transform(ybase64.begin(), ybase64.end(), ybase64.begin(), base64tobase64url);
}

std::shared_ptr<FakeJni::JString> EcdsaPublicKey::getBase64UrlX() {
    return std::make_shared<FakeJni::JString>(xbase64);
}

std::shared_ptr<FakeJni::JString> EcdsaPublicKey::getBase64UrlY() {
    return std::make_shared<FakeJni::JString>(ybase64);
}
