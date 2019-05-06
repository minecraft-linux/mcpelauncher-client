#pragma once

#include "../minecraft_game_wrapper.h"
#include <minecraft/legacy/MinecraftGame.h>
#include <minecraft/legacy/App.h>

class MinecraftGameAppWrapper_Pre_1_8 : public MinecraftGameDefaultWrapper {

public:
    using MinecraftGameDefaultWrapper::MinecraftGameDefaultWrapper;

    void quit(mcpe::string const &cat, mcpe::string const &name) override {
        ((Legacy::Pre_1_8::App*) getWrapped())->quit();
    }

};

class MinecraftGameAppWrapper_Pre_1_2_10 : public MinecraftGameAppWrapper_Pre_1_8 {

public:
    using MinecraftGameAppWrapper_Pre_1_8::MinecraftGameAppWrapper_Pre_1_8;

    void setTextboxText(mcpe::string const &text, int u) override {
        ((Legacy::Pre_1_2_10::MinecraftGame*) getWrapped())->setTextboxText(text);
    }

};

class MinecraftGameAppWrapper_Pre_1_2 : public MinecraftGameAppWrapper_Pre_1_2_10 {

public:
    using MinecraftGameAppWrapper_Pre_1_2_10::MinecraftGameAppWrapper_Pre_1_2_10;

    OptionsRef getPrimaryUserOptions() override {
        return ((Legacy::Pre_1_2::MinecraftGame*) getWrapped())->getOptions();
    }

};