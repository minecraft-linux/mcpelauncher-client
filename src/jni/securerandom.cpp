#include "securerandom.h"
#include <openssl/rand.h>

std::shared_ptr<FakeJni::JByteArray> SecureRandom::GenerateRandomBytes(int bytes) {
    auto randomBytes = std::make_shared<FakeJni::JByteArray>(bytes);
    RAND_bytes((unsigned char *)randomBytes->getArray(), bytes);
    return randomBytes;
}
