#pragma once

#include <fake-jni/fake-jni.h>

class StoreListener : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/StoreListener")
};

class NativeStoreListener : public StoreListener {

public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/NativeStoreListener", StoreListener)

    FakeJni::JLong nativePtr;

    NativeStoreListener(FakeJni::JLong nativePtr) : nativePtr(nativePtr) {
    }

    void onStoreInitialized(bool available) {
        auto method = getClass().getMethod("(JZ)V", "onStoreInitialized");
        FakeJni::LocalFrame frame;
        method->invoke(frame.getJniEnv(), this, nativePtr, true);
    }

};

class Store : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/Store")

    explicit Store(std::shared_ptr<NativeStoreListener> storeListener) {
        storeListener->onStoreInitialized(true);
    }

    FakeJni::JBoolean receivedLicenseResponse() {
        return true;
    }

    FakeJni::JBoolean hasVerifiedLicense() {
        return true;
    }

};

class StoreFactory : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/StoreFactory")

    static std::shared_ptr<Store> createGooglePlayStore(std::shared_ptr<FakeJni::JString> googlePlayLicenseKey, std::shared_ptr<StoreListener> storeListener) {
        return std::make_shared<Store>(std::dynamic_pointer_cast<NativeStoreListener>(storeListener));
    }

};