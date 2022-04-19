#pragma once

#include <fake-jni/fake-jni.h>

class StoreListener : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/StoreListener")
};

class Product : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/Product")
};

class Purchase : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/Purchase")
};

class NativeStoreListener : public StoreListener {

public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/NativeStoreListener", StoreListener)

    FakeJni::JLong nativePtr;

    NativeStoreListener(FakeJni::JLong nativePtr);

    void onStoreInitialized(bool available);

    void onPurchaseFailed(std::shared_ptr<FakeJni::JString> message);

    void onQueryProductsSuccess(std::shared_ptr<FakeJni::JArray<Product>> products);

    void onQueryPurchasesSuccess(std::shared_ptr<FakeJni::JArray<Purchase>> purchases);
};

class Store : public FakeJni::JObject {
    bool _hasVerifiedLicense = false;
    std::shared_ptr<NativeStoreListener> storeListener;
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/Store")

    explicit Store(std::shared_ptr<NativeStoreListener> storeListener, bool _hasVerifiedLicense);

    FakeJni::JBoolean receivedLicenseResponse();

    FakeJni::JBoolean hasVerifiedLicense();

    std::shared_ptr<FakeJni::JString> getStoreId();
    std::shared_ptr<FakeJni::JString> getProductSkuPrefix();
    std::shared_ptr<FakeJni::JString> getRealmsSkuPrefix();
    // std::shared_ptr<jnivm::com::mojang::minecraftpe::store::ExtraLicenseResponseData> getExtraLicenseData();
    void queryProducts(std::shared_ptr<FakeJni::JArray<FakeJni::JString>>);
    void purchase(std::shared_ptr<FakeJni::JString>, FakeJni::JBoolean, std::shared_ptr<FakeJni::JString>);
    void acknowledgePurchase(std::shared_ptr<FakeJni::JString>, std::shared_ptr<FakeJni::JString>);
    void queryPurchases();
    void destructor();
};

class NotificationListenerService : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/NotificationListenerService")
    static std::shared_ptr<FakeJni::JString> getDeviceRegistrationToken() {
        return std::make_shared<FakeJni::JString>("ebe97d6c-5b83-11ec-9193-9fbef390d94b");
    }
};

class StoreFactory : public FakeJni::JObject {

public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/StoreFactory")

    static bool hasVerifiedGooglePlayStoreLicense;
    static bool hasVerifiedAmazonAppStoreLicense;
    static std::shared_ptr<Store> createGooglePlayStore(std::shared_ptr<FakeJni::JString> googlePlayLicenseKey, std::shared_ptr<StoreListener> storeListener);
    static std::shared_ptr<Store> createAmazonAppStore(std::shared_ptr<StoreListener> storeListener);
};