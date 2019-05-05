#pragma once

#include <memory>
#include <minecraft/Xbox.h>
#include <log.h>

class XboxLiveGameInterface {

public:
    static XboxLiveGameInterface& getInstance();

    virtual void onInvokeAndroidAuthFlow(void* th) = 0;

    virtual xbox::services::xbox_live_result<void> onInitSignInActivity(void* th, int requestCode) = 0;

    virtual std::string getCllXToken(bool refresh) = 0;

    virtual std::string getCllXTicket(std::string const& xuid) = 0;


};

class XboxLiveDefaultGameInterface : public XboxLiveGameInterface {

protected:
    static const char* const TAG;

    template <typename AuthMgrT>
    auto invokeXblLogin(std::string const& cid, std::string const& binaryToken) {
        using token_identity_type = xbox::services::system::token_identity_type;
        auto auth_mgr = AuthMgrT::get_auth_manager_instance();
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

    template <typename AuthMgrT>
    auto invokeEventInit() {
        auto auth_mgr = AuthMgrT::get_auth_manager_instance();
        std::string endpoint = "https://vortex-events.xboxlive.com";
        auto task = auth_mgr->internal_get_token_and_signature("GET", endpoint, endpoint, std::string(), std::vector<unsigned char>(), false, false, std::string());
        auto ret = task.get();

        auto tid = xbox::services::xbox_live_app_config::get_app_config_singleton()->title_id();
        auth_mgr->initialize_title_nsal(std::to_string(tid)).get();

        return ret;
    }

    template <typename AuthMgrT>
    std::string getCllXTokenImpl(bool refresh) {
        using token_identity_type = xbox::services::system::token_identity_type;
        auto auth_mgr = AuthMgrT::get_auth_manager_instance();
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

public:
    void onInvokeAndroidAuthFlow(void* th) override;

    xbox::services::xbox_live_result<void> onInitSignInActivity(void* th, int requestCode) override;

    virtual void completeAndroidAuthFlow(std::string const &cid, std::string const &token);

    virtual void completeAndroidAuthFlowWithError();

    std::string getCllXToken(bool refresh) override;

    std::string getCllXTicket(std::string const &xuid) override;

    virtual void setAndroidUserAuthCid(std::string const& cid);

    virtual std::string getLocalStorageValue(std::string const& key);

};