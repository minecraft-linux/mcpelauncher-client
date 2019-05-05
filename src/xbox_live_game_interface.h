#pragma once

#include <memory>
#include <minecraft/Xbox.h>

class XboxLiveGameInterface {

public:
    static XboxLiveGameInterface& getInstance();

    virtual void onInvokeAndroidAuthFlow(void* th) = 0;

    virtual xbox::services::xbox_live_result<void> onInitSignInActivity(void* th, int requestCode) = 0;

    virtual std::string getCllXToken(bool refresh) = 0;

    virtual std::string getCllXTicket(std::string const& xuid) = 0;


};

class XboxLiveDefaultGameInterface : public XboxLiveGameInterface {

private:
    xbox::services::xbox_live_result<xbox::services::system::token_and_signature_result> invokeXblLogin(
            std::string const& cid, std::string const& binaryToken);

    xbox::services::xbox_live_result<xbox::services::system::token_and_signature_result> invokeEventInit();

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