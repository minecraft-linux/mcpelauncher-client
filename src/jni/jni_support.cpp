#include <log.h>
#include <mcpelauncher/path_helper.h>
#include <mcpelauncher/linker.h>
#include "jni_support.h"
#include "xbox_live.h"
#include "cert_manager.h"

void JniSupport::registerJniClasses() {
    vm.registerClass<FakeJni::JArray<FakeJni::JString>>();

    vm.registerClass<File>();
    vm.registerClass<ClassLoader>();

    vm.registerClass<BuildVersion>();
    vm.registerClass<Context>();
    vm.registerClass<ContextWrapper>();
    vm.registerClass<HardwareInfo>();
    vm.registerClass<Activity>();
    vm.registerClass<NativeActivity>();
    vm.registerClass<MainActivity>();

    vm.registerClass<StoreListener>();
    vm.registerClass<NativeStoreListener>();
    vm.registerClass<Store>();
    vm.registerClass<StoreFactory>();

    vm.registerClass<InputStream>();
    vm.registerClass<ByteArrayInputStream>();
    vm.registerClass<Certificate>();
    vm.registerClass<X509Certificate>();
    vm.registerClass<CertificateFactory>();
    vm.registerClass<TrustManager>();
    vm.registerClass<TrustManagerFactory>();
    vm.registerClass<StrictHostnameVerifier>();

}

void JniSupport::registerMinecraftNatives(void *(*symResolver)(const char *)) {
    registerNatives(MainActivity::getDescriptor(), {
            {"nativeRegisterThis", "()V"},
            {"nativeShutdown", "()V"},
            {"nativeResize", "(II)V"},
            {"nativeSetTextboxText", "(Ljava/lang/String;)V"},
            {"nativeReturnKeyPressed", "()V"}
    }, symResolver);
    registerNatives(NativeStoreListener::getDescriptor(), {
            {"onStoreInitialized", "(JZ)V"}
    }, symResolver);
    registerNatives(JellyBeanDeviceManager::getDescriptor(), {
            {"onInputDeviceAddedNative", "(I)V"},
            {"onInputDeviceRemovedNative", "(I)V"}
    }, symResolver);
}

JniSupport::JniSupport() : textInput([this](std::string const &str) { return onSetTextboxText(str); }) {
    registerJniClasses();
}

void JniSupport::registerNatives(std::shared_ptr<FakeJni::JClass const> clazz,
        std::vector<JniSupport::NativeEntry> entries, void *(*symResolver)(const char *)) {
    FakeJni::LocalFrame frame (vm);

    std::string cppClassName = clazz->getName();
    std::replace(cppClassName.begin(), cppClassName.end(), '/', '_');

    std::vector<JNINativeMethod> javaEntries;
    for (auto const &ent : entries) {
        auto cppSymName = std::string("Java_") + cppClassName + "_" + ent.name;
        auto cppSym = symResolver(cppSymName.c_str());
        if (cppSym == nullptr) {
            Log::error("JniSupport", "Missing native symbol: %s", cppSymName.c_str());
            continue;
        }

        javaEntries.push_back({(char *) ent.name, (char *) ent.sig, cppSym});
    }

    auto jClazz = frame.getJniEnv().createLocalReference(std::const_pointer_cast<FakeJni::JClass>(clazz));
    if (frame.getJniEnv().RegisterNatives((jclass) jClazz, javaEntries.data(), javaEntries.size()) != JNI_OK)
        throw std::runtime_error("RegisterNatives failed");
}

void JniSupport::startGame(ANativeActivity_createFunc *activityOnCreate) {
    FakeJni::LocalFrame frame (vm);

    vm.attachLibrary("libminecraftpe.so", "", {linker::dlopen, linker::dlsym, linker::dlclose});

    activity = std::make_shared<MainActivity>();
    activityRef = vm.createGlobalReference(activity);

    activity->textInput = &textInput;
    activity->quitCallback = [this]() { requestExitGame(); };
    activity->storageDirectory = PathHelper::getPrimaryDataDirectory();
    assetManager = std::make_unique<FakeAssetManager>(PathHelper::getGameDir() + "/assets");

    nativeActivity.callbacks = &nativeActivityCallbacks;
    nativeActivity.vm = (JavaVM *) &vm;
    nativeActivity.assetManager = (AAssetManager *) assetManager.get();
    nativeActivity.env = (JNIEnv *) &frame.getJniEnv();
    nativeActivity.internalDataPath = "/internal";
    nativeActivity.externalDataPath = "/external";
    nativeActivity.clazz = activityRef;
    nativeActivity.sdkVersion = activity->getAndroidVersion();

    Log::trace("JniSupport", "Invoking nativeRegisterThis\n");
    auto registerThis = activity->getClass().getMethod("()V", "nativeRegisterThis");
    registerThis->invoke(frame.getJniEnv(), activity.get());

    Log::trace("JniSupport", "Invoking ANativeActivity_onCreate\n");
    activityOnCreate(&nativeActivity, nullptr, 0);

    Log::trace("JniSupport", "Invoking start activity callbacks\n");
    nativeActivityCallbacks.onInputQueueCreated(&nativeActivity, inputQueue);
    nativeActivityCallbacks.onNativeWindowCreated(&nativeActivity, window);
    nativeActivityCallbacks.onStart(&nativeActivity);
    nativeActivityCallbacks.onResume(&nativeActivity);
}

void JniSupport::stopGame() {
    FakeJni::LocalFrame frame (vm);

    Log::trace("JniSupport", "Invoking stop activity callbacks\n");
    nativeActivityCallbacks.onPause(&nativeActivity);
    nativeActivityCallbacks.onStop(&nativeActivity);
    nativeActivityCallbacks.onDestroy(&nativeActivity);

    Log::trace("JniSupport", "Waiting for looper clean up\n");
    std::unique_lock<std::mutex> lock (gameExitMutex);
    gameExitCond.wait(lock, [this]{ return !looperRunning; });
}

void JniSupport::waitForGameExit() {
    std::unique_lock<std::mutex> lock (gameExitMutex);
    gameExitCond.wait(lock, [this]{ return gameExitVal; });
}

void JniSupport::requestExitGame() {
    std::unique_lock<std::mutex> lock (gameExitMutex);
    gameExitVal = true;
    gameExitCond.notify_all();
}

void JniSupport::setLooperRunning(bool running) {
    std::unique_lock<std::mutex> lock (gameExitMutex);
    looperRunning = running;
    if (!running)
        gameExitCond.notify_all();
}

void JniSupport::onWindowCreated(ANativeWindow *window, AInputQueue *inputQueue) {
    // Note on thread safety: This code is fine thread-wise because ANativeActivity_onCreate locks until the thread is
    // initialized; the thread initialization code runs ALooper_prepare before signaling it's ready.
    this->window = window;
    this->inputQueue = inputQueue;
}

void JniSupport::onWindowClosed() {
    FakeJni::LocalFrame frame (vm);
    auto shutdown = activity->getClass().getMethod("()V", "nativeShutdown");
    shutdown->invoke(frame.getJniEnv(), activity.get());
}

void JniSupport::onWindowResized(int newWidth, int newHeight) {
    FakeJni::LocalFrame frame (vm);
    auto resize = activity->getClass().getMethod("(II)V", "nativeResize");
    if (resize)
        resize->invoke(frame.getJniEnv(), activity.get(), newWidth, newHeight);
}

void JniSupport::onSetTextboxText(std::string const &text) {
    FakeJni::LocalFrame frame (vm);
    auto setText = activity->getClass().getMethod("(Ljava/lang/String;)V", "nativeSetTextboxText");
    if (setText) {
        auto str = std::make_shared<FakeJni::JString>(text);
        setText->invoke(frame.getJniEnv(), activity.get(), frame.getJniEnv().createLocalReference(str));
    }
}

void JniSupport::onReturnKeyPressed() {
    FakeJni::LocalFrame frame (vm);
    auto returnPressed = activity->getClass().getMethod("()V", "nativeReturnKeyPressed");
    if (returnPressed)
        returnPressed->invoke(frame.getJniEnv(), activity.get());
}

void JniSupport::setGameControllerConnected(int devId, bool connected) {
    static auto addedMethod = JellyBeanDeviceManager::getDescriptor()->getMethod("(I)V", "onInputDeviceAddedNative");
    static auto removedMethod = JellyBeanDeviceManager::getDescriptor()->getMethod("(I)V", "onInputDeviceRemovedNative");

    FakeJni::LocalFrame frame (vm);
    if (connected && addedMethod)
        addedMethod->invoke(frame.getJniEnv(), nullptr, devId);
    else if (connected && removedMethod)
        removedMethod->invoke(frame.getJniEnv(), nullptr, devId);
}