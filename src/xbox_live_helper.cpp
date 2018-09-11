#include <minecraft/Xbox.h>
#include <log.h>
#include <mcpelauncher/path_helper.h>
#include <FileUtil.h>
#include <minecraft/Common.h>
#include "xbox_live_helper.h"
#include "xbox_live_msa_login.h"
#include "xbox_live_msa_remote_login.h"

XboxLiveHelper::XboxLiveHelper() {
    try {
        loginInterface = std::unique_ptr<XboxLiveLoginInterface>(new XboxLiveMsaLogin());
    } catch (std::exception& exception) {
        Log::error("XboxLiveHelper", "Failed to create the daemon login interface: %s", exception.what());

        loginInterface = std::unique_ptr<XboxLiveLoginInterface>(new XboxLiveMsaRemoteLogin());
    }
}

xbox::services::xbox_live_result<xbox::services::system::token_and_signature_result> XboxLiveHelper::invokeXblLogin(
        std::string const& cid, std::string const& binaryToken) {
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
    Log::trace("XboxLiveHelper", "Xbox Live Endpoint: %s", endpoint.c_str());
    auto task = auth_mgr->internal_get_token_and_signature("GET", endpoint, endpoint, std::string(), std::vector<unsigned char>(), false, false, std::string());
    Log::trace("XboxLiveHelper", "Get token and signature task started!");
    auto ret = task.get();
    Log::debug("XboxLiveHelper", "User info received! Status: %i", ret.code);
    Log::debug("XboxLiveHelper", "Gamertag = %s, age group = %s, web account id = %s\n", ret.data.gamertag.c_str(), ret.data.age_group.c_str(), ret.data.web_account_id.c_str());
    return ret;
}

xbox::services::xbox_live_result<xbox::services::system::token_and_signature_result> XboxLiveHelper::invokeEventInit() {
    using namespace xbox::services::system;
    auto auth_mgr = auth_manager::get_auth_manager_instance();
    std::string endpoint = "https://vortex-events.xboxlive.com";
    auto task = auth_mgr->internal_get_token_and_signature("GET", endpoint, endpoint, std::string(), std::vector<unsigned char>(), false, false, std::string());
    auto ret = task.get();

    auto tid = xbox::services::xbox_live_app_config::get_app_config_singleton()->title_id();
    auth_mgr->initialize_title_nsal(std::to_string(tid)).get();

    return ret;
}

void XboxLiveHelper::initCll(std::string const& cid) {
    std::lock_guard<std::mutex> lock (cllMutex);
    if (!cid.empty())
        cllAuthStep.setAccount(cid);
    if (cll)
        return;
    auto tid = xbox::services::xbox_live_app_config::get_app_config_singleton()->title_id();
    std::string iKey = "P-XBL-T" + std::to_string(tid);
    auto cllEvents = PathHelper::getPrimaryDataDirectory() + "cll_events";
    auto cacheDir = PathHelper::getCacheDirectory() + "cll";
    FileUtil::mkdirRecursive(cllEvents);
    FileUtil::mkdirRecursive(cacheDir);
    cll = std::unique_ptr<cll::EventManager>(new cll::EventManager(iKey, cllEvents, cacheDir));
    cll->addUploadStep(cllAuthStep);
    cll->setApp("A:com.mojang.minecraftpe", Common::getGameVersionStringNet().std());
    cll->start();
}

std::string XboxLiveHelper::getCllXToken(bool refresh) {
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

std::string XboxLiveHelper::getCllXTicket(std::string const& xuid) {
    auto local_conf = xbox::services::local_config::get_local_config_singleton();
    return local_conf->get_value_from_local_storage(xuid).std();
}

void XboxLiveHelper::logCll(cll::Event const& event) {
    initCll();
    cll->add(event);
}