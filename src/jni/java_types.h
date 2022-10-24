#pragma once

#include <fake-jni/fake-jni.h>

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
        static std::shared_ptr<ClassLoader> instance(new ClassLoader);
        return instance;
    }

    std::shared_ptr<FakeJni::JClass> loadClass(std::shared_ptr<FakeJni::JString> str) {
        FakeJni::JniEnvContext context;
        return std::const_pointer_cast<FakeJni::JClass>(
            context.getJniEnv().getVM().findClass(str->asStdString().c_str()));
    }
};
