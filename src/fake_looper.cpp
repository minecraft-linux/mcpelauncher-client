#include "fake_looper.h"
#include "main.h"
#include "shader_error_patch.h"
#include "splitscreen_patch.h"
#include "gl_core_patch.h"
#include "core_patches.h"
#include "fake_egl.h"

#include <sys/poll.h>

#include <game_window_manager.h>
#include <log.h>
#ifdef MCPELAUNCHER_ENABLE_ERROR_WINDOW
#include <errorwindow.h>
#endif

JniSupport *FakeLooper::jniSupport;
thread_local std::unique_ptr<FakeLooper> FakeLooper::currentLooper;

void FakeLooper::initWindow() {
    if(!currentLooper) {
        currentLooper = std::make_unique<FakeLooper>();
    }
    currentLooper->initializeWindow();
}

void FakeLooper::initHybrisHooks(std::unordered_map<std::string, void *> &syms) {
    syms["ALooper_prepare"] = (void *)+[]() {
        if(currentLooper && currentLooper->prepared)
            throw std::runtime_error("Looper already prepared");
        if(!currentLooper) {
            currentLooper = std::make_unique<FakeLooper>();
        }
        currentLooper->prepared = true;

        currentLooper->prepare();
        return (ALooper *)(void *)currentLooper.get();
    };
    syms["ALooper_addFd"] = (void *)+[](ALooper *looper, int fd, int ident, int events, ALooper_callbackFunc callback, void *data) {
        return ((FakeLooper *)(void *)looper)->addFd(fd, ident, events, callback, data);
    };
    syms["ALooper_pollAll"] = (void *)+[](int timeoutMillis, int *outFd, int *outEvents, void **outData) {
        return currentLooper->pollAll(timeoutMillis, outFd, outEvents, outData);
    };
    syms["AInputQueue_attachLooper"] = (void *)+[](AInputQueue *queue, ALooper *looper, int ident, ALooper_callbackFunc callback, void *data) {
        ((FakeLooper *)(void *)looper)->attachInputQueue(ident, callback, data);
    };

    syms["ANativeActivity_finish"] = (void *)+[](ANativeActivity *native) {
        FakeJni::JniEnvContext ctx(*(FakeJni::Jvm *)native->vm);
        auto activity = std::dynamic_pointer_cast<MainActivity>(ctx.getJniEnv().resolveReference(native->clazz));
        activity->quitCallback();
    };
}

void FakeLooper::initializeWindow() {
    if(associatedWindow) {
        return;
    }
    Log::info("Launcher", "Loading gamepad mappings");
    WindowCallbacks::loadGamepadMappings();
#ifdef MCPELAUNCHER_ENABLE_ERROR_WINDOW
    GameWindowManager::getManager()->setErrorHandler(std::make_shared<ErrorWindow>());
#endif

    Log::info("Launcher", "Creating window");
    associatedWindow = GameWindowManager::getManager()->createWindow("Minecraft",
                                                                     options.windowWidth, options.windowHeight, options.graphicsApi);
    FakeEGL::setupGLOverrides();
}

void FakeLooper::prepare() {
    jniSupport->setLooperRunning(true);
    initializeWindow();
    jniSupport->onWindowCreated((ANativeWindow *)(void *)associatedWindow.get(),
                                (AInputQueue *)(void *)&fakeInputQueue);
    associatedWindowCallbacks = std::make_shared<WindowCallbacks>(*associatedWindow, *jniSupport, fakeInputQueue);
    associatedWindowCallbacks->registerCallbacks();

    CorePatches::setGameWindow(associatedWindow);
    CorePatches::setGameWindowCallbacks(associatedWindowCallbacks);

    associatedWindow->show();
    SplitscreenPatch::onGLContextCreated();
    ShaderErrorPatch::onGLContextCreated();
    associatedWindow->makeCurrent(false);
}

FakeLooper::~FakeLooper() {
    CorePatches::setGameWindow(nullptr);
    associatedWindow.reset();
    associatedWindowCallbacks.reset();
}

int FakeLooper::addFd(int fd, int ident, int events, ALooper_callbackFunc callback, void *data) {
    if(androidEvent)
        return -1;
    if(callback != nullptr)
        throw std::runtime_error("callback is not supported");
    androidEvent = EventEntry(fd, ident, events, data);
    return 1;
}

void FakeLooper::attachInputQueue(int ident, ALooper_callbackFunc callback, void *data) {
    if(inputEntry)
        throw std::runtime_error("attachInputQueue already called on this looper");
    if(callback != nullptr)
        throw std::runtime_error("callback is not supported");
    inputEntry = EventEntry(-1, ident, 0, data);
}

int FakeLooper::pollAll(int timeoutMillis, int *outFd, int *outEvents, void **outData) {
    associatedWindowCallbacks->startSendEvents();
    if(androidEvent) {
        pollfd f;
        f.fd = androidEvent.fd;
        f.events = androidEvent.events;
        if(poll(&f, 1, 0) > 0) {
            androidEvent.fill(outFd, outData);
            if(outEvents)
                *outEvents = f.revents;
            return androidEvent.ident;
        }
    }

    if(inputEntry && fakeInputQueue.hasEvents()) {
        inputEntry.fill(outFd, outData);
        return inputEntry.ident;
    }

    associatedWindow->pollEvents();
    associatedWindowCallbacks->markRequeueGamepadInput();
    return ALOOPER_POLL_TIMEOUT;
}
