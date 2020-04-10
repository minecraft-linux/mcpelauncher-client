#include "fake_looper.h"
#include "main.h"
#include "shader_error_patch.h"
#include "splitscreen_patch.h"
#include "gl_core_patch.h"

#include <hybris/hook.h>
#include <sys/poll.h>

#include <game_window_manager.h>

JniSupport *FakeLooper::jniSupport;
thread_local std::unique_ptr<FakeLooper> FakeLooper::currentLooper;

void FakeLooper::initHybrisHooks() {
    hybris_hook("ALooper_prepare", (void *) +[]() {
        if (currentLooper)
            throw std::runtime_error("Looper already prepared");
        currentLooper = std::make_unique<FakeLooper>();
        currentLooper->prepare();
        return (ALooper *) (void *) currentLooper.get();
    });
    hybris_hook("ALooper_addFd", (void *) +[](ALooper *looper, int fd, int ident, int events, ALooper_callbackFunc callback, void *data) {
        return ((FakeLooper *) (void *) looper)->addFd(fd, ident, events, callback, data);
    });
    hybris_hook("ALooper_pollAll", (void *) +[](int timeoutMillis, int *outFd, int *outEvents, void **outData) {
        return currentLooper->pollAll(timeoutMillis, outFd, outEvents, outData);
    });
    hybris_hook("AInputQueue_attachLooper", (void *) +[](AInputQueue *queue, ALooper *looper, int ident, ALooper_callbackFunc callback, void *data) {
        ((FakeLooper *) (void *) looper)->attachInputQueue(ident, callback, data);
    });
}

void FakeLooper::prepare() {
    associatedWindow = GameWindowManager::getManager()->createWindow("Minecraft",
            options.windowWidth, options.windowHeight, options.graphicsApi);
    jniSupport->onWindowCreated((ANativeWindow *) (void *) associatedWindow.get(),
            (AInputQueue *) (void *) &fakeInputQueue);
    associatedWindowCallbacks = std::make_shared<WindowCallbacks>(*associatedWindow, *jniSupport, fakeInputQueue);
    associatedWindowCallbacks->registerCallbacks();

    associatedWindow->show();
    SplitscreenPatch::onGLContextCreated();
    GLCorePatch::onGLContextCreated();
    ShaderErrorPatch::onGLContextCreated();
}

int FakeLooper::addFd(int fd, int ident, int events, ALooper_callbackFunc callback, void *data) {
    if (androidEvent)
        return -1;
    if (callback != nullptr)
        throw std::runtime_error("callback is not supported");
    androidEvent = EventEntry(fd, ident, events, data);
    return 1;
}

void FakeLooper::attachInputQueue(int ident, ALooper_callbackFunc callback, void *data) {
    if (inputEntry)
        throw std::runtime_error("attachInputQueue already called on this looper");
    if (callback != nullptr)
        throw std::runtime_error("callback is not supported");
    inputEntry = EventEntry(-1, ident, 0, data);
}

int FakeLooper::pollAll(int timeoutMillis, int *outFd, int *outEvents, void **outData) {
    if (androidEvent) {
        pollfd f;
        f.fd = androidEvent.fd;
        f.events = androidEvent.events;
        if (poll(&f, 1, 0) > 0) {
            androidEvent.fill(outFd, outData);
            if (outEvents)
                *outEvents = f.revents;
            return androidEvent.ident;
        }
    }

    associatedWindow->pollEvents();
    if (inputEntry && fakeInputQueue.hasEvents()) {
        inputEntry.fill(outFd, outData);
        return inputEntry.ident;
    }
    return ALOOPER_POLL_TIMEOUT;
}