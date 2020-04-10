#pragma once

#include "main_activity.h"
#include "store.h"
#include "../fake_assetmanager.h"
#include <baron/baron.h>
#include <android/native_activity.h>
#include <game_window.h>
#include <condition_variable>
#include <mutex>

struct JniSupport {

private:
    struct NativeEntry {
        const char *name;
        const char *sig;
    };

    Baron::Jvm vm;
    ANativeActivityCallbacks nativeActivityCallbacks;
    ANativeActivity nativeActivity;
    std::shared_ptr<MainActivity> activity;
    jobject activityRef;
    std::unique_ptr<FakeAssetManager> assetManager;
    std::shared_ptr<GameWindow> window;
    std::condition_variable gameExitCond;
    std::mutex gameExitMutex;
    bool gameExitVal = false;

    void registerJniClasses();

    void registerNatives(std::shared_ptr<FakeJni::JClass const> clazz, std::vector<NativeEntry> entries,
                         void *(*symResolver)(const char *));

public:
    JniSupport();

    void registerMinecraftNatives(void *(*symResolver)(const char *));

    void startGame(ANativeActivity_createFunc *activityOnCreate);

    void waitForGameExit();

    void onWindowCreated(std::shared_ptr<GameWindow> gameWindow);

    void onWindowResized(int newWidth, int newHeight);

};