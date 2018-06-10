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


    ptr = hybris_dlsym(handle, "_ZNK13MinecraftGame26useMinecraftVersionOfXBLUIEv");
    PatchUtils::patchCallInstruction(ptr, (void*) &useMinecraftVersionOfXBLUI, true);

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
    th->cid = local_conf->get_value_from_local_storage("cid");

    if (requestCode == 1) { // silent signin
        if (th->cid.length() > 0) {
            XboxLiveHelper::getInstance().requestXblToken(th->cid.std(), true,
                    [](std::string const& cid, std::string const& token) {
                        XboxLiveHelper::getInstance().getCllAuthStep().setAccount(cid);

                        xbox::services::system::java_rps_ticket ticket;
                        ticket.token = token;
                        ticket.error_code = 0;
                        ticket.error_text = "Got ticket";
                        xbox::services::system::user_auth_android::s_rpsTicketCompletionEvent->set(ticket);
                    }, [](simpleipc::rpc_error_code, std::string const& msg) {
                        xbox::services::system::java_rps_ticket ticket;
                        Log::error(TAG, "Xbox Live sign in failed: %s", msg.c_str());
                        ticket.error_code = 0x800704CF;
                        ticket.error_text = msg.c_str();
                        xbox::services::system::user_auth_android::s_rpsTicketCompletionEvent->set(ticket);
                    });
        } else {
            xbox::services::system::java_rps_ticket ticket;
            ticket.error_code = 1;
            ticket.error_text = "Must show UI to acquire an account.";
            xbox::services::system::user_auth_android::s_rpsTicketCompletionEvent->set(ticket);
        }
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
        Log::trace(TAG, "Invoking XBL login");
        auto ret = XboxLiveHelper::getInstance().invokeXblLogin(cid, token);
        Log::trace(TAG, "Invoking XBL event init");
        XboxLiveHelper::getInstance().getCllAuthStep().setAccount(cid);
        auto retEv = XboxLiveHelper::getInstance().invokeEventInit();
        Log::trace(TAG, "Xbox Live login completed");

        auth->auth_flow_result.code = 0;
        auth->auth_flow_result.xbox_user_id = ret.data.xbox_user_id;
        auth->auth_flow_result.gamertag = ret.data.gamertag;
        auth->auth_flow_result.age_group = ret.data.age_group;
        auth->auth_flow_result.privileges = ret.data.privileges;
        auth->auth_flow_result.user_settings_restrictions = ret.data.user_settings_restrictions;
        auth->auth_flow_result.user_enforcement_restrictions = ret.data.user_enforcement_restrictions;
        auth->auth_flow_result.user_title_restrictions = ret.data.user_title_restrictions;
        auth->auth_flow_result.event_token = retEv.data.token;
        auth->auth_flow_result.cid = cid;
        auth->auth_flow_event.set(auth->auth_flow_result);
    }, [auth](simpleipc::rpc_error_code, std::string const& msg) {
        Log::trace(TAG, "Sign in error: %s", msg.c_str());
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
    unsigned int ebx = off + 0xb;
    ebx += *((unsigned int*) (off + 0xc + 2));
    unsigned int ptr = ebx + *((unsigned int*) (off + (0x661 - 0x4F0) + 2));
    ((std::shared_ptr<xbox::services::xsapi_singleton>*) ptr)->reset();
}