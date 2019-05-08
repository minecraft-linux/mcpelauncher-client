#include "xbox_live_patches.h"

#include <fstream>
#include <sstream>
#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>
#include <mcpelauncher/path_helper.h>
#include <mcpelauncher/minecraft_version.h>
#include <minecraft/Xbox.h>
#include <minecraft/legacy/Xbox.h>
#include <log.h>
#include <minecraft/std/shared_ptr.h>
#include "fake_jni.h"
#include "xbox_live_helper.h"
#include "xbox_live_game_interface.h"

const char* XboxLivePatches::TAG = "XboxLive";

void XboxLivePatches::install(void *handle) {
    // Function patches

    void* ptr = hybris_dlsym(handle, "_ZN3web4http6client7details35verify_cert_chain_platform_specificERN5boost4asio3ssl14verify_contextERKSs");
    PatchUtils::patchCallInstruction(ptr, (void*) verifyCertChain, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop16read_config_fileEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &readXboxConfigFile, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop20log_telemetry_signinEbRKSs");
    if (ptr != nullptr) // <0.15.90.8
        PatchUtils::patchCallInstruction(ptr, (void*) &xboxLogTelemetrySignin, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop22get_local_storage_pathEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &getLocalStoragePath, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services6system17user_auth_android21init_sign_in_activityEi");
    if (ptr == nullptr) // <1.2
        ptr = hybris_dlsym(handle, "_ZN4xbox8services6system17user_impl_android21init_sign_in_activityEi");
    PatchUtils::patchCallInstruction(ptr, (void*) &initSignInActivity, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services6system17user_auth_android16invoke_auth_flowEv");
    if (ptr == nullptr) // <1.2
        ptr = hybris_dlsym(handle, "_ZN4xbox8services6system17user_impl_android16invoke_auth_flowEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &invokeAuthFlow, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services5utils15get_locale_listEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &getLocaleList, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop16register_nativesEP15JNINativeMethod");
    PatchUtils::patchCallInstruction(ptr, (void*) &registerJavaInteropNatives, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop7log_cllERKSsS3_S3_");
    PatchUtils::patchCallInstruction(ptr, (void*) &logCLL, true);


    ptr = hybris_dlsym(handle, "_ZNK13MinecraftGame26useMinecraftVersionOfXBLUIEv");
    if (ptr)
        PatchUtils::patchCallInstruction(ptr, (void*) &useMinecraftVersionOfXBLUI, true);

    // Set global variables

    JavaVM** jvmPtr = (JavaVM**) hybris_dlsym(handle, "_ZN9crossplat3JVME");
    *jvmPtr = FakeJNI::instance.getVM();

    std::shared_ptr<xbox::services::java_interop> javaInterop = xbox::services::java_interop::get_java_interop_singleton();
    javaInterop->activity = (void*) 1; // this just needs not to be null

    if (MinecraftVersion::isAtLeast(1, 2, 3)) {
        xbox::services::system::xbox_live_services_settings::get_singleton_instance(true)->set_diagnostics_trace_level(0);
        xbox::services::system::xbox_live_services_settings::get_singleton_instance(true)->set_diagnostics_trace_level(5);
    }
}

bool XboxLivePatches::verifyCertChain() {
    Log::trace(TAG, "verify_cert_chain_platform_specific stub called");
    return true;
}

mcpe::string XboxLivePatches::readXboxConfigFile(void* th) {
    Log::trace(TAG, "Reading xbox config file");
    std::ifstream f(PathHelper::findGameFile("assets/xboxservices.config"));
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}

xbox::services::xbox_live_result<void> XboxLivePatches::xboxLogTelemetrySignin(void *th, bool b,
                                                                               mcpe::string const &s) {
    Log::trace(TAG, "log_telemetry_signin %i %s", (int) b, s.c_str());
    xbox::services::xbox_live_result<void> ret;
    ret.code = 0;
    ret.error_code_category = xbox::services::xbox_services_error_code_category();
    ret.message = " ";
    return ret;
}

mcpe::string XboxLivePatches::getLocalStoragePath() {
    return PathHelper::getPrimaryDataDirectory();
}

xbox::services::xbox_live_result<void> XboxLivePatches::initSignInActivity(
        xbox::services::system::user_auth_android *th, int requestCode) {
    Log::trace("Launcher", "init_sign_in_activity %i", requestCode);
    return XboxLiveGameInterface::getInstance().onInitSignInActivity(th, requestCode);
}

void XboxLivePatches::invokeAuthFlow(void* auth) {
    Log::trace(TAG, "invoke_auth_flow");
    XboxLiveGameInterface::getInstance().onInvokeAndroidAuthFlow(auth);
}

std::vector<mcpe::string> XboxLivePatches::getLocaleList() {
    return {"en-US"};
}

void XboxLivePatches::registerJavaInteropNatives() {
    Log::trace(TAG, "register_natives stub");
}

xbox::services::xbox_live_result<void> XboxLivePatches::logCLL(void* th, mcpe::string const& ticket,
                                                               mcpe::string const& name, mcpe::string const& data) {
    Log::trace(TAG, "log_cll %s %s %s", ticket.c_str(), name.c_str(), data.c_str());
    cll::Event event(name.std(), nlohmann::json::parse(data.std()),
                     cll::EventFlags::PersistenceCritical | cll::EventFlags::LatencyRealtime, {ticket.std()});
    XboxLiveHelper::getInstance().logCll(event);

    xbox::services::xbox_live_result<void> ret;
    ret.code = 0;
    ret.error_code_category = xbox::services::xbox_services_error_code_category();
    ret.message = " ";
    return ret;
}

bool XboxLivePatches::useMinecraftVersionOfXBLUI() {
    return true;
}

void XboxLivePatches::workaroundShutdownFreeze(void* handle) {
    auto userAndroidInstance = xbox::services::system::user_impl_android::get_instance();
    if (userAndroidInstance)
        userAndroidInstance->user_signed_out();
    destroyXsapiSingleton(handle);
}

void XboxLivePatches::destroyXsapiSingleton(void* handle) {
    unsigned int off = (unsigned int) hybris_dlsym(handle, "_ZN4xbox8services19get_xsapi_singletonEb");
    if (MinecraftVersion::isAtLeast(1, 9)) {
        unsigned int ebx = off + 0xa;
        ebx += *((unsigned int*) (off + 0xb + 2));
        unsigned int ptr = ebx + *((unsigned int*) (off + 0x11 + 2));
        ((mcpe::shared_ptr<xbox::services::xsapi_singleton>*) ptr)->reset();
    } else {
        unsigned int ebx = off + 0xb;
        ebx += *((unsigned int*) (off + 0xc + 2));
        unsigned int ptr = ebx + *((unsigned int*) (off + (0x661 - 0x4F0) + 2));
        ((mcpe::shared_ptr<xbox::services::xsapi_singleton>*) ptr)->reset();
    }
}