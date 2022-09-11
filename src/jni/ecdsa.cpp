#include "ecdsa.h"

Ecdsa::Ecdsa() = default;

EC_KEY *Ecdsa::eckey = NULL;
EC_GROUP *Ecdsa::ecgroup = NULL;
std::shared_ptr<FakeJni::JString> Ecdsa::unique_id = NULL;

Ecdsa::~Ecdsa()
{
    EC_KEY_free(eckey);
    EC_GROUP_free(ecgroup);
}

void Ecdsa::generateKey(std::shared_ptr<FakeJni::JString> unique_id)
{
    this->unique_id = unique_id;
    ecgroup = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);  // openssl alias of secp256r1
    if (!ecgroup)
    {
        throw std::runtime_error("OpenSSL failed to allocate group prime256v1");
    }
    eckey = EC_KEY_new();
    if (!eckey)
    {
        throw std::runtime_error("OpenSSL failed to allocate eckey");
    }
    if (EC_KEY_set_group(eckey, ecgroup) != 1)
    {
        throw std::runtime_error("OpenSSL failed to get affine coordinates");
    }
    if (EC_KEY_generate_key(eckey) != 1)
    {
        throw std::runtime_error("OpenSSL failed to get affine coordinates");
    }
}

std::shared_ptr<FakeJni::JByteArray> Ecdsa::sign(std::shared_ptr<FakeJni::JByteArray> a)
{
    ECDSA_SIG *sig = ECDSA_do_sign((const unsigned char *)a->getArray(), a->getSize(), eckey);
    if (!sig)
    {
        throw std::runtime_error("OpenSSL failed to sign bytearray");
    }
    auto n = (EC_GROUP_order_bits(ecgroup) + 7) / 8;
    const BIGNUM *r = ECDSA_SIG_get0_r(sig);
    if (!r)
    {
        throw std::runtime_error("OpenSSL failed to get r");
    }
    const BIGNUM *s = ECDSA_SIG_get0_s(sig);
    if (!s)
    {
        throw std::runtime_error("OpenSSL failed to get s");
    }
    auto buf = std::make_shared<FakeJni::JByteArray>(2 * n);
    if (BN_bn2binpad(r, (unsigned char *)buf->getArray(), n) != n)
    {
        throw std::runtime_error("OpenSSL failed to export bignum");
    }
    if (BN_bn2binpad(s, (unsigned char *)buf->getArray() + n, n) != n)
    {
        throw std::runtime_error("OpenSSL failed to export bignum");
    }
    ECDSA_SIG_free(sig);
    return buf;
}

std::shared_ptr<EcdsaPublicKey> Ecdsa::getPublicKey() { return std::make_shared<EcdsaPublicKey>(eckey, ecgroup); }

std::shared_ptr<FakeJni::JString> Ecdsa::getUniqueId() { return unique_id; }

std::shared_ptr<Ecdsa> Ecdsa::restoreKeyAndId(std::shared_ptr<Context> ctx)
{
    if (eckey == NULL || ecgroup == NULL)
    {
        return nullptr;
    }
    return std::make_shared<Ecdsa>();
}

EcdsaPublicKey::EcdsaPublicKey(EC_KEY *eckey, EC_GROUP *ecgroup)
{
    const EC_POINT *point = EC_KEY_get0_public_key(eckey);
    if (!point)
    {
        throw std::runtime_error("OpenSSL failed to get ecdsa public key");
    }
    BIGNUM *x = BN_new(), *y = BN_new();
    if (!x || !y)
    {
        throw std::runtime_error("OpenSSL failed to allocate bignum");
    }
    if (EC_POINT_get_affine_coordinates(ecgroup, point, x, y, NULL) != 1)
    {
        throw std::runtime_error("OpenSSL failed to get affine coordinates");
    }
    auto len = BN_num_bytes(x);
    std::string buf(len, '_');
    if (BN_bn2bin(x, (unsigned char *)buf.data()) != len)
    {
        throw std::runtime_error("OpenSSL failed to export bignum");
    }
    xbase64 = Base64::encode(buf);
    len = BN_num_bytes(y);
    buf = std::string(len, '_');
    if (BN_bn2bin(y, (unsigned char *)buf.data()) != len)
    {
        throw std::runtime_error("OpenSSL failed to export bignum");
    }
    ybase64 = Base64::encode(buf);
}

std::shared_ptr<FakeJni::JString> EcdsaPublicKey::getBase64UrlX()
{
    return std::make_shared<FakeJni::JString>(xbase64);
}

std::shared_ptr<FakeJni::JString> EcdsaPublicKey::getBase64UrlY()
{
    return std::make_shared<FakeJni::JString>(ybase64);
}