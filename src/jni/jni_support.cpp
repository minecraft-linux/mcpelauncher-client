#include <log.h>
#include <mcpelauncher/path_helper.h>
#include <hybris/dlfcn.h>
#include "jni_support.h"

void JniSupport::registerJniClasses() {
    vm.registerClass<FakeJni::JArray<FakeJni::JString>>();

    vm.registerClass<File>();
    vm.registerClass<ClassLoader>();
    vm.registerClass<Context>();
    vm.registerClass<ContextWrapper>();
    vm.registerClass<HardwareInfo>();
    vm.registerClass<NativeActivity>();
    vm.registerClass<MainActivity>();
    vm.registerClass<StoreListener>();
    vm.registerClass<NativeStoreListener>();
    vm.registerClass<Store>();
    vm.registerClass<StoreFactory>();
}

void JniSupport::registerMinecraftNatives(void *(*symResolver)(const char *)) {
    registerNatives(MainActivity::getDescriptor(), {
            {"nativeRegisterThis", "()V"},
            {"nativeResize", "(II)V"}
    }, symResolver);
    registerNatives(NativeStoreListener::getDescriptor(), {
            {"onStoreInitialized", "(JZ)V"}
    }, symResolver);
}

JniSupport::JniSupport() {
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

    vm.attachLibrary("libminecraftpe.so", "", {hybris_dlopen, hybris_dlsym, hybris_dlclose});

    activity = std::make_shared<MainActivity>();
    activityRef = vm.createGlobalReference(activity);

    activity->storageDirectory = PathHelper::getPrimaryDataDirectory();
    assetManager = std::make_unique<FakeAssetManager>(PathHelper::getGameDir() + "/assets");

    nativeActivity.callbacks = &nativeActivityCallbacks;
    nativeActivity.vm = (JavaVM *) &vm;
    nativeActivity.assetManager = (AAssetManager *) assetManager.get();
    nativeActivity.env = (JNIEnv *) &frame.getJniEnv();
    nativeActivity.internalDataPath = "/internal";
    nativeActivity.externalDataPath = "/external";
    nativeActivity.clazz = activityRef;

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

void JniSupport::waitForGameExit() {
    std::unique_lock<std::mutex> lock (gameExitMutex);
    gameExitCond.wait(lock, [this]{ return gameExitVal; });
}

void JniSupport::onWindowCreated(ANativeWindow *window, AInputQueue *inputQueue) {
    // Note on thread safety: This code is fine thread-wise because ANativeActivity_onCreate locks until the thread is
    // initialized; the thread initialization code runs ALooper_prepare before signaling it's ready.
    this->window = window;
    this->inputQueue = inputQueue;
}

void JniSupport::onWindowResized(int newWidth, int newHeight) {
    FakeJni::LocalFrame frame (vm);
    auto resize = activity->getClass().getMethod("(II)V", "nativeResize");
    if (resize)
        resize->invoke(frame.getJniEnv(), activity.get(), newWidth, newHeight);
}