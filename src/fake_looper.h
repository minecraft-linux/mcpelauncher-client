#pragma once

#include <android/looper.h>
#include <game_window.h>
#include <memory>
#include "fake_inputqueue.h"
#include "jni/jni_support.h"
#include "window_callbacks.h"

class FakeLooper {
    private:
    static JniSupport *jniSupport;
    static thread_local std::unique_ptr<FakeLooper> currentLooper;
    bool prepared = false;

    struct EventEntry {
        int fd, ident, events;
        void *data;

        EventEntry() : ident(-1) {}
        EventEntry(int fd, int ident, int events, void *data)
            : fd(fd), ident(ident), events(events), data(data) {}

        void fill(int *outFd, void **outData) const {
            if(outFd)
                *outFd = fd;
            if(outData)
                *outData = data;
        }

        operator bool const() { return ident != -1; }
    };
    EventEntry androidEvent;
    EventEntry inputEntry;
    FakeInputQueue fakeInputQueue;

    std::shared_ptr<GameWindow> associatedWindow;
    std::shared_ptr<WindowCallbacks> associatedWindowCallbacks;

    void initializeWindow();

    public:
    static void setJniSupport(JniSupport *support) { jniSupport = support; }

    ~FakeLooper();

    void prepare();

    int addFd(int fd, int ident, int events, ALooper_callbackFunc callback,
              void *data);

    void attachInputQueue(int ident, ALooper_callbackFunc callback, void *data);

    int pollAll(int timeoutMillis, int *outFd, int *outEvents, void **outData);

    static void initWindow();

    static void initHybrisHooks(std::unordered_map<std::string, void *> &syms);
};
