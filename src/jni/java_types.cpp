#include "java_types.h"
#include <locale>
#include "main_activity.h"

Locale::Locale(std::locale locale) : l(locale) {
}

std::shared_ptr<Locale> Locale::getDefault() {
    return std::make_shared<Locale>(Locale(std::locale()));
}

std::shared_ptr<FakeJni::JString> Locale::toString() {
    return std::make_shared<FakeJni::JString>(l.name());
}

UUID::UUID(std::shared_ptr<FakeJni::JString> uuid) : uuid(uuid) {
    
}

std::shared_ptr<UUID> UUID::randomUUID() {
    auto uuid = MainActivity().createUUID();
    return std::make_shared<UUID>(uuid);
}

std::shared_ptr<FakeJni::JString> UUID::toString() {
    return uuid;
}
