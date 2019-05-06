#pragma once

#include <minecraft/legacy/Xbox.h>
#include "../xbox_live_game_interface.h"

class XboxLiveGameInterface_Pre_1_2 : public XboxLiveDefaultGameInterface {

public:
    void completeAndroidAuthFlow(std::string const &cid, std::string const &token) override;

    void completeAndroidAuthFlowWithError() override;

    std::string getCllXToken(bool refresh) override;

    std::string getLocalStorageValue(std::string const& key) override;

    void setAndroidUserAuthCid(std::string const& cid) override;

    void setRpsTicketCompletionEventValue(xbox::services::system::java_rps_ticket const &ticket) override;

    void setSignOutCompleteEventValue(xbox::services::xbox_live_result<void> const &value) override;

};
