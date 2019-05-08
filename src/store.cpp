#include "store.h"

#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>

void** LauncherStore::myVtable;

void LauncherStore::install(void* handle) {
    initVtable(handle);

    void* ptr = hybris_dlsym(handle, "_ZN12AndroidStore21createGooglePlayStoreERKSsR13StoreListener");
    PatchUtils::patchCallInstruction(ptr, (void*) &LauncherStore::createStore, true);
}

void LauncherStore::initVtable(void *lib) {
    void** vta = &((void**) hybris_dlsym(lib, "_ZTV12AndroidStore"))[2];
    size_t myVtableSize = PatchUtils::getVtableSize(vta);
    Log::trace("Store", "Vtable size = %u", myVtableSize);

    myVtable = (void**) ::operator new((myVtableSize + 1) * sizeof(void*));
    myVtable[myVtableSize] = nullptr;
    myVtable[0] = (void*) (void (*) (LauncherStore* p)) [](LauncherStore* p) { p->~LauncherStore(); };
    myVtable[1] = (void*) (void (*) (LauncherStore* p)) [](LauncherStore* p) { delete p; };

    PatchUtils::VtableReplaceHelper vtr (lib, myVtable, vta);
    vtr.replace("_ZNK5Store22isReadyToMakePurchasesEv", &LauncherStore::isReadyToMakePurchases);
    vtr.replace("_ZNK12AndroidStore30requiresRestorePurchasesButtonEv", &LauncherStore::requiresRestorePurchasesButton);
    vtr.replace("_ZNK12AndroidStore19allowsSubscriptionsEv", &LauncherStore::allowsSubscriptions);
    vtr.replace("_ZNK12AndroidStore10getStoreIdEv", &LauncherStore::getStoreId);
    vtr.replace("_ZNK5Store20getCatalogPlatformIdEv", &LauncherStore::getStoreId);
    vtr.replace("_ZNK5Store21getSubPlatformStoreIdEv", &LauncherStore::getSubPlatformStoreId);
    vtr.replace("_ZNK12AndroidStore19getProductSkuPrefixEv", &LauncherStore::getProductSkuPrefix);
    vtr.replace("_ZNK12AndroidStore18getRealmsSkuPrefixEv", &LauncherStore::getRealmsSkuPrefix);
    vtr.replace("_ZN12AndroidStore13queryProductsERKSt6vectorI10ProductSkuSaIS1_EE", &LauncherStore::queryProducts);
    vtr.replace("_ZN12AndroidStore8purchaseERK10ProductSku11ProductTypeRKSs", &LauncherStore::purchase);
    vtr.replace("_ZN12AndroidStore19acknowledgePurchaseERK12PurchaseInfo11ProductType", &LauncherStore::acknowledgePurchase);
    vtr.replace("_ZN12AndroidStore14queryPurchasesEb", &LauncherStore::queryPurchases);
    vtr.replace("_ZN12AndroidStore16restorePurchasesEv", &LauncherStore::restorePurchases);
    vtr.replace("_ZNK12AndroidStore7isTrialEv", &LauncherStore::isTrial);
    vtr.replace("_ZN12AndroidStore12purchaseGameEv", &LauncherStore::purchaseGame);
    vtr.replace("_ZNK12AndroidStore14isGameLicensedEv", &LauncherStore::isGameLicensed);
    vtr.replace("_ZN12AndroidStore23receivedLicenseResponseEv", &LauncherStore::receivedLicenseResponse);
    vtr.replace("_ZNK12AndroidStore19getExtraLicenseDataEv", &LauncherStore::getExtraLicenseData);
    vtr.replace("_ZNK12AndroidStore13getAppReceiptEv", &LauncherStore::getAppReceipt);
    vtr.replace("_ZN12AndroidStore29registerLicenseChangeCallbackESt8functionIFvvEE", &LauncherStore::registerLicenseChangeCallback);
    vtr.replace("_ZN12AndroidStore19handleLicenseChangeEv", &LauncherStore::handleLicenseChange);
    vtr.replace("_ZN5Store16restoreFromCacheEP13PurchaseCache", &LauncherStore::restoreFromCache);
    vtr.replace("_ZN5Store23getUserAccessTokenAsyncERKSsSt8functionIFvbSsEE", &LauncherStore::getUserAccessTokenAsync);
    vtr.replace("_ZN5Store36getFullSKUWithMetadataFromProductSkuERKSs", &LauncherStore::getFullSKUWithMetadataFromProductSku);
    vtr.replace("_ZNK5Store21getFullGameProductSkuEv", &LauncherStore::getFullGameProductSku);
    vtr.replace("_ZNK5Store15getLanguageCodeEv", &LauncherStore::getLanguageCode);
    vtr.replace("_ZNK5Store13getRegionCodeEv", &LauncherStore::getRegionCode);
    vtr.replace("_ZN5Store15refreshLicensesEv", &LauncherStore::refreshLicenses);
    vtr.replace("_ZN5Store10updateXUIDERKSs", &LauncherStore::updateXUID);
    vtr.replace("_ZN5Store16onNewPrimaryUserERKN6Social4UserE", &LauncherStore::onNewPrimaryUser);
    vtr.replace("_ZN5Store32onPrimaryUserConnectedToPlatformERKN6Social4UserE", &LauncherStore::onPrimaryUserConnectedToPlatform);
    vtr.replace("_ZN5Store12getPurchasesEv", &LauncherStore::getPurchases);

    // <1.2.3 legacy
    vtr.replace("_ZN12AndroidStore14queryPurchasesEv", &LauncherStore::queryPurchases);

    // <0.17.2 legacy
    vtr.replace("_ZN12AndroidStore10getStoreIdEv", &LauncherStore::getStoreId);
}