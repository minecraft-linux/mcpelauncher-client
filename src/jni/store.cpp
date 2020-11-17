#include "store.h"

NativeStoreListener::NativeStoreListener(FakeJni::JLong nativePtr) : nativePtr(nativePtr) {
}

void NativeStoreListener::onStoreInitialized(bool available) {
    auto method = getClass().getMethod("(JZ)V", "onStoreInitialized");
    FakeJni::LocalFrame frame;
    method->invoke(frame.getJniEnv(), this, nativePtr, true);
}

Store::Store(std::shared_ptr<NativeStoreListener> storeListener, bool _hasVerifiedLicense) {
    this->_hasVerifiedLicense = _hasVerifiedLicense;
    storeListener->onStoreInitialized(true);
}

FakeJni::JBoolean Store::receivedLicenseResponse() {
    return true;
}

FakeJni::JBoolean Store::hasVerifiedLicense() {
    return _hasVerifiedLicense;
}

bool StoreFactory::hasVerifiedGooglePlayStoreLicense = true;
bool StoreFactory::hasVerifiedAmazonAppStoreLicense = true;

std::shared_ptr<Store> StoreFactory::createGooglePlayStore(std::shared_ptr<FakeJni::JString> googlePlayLicenseKey, std::shared_ptr<StoreListener> storeListener) {
    return std::make_shared<Store>(std::dynamic_pointer_cast<NativeStoreListener>(storeListener), hasVerifiedGooglePlayStoreLicense);
}

std::shared_ptr<Store> StoreFactory::createAmazonAppStore(std::shared_ptr<StoreListener> storeListener) {
    return std::make_shared<Store>(std::dynamic_pointer_cast<NativeStoreListener>(storeListener), hasVerifiedAmazonAppStoreLicense);
}