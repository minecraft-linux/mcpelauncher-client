#include "xbox_live_game_interface_legacy.h"
#include "xbox_live_helper.h"

#include <minecraft/legacy/Xbox.h>
#include <log.h>

#define TAG "XboxLiveGameInterface/1.4"

xbox::services::xbox_live_result<Legacy::Pre_1_2_3::xbox::services::system::token_and_signature_result>
XboxLiveGameInterface_Pre_1_2_3::invokeXblLogin(std::string const &cid, std::string const &binaryToken) {
    using namespace Legacy::Pre_1_2_3::xbox::services::system;
    using token_identity_type = xbox::services::system::token_identity_type;
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

xbox::services::xbox_live_result<Legacy::Pre_1_2_3::xbox::services::system::token_and_signature_result>
XboxLiveGameInterface_Pre_1_2_3::invokeEventInit() {
    using namespace Legacy::Pre_1_2_3::xbox::services::system;
    auto auth_mgr = auth_manager::get_auth_manager_instance();
    std::string endpoint = "https://vortex-events.xboxlive.com";
    auto task = auth_mgr->internal_get_token_and_signature("GET", endpoint, endpoint, std::string(), std::vector<unsigned char>(), false, false, std::string());
    auto ret = task.get();

    auto tid = xbox::services::xbox_live_app_config::get_app_config_singleton()->title_id();
    auth_mgr->initialize_title_nsal(std::to_string(tid)).get();

    return ret;
}

void XboxLiveGameInterface_Pre_1_2_3::completeAndroidAuthFlow(std::string const &cid, std::string const &token) {
    Log::trace(TAG, "Got account CID and token");
    Log::trace(TAG, "Invoking XBL login");
    auto ret = invokeXblLogin(cid, token);
    Log::trace(TAG, "Invoking XBL event init");
    XboxLiveHelper::getInstance().initCll(cid);
    auto retEv = invokeEventInit();
    Log::trace(TAG, "Xbox Live login completed");

    auto auth = Legacy::Pre_1_2_3::xbox::services::system::user_auth_android::get_instance();
    auth->auth_flow_result.code = 0;
    auth->auth_flow_result.xbox_user_id = ret.data.xbox_user_id;
    auth->auth_flow_result.gamertag = ret.data.gamertag;
    auth->auth_flow_result.age_group = ret.data.age_group;
    auth->auth_flow_result.privileges = ret.data.privileges;
    auth->auth_flow_result.event_token = retEv.data.token;
    auth->auth_flow_result.cid = cid;
    auth->auth_flow_event.set(auth->auth_flow_result);
}

void XboxLiveGameInterface_Pre_1_2_3::completeAndroidAuthFlowWithError() {
    auto auth = Legacy::Pre_1_2_3::xbox::services::system::user_auth_android::get_instance();
    auth->auth_flow_result.code = 2;
    auth->auth_flow_event.set(auth->auth_flow_result);
}

std::string XboxLiveGameInterface_Pre_1_2_3::getCllXToken(bool refresh) {
    using namespace Legacy::Pre_1_2_3::xbox::services::system;
    using token_identity_type = xbox::services::system::token_identity_type;
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

void XboxLiveGameInterface_Pre_1_2_3::setAndroidUserAuthCid(std::string const &cid) {
    using namespace Legacy::Pre_1_2_3::xbox::services::system;
    auto auth = user_auth_android::get_instance();
    auth->cid = cid;
}
