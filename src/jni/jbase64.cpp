#include "jbase64.h"
#include <base64.h>

std::shared_ptr<FakeJni::JByteArray> JBase64::decode(std::shared_ptr<FakeJni::JString> value, int flags) {
    auto dec = Base64::decode(value->asStdString());
    auto ret = std::make_shared<FakeJni::JByteArray>(dec.length());
    memcpy(ret->getArray(), dec.data(), dec.length());
    return ret;
}

std::shared_ptr<FakeJni::JByteArray> Arrays::copyOfRange(std::shared_ptr<FakeJni::JByteArray> in, int i, int n) {
    auto ret = std::make_shared<FakeJni::JByteArray>(n);
    memcpy(ret->getArray(), in->getArray() + i, n);
    return ret;
}

void Signature::initVerify(std::shared_ptr<PublicKey> key) {

}

FakeJni::JBoolean Signature::verify(std::shared_ptr<FakeJni::JByteArray> a) {
    return true;
}

std::shared_ptr<Signature> Signature::getInstance(std::shared_ptr<FakeJni::JString>) {
    return std::make_shared<Signature>();
}
