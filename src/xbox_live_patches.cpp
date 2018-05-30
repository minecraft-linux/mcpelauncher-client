#include "xbox_live_patches.h"

#include <fstream>
#include <sstream>
#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>
#include <mcpelauncher/path_helper.h>
#include <minecraft/Xbox.h>
#include <log.h>
#include "fake_jni.h"
#include "xbox_live_helper.h"

const char* XboxLivePatches::TAG = "XboxLive";

void XboxLivePatches::install(void *handle) {
    // Function patches

    void* ptr = hybris_dlsym(handle, "_ZN3web4http6client7details35verify_cert_chain_platform_specificERN5boost4asio3ssl14verify_contextERKSs");
    PatchUtils::patchCallInstruction(ptr, (void*) verifyCertChain, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop16read_config_fileEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &readXboxConfigFile, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop20log_telemetry_signinEbRKSs");
    PatchUtils::patchCallInstruction(ptr, (void*) &xboxLogTelemetrySignin, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop22get_local_storage_pathEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &getLocalStoragePath, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services6system17user_auth_android21init_sign_in_activityEi");
    PatchUtils::patchCallInstruction(ptr, (void*) &initSignInActivity, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services6system17user_auth_android16invoke_auth_flowEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &invokeAuthFlow, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services5utils15get_locale_listEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &getLocaleList, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop16register_nativesEP15JNINativeMethod");
    PatchUtils::patchCallInstruction(ptr, (void*) &registerJavaInteropNatives, true);

    ptr = hybris_dlsym(handle, "_ZN4xbox8services12java_interop7log_cllERKSsS3_S3_");
    PatchUtils::patchCallInstruction(ptr, (void*) &logCLL, true);

    // Set global variables

    JavaVM** jvmPtr = (JavaVM**) hybris_dlsym(handle, "_ZN9crossplat3JVME");
    *jvmPtr = FakeJNI::instance.getVM();

    std::shared_ptr<xbox::services::java_interop> javaInterop = xbox::services::java_interop::get_java_interop_singleton();
    javaInterop->activity = (void*) 1; // this just needs not to be null
}

bool XboxLivePatches::verifyCertChain() {
    Log::trace(TAG, "verify_cert_chain_platform_specific stub called");
    return true;
}

mcpe::string XboxLivePatches::readXboxConfigFile(void* th) {
    Log::trace(TAG, "Reading xbox config file");
    std::ifstream f(PathHelper::findDataFile("assets/xboxservices.config"));
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
    xbox::services::xbox_live_result<void> ret;
    ret.code = 0;
    ret.error_code_category = xbox::services::xbox_services_error_code_category();

    auto local_conf = xbox::services::local_config::get_local_config_singleton();
    th->cid = local_conf->get_value_from_local_storage("cid").std();

    if (requestCode == 1) { // silent signin
        xbox::services::system::java_rps_ticket ticket;
        ticket.error_code = 1;
        ticket.error_text = "Must show UI to acquire an account.";
        xbox::services::system::user_auth_android::s_rpsTicketCompletionEvent->set(ticket);
    } else if (requestCode == 6) { // sign out
        xbox::services::xbox_live_result<void> arg;
        arg.code = 0;
        arg.error_code_category = xbox::services::xbox_services_error_code_category();
        xbox::services::system::user_auth_android::s_signOutCompleteEvent->set(arg);
    }

    return ret;
}

void XboxLivePatches::invokeAuthFlow(xbox::services::system::user_auth_android* auth) {
    Log::trace(TAG, "invoke_auth_flow");

    XboxLiveHelper::getInstance().invokeMsaAuthFlow([auth](std::string const& cid, std::string const& token) {
        Log::trace(TAG, "Got account CID and token");
        auto ret = XboxLiveHelper::getInstance().invokeXblLogin(auth, cid, token);
        Log::trace(TAG, "Xbox Live login completed");

        auth->auth_flow_result.code = 0;
        auth->auth_flow_result.xbox_user_id = ret.data.xbox_user_id;
        auth->auth_flow_result.gamertag = ret.data.gamertag;
        auth->auth_flow_result.age_group = ret.data.age_group;
        auth->auth_flow_result.privileges = ret.data.privileges;
        auth->auth_flow_result.user_settings_restrictions = ret.data.user_settings_restrictions;
        auth->auth_flow_result.user_enforcement_restrictions = ret.data.user_enforcement_restrictions;
        auth->auth_flow_result.user_title_restrictions = ret.data.user_title_restrictions;
        auth->auth_flow_result.cid = cid;
        auth->auth_flow_event.set(auth->auth_flow_result);
    }, [auth]() {
        Log::trace(TAG, "Sign in error");
        auth->auth_flow_result.code = 2;
        auth->auth_flow_event.set(auth->auth_flow_result);
    });
}

std::vector<mcpe::string> XboxLivePatches::getLocaleList() {
    return {"en-US"};
}

void XboxLivePatches::registerJavaInteropNatives() {
    Log::trace(TAG, "register_natives stub");
}

xbox::services::xbox_live_result<void> XboxLivePatches::logCLL(void *th, mcpe::string const &a, mcpe::string const &b,
                                                               mcpe::string const &c) {
    Log::trace(TAG, "log_cll %s %s %s", a.c_str(), b.c_str(), c.c_str());
    // TODO: add it as an CLL event
    xbox::services::xbox_live_result<void> ret;
    ret.code = 0;
    ret.error_code_category = xbox::services::xbox_services_error_code_category();
    ret.message = " ";
    return ret;
}