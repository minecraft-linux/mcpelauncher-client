#pragma once

#include <memory>
#include <minecraft/Xbox.h>
#include <cll/event_manager.h>
#include "cll_upload_auth_step.h"
#include "msa_remote_login_task.h"
#include "xbox_live_login_interface.h"

class XboxLiveHelper {

private:
    std::mutex cllMutex;
    std::unique_ptr<cll::EventManager> cll;
    CllUploadAuthStep cllAuthStep;
    std::unique_ptr<XboxLiveLoginInterface> loginInterface;

public:
    static XboxLiveHelper& getInstance() {
        static XboxLiveHelper instance;
        return instance;
    }

    XboxLiveHelper();

    ~XboxLiveHelper() {
        cll.reset();
    }

    void shutdownCll() {
        cll.reset();
    }

    XboxLiveLoginInterface& getLoginInterface() {
        return *loginInterface;
    }


    xbox::services::xbox_live_result<xbox::services::system::token_and_signature_result> invokeXblLogin(
            std::string const& cid, std::string const& binaryToken);

    xbox::services::xbox_live_result<xbox::services::system::token_and_signature_result> invokeEventInit();


    static std::string getCllXToken(bool refresh);

    static std::string getCllXTicket(std::string const& xuid);

    void initCll(std::string const& cid = std::string());

    void logCll(cll::Event const& event);

};