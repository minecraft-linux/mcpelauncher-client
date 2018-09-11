#include "xbox_live_patches.h"

#include <fstream>
#include <sstream>
#include <hybris/dlfcn.h>
#include <mcpelauncher/patch_utils.h>
#include <mcpelauncher/path_helper.h>
#include <minecraft/ClientInstance.h>
#include <log.h>
#include <mcpelauncher/hook.h>
#include <minecraft/Crypto.h>
#include "fake_jni.h"
#include "xbox_live_helper.h"
#include "client_app_platform.h"
#include "xbox_live_msa_remote_login.h"

const char* XboxLivePatches::TAG = "XboxLive";

web::json::value (*XboxLivePatches::createDeviceTokenRequestOriginal)(mcpe::string, mcpe::string, void*, mcpe::string,
                                                                      mcpe::string, mcpe::string);
void (*XboxLivePatches::signInOriginal)(MinecraftScreenModel*, mcpe::function<void ()>,
                                        mcpe::function<void (Social::SignInResult, bool)>);


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

    (void*&) createDeviceTokenRequestOriginal = hybris_dlsym(handle, "_ZN4xbox8services6system13token_request27create_device_token_requestESsSsSt10shared_ptrINS1_5ecdsaEESsSsSs");
    ptr = hybris_dlsym(handle, "_ZN4xbox8services6system25device_token_service_impl24get_d_token_from_serviceERKSsSt10shared_ptrINS1_5ecdsaEES5_INS1_11auth_configEES5_INS0_26xbox_live_context_settingsEEN4pplx18cancellation_tokenE");
    ptr = (void*) ((size_t) ptr + 0x8BD1 - 0x8A40);
    PatchUtils::patchCallInstruction(ptr, (void*) &createDeviceTokenRequestHook, false);

    (void*&) signInOriginal = hybris_dlsym(handle, "_ZN20MinecraftScreenModel6signInESt8functionIFvvEES0_IFvN6Social12SignInResultEbEE");
    ptr = hybris_dlsym(handle, "_ZN25MinecraftScreenController13_handleSignInESt8functionIFvN6Social12SignInResultEbEE");
    ptr = (void*) ((size_t) ptr + 0x1420 - 0x1210);
    PatchUtils::patchCallInstruction(ptr, (void*) &signInHook, false);

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
    xbox::services::xbox_live_result<void> ret;
    ret.code = 0;
    ret.error_code_category = xbox::services::xbox_services_error_code_category();

    auto local_conf = xbox::services::local_config::get_local_config_singleton();
    th->cid = local_conf->get_value_from_local_storage("cid");

    if (requestCode == 1) { // silent signin
        if (th->cid.length() > 0) {
            XboxLiveHelper::getInstance().getLoginInterface().requestXblToken(th->cid.std(), true,
                    [](std::string const& cid, std::string const& token) {
                        XboxLiveHelper::getInstance().initCll(cid);

                        xbox::services::system::java_rps_ticket ticket;
                        ticket.token = token;
                        ticket.error_code = 0;
                        ticket.error_text = "Got ticket";
                        xbox::services::system::user_auth_android::s_rpsTicketCompletionEvent->set(ticket);
                    }, [](XboxLiveLoginInterface::ErrorCode err, std::string const& msg) {
                        xbox::services::system::java_rps_ticket ticket;
                        Log::error(TAG, "Xbox Live sign in failed: %s", msg.c_str());
                        if (err == XboxLiveLoginInterface::ErrorCode::InternalError) { // No such account
                            ticket.error_code = 1;
                            ticket.error_text = "Must show UI to acquire an account.";
                        } else {
                            ticket.error_code = 0x800704CF;
                            ticket.error_text = msg.c_str();
                        }
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

    XboxLiveHelper::getInstance().getLoginInterface().invokeMsaAuthFlow([auth](std::string const& cid,
                                                                               std::string const& token) {
        Log::trace(TAG, "Got account CID and token");
        Log::trace(TAG, "Invoking XBL login");
        auto ret = XboxLiveHelper::getInstance().invokeXblLogin(cid, token);
        Log::trace(TAG, "Invoking XBL event init");
        XboxLiveHelper::getInstance().initCll(cid);
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
    }, [auth](XboxLiveLoginInterface::ErrorCode e, std::string const& msg) {
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

web::json::value XboxLivePatches::createDeviceTokenRequestHook(mcpe::string a, mcpe::string b, void* c, mcpe::string d,
                                                               mcpe::string e, mcpe::string f) {
    if (!XboxLiveHelper::getInstance().getLoginInterface().isRemoteConnect())
        return createDeviceTokenRequestOriginal(a, b, c, d, e, f);

    web::json::value ret = createDeviceTokenRequestOriginal(a, b, c, d, e, "ProofOfPossession");
    auto& props = ret["Properties"];
    props.erase("RpsTicket");
    props.erase("SiteName");
    props["DeviceType"] = web::json::value("Nintendo");
    props["Id"] = web::json::value(Crypto::Random::generateUUID().asString());
    props["SerialNumber"] = web::json::value(Crypto::Random::generateUUID().asString());
    props["Version"] = web::json::value("0.0.0.0");
    return ret;
}

void XboxLivePatches::signInHook(MinecraftScreenModel* th, mcpe::function<void()> cancelCb,
                                 mcpe::function<void(Social::SignInResult, bool)> cb) {
    if (!XboxLiveHelper::getInstance().getLoginInterface().isRemoteConnect()) {
        signInOriginal(th, std::move(cancelCb), std::move(cb));
        return;
    }
    XboxLiveHelper::getInstance().getLoginInterface().invokeMsaRemoteAuthFlow([th](std::string const& code) {
        Log::trace(TAG, "Got code");
        ((LauncherAppPlatform*) *AppPlatform::instance)->queueForMainThread([th, code]() {
            th->navigateToXblConsoleSignInScreen(code, "xbox.signin.url");
        });
    }, [th, cb](std::string const& cid, std::string const& token) {
        Log::trace(TAG, "Invoking XBL login: %s", token.c_str());
        auto ret = XboxLiveHelper::getInstance().invokeXblLogin(cid, token);
        Log::trace(TAG, "Invoking XBL event init");
        XboxLiveHelper::getInstance().initCll(resp.userId);
        auto retEv = XboxLiveHelper::getInstance().invokeEventInit();
        Log::trace(TAG, "Xbox Live login completed");
        bool newAccount = retEv.code == 0x8015DC09 /* creation required error code */;

        auto auth = xbox::services::system::user_auth_android::get_instance();
        auth->auth_flow_result.code = 0;
        auth->auth_flow_result.xbox_user_id = ret.data.xbox_user_id;
        auth->auth_flow_result.gamertag = ret.data.gamertag;
        auth->auth_flow_result.age_group = ret.data.age_group;
        auth->auth_flow_result.privileges = ret.data.privileges;
        auth->auth_flow_result.user_settings_restrictions = ret.data.user_settings_restrictions;
        auth->auth_flow_result.user_enforcement_restrictions = ret.data.user_enforcement_restrictions;
        auth->auth_flow_result.user_title_restrictions = ret.data.user_title_restrictions;
        auth->auth_flow_result.event_token = ret.data.token;
        auth->auth_flow_result.cid = cid;

        ((LauncherAppPlatform*) *AppPlatform::instance)->queueForMainThread([th, cb, newAccount, auth]() {
            auth->complete_sign_in_with_ui(auth->auth_flow_result);

            xbox::services::system::sign_in_result res;
            res.result = xbox::services::system::sign_in_status::success;
            res.new_account = newAccount;
            th->clientInstance->getUser()->getLiveUser()->_handleUISignInNoError(res, [th, cb](
                    Social::SignInResult a, bool b) {
                cb(a, b);
                th->navigateToXblConsoleSignInSucceededScreen(a, [](Social::SignInResult r) {
                    Log::trace(TAG, "XblConsoleSignInSucceededScreen callback called");
                }, b);
            });
        });
    }, [cb](std::exception_ptr err) {
        cb(Social::SignInResult::Error, false);
    });
}

void XboxLivePatches::destroyXsapiSingleton(void* handle) {
    unsigned int off = (unsigned int) hybris_dlsym(handle, "_ZN4xbox8services19get_xsapi_singletonEb");
    unsigned int ebx = off + 0xb;
    ebx += *((unsigned int*) (off + 0xc + 2));
    unsigned int ptr = ebx + *((unsigned int*) (off + (0x661 - 0x4F0) + 2));
    ((std::shared_ptr<xbox::services::xsapi_singleton>*) ptr)->reset();
}