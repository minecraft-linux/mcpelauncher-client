#include <log.h>
#include <mcpelauncher/path_helper.h>
#include <mcpelauncher/linker.h>
#include "jni_support.h"
#include "xbox_live.h"
#include "lib_http_client.h"
#include "lib_http_client_websocket.h"
#include "cert_manager.h"
#include "package_source.h"
#include "../xbox_live_helper.h"
#include "http_stub.h"
#ifdef HAVE_PULSEAUDIO
#include "pulseaudio.h"
#endif
#include "accounts.h"
#include "ecdsa.h"
#include "webview.h"
#include "jbase64.h"
#include "arrays.h"
#include "locale.h"
#include "signature.h"
#include "uuid.h"
#include "shahasher.h"
#include "securerandom.h"
#include "../main.h"
#include <thread>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE > 8
#include <filesystem>
#endif


void JniSupport::registerJniClasses() {
    vm.registerClass<File>();
    vm.registerClass<ClassLoader>();
    vm.registerClass<Locale>();
    vm.registerClass<UUID>();

    vm.registerClass<BuildVersion>();
    vm.registerClass<PackageInfo>();
    vm.registerClass<PackageManager>();
    vm.registerClass<Context>();
    vm.registerClass<ContextWrapper>();
    vm.registerClass<HardwareInfo>();
    vm.registerClass<Activity>();
    vm.registerClass<NativeActivity>();
    vm.registerClass<NetworkMonitor>();
    vm.registerClass<MainActivity>();
    vm.registerClass<AccountManager>();
    vm.registerClass<Account>();

    vm.registerClass<StoreListener>();
    vm.registerClass<NativeStoreListener>();
    vm.registerClass<Store>();
    vm.registerClass<StoreFactory>();
    vm.registerClass<ExtraLicenseResponseData>();

    vm.registerClass<XboxInterop>();
    vm.registerClass<XboxLocalStorage>();
    vm.registerClass<Ecdsa>();
    vm.registerClass<EcdsaPublicKey>();
    vm.registerClass<HttpClientRequest>();
    vm.registerClass<HttpClientResponse>();
    vm.registerClass<HttpClientWebSocket>();

    vm.registerClass<InputStream>();
    vm.registerClass<ByteArrayInputStream>();
    vm.registerClass<Certificate>();
    vm.registerClass<X509Certificate>();
    vm.registerClass<CertificateFactory>();
    vm.registerClass<TrustManager>();
    vm.registerClass<X509TrustManager>();
    vm.registerClass<TrustManagerFactory>();
    vm.registerClass<StrictHostnameVerifier>();

    vm.registerClass<PackageSource>();
    vm.registerClass<PackageSourceListener>();
    vm.registerClass<NativePackageSourceListener>();
    vm.registerClass<PackageSourceFactory>();

    vm.registerClass<Header>();
    vm.registerClass<HTTPResponse>();
    vm.registerClass<HTTPRequest>();

    vm.registerClass<ShaHasher>();
    vm.registerClass<SecureRandom>();
    // Minecraft 1.16.20-210
    vm.registerClass<WebView>();
    // Minecraft 1.16.220+
    vm.registerClass<BrowserLaunchActivity>();

    vm.registerClass<JBase64>();
    vm.registerClass<Arrays>();
    vm.registerClass<Signature>();
    vm.registerClass<PublicKey>();
    vm.registerClass<Product>();
    vm.registerClass<Purchase>();
    vm.registerClass<NotificationListenerService>();

    vm.registerClass<PlayIntegrity>();

#ifdef HAVE_PULSEAUDIO
    vm.registerClass<AudioDevice>();
#endif
}

void JniSupport::registerMinecraftNatives(void *(*symResolver)(const char *)) {
    registerNatives(MainActivity::getDescriptor(), {{"nativeRegisterThis", "()V"}, {"nativeWaitCrashManagementSetupComplete", "()V"}, {"nativeInitializeWithApplicationContext", "(Landroid/content/Context;)V"}, {"nativeShutdown", "()V"}, {"nativeUnregisterThis", "()V"}, {"nativeStopThis", "()V"}, {"nativeOnDestroy", "()V"}, {"nativeResize", "(II)V"}, {"nativeSetTextboxText", "(Ljava/lang/String;)V"}, {"nativeReturnKeyPressed", "()V"}, {"nativeOnPickImageSuccess", "(JLjava/lang/String;)V"}, {"nativeOnPickImageCanceled", "(J)V"}, {"nativeOnPickFileSuccess", "(Ljava/lang/String;)V"}, {"nativeOnPickFileCanceled", "()V"}, {"nativeInitializeXboxLive", "(JJ)V"}, {"nativeinitializeLibHttpClient", "(J)J"}, {"nativeInitializeLibHttpClient", "(J)J"}, {"nativeProcessIntentUriQuery", "(Ljava/lang/String;Ljava/lang/String;)V"}, {"nativeSetIntegrityToken", "(Ljava/lang/String;)V"}}, symResolver);
    registerNatives(NetworkMonitor::getDescriptor(), {{"nativeUpdateNetworkStatus", "(ZZZ)V"}}, symResolver);
    registerNatives(NativeStoreListener::getDescriptor(), {
                                                              {"onStoreInitialized", "(JZ)V"},
                                                              {"onPurchaseFailed", "(JLjava/lang/String;)V"},
                                                              {"onQueryProductsSuccess", "(J[Lcom/mojang/minecraftpe/store/Product;)V"},
                                                              {"onQueryPurchasesSuccess", "(J[Lcom/mojang/minecraftpe/store/Purchase;)V"},
                                                          },
                    symResolver);
    registerNatives(JellyBeanDeviceManager::getDescriptor(), {{"onInputDeviceAddedNative", "(I)V"}, {"onInputDeviceRemovedNative", "(I)V"}}, symResolver);
    registerNatives(HttpClientRequest::getDescriptor(), {{"OnRequestCompleted", "(JLcom/xbox/httpclient/HttpClientResponse;)V"}, {"OnRequestFailed", "(JLjava/lang/String;)V"}}, symResolver);
    registerNatives(HttpClientWebSocket::getDescriptor(), {{"onMessage", "(Ljava/lang/String;)V"}, {"onBinaryMessage", "(Ljava/nio/ByteBuffer;)V"}, {"onOpen", "()V"}, {"onClose", "(I)V"}, {"onFailure", "()V"}}, symResolver);
    registerNatives(WebView::getDescriptor(), {
                                                  {"urlOperationSucceeded", "(JLjava/lang/String;ZLjava/lang/String;)V"},
                                              },
                    symResolver);
    registerNatives(BrowserLaunchActivity::getDescriptor(), {
                                                                {"urlOperationSucceeded", "(JLjava/lang/String;ZLjava/lang/String;)V"},
                                                            },
                    symResolver);
    registerNatives(NativeInputStream::getDescriptor(), {
                                                            {"nativeRead", "(JJ[BJJ)I"},
                                                        },
                    symResolver);
    registerNatives(NativeOutputStream::getDescriptor(), {
                                                             {"nativeWrite", "(J[BII)V"},
                                                         },
                    symResolver);
    registerNatives(PlayIntegrity::getDescriptor(), {
                                                             {"nativePlayIntegrityComplete", "()V"},
                                                         },
                    symResolver);
}

JniSupport::JniSupport() : textInput([this](std::string const &str) { return onSetTextboxText(str); })
#if defined(__APPLE__) && defined(__aarch64__)
    , vm([](Baron::Jvm *jvm) {
        auto lib = linker::dlopen(PathHelper::findDataFile("lib/" + std::string(PathHelper::getAbiDir()) + "/libjnivmsupport.so").c_str(), 0);
        if(lib == nullptr) {
            Log::error("LAUNCHER", "Failed to load arm64 variadic compat libjnivmsupport.so Original Error: %s", linker::dlerror());
            return;
        }
        void** GetJMethodIDSignature = (void**)linker::dlsym(lib, "GetJMethodIDSignature");
        if(GetJMethodIDSignature == nullptr) {
            Log::error("LAUNCHER", "Failed to get GetJMethodIDSignature Original Error: %s", linker::dlerror());
            return;
        }
        *GetJMethodIDSignature = (void*)jnivm::GetJMethodIDSignature;
        void(*PatchJNINativeInterface)(JNINativeInterface& interface) = (void(*)(JNINativeInterface& interface))linker::dlsym(lib, "PatchJNINativeInterface");
        if(PatchJNINativeInterface == nullptr) {
            Log::error("LAUNCHER", "Failed to get PatchJNINativeInterface Original Error: %s", linker::dlerror());
            return;
        }
        jvm->AddHook([PatchJNINativeInterface](JNINativeInterface& in) {
            PatchJNINativeInterface(in);
        });
    })
#endif
{
    registerJniClasses();
}

void JniSupport::registerNatives(std::shared_ptr<FakeJni::JClass const> clazz,
                                 std::vector<JniSupport::NativeEntry> entries, void *(*symResolver)(const char *)) {
    FakeJni::LocalFrame frame(vm);

    std::string cppClassName = clazz->getName();
    std::replace(cppClassName.begin(), cppClassName.end(), '/', '_');

    std::vector<JNINativeMethod> javaEntries;
    for(auto const &ent : entries) {
        auto cppSymName = std::string("Java_") + cppClassName + "_" + ent.name;
        auto cppSym = symResolver(cppSymName.c_str());
        if(cppSym == nullptr) {
            Log::error("JniSupport", "Missing native symbol: %s", cppSymName.c_str());
            continue;
        }

        javaEntries.push_back({(char *)ent.name, (char *)ent.sig, cppSym});
    }

    auto jClazz = frame.getJniEnv().createLocalReference(std::const_pointer_cast<FakeJni::JClass>(clazz));
    if(frame.getJniEnv().RegisterNatives((jclass)jClazz, javaEntries.data(), javaEntries.size()) != JNI_OK)
        throw std::runtime_error("RegisterNatives failed");
}

void JniSupport::startGame(ANativeActivity_createFunc *activityOnCreate,
                           void *stbiLoadFromMemory, void *stbiImageFree) {
    FakeJni::LocalFrame frame(vm);

    vm.attachLibrary("libfmod.so", "", {linker::dlopen, linker::dlsym, linker::dlclose_unlocked});
    vm.attachLibrary("libminecraftpe.so", "", {linker::dlopen, linker::dlsym, linker::dlclose_unlocked});

    auto clz = vm.findClass("android/os/Build$VERSION");
    auto clzRef = (jclass)frame.getJniEnv().createLocalReference(std::const_pointer_cast<FakeJni::JClass>(clz));
    auto sdkInt = frame.getJniEnv().GetStaticFieldID(clzRef, "SDK_INT", "I");
    jint test = frame.getJniEnv().GetStaticIntField(clzRef, sdkInt);
    if(test != 27)
        abort();

    activity = std::make_shared<MainActivity>();
    activityRef = vm.createGlobalReference(activity);

    activity->textInput = &textInput;
    activity->quitCallback = [this]() { requestExitGame(); };
    activity->storageDirectory = PathHelper::getPrimaryDataDirectory();
    activity->stbi_load_from_memory = (decltype(activity->stbi_load_from_memory))stbiLoadFromMemory;
    activity->stbi_image_free = (decltype(activity->stbi_image_free))stbiImageFree;

    assetManager = std::make_unique<FakeAssetManager>(PathHelper::getGameDir() + "assets");

    XboxLiveHelper::getInstance().setJvm(&vm);

    nativeActivity.callbacks = &nativeActivityCallbacks;
    nativeActivity.vm = (JavaVM *)&vm;
    nativeActivity.assetManager = (AAssetManager *)assetManager.get();
    nativeActivity.env = (JNIEnv *)&frame.getJniEnv();
    nativeActivity.internalDataPath = "/internal";
    nativeActivity.externalDataPath = "/external";
    nativeActivity.clazz = activityRef;
    nativeActivity.sdkVersion = activity->getAndroidVersion();

    Log::trace("JniSupport", "Invoking nativeRegisterThis\n");
    auto registerThis = activity->getClass().getMethod("()V", "nativeRegisterThis");
    if(registerThis)
        registerThis->invoke(frame.getJniEnv(), activity.get());

    Log::trace("JniSupport", "Invoking ANativeActivity_onCreate\n");
    activityOnCreate(&nativeActivity, nullptr, 0);

    Log::trace("JniSupport", "Invoking start activity callbacks\n");
    nativeActivityCallbacks.onInputQueueCreated(&nativeActivity, inputQueue);
    nativeActivityCallbacks.onStart(&nativeActivity);
    nativeActivityCallbacks.onNativeWindowCreated(&nativeActivity, window);
    // nativeActivityCallbacks.onResume(&nativeActivity);

    std::shared_ptr<NetworkMonitor> network;
    network = std::make_shared<NetworkMonitor>();
    auto updateNetworkStatus = network->getClass().getMethod("(ZZZ)V", "nativeUpdateNetworkStatus");
    if(updateNetworkStatus)
        updateNetworkStatus->invoke(frame.getJniEnv(), network.get(), true, true, true);

    if (!options.importFilePath.empty()) {
        importFile(options.importFilePath);
    }
    if (options.useStdinImport) {
        std::thread([=]() {
            for (std::string line; std::getline(std::cin, line);) {
                struct stat buffer;
                if ((stat (line.c_str(), &buffer) == 0)) {
                    importFile(line);
                }
            }
        }).detach();
    }

}

void JniSupport::importFile(std::string path) {
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE > 8
    std::string tmpDir = std::filesystem::temp_directory_path().generic_string();
#else
    std::string tmpDir = "/tmp";
#endif
    std::string fileExt = path.substr(path.find_last_of(".") + 1);
    if (fileExt == "mcworld" || fileExt == "mcpack" || fileExt == "mcaddon" || fileExt == "mctemplate") {
        FakeJni::LocalFrame frame(vm);
        try {
            std::string fileName = path.substr(path.find_last_of("/") + 1);
            if (path.find("&") == std::string::npos) {
                std::ifstream src(path, std::ios::binary);
                std::ofstream dest(tmpDir + "/" + fileName, std::ios::binary);
                dest << src.rdbuf(); // We have to copy it to the temp folder because the game will delete the archive if importing succeeds.

                auto fileOpen = activity->getClass().getMethod("(Ljava/lang/String;Ljava/lang/String;)V", "nativeProcessIntentUriQuery");
                fileOpen->invoke(frame.getJniEnv(), activity.get(), std::make_shared<FakeJni::JString>("contentIntent"), std::make_shared<FakeJni::JString>(path + "&" + tmpDir + "/" + fileName));
            } else {
                Log::warn("JniSupport", "Not importing file at %s; file path cannot contain &", path.c_str());
            }
        } catch (std::exception& e) {
            Log::error("JniSupport", "Failed to import file at %s: %s", path.c_str(), e.what());
        }
    } else {
        Log::warn("JniSupport", "Not importing file at %s; file extension must be .mcworld, .mcpack, .mcaddon, or .mctemplate", path.c_str());
    }
}

void JniSupport::stopGame() {
    FakeJni::LocalFrame frame(vm);

    Log::trace("JniSupport", "Invoking stop activity callbacks\n");
    auto nativeStopThis = activity->getClass().getMethod("()V", "nativeStopThis");
    nativeStopThis->invoke(frame.getJniEnv(), activity.get());
    auto nativeUnregisterThis = activity->getClass().getMethod("()V", "nativeUnregisterThis");
    if(nativeUnregisterThis)
        nativeUnregisterThis->invoke(frame.getJniEnv(), activity.get());
    auto nativeOnDestroy = activity->getClass().getMethod("()V", "nativeOnDestroy");
    nativeOnDestroy->invoke(frame.getJniEnv(), activity.get());
    nativeActivityCallbacks.onPause(&nativeActivity);
    nativeActivityCallbacks.onStop(&nativeActivity);
    nativeActivityCallbacks.onDestroy(&nativeActivity);

    Log::trace("JniSupport", "Waiting for looper clean up\n");
    std::unique_lock<std::mutex> lock(gameExitMutex);
    gameExitCond.wait(lock, [this] { return !looperRunning; });
    Log::trace("JniSupport", "exited\n");
}

void JniSupport::waitForGameExit() {
    std::unique_lock<std::mutex> lock(gameExitMutex);
    gameExitCond.wait(lock, [this] { return gameExitVal; });
}

void JniSupport::requestExitGame() {
    std::unique_lock<std::mutex> lock(gameExitMutex);
    gameExitVal = true;
    gameExitCond.notify_all();
    std::thread([this]() {
        JniSupport::stopGame();
    }).detach();
}

void JniSupport::setLooperRunning(bool running) {
    std::unique_lock<std::mutex> lock(gameExitMutex);
    looperRunning = running;
    if(!running)
        gameExitCond.notify_all();
}

void JniSupport::onWindowCreated(ANativeWindow *window, AInputQueue *inputQueue) {
    // Note on thread safety: This code is fine thread-wise because ANativeActivity_onCreate locks until the thread is
    // initialized; the thread initialization code runs ALooper_prepare before signaling it's ready.
    this->window = window;
    this->inputQueue = inputQueue;
    activity->window = (GameWindow *)window;
}

void JniSupport::onWindowClosed() {
    FakeJni::LocalFrame frame(vm);
    auto shutdown = activity->getClass().getMethod("()V", "nativeShutdown");
    shutdown->invoke(frame.getJniEnv(), activity.get());
}

void JniSupport::onWindowResized(int newWidth, int newHeight) {
    FakeJni::LocalFrame frame(vm);
    auto resize = activity->getClass().getMethod("(II)V", "nativeResize");
    if(resize)
        resize->invoke(frame.getJniEnv(), activity.get(), newWidth, newHeight);
}

void JniSupport::onSetTextboxText(std::string const &text) {
    FakeJni::LocalFrame frame(vm);
    auto setText = activity->getClass().getMethod("(Ljava/lang/String;)V", "nativeSetTextboxText");
    if(setText) {
        auto str = std::make_shared<FakeJni::JString>(text);
        setText->invoke(frame.getJniEnv(), activity.get(), frame.getJniEnv().createLocalReference(str));
    }
}

void JniSupport::onReturnKeyPressed() {
    FakeJni::LocalFrame frame(vm);
    auto returnPressed = activity->getClass().getMethod("()V", "nativeReturnKeyPressed");
    if(returnPressed)
        returnPressed->invoke(frame.getJniEnv(), activity.get());
}

void JniSupport::setGameControllerConnected(int devId, bool connected) {
    static auto addedMethod = JellyBeanDeviceManager::getDescriptor()->getMethod("(I)V", "onInputDeviceAddedNative");
    static auto removedMethod = JellyBeanDeviceManager::getDescriptor()->getMethod("(I)V", "onInputDeviceRemovedNative");

    FakeJni::LocalFrame frame(vm);
    if(connected && addedMethod)
        addedMethod->invoke(frame.getJniEnv(), JellyBeanDeviceManager::getDescriptor().get(), devId);
    else if(connected && removedMethod)
        removedMethod->invoke(frame.getJniEnv(), JellyBeanDeviceManager::getDescriptor().get(), devId);
}
