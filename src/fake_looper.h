#pragma once

#include <android/looper.h>
#include <memory>
#include <game_window.h>
#include "jni/jni_support.h"

class FakeLooper {

private:
    static JniSupport* jniSupport;
    static thread_local std::unique_ptr<FakeLooper> currentLooper;

    struct EventEntry {
        int fd, ident, events;
        void *data;
    };
    EventEntry androidEvent;
    bool androidEventSet = false;

    std::shared_ptr<GameWindow> associatedWindow;

public:
    static void setJniSupport(JniSupport *support) {
        jniSupport = support;
    }

    void prepare();

    int addFd(int fd, int ident, int events, ALooper_callbackFunc callback, void *data);

    int pollAll(int timeoutMillis, int *outFd, int *outEvents, void **outData);


    static void initHybrisHooks();

};