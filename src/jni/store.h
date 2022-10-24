#pragma once

#include <fake-jni/fake-jni.h>

class StoreListener : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/StoreListener")
};

class Product : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/Product")

    Product() {
        mId = std::make_shared<FakeJni::JString>("");
        mPrice = std::make_shared<FakeJni::JString>("");
        mCurrencyCode = std::make_shared<FakeJni::JString>("");
        mUnformattedPrice = std::make_shared<FakeJni::JString>("");
    }

    std::shared_ptr<FakeJni::JString> mId;
    std::shared_ptr<FakeJni::JString> mPrice;
    std::shared_ptr<FakeJni::JString> mCurrencyCode;
    std::shared_ptr<FakeJni::JString> mUnformattedPrice;
};

class Purchase : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/Purchase")

    Purchase() {
        mProductId = std::make_shared<FakeJni::JString>("");
        mReceipt = std::make_shared<FakeJni::JString>("");
        mPurchaseActive = true;
    }

    std::shared_ptr<FakeJni::JString> mProductId;
    std::shared_ptr<FakeJni::JString> mReceipt;
    FakeJni::JBoolean mPurchaseActive;
};

class ExtraLicenseResponseData : public FakeJni::JObject {
public:
    DEFINE_CLASS_NAME("com/mojang/minecraftpe/store/ExtraLicenseResponseData")

    FakeJni::JLong getValidationTime() {
        return 60000;
    }
    FakeJni::JLong getRetryUntilTime() {
        return 0;
    }
    FakeJni::JLong getRetryAttempts() {
        return 0;
    }
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
    std::shared_ptr<ExtraLicenseResponseData> getExtraLicenseData();
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
