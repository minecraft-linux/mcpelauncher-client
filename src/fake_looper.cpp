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

}

void FakeLooper::prepare() {
    associatedWindow = GameWindowManager::getManager()->createWindow("Minecraft",
            options.windowWidth, options.windowHeight, options.graphicsApi);
    jniSupport->onWindowCreated(associatedWindow);
    associatedWindowCallbacks = std::make_shared<WindowCallbacks>(*associatedWindow, *jniSupport);
    associatedWindowCallbacks->registerCallbacks();

    associatedWindow->show();
    SplitscreenPatch::onGLContextCreated();
    GLCorePatch::onGLContextCreated();
    ShaderErrorPatch::onGLContextCreated();
}

int FakeLooper::addFd(int fd, int ident, int events, ALooper_callbackFunc callback, void *data) {
    if (androidEventSet)
        return -1;
    androidEventSet = true;
    androidEvent = {fd, ident, events, data};
    return 1;
}

int FakeLooper::pollAll(int timeoutMillis, int *outFd, int *outEvents, void **outData) {
    if (androidEventSet != 0) {
        pollfd f;
        f.fd = androidEvent.fd;
        f.events = androidEvent.events;
        if (poll(&f, 1, 0) > 0) {
            if (outFd)
                *outFd = androidEvent.fd;
            if (outEvents)
                *outEvents = androidEvent.events;
            if (outData)
                *outData = androidEvent.data;
            return androidEvent.ident;
        }
    }

    associatedWindow->pollEvents();
    return ALOOPER_POLL_TIMEOUT;
}