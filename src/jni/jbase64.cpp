#include "jbase64.h"
#include <base64.h>

std::shared_ptr<FakeJni::JByteArray> JBase64::decode(std::shared_ptr<FakeJni::JString> value, int flags) {
    auto dec = Base64::decode(value->asStdString());
    auto ret = std::make_shared<FakeJni::JByteArray>(dec.length());
    memcpy(ret->getArray(), dec.data(), dec.length());
    return ret;
}
