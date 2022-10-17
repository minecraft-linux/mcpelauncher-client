#include "signature.h"

void Signature::initVerify(std::shared_ptr<PublicKey> key) {
}

FakeJni::JBoolean Signature::verify(std::shared_ptr<FakeJni::JByteArray> a) {
    return true;
}

std::shared_ptr<Signature> Signature::getInstance(std::shared_ptr<FakeJni::JString>) {
    return std::make_shared<Signature>();
}
