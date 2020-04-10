#include <utility>

#pragma once

#include <string>
#include <memory>

struct AAssetManager;

struct FakeAssetManager {

    std::string rootDir;

    FakeAssetManager(std::string rootDir);

    static void initHybrisHooks();

    explicit operator AAssetManager*() const {
        return (AAssetManager *) this;
    }

};