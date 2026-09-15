#pragma once

#include <android/looper.h>
#include <memory>
#include <game_window.h>
#include "jni/jni_support.h"
#include "window_callbacks.h"
#include "fake_inputqueue.h"

class FakeLooper {
private:
    static JniSupport *jniSupport;
    bool prepared = false;
    bool textInput = false;
    int menuSize = 0;

    struct EventEntry {
        int fd, ident, events;
        void *data;
        ALooper_callbackFunc callback;

        EventEntry() : ident(-1), callback(nullptr) {}
        EventEntry(int fd, int ident, int events, void *data, ALooper_callbackFunc callback) : fd(fd), ident(ident), events(events), data(data), callback(callback) {}

        void fill(int *outFd, void **outData) const {
            if(outFd)
                *outFd = fd;
            if(outData)
                *outData = data;
        }

        operator bool const() {
            return ident != -1;
        }
    };
    std::vector<EventEntry> androidEvents;
    EventEntry inputEntry;
    FakeInputQueue fakeInputQueue;

    std::shared_ptr<GameWindow> associatedWindow;
    std::shared_ptr<WindowCallbacks> associatedWindowCallbacks;

    void initializeWindow();

public:
    static thread_local std::unique_ptr<FakeLooper> currentLooper;
    static void setJniSupport(JniSupport *support) {
        jniSupport = support;
    }

    ~FakeLooper();

    void prepare();

    int addFd(int fd, int ident, int events, ALooper_callbackFunc callback, void *data);

    void attachInputQueue(int ident, ALooper_callbackFunc callback, void *data);

    int pollAll(int timeoutMillis, int *outFd, int *outEvents, void **outData);

    static void initWindow();

    static void initHybrisHooks(std::unordered_map<std::string, void *> &syms);

    static void onGameActivityClose(GameActivity *native);
};
