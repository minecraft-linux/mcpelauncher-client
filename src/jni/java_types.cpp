#include "java_types.h"
#include <locale>

Locale::Locale(std::locale locale) : l(locale) {
}

std::shared_ptr<Locale> Locale::getDefault() {
    return std::make_shared<Locale>(Locale(std::locale()));
}

std::shared_ptr<FakeJni::JString> Locale::toString() {
    return std::make_shared<FakeJni::JString>(l.name());
}

UUID::UUID(uuid_t *u) {
    std::copy(std::begin(*u), std::end(*u), uuid);
}

std::shared_ptr<UUID> UUID::randomUUID() {
    uuid_t uuid;
    uuid_generate(uuid);
    return std::make_shared<UUID>(UUID(&uuid));
}

std::shared_ptr<FakeJni::JString> UUID::toString() {
    char uuidStr[37];
    uuid_unparse_lower(uuid, uuidStr);
    return std::make_shared<FakeJni::JString>(std::string(uuidStr));
}
