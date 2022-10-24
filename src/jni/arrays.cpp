#include "arrays.h"

std::shared_ptr<FakeJni::JByteArray> Arrays::copyOfRange(std::shared_ptr<FakeJni::JByteArray> in, int i, int n) {
    auto ret = std::make_shared<FakeJni::JByteArray>(n - i);
    memcpy(ret->getArray(), in->getArray() + i, n - i);
    return ret;
}
