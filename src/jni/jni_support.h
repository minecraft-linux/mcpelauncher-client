#pragma once

#include <android/native_activity.h>
#include <baron/baron.h>
#include <game_window.h>
#include <condition_variable>
#include <mutex>
#include "../fake_assetmanager.h"
#include "../text_input_handler.h"
#include "main_activity.h"
#include "store.h"

struct JniSupport
{
   private:
    struct NativeEntry
    {
        const char *name;
        const char *sig;
    };

    Baron::Jvm vm;
    ANativeActivityCallbacks nativeActivityCallbacks;
    ANativeActivity nativeActivity;
    std::shared_ptr<MainActivity> activity;
    jobject activityRef;
    std::unique_ptr<FakeAssetManager> assetManager;
    ANativeWindow *window;
    AInputQueue *inputQueue;
    std::condition_variable gameExitCond;
    std::mutex gameExitMutex;
    bool gameExitVal = false, looperRunning = false;
    TextInputHandler textInput;

    void registerJniClasses();

    void registerNatives(std::shared_ptr<FakeJni::JClass const> clazz, std::vector<NativeEntry> entries,
                         void *(*symResolver)(const char *));

   public:
    JniSupport();

    void registerMinecraftNatives(void *(*symResolver)(const char *));

    void startGame(ANativeActivity_createFunc *activityOnCreate, void *stbiLoadFromMemory, void *stbiImageFree);

    void stopGame();

    void waitForGameExit();

    void requestExitGame();

    void setLooperRunning(bool running);

    void onWindowCreated(ANativeWindow *window, AInputQueue *inputQueue);

    void onWindowClosed();

    void onWindowResized(int newWidth, int newHeight);

    void onSetTextboxText(std::string const &text);

    void onReturnKeyPressed();

    void setGameControllerConnected(int devId, bool connected);

    TextInputHandler &getTextInputHandler() { return textInput; }
};