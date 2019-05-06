#include "xbox_live_game_interface_legacy_1_2.h"
#include "../xbox_live_helper.h"
#include <minecraft/legacy/Xbox.h>

#define TAG "XboxLiveGameInterface/1.2"

void XboxLiveGameInterface_Pre_1_2::completeAndroidAuthFlow(std::string const &cid, std::string const &token) {
    Log::trace(TAG, "Got account CID and token");
    Log::trace(TAG, "Invoking XBL login");
    auto ret = invokeXblLogin<Legacy::Pre_1_2_3::xbox::services::system::auth_manager>(cid, token);
    Log::trace(TAG, "Invoking XBL event init");
    XboxLiveHelper::getInstance().initCll(cid);
    auto retEv = invokeEventInit<Legacy::Pre_1_2_3::xbox::services::system::auth_manager>();
    Log::trace(TAG, "Xbox Live login completed");

    auto auth = Legacy::Pre_1_2::xbox::services::system::user_impl_android::get_instance();
    auth->auth_flow_result.code = 0;
    auth->auth_flow_result.xbox_user_id = ret.data.xbox_user_id;
    auth->auth_flow_result.gamertag = ret.data.gamertag;
    auth->auth_flow_result.age_group = ret.data.age_group;
    auth->auth_flow_result.privileges = ret.data.privileges;
    auth->auth_flow_result.event_token = retEv.data.token;
    auth->auth_flow_result.cid = cid;
    auth->auth_flow_event.set(auth->auth_flow_result);
}

void XboxLiveGameInterface_Pre_1_2::completeAndroidAuthFlowWithError() {
    auto auth = Legacy::Pre_1_2::xbox::services::system::user_impl_android::get_instance();
    auth->auth_flow_result.code = 2;
    auth->auth_flow_event.set(auth->auth_flow_result);
}

std::string XboxLiveGameInterface_Pre_1_2::getCllXToken(bool refresh) {
    return getCllXTokenImpl<Legacy::Pre_1_2_3::xbox::services::system::auth_manager>(refresh);
}

std::string XboxLiveGameInterface_Pre_1_2::getLocalStorageValue(std::string const &key) {
    auto local_conf = Legacy::Pre_1_2::xbox::services::local_config_impl::get_local_config_singleton();
    return local_conf->get_value_from_local_storage(key).std();
}

void XboxLiveGameInterface_Pre_1_2::setAndroidUserAuthCid(std::string const &cid) {
    using namespace Legacy::Pre_1_2::xbox::services::system;
    auto auth = user_impl_android::get_instance();
    auth->cid = cid;
}

void XboxLiveGameInterface_Pre_1_2::setRpsTicketCompletionEventValue(
        xbox::services::system::java_rps_ticket const &ticket) {
    Legacy::Pre_1_2::xbox::services::system::user_impl_android::s_rpsTicketCompletionEvent->set(ticket);
}

void XboxLiveGameInterface_Pre_1_2::setSignOutCompleteEventValue(
        xbox::services::xbox_live_result<void> const &value) {
    Legacy::Pre_1_2::xbox::services::system::user_impl_android::s_signOutCompleteEvent->set(value);
}
