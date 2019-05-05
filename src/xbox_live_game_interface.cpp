#include "xbox_live_game_interface.h"
#include "xbox_live_helper.h"
#include <mcpelauncher/minecraft_version.h>
#include <minecraft/legacy/Xbox.h>
#include "xbox_live_game_interface_legacy_1_2.h"
#include "xbox_live_game_interface_legacy_1_2_3.h"
#include "xbox_live_game_interface_legacy_1_4.h"

const char* const XboxLiveDefaultGameInterface::TAG = "XboxLiveGameInterface";

XboxLiveGameInterface& XboxLiveGameInterface::getInstance() {
    if (!MinecraftVersion::isAtLeast(1, 2)) {
        static XboxLiveGameInterface_Pre_1_2 instance;
        return instance;
    }
    if (!MinecraftVersion::isAtLeast(1, 2, 3)) {
        static XboxLiveGameInterface_Pre_1_2_3 instance;
        return instance;
    }
    if (!MinecraftVersion::isAtLeast(1, 4)) {
        static XboxLiveGameInterface_Pre_1_4 instance;
        return instance;
    }
    static XboxLiveDefaultGameInterface instance;
    return instance;
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
                    [this](std::string const& cid, std::string const& token) {
                        XboxLiveHelper::getInstance().initCll(cid);

                        xbox::services::system::java_rps_ticket ticket;
                        ticket.token = token;
                        ticket.error_code = 0;
                        ticket.error_text = "Got ticket";
                        setRpsTicketCompletionEventValue(ticket);
                    }, [this](simpleipc::rpc_error_code err, std::string const& msg) {
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
                        setRpsTicketCompletionEventValue(ticket);
                    });
        } else {
            xbox::services::system::java_rps_ticket ticket;
            ticket.error_code = 1;
            ticket.error_text = "Must show UI to acquire an account.";
            setRpsTicketCompletionEventValue(ticket);
        }
    } else if (requestCode == 6) { // sign out
        xbox::services::xbox_live_result<void> arg;
        arg.code = 0;
        arg.error_code_category = xbox::services::xbox_services_error_code_category();
        setSignOutCompleteEventValue(arg);
    }

    return ret;
}

void XboxLiveDefaultGameInterface::setRpsTicketCompletionEventValue(
        xbox::services::system::java_rps_ticket const &ticket) {
    xbox::services::system::user_auth_android::s_rpsTicketCompletionEvent->set(ticket);
}

void XboxLiveDefaultGameInterface::setSignOutCompleteEventValue(xbox::services::xbox_live_result<void> const &value) {
    xbox::services::system::user_auth_android::s_signOutCompleteEvent->set(value);
}

void XboxLiveDefaultGameInterface::completeAndroidAuthFlow(std::string const &cid, std::string const &token) {
    Log::trace(TAG, "Got account CID and token");
    Log::trace(TAG, "Invoking XBL login");
    auto ret = invokeXblLogin<xbox::services::system::auth_manager>(cid, token);
    Log::trace(TAG, "Invoking XBL event init");
    XboxLiveHelper::getInstance().initCll(cid);
    auto retEv = invokeEventInit<xbox::services::system::auth_manager>();
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
    return getCllXTokenImpl<xbox::services::system::auth_manager>(refresh);
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
    auto auth = user_auth_android::get_instance();
    auth->cid = cid;
}