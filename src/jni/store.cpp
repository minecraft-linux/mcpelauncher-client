#include "store.h"

NativeStoreListener::NativeStoreListener(FakeJni::JLong nativePtr) : nativePtr(nativePtr) {
}

void NativeStoreListener::onStoreInitialized(bool available) {
    auto method = getClass().getMethod("(JZ)V", "onStoreInitialized");
    FakeJni::LocalFrame frame;
    method->invoke(frame.getJniEnv(), this, nativePtr, true);
}

void NativeStoreListener::onPurchaseFailed(std::shared_ptr<FakeJni::JString> message) {
    auto method = getClass().getMethod("(JLjava/lang/String;)V", "onPurchaseFailed");
    FakeJni::LocalFrame frame;
    method->invoke(frame.getJniEnv(), this, nativePtr, frame.getJniEnv().createLocalReference(message));
}

void NativeStoreListener::onQueryProductsSuccess(std::shared_ptr<FakeJni::JArray<Product>> products) {
    auto method = getClass().getMethod("(J[Lcom/mojang/minecraftpe/store/Product;)V", "onQueryProductsSuccess");
    FakeJni::LocalFrame frame;
    method->invoke(frame.getJniEnv(), this, nativePtr, frame.getJniEnv().createLocalReference(products));
}

void NativeStoreListener::onQueryPurchasesSuccess(std::shared_ptr<FakeJni::JArray<Purchase>> purchases) {
    auto method = getClass().getMethod("(J[Lcom/mojang/minecraftpe/store/Purchase;)V", "onQueryPurchasesSuccess");
    FakeJni::LocalFrame frame;
    method->invoke(frame.getJniEnv(), this, nativePtr, frame.getJniEnv().createLocalReference(purchases));
}

Store::Store(std::shared_ptr<NativeStoreListener> storeListener, bool _hasVerifiedLicense) {
    this->_hasVerifiedLicense = _hasVerifiedLicense;
    storeListener->onStoreInitialized(true);
    this->storeListener = storeListener;
}

FakeJni::JBoolean Store::receivedLicenseResponse() {
    return true;
}

FakeJni::JBoolean Store::hasVerifiedLicense() {
    return _hasVerifiedLicense;
}

std::shared_ptr<FakeJni::JString> Store::getStoreId() {
    return std::make_shared<FakeJni::JString>("android.googleplay");
}

std::shared_ptr<FakeJni::JString> Store::getProductSkuPrefix() {
    return {};
}

std::shared_ptr<FakeJni::JString> Store::getRealmsSkuPrefix() {
    return {};
}

std::shared_ptr<ExtraLicenseResponseData> Store::getExtraLicenseData() {
    return std::make_shared<ExtraLicenseResponseData>();
}

void Store::queryProducts(std::shared_ptr<FakeJni::JArray<FakeJni::JString>> arg0) {
    auto products = std::make_shared<FakeJni::JArray<Product>>(arg0->getSize());
    for(int i = 0; i < arg0->getSize(); i++) {
        auto product = std::make_shared<Product>();
        product->mId = (*arg0)[i];
        product->mPrice->append("10000#");
        product->mUnformattedPrice->append("10000#");
        product->mCurrencyCode->append("mcl");
        (*products)[i] = product;
    }
    this->storeListener->onQueryProductsSuccess(products);
}

void Store::purchase(std::shared_ptr<FakeJni::JString> arg0, FakeJni::JBoolean arg1, std::shared_ptr<FakeJni::JString> arg2) {
    this->storeListener->onPurchaseFailed(std::make_shared<FakeJni::JString>("Hi"));
}

void Store::acknowledgePurchase(std::shared_ptr<FakeJni::JString> arg0, std::shared_ptr<FakeJni::JString> arg1) {
}

void Store::queryPurchases() {
    this->storeListener->onQueryPurchasesSuccess(std::make_shared<FakeJni::JArray<Purchase>>());
}

void Store::destructor() {
}

bool StoreFactory::hasVerifiedGooglePlayStoreLicense = true;
bool StoreFactory::hasVerifiedAmazonAppStoreLicense = true;

std::shared_ptr<Store> StoreFactory::createGooglePlayStore(std::shared_ptr<FakeJni::JString> googlePlayLicenseKey, std::shared_ptr<StoreListener> storeListener) {
    return std::make_shared<Store>(std::dynamic_pointer_cast<NativeStoreListener>(storeListener), hasVerifiedGooglePlayStoreLicense);
}

std::shared_ptr<Store> StoreFactory::createAmazonAppStore(std::shared_ptr<StoreListener> storeListener) {
    return std::make_shared<Store>(std::dynamic_pointer_cast<NativeStoreListener>(storeListener), hasVerifiedAmazonAppStoreLicense);
}
