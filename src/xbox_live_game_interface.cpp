#include "xbox_live_game_interface.h"
#include "xbox_live_helper.h"
#include <log.h>
#include <mcpelauncher/minecraft_version.h>
#include <minecraft/legacy/Xbox.h>

#define TAG "XboxLiveGameInterface"

XboxLiveGameInterface& XboxLiveGameInterface::getInstance() {
    static XboxLiveDefaultGameInterface instance;
    return instance;
}

xbox::services::xbox_live_result<xbox::services::system::token_and_signature_result>
XboxLiveDefaultGameInterface::invokeXblLogin(std::string const &cid, std::string const &binaryToken) {
    using namespace xbox::services::system;
    auto auth_mgr = auth_manager::get_auth_manager_instance();
    auth_mgr->set_rps_ticket(binaryToken);
    auto initTask = auth_mgr->initialize_default_nsal();
    auto initRet = initTask.get();
    if (initRet.code != 0)
        throw std::runtime_error("Failed to initialize default nsal");
    std::vector<token_identity_type> types = {(token_identity_type) 3, (token_identity_type) 1,
                                              (token_identity_type) 2};
    auto config = auth_mgr->get_auth_config();
    config->set_xtoken_composition(types);
    std::string const& endpoint = config->xbox_live_endpoint().std();
    Log::trace(TAG, "Xbox Live Endpoint: %s", endpoint.c_str());
    auto task = auth_mgr->internal_get_token_and_signature("GET", endpoint, endpoint, std::string(), std::vector<unsigned char>(), false, false, std::string());
    Log::trace(TAG, "Get token and signature task started!");
    auto ret = task.get();
    Log::debug(TAG, "User info received! Status: %i", ret.code);
    Log::debug(TAG, "Gamertag = %s, age group = %s, web account id = %s\n", ret.data.gamertag.c_str(), ret.data.age_group.c_str(), ret.data.web_account_id.c_str());
    return ret;
}

xbox::services::xbox_live_result<xbox::services::system::token_and_signature_result>
XboxLiveDefaultGameInterface::invokeEventInit() {
    using namespace xbox::services::system;
    auto auth_mgr = auth_manager::get_auth_manager_instance();
    std::string endpoint = "https://vortex-events.xboxlive.com";
    auto task = auth_mgr->internal_get_token_and_signature("GET", endpoint, endpoint, std::string(), std::vector<unsigned char>(), false, false, std::string());
    auto ret = task.get();

    auto tid = xbox::services::xbox_live_app_config::get_app_config_singleton()->title_id();
    auth_mgr->initialize_title_nsal(std::to_string(tid)).get();

    return ret;
}

void XboxLiveDefaultGameInterface::onInvokeAndroidAuthFlow(void*) {
    XboxLiveHelper::getInstance().invokeMsaAuthFlow([this](std::string const& cid, std::string const& token) {
        completeAndroidAuthFlow(cid, token);
    }, [this](simpleipc::rpc_error_code, std::string const& msg) {
        Log::trace(TAG, "Sign in error: %s", msg.c_str());
        completeAndroidAuthFlowWithError();
    });
}

xbox::services::xbox_live_result<void>
XboxLiveDefaultGameInterface::onInitSignInActivity(void*, int requestCode) {
    xbox::services::xbox_live_result<void> ret;
    ret.code = 0;
    ret.error_code_category = xbox::services::xbox_services_error_code_category();

    std::string cid = getLocalStorageValue("cid");
    setAndroidUserAuthCid(cid);

    if (requestCode == 1) { // silent signin
        if (!cid.empty()) {
            XboxLiveHelper::getInstance().requestXblToken(cid, true,
                    [](std::string const& cid, std::string const& token) {
                        XboxLiveHelper::getInstance().initCll(cid);

                        xbox::services::system::java_rps_ticket ticket;
                        ticket.token = token;
                        ticket.error_code = 0;
                        ticket.error_text = "Got ticket";
                        xbox::services::system::user_auth_android::s_rpsTicketCompletionEvent->set(ticket);
                    }, [](simpleipc::rpc_error_code err, std::string const& msg) {
                        xbox::services::system::java_rps_ticket ticket;
                        Log::error(TAG, "Xbox Live sign in failed: %s", msg.c_str());
                        if (err == -100) { // No such account
                            ticket.error_code = 1;
                            ticket.error_text = "Must show UI to acquire an account.";
                        } else if (err == -102) { // Must show UI
                            ticket.error_code = 1;
                            ticket.error_text = "Must show UI to update account information.";
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

void XboxLiveDefaultGameInterface::completeAndroidAuthFlow(std::string const &cid, std::string const &token) {
    Log::trace(TAG, "Got account CID and token");
    Log::trace(TAG, "Invoking XBL login");
    auto ret = invokeXblLogin(cid, token);
    Log::trace(TAG, "Invoking XBL event init");
    XboxLiveHelper::getInstance().initCll(cid);
    auto retEv = invokeEventInit();
    Log::trace(TAG, "Xbox Live login completed");

    auto auth = xbox::services::system::user_auth_android::get_instance();
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
}

void XboxLiveDefaultGameInterface::completeAndroidAuthFlowWithError() {
    auto auth = xbox::services::system::user_auth_android::get_instance();
    auth->auth_flow_result.code = 2;
    auth->auth_flow_event.set(auth->auth_flow_result);
}

std::string XboxLiveDefaultGameInterface::getCllXToken(bool refresh) {
    using namespace xbox::services::system;
    auto auth_mgr = auth_manager::get_auth_manager_instance();
    if (refresh) {
        auto initRet = auth_mgr->initialize_default_nsal().get();
        if (initRet.code != 0)
            throw std::runtime_error("Failed to initialize default nsal");
    }
    std::vector<token_identity_type> types = {(token_identity_type) 3, (token_identity_type) 1,
                                              (token_identity_type) 2};
    auto config = auth_mgr->get_auth_config();
    config->set_xtoken_composition(types);
    std::string endpoint = "https://test.vortex.data.microsoft.com";
    auto task = auth_mgr->internal_get_token_and_signature("GET", endpoint, endpoint, std::string(), std::vector<unsigned char>(), false, refresh, std::string());
    auto ret = task.get();
    return ret.data.token.std();
}

std::string XboxLiveDefaultGameInterface::getCllXTicket(std::string const &xuid) {
    return getLocalStorageValue(xuid);
}

std::string XboxLiveDefaultGameInterface::getLocalStorageValue(std::string const &key) {
    auto local_conf = xbox::services::local_config::get_local_config_singleton();
    if (MinecraftVersion::isAtLeast(1, 8) && !MinecraftVersion::isExactly(1, 9, 0, 0) &&
        !MinecraftVersion::isExactly(1, 9, 0, 2))
        return local_conf->get_value_from_local_storage(key).std();
    else
        return ((Legacy::Pre_1_8::xbox::services::local_config&) *local_conf).get_value_from_local_storage(key).std();
}

void XboxLiveDefaultGameInterface::setAndroidUserAuthCid(std::string const &cid) {
    using namespace xbox::services::system;
    auto auth = xbox::services::system::user_auth_android::get_instance();
    auth->cid = cid;
}