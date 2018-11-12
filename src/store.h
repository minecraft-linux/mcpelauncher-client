#pragma once

#include <memory>
#include <iostream>
#include <minecraft/std/string.h>
#include <log.h>

struct StoreListener;
struct PurchaseInfo;
struct ExtraLicenseData {
    long long validationTime = 0L;
    long long retryUntilTime = 0L;
    long long retryAttempts = 0L;
};

class LauncherStore {

private:
    void* vtable;

    static void** myVtable;
    static void initVtable(void* lib);

    static std::unique_ptr<LauncherStore> createStore(const mcpe::string& idk, StoreListener& listener) {
        //Log::trace("Launcher", "Creating store (%s)", idk.c_str());
        Log::trace("Launcher", "Creating store");
        return std::unique_ptr<LauncherStore>(new LauncherStore());
    }

public:
    static void install(void* handle);

    LauncherStore() {
        vtable = myVtable;
    }
    ~LauncherStore() {
        Log::trace("Store", "Destroying LinuxStore");
    }
    bool isReadyToMakePurchases() {
        Log::trace("Store", "isReadyToMakePurchases: false");
        return true;
    }
    bool requiresRestorePurchasesButton() {
        Log::trace("Store", "requiresRestorePurchasesButton: false");
        return false;
    }
    bool allowsSubscriptions() {
        // Log::trace("Store", "allowsSubscriptions: true");
        return true;
    }
    mcpe::string getStoreId() {
        Log::trace("Store", "getStoreId: android.googleplay");
        return "android.googleplay";
    }
    mcpe::string getSubPlatformStoreId() {
        Log::trace("Store", "getSubPlatformStoreId: ");
        return "";
    }
    mcpe::string getProductSkuPrefix() {
        // Log::trace("Store", "getProductSkuPrefix: ");
        return "";
    }
    mcpe::string getRealmsSkuPrefix() {
        // Log::trace("Store", "getRealmsSkuPrefix: ");
        return "";
    }
    void queryProducts(std::vector<std::string> const& arr) {
        Log::trace("Store", "queryProducts");
    }
    void purchase(std::string const& name) {
        Log::trace("Store", "purchase: %s", name.c_str());
    }
    void acknowledgePurchase(PurchaseInfo const& info, int type) {
        Log::trace("Store", "acknowledgePurchase: type=%i", type);
    }
    void queryPurchases() {
        Log::trace("Store", "queryPurchases");
    }
    void restorePurchases() {
        Log::trace("Store", "restorePurchases");
    }
    bool isTrial() {
        // Log::trace("Store", "isTrial: false");
        return false;
    }
    void purchaseGame() {
        Log::trace("Store", "purchaseGame");
    }
    bool isGameLicensed() {
        Log::trace("Store", "isGameLicensed: true");
        return true;
    }
    bool receivedLicenseResponse() {
        Log::trace("Store", "receivedLicenseResponse: true");
        return true;
    }
    ExtraLicenseData getExtraLicenseData() {
        Log::warn("Store", "getExtraLicenseData - not implemented");
        return ExtraLicenseData();
    }
    mcpe::string getAppReceipt() {
        Log::trace("Store", "getAppReceipt");
        return mcpe::string();
    }
    void registerLicenseChangeCallback() {
        Log::trace("Store", "registerLicenseChangeCallback");
    }
    void handleLicenseChange() {
        Log::trace("Store", "handleLicenseChange");
    }
    void restoreFromCache() {
        Log::trace("Store", "restoreFromCache");
    }
    void getUserAccessTokenAsync() {
        Log::trace("Store", "getUserAccessTokenAsync");
    }
    void getFullSKUWithMetadataFromProductSku() {
        Log::trace("Store", "getFullSKUWithMetadataFromProductSku");
    }
    mcpe::string getFullGameProductSku() {
        Log::trace("Store", "getFullGameProductSku");
        return "idk";
    }
    mcpe::string getLanguageCode() {
        Log::trace("Store", "getLanguageCode");
        return "idk";
    }
    mcpe::string getRegionCode() {
        Log::trace("Store", "getRegionCode");
        return "idk";
    }
    void refreshLicenses() {
        Log::trace("Store", "refreshLicenses");
    }
    void updateXUID() {
        Log::trace("Store", "updateXUID");
    }
    void onNewPrimaryUser() {
        Log::trace("Store", "onNewPrimaryUser");
    }
    void onPrimaryUserConnectedToPlatform() {
        Log::trace("Store", "onPrimaryUserConnectedToPlatform");
    }
    void getPurchases() {
        Log::trace("Store", "getPurchases");
    }

};