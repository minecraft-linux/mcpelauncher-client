#include "shahasher.h"

ShaHasher::ShaHasher() {
    const EVP_MD* md = EVP_get_digestbynid(NID_sha256);
    if(md == NULL) {
        throw std::runtime_error("OpenSSL failed to get \"SHA2-256\"");
    }
    mdctx = EVP_MD_CTX_new();
    if(md == NULL) {
        throw std::runtime_error("OpenSSL failed to create mdctx");
    }
    if(EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        throw std::runtime_error("OpenSSL failed to initialize evp digest");
    }
}

ShaHasher::~ShaHasher() {
    EVP_MD_CTX_free(mdctx);
}

void ShaHasher::AddBytes(std::shared_ptr<FakeJni::JByteArray> barray) {
    auto data = barray->getArray();
    auto len = barray->getSize();
    if(EVP_DigestUpdate(mdctx, data, len) != 1) {
        throw std::runtime_error("OpenSSL failed to update evp digest");
    }
}

std::shared_ptr<FakeJni::JByteArray> ShaHasher::SignHash() {
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
    if(EVP_DigestFinal_ex(mdctx, md_value, &md_len) != 1) {
        throw std::runtime_error("OpenSSL failed to finish evp digest");
    }
    auto arr = std::make_shared<FakeJni::JByteArray>(md_len);
    memcpy(arr->getArray(), md_value, md_len);
    return arr;
}
