#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <utility>
#include <jnivm.h>

struct AAssetManager;

struct FakeAssetManager : jnivm::Object {
    std::string rootDir;

    FakeAssetManager(std::string rootDir);

    static void initHybrisHooks(std::unordered_map<std::string, void *> &syms);

    explicit operator AAssetManager *() const {
        return (AAssetManager *)this;
    }
};
