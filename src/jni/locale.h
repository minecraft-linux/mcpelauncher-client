#pragma once
#include <fake-jni/fake-jni.h>
#include <locale>

class Locale : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/util/Locale")

    Locale(std::locale locale);
    static std::shared_ptr<Locale> getDefault();
    std::shared_ptr<FakeJni::JString> toString();

private:
    std::locale l;
};
