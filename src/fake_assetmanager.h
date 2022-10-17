#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

struct AAssetManager;

struct FakeAssetManager {
    std::string rootDir;

    FakeAssetManager(std::string rootDir);

    static void initHybrisHooks(std::unordered_map<std::string, void *> &syms);

    explicit operator AAssetManager *() const {
        return (AAssetManager *)this;
    }
};