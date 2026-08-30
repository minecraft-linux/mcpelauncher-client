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
#include "../fake_text_input.h"

struct JniSupport {
private:
    struct NativeEntry {
        const char* name;
        const char* sig;
    };

    bool isGameActivity;
    bool useLegacyTextInput = false;

    Baron::Jvm vm;
    ANativeActivityCallbacks nativeActivityCallbacks;
    GameActivityCallbacks gameActivityCallbacks;
    ANativeActivity nativeActivity;
    GameActivity gameActivity;
    std::shared_ptr<MainActivity> activity;
    jobject activityRef;
    std::unique_ptr<FakeAssetManager> assetManager;
    ANativeWindow* window;
    AInputQueue* inputQueue;
    std::condition_variable gameExitCond;
    std::mutex gameExitMutex;
    bool gameExitVal = false, looperRunning = false;
    TextInputHandler textInput;

    void registerJniClasses();

    void registerNatives(std::shared_ptr<FakeJni::JClass const> clazz, std::vector<NativeEntry> entries,
                         void* (*symResolver)(const char*));

public:
    JniSupport();

    void registerMinecraftNatives(void* (*symResolver)(const char*));

    void startGame(ANativeActivity_createFunc* activityOnCreate, GameActivity_createFunc* gameCreate,
                   void* stbiLoadFromMemory, void* stbiImageFree);

    void importFile(std::string path);

    void sendUri(std::string uri);

    void stopGame();

    void waitForGameExit();

    void requestExitGame();

    void setLooperRunning(bool running);

    void onWindowCreated(ANativeWindow* window, AInputQueue* inputQueue);

    void onWindowClosed();

    void onWindowResized(int newWidth, int newHeight);

    void onSetTextboxText(std::string const& text);

    void onCaretPosition(int pos);

    void onReturnKeyPressed();

    void onBackPressed();

    void setGameControllerConnected(int devId, bool connected);

    TextInputHandler& getTextInputHandler() { return textInput; }

    void setLastChar(FakeJni::JInt sym);

    void sendKeyDown(const GameActivityKeyEvent* event) {
        gameActivityCallbacks.onKeyDown(&gameActivity, event);
    }

    void sendKeyUp(const GameActivityKeyEvent* event) {
        gameActivityCallbacks.onKeyUp(&gameActivity, event);
    }

    void sendMotionEvent(const GameActivityMotionEvent* event) {
        gameActivityCallbacks.onTouchEvent(&gameActivity, event);
    }

    bool isGameActivityVersion() {
        return isGameActivity;
    }

    bool isTextInputEnabled() {
        return usesGameActivityTextInput() ? FakeTextInput::isEnabled(&gameActivity)
                                           : textInput.isEnabled();
    }

    size_t getTextInputEnabledNo() {
        return usesGameActivityTextInput() ? FakeTextInput::getEnabledNo(&gameActivity)
                                           : textInput.getEnabledNo();
    }

    bool isTextInputMultiline() {
        return usesGameActivityTextInput() ? FakeTextInput::isMultiline(&gameActivity)
                                           : textInput.isMultiline();
    }

    bool textInputConsumesKey(KeyCode key, int mods) {
        return usesGameActivityTextInput() && FakeTextInput::consumesKey(&gameActivity, key, mods);
    }

    void onTextInput(std::string const& value) {
        if(usesGameActivityTextInput())
            FakeTextInput::onTextInput(&gameActivity, value);
        else
            textInput.onTextInput(value);
    }

    void onTextInputKeyPressed(KeyCode key, KeyAction action, int mods) {
        if(usesGameActivityTextInput())
            FakeTextInput::onKeyPressed(&gameActivity, key, action, mods);
        else
            textInput.onKeyPressed(key, action, mods);
    }

    void flushRepeatedTextInputEvent() {
        if(usesGameActivityTextInput())
            FakeTextInput::tryFlushRepeatingState(&gameActivity);
    }

    std::string getTextInputCopyText() {
        return usesGameActivityTextInput() ? FakeTextInput::getCopyText(&gameActivity)
                                           : textInput.getCopyText();
    }

    bool usesGameActivityTextInput() {
        return isGameActivity && !textInput.getUsesLegacyTextInput();
    }

    GameActivity* getGameActivity() {
        return &gameActivity;
    }
};
