#pragma once

#include "xbox_live_game_interface.h"
#include <minecraft/legacy/Xbox.h>

class XboxLiveGameInterface_Pre_1_2_3 : public XboxLiveDefaultGameInterface {

public:
    void completeAndroidAuthFlow(std::string const &cid, std::string const &token) override;

    void completeAndroidAuthFlowWithError() override;

    std::string getCllXToken(bool refresh) override;

    void setAndroidUserAuthCid(std::string const &cid) override;

};
