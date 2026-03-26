#pragma once

#include "main_activity.h"
#include "store.h"
#include "../fake_assetmanager.h"
#include <baron/baron.h>
#include <android/native_activity.h>
#include <android/game_activity.h>
#include <game_window.h>
#include <condition_variable>
#include <mutex>
#include "../text_input_handler.h"

struct JniSupport {
private:
    struct NativeEntry {
        const char *name;
        const char *sig;
    };

    bool isGameActivity;

    Baron::Jvm vm;
    ANativeActivityCallbacks nativeActivityCallbacks = {};
    GameActivityCallbacks gameActivityCallbacks = {};
    ANativeActivity nativeActivity = {};
    GameActivity gameActivity = {};
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

    void startGame(ANativeActivity_createFunc *activityOnCreate, GameActivity_createFunc *gameCreate,
                   void *stbiLoadFromMemory, void *stbiImageFree);

    void importFile(std::string path);

    void sendUri(std::string uri);

    void stopGame();

    void waitForGameExit();

    void requestExitGame();

    void setLooperRunning(bool running);

    void onWindowCreated(ANativeWindow *window, AInputQueue *inputQueue);

    void onWindowClosed();

    void onWindowResized(int newWidth, int newHeight);

    void onSetTextboxText(std::string const &text);

    void onCaretPosition(int pos);

    void onReturnKeyPressed();

    void onBackPressed();

    void setGameControllerConnected(int devId, bool connected);

    TextInputHandler &getTextInputHandler() { return textInput; }

    void setLastChar(FakeJni::JInt sym);

    void sendKeyDown(const GameActivityKeyEvent *event) {
        if(gameActivityCallbacks.onKeyDown)
            gameActivityCallbacks.onKeyDown(&gameActivity, event);
    }

    void sendKeyUp(const GameActivityKeyEvent *event) {
        if(gameActivityCallbacks.onKeyUp)
            gameActivityCallbacks.onKeyUp(&gameActivity, event);
    }

    void sendMotionEvent(const GameActivityMotionEvent *event) {
        if(gameActivityCallbacks.onTouchEvent)
            gameActivityCallbacks.onTouchEvent(&gameActivity, event);
    }

    bool isGameActivityVersion() {
        return isGameActivity;
    }
};
