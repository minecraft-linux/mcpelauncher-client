#include "main_activity.h"
#include "store.h"

using namespace FakeJni;

BEGIN_NATIVE_DESCRIPTOR(BuildVersion)
{Field<&BuildVersion::SDK_INT> {}, "SDK_INT"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(File)
{Function<&File::getPath> {}, "getPath"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(ClassLoader)
{Function<&ClassLoader::loadClass> {}, "loadClass"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(Context)
{Function<&Context::getFilesDir> {}, "getFilesDir"},
{Function<&Context::getCacheDir> {}, "getCacheDir"},
{Function<&Context::getClassLoader> {}, "getClassLoader"},
{Function<&Context::getApplicationContext> {}, "getApplicationContext"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(ContextWrapper)
{Function<&ContextWrapper::getFilesDir> {}, "getFilesDir"},
{Function<&ContextWrapper::getCacheDir> {}, "getCacheDir"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(HardwareInfo)
{Function<&HardwareInfo::getAndroidVersion> {}, "getAndroidVersion"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(NativeActivity)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(MainActivity)
{Constructor<MainActivity> {}},
{Function<&MainActivity::getAndroidVersion> {}, "getAndroidVersion"},
{Function<&MainActivity::getLocale> {}, "getLocale"},
{Function<&MainActivity::getDeviceModel> {}, "getDeviceModel"},
{Function<&MainActivity::getExternalStoragePath> {}, "getExternalStoragePath"},
{Function<&MainActivity::hasWriteExternalStoragePermission> {}, "hasWriteExternalStoragePermission"},
{Function<&MainActivity::getHardwareInfo> {}, "getHardwareInfo"},
{Function<&MainActivity::createUUID> {}, "createUUID"},
{Function<&MainActivity::getFileDataBytes> {}, "getFileDataBytes"},
{Function<&MainActivity::getIPAddresses> {}, "getIPAddresses"},
{Function<&MainActivity::getBroadcastAddresses> {}, "getBroadcastAddresses"},
{Function<&MainActivity::showKeyboard> {}, "showKeyboard"},
{Function<&MainActivity::hideKeyboard> {}, "hideKeyboard"},
{Function<&MainActivity::updateTextboxText> {}, "updateTextboxText"},
{Function<&MainActivity::getCursorPosition> {}, "getCursorPosition"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(JellyBeanDeviceManager)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(StoreListener)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(NativeStoreListener)
{Constructor<NativeStoreListener, JLong> {}},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(Store)
{Function<&Store::receivedLicenseResponse> {}, "receivedLicenseResponse"},
{Function<&Store::hasVerifiedLicense> {}, "hasVerifiedLicense"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(StoreFactory)
{Function<&StoreFactory::createGooglePlayStore> {}, "createGooglePlayStore"},
END_NATIVE_DESCRIPTOR