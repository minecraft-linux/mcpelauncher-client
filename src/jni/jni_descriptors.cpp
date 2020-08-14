#include "main_activity.h"
#include "store.h"
#include "xbox_live.h"
#include "lib_http_client.h"
#include "cert_manager.h"
#include "package_source.h"
#include "http_stub.h"
#include "java_types.h"
#include "accounts.h"


using namespace FakeJni;

BEGIN_NATIVE_DESCRIPTOR(BuildVersion)
{Field<&BuildVersion::SDK_INT> {}, "SDK_INT"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(PackageInfo)
{Field<&PackageInfo::versionName> {}, "versionName"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(PackageManager)
{Function<&PackageManager::getPackageInfo> {}, "getPackageInfo"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(File)
{Function<&File::getPath> {}, "getPath"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(ClassLoader)
{Function<&ClassLoader::loadClass> {}, "loadClass"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(Locale)
{Function<&Locale::getDefault> {}, "getDefault"},
{Function<&Locale::toString> {}, "toString"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(UUID)
{Function<&UUID::randomUUID> {}, "randomUUID"},
{Function<&UUID::toString> {}, "toString"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(Context)
{Function<&Context::getFilesDir> {}, "getFilesDir"},
{Function<&Context::getCacheDir> {}, "getCacheDir"},
{Function<&Context::getClassLoader> {}, "getClassLoader"},
{Function<&Context::getApplicationContext> {}, "getApplicationContext"},
{Function<&Context::getPackageName> {}, "getPackageName"},
{Function<&Context::getPackageManager> {}, "getPackageManager"},
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
{Function<&MainActivity::pickImage> {}, "pickImage"},
{Function<&MainActivity::initializeXboxLive> {}, "initializeXboxLive"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(AccountManager)
{Function<&AccountManager::get> {}, "get"},
{Function<&AccountManager::getAccountsByType> {}, "getAccountsByType"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(Account)
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
{Function<&XboxInterop::initCLL> {}, "InitCLL"},
{Function<&XboxInterop::logCLL> {}, "LogCLL"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(Ecdsa)
{Constructor<Ecdsa> {}},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(HttpClientRequest)
{Constructor<HttpClientRequest> {}},
{Function<&HttpClientRequest::isNetworkAvailable> {}, "isNetworkAvailable"},
{Function<&HttpClientRequest::createClientRequest> {}, "createClientRequest"},
{Function<&HttpClientRequest::setHttpUrl> {}, "setHttpUrl"},
{Function<&HttpClientRequest::setHttpMethodAndBody> {}, "setHttpMethodAndBody"},
{Function<&HttpClientRequest::setHttpHeader> {}, "setHttpHeader"},
{Function<&HttpClientRequest::doRequestAsync> {}, "doRequestAsync"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(HttpClientResponse)
{Function<&HttpClientResponse::getNumHeaders> {}, "getNumHeaders"},
{Function<&HttpClientResponse::getHeaderNameAtIndex> {}, "getHeaderNameAtIndex"},
{Function<&HttpClientResponse::getHeaderValueAtIndex> {}, "getHeaderValueAtIndex"},
{Function<&HttpClientResponse::getResponseBodyBytes> {}, "getResponseBodyBytes"},
{Function<&HttpClientResponse::getResponseCode> {}, "getResponseCode"},
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

BEGIN_NATIVE_DESCRIPTOR(X509TrustManager)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(TrustManagerFactory)
{Function<&TrustManagerFactory::getInstance> {}, "getInstance"},
{Function<&TrustManagerFactory::getTrustManagers> {}, "getTrustManagers"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(StrictHostnameVerifier)
{Constructor<StrictHostnameVerifier> {}},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(PackageSource)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(PackageSourceListener)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(NativePackageSourceListener)
{Constructor<NativePackageSourceListener> {}},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(PackageSourceFactory)
{Function<&PackageSourceFactory::createGooglePlayPackageSource> {}, "createGooglePlayPackageSource"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(Header)
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(HTTPResponse)
{Function<&HTTPResponse::getStatus> {}, "getStatus"},
{Function<&HTTPResponse::getBody> {}, "getBody"},
{Function<&HTTPResponse::getResponseCode> {}, "getResponseCode"},
{Function<&HTTPResponse::getHeaders> {}, "getHeaders"},
END_NATIVE_DESCRIPTOR

BEGIN_NATIVE_DESCRIPTOR(HTTPRequest)
{Function<&HTTPRequest::setURL> {}, "setURL"},
{Function<&HTTPRequest::setRequestBody> {}, "setRequestBody"},
{Function<&HTTPRequest::setCookieData> {}, "setCookieData"},
{Function<&HTTPRequest::setContentType> {}, "setContentType"},
{Function<&HTTPRequest::send> {}, "send"},
{Function<&HTTPRequest::abort> {}, "abort"},
{Constructor<HTTPRequest> {}},
END_NATIVE_DESCRIPTOR
