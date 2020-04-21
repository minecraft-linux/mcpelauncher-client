#include "main_activity.h"
#include "store.h"
#include "xbox_live.h"
#include "cert_manager.h"

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

BEGIN_NATIVE_DESCRIPTOR(Activity)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(NativeActivity)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(MainActivity)
{Constructor<MainActivity> {}},
{Function<&MainActivity::getAndroidVersion> {}, "getAndroidVersion"},
{Function<&MainActivity::isNetworkEnabled> {}, "isNetworkEnabled"},
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
{Function<&MainActivity::getUsedMemory> {}, "getUsedMemory"},
{Function<&MainActivity::getFreeMemory> {}, "getFreeMemory"},
{Function<&MainActivity::getTotalMemory> {}, "getTotalMemory"},
{Function<&MainActivity::getMemoryLimit> {}, "getMemoryLimit"},
{Function<&MainActivity::getAvailableMemory> {}, "getAvailableMemory"},
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

BEGIN_NATIVE_DESCRIPTOR(XboxInterop)
{Function<&XboxInterop::getLocalStoragePath> {}, "GetLocalStoragePath"},
{Function<&XboxInterop::readConfigFile> {}, "ReadConfigFile"},
{Function<&XboxInterop::getLocale> {}, "getLocale"},
{Function<&XboxInterop::invokeMSA> {}, "InvokeMSA"},
{Function<&XboxInterop::invokeAuthFlow> {}, "InvokeAuthFlow"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(XboxLoginCallback)
{Function<&XboxLoginCallback::onLogin> {}, "onLogin"},
{Function<&XboxLoginCallback::onSuccess> {}, "onSuccess"},
{Function<&XboxLoginCallback::onError> {}, "onError"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(InputStream)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(ByteArrayInputStream)
{Constructor<ByteArrayInputStream, std::shared_ptr<FakeJni::JByteArray>> {}},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(Certificate)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(X509Certificate)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(CertificateFactory)
{Function<&CertificateFactory::getInstance> {}, "getInstance"},
{Function<&CertificateFactory::generateCertificate> {}, "generateCertificate"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(TrustManager)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(TrustManagerFactory)
{Function<&TrustManagerFactory::getInstance> {}, "getInstance"},
{Function<&TrustManagerFactory::getTrustManagers> {}, "getTrustManagers"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(StrictHostnameVerifier)
{Constructor<StrictHostnameVerifier> {}},
END_NATIVE_DESCRIPTOR