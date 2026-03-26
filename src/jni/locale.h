#pragma once
#include <fake-jni/fake-jni.h>
#include <locale>

class Locale : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/util/Locale")

    Locale(std::locale locale);
    static std::shared_ptr<Locale> getDefault();
    std::shared_ptr<FakeJni::JString> toString();
    std::shared_ptr<FakeJni::JString> getLanguage() {
        return std::make_shared<FakeJni::JString>("en");
    }
    std::shared_ptr<FakeJni::JString> getScript() {
        return std::make_shared<FakeJni::JString>("");
    }
    std::shared_ptr<FakeJni::JString> getCountry() {
        return std::make_shared<FakeJni::JString>("US");
    }
    std::shared_ptr<FakeJni::JString> getVariant() {
        return std::make_shared<FakeJni::JString>("");
    }

private:
    std::locale l;
};
