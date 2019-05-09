#pragma once

#include "xbox_live_game_interface_legacy_1_2.h"

class XboxLiveGameInterface_Pre_0_15_2 : public XboxLiveGameInterface_Pre_1_2 {

public:
    void completeAndroidAuthFlow(std::string const &cid, std::string const &token) override;

    void completeAndroidAuthFlowWithError() override;

    void setAndroidUserAuthCid(std::string const &cid) override;

};
