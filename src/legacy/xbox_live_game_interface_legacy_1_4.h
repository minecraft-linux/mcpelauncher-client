#pragma once

#include "../xbox_live_game_interface.h"
#include <minecraft/legacy/Xbox.h>

class XboxLiveGameInterface_Pre_1_4 : public XboxLiveDefaultGameInterface {

public:
    void completeAndroidAuthFlow(std::string const &cid, std::string const &token) override;

    std::string getCllXToken(bool refresh) override;

};
