#pragma once

#include <fake-jni/fake-jni.h>
#include <locale>

class File : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("java/io/File")

    std::string path;

    explicit File(std::string path) : path(std::move(path)) {
    }

    std::shared_ptr<FakeJni::JString> getPath() {
        return std::make_shared<FakeJni::JString>(path.c_str());
    }

};

class ClassLoader : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("java/lang/ClassLoader")

    static std::shared_ptr<ClassLoader> getInstance() {
        static std::shared_ptr<ClassLoader> instance (new ClassLoader);
        return instance;
    }

    std::shared_ptr<FakeJni::JClass> loadClass(std::shared_ptr<FakeJni::JString> str) {
        FakeJni::JniEnvContext context;
        return std::const_pointer_cast<FakeJni::JClass>(
                context.getJniEnv().getVM().findClass(str->asStdString().c_str()));
    }

};

class Locale : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("java/util/Locale")

    Locale(std::locale locale);
    static std::shared_ptr<Locale> getDefault();
    std::shared_ptr<FakeJni::JString> toString();

private:
    std::locale l;

};

class UUID : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("java/util/UUID")

    UUID(std::shared_ptr<FakeJni::JString> uuid);
    static std::shared_ptr<UUID> randomUUID();
    std::shared_ptr<FakeJni::JString> toString();

private:
    std::shared_ptr<FakeJni::JString> uuid;

};