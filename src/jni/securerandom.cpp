#include "securerandom.h"

std::shared_ptr<FakeJni::JByteArray> SecureRandom::GenerateRandomBytes(int bytes) {
    return std::make_shared<FakeJni::JByteArray>(bytes);
}