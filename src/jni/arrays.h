#pragma once
#include <fake-jni/fake-jni.h>

class Arrays : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/util/Arrays")

    static std::shared_ptr<FakeJni::JByteArray> copyOfRange(std::shared_ptr<FakeJni::JByteArray>, int i, int n);
};
