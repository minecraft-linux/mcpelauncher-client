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

    NativeStoreListener(FakeJni::JLong nativePtr);

    void onStoreInitialized(bool available);

};

class Store : public FakeJni::JObject {
    bool _hasVerifiedLicense = false;
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/Store")

    explicit Store(std::shared_ptr<NativeStoreListener> storeListener, bool _hasVerifiedLicense);

    FakeJni::JBoolean receivedLicenseResponse();

    FakeJni::JBoolean hasVerifiedLicense();

};

class StoreFactory : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/StoreFactory")

    static bool hasVerifiedGooglePlayStoreLicense;
    static bool hasVerifiedAmazonAppStoreLicense;
    static std::shared_ptr<Store> createGooglePlayStore(std::shared_ptr<FakeJni::JString> googlePlayLicenseKey, std::shared_ptr<StoreListener> storeListener);
    static std::shared_ptr<Store> createAmazonAppStore(std::shared_ptr<StoreListener> storeListener);
};